// HPDG Link, the plug-in side (docs/REVOLUTION_V2.md V12): a test writer stands in for HPDG (the same protocol file
// HPDG publishes with). Round trip of every field, new payloads only once, HPDG gone -> LOST with the last snapshot
// kept, HPDG back -> LINKED, a newer protocol refused, AUTO picks a live HPDG, the polling thread, the saved link.

#include <cstring>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Link/HamloLinkClient.h"
#include "Standalone/ModulePluginProcessor.h"

namespace hamlo::test
{
namespace
{
using namespace hamlo::link;

std::uint64_t testId()
{
    juce::Random r;
    return 0x7e57000000000000ULL | (static_cast<std::uint64_t>(r.nextInt64()) & 0xffffffffffffULL);
}

std::unique_ptr<Snapshot> hpdgLike(std::uint64_t id, int variant)
{
    auto s = std::make_unique<Snapshot>();
    initialise(*s);
    s->instanceId = id;
    juce::String("HPDG TEST").copyToUTF8(s->instanceName, sizeof(s->instanceName));
    s->bpm = 140.0f + static_cast<float>(variant);
    s->genre = Trap;
    s->substyle = 2;
    s->swingPercent = 58.0f;
    s->timing = 0.3f;
    s->humanize = 0.2f;
    s->density = 0.8f;
    s->velocityAmount = 0.5f;
    s->intensity = 5;
    s->keyRoot = 4;
    s->scaleMode = 1;
    s->bars = 2;
    s->phraseLengthBars = 2;
    s->seed = 99;
    s->generationCounter = 3 + variant;
    s->eventCount = 32;
    juce::String("hat_roll").copyToUTF8(s->roleNames[1], sizeof(s->roleNames[1]));
    for (int i = 0; i < 32; ++i)
        s->events[i] = { HiHat, static_cast<std::uint8_t>(i % 8 == 7 ? Fill : 0), static_cast<std::uint8_t>(80 + i), 1, i * 240 + (i % 2) * 20, 100 };
    return s;
}
} // namespace

class LinkTests final : public juce::UnitTest
{
public:
    LinkTests() : juce::UnitTest("HPDG Link", "HamloRevolution") {}

    void runTest() override
    {
        const auto id = testId();
        const auto file = slotFile(id);
        auto hpdg = SharedSlot::openForWriting(file);
        expect(hpdg != nullptr, file.getFullPathName());
        if (hpdg == nullptr)
            return;
        const juce::int64 t0 = juce::Time::currentTimeMillis();
        auto a = hpdgLike(id, 0);
        hpdg->write(*a, t0);

        beginTest("V12: connected to an HPDG: LINKED, the snapshot arrives field for field, once");
        Client client;
        client.connect(id, false); // polled by hand below, with chosen times
        client.pollOnce(t0 + 10);
        expect(client.getState() == Client::State::Linked, Client::stateName(client.getState()));
        auto got = std::make_unique<Snapshot>();
        expect(client.takeIfNew(*got));
        expect(samePayload(*got, *a), "payload");
        expectEquals(got->bpm, 140.0f);
        expectEquals(got->genre, static_cast<std::int32_t>(Trap));
        expectEquals(got->eventCount, 32);
        expectEquals(got->events[31].tick, 31 * 240 + 20);
        expectEquals(roleName(*got, got->events[5].roleId), juce::String("hat_roll"));
        client.pollOnce(t0 + 20);
        expect(!client.takeIfNew(*got), "nothing new: not taken again");

        beginTest("A new pattern in HPDG: taken once");
        auto b = hpdgLike(id, 1);
        hpdg->write(*b, t0 + 100);
        client.pollOnce(t0 + 110);
        expect(client.takeIfNew(*got) && samePayload(*got, *b));
        expectEquals(got->bpm, 141.0f);

        beginTest("HPDG gone (no heartbeat 2 s): LOST, the last snapshot kept; back: LINKED");
        client.pollOnce(t0 + 120);
        client.pollOnce(t0 + 100 + kAliveMs + 500);
        expect(client.getState() == Client::State::Lost, Client::stateName(client.getState()));
        expect(client.last(*got) && samePayload(*got, *b), "last kept");
        hpdg->beat(t0 + 100 + kAliveMs + 600);
        client.pollOnce(t0 + 100 + kAliveMs + 700);
        expect(client.getState() == Client::State::Linked, Client::stateName(client.getState()));

        beginTest("HPDG closed (heartbeat 0, file deleted): LOST");
        hpdg->beat(0);
        client.pollOnce(t0 + 100 + kAliveMs + 800);
        expect(client.getState() == Client::State::Lost);

        beginTest("A newer protocol (major 2): refused, UPDATE");
        auto c = hpdgLike(id, 2);
        c->versionMajor = kVersionMajor + 1;
        hpdg->write(*c, t0 + 5000);
        client.pollOnce(t0 + 5010);
        expect(client.getState() == Client::State::Update, Client::stateName(client.getState()));
        client.disconnect();

        beginTest("AUTO (no HPDG chosen): links to a live HPDG; with none, SEARCHING");
        hpdg->write(*a, juce::Time::currentTimeMillis());
        {
            Client automatic;
            automatic.connect(0, false);
            automatic.pollOnce(juce::Time::currentTimeMillis());
            expect(automatic.getState() == Client::State::Linked && automatic.getInstanceId() != 0);
        }
        {
            Client nobody;
            nobody.connect(testId(), false); // an id no HPDG has
            nobody.pollOnce(juce::Time::currentTimeMillis());
            expect(nobody.getState() == Client::State::Searching, Client::stateName(nobody.getState()));
        }

        beginTest("The polling thread: LINKED within a second, a new pattern arrives by itself");
        {
            Client threaded;
            hpdg->write(*a, juce::Time::currentTimeMillis());
            threaded.connect(id);
            for (int i = 0; i < 50 && threaded.getState() != Client::State::Linked; ++i)
                juce::Thread::sleep(20);
            expect(threaded.getState() == Client::State::Linked);
            threaded.takeIfNew(*got);
            hpdg->write(*b, juce::Time::currentTimeMillis());
            bool arrived = false;
            for (int i = 0; i < 50 && !arrived; ++i)
            {
                juce::Thread::sleep(20);
                arrived = threaded.takeIfNew(*got) && samePayload(*got, *b);
            }
            expect(arrived);
        }

        beginTest("V13: the link (on, which HPDG) is saved with the plug-in and restored");
        {
            auto p = std::make_unique<hpeq::standalone::ModulePluginProcessor>(hpeq::standalone::ModuleKind::Revolution);
            p->getHpdgLink().connect(id);
            juce::MemoryBlock saved;
            p->getStateInformation(saved);
            auto q = std::make_unique<hpeq::standalone::ModulePluginProcessor>(hpeq::standalone::ModuleKind::Revolution);
            q->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            expect(q->getHpdgLink().isConnected());
            expect(q->getHpdgLink().getInstanceId() == id);
            expect(q->getRevolutionState().pattern == p->getRevolutionState().pattern);
            p->getHpdgLink().disconnect();
            p->getStateInformation(saved);
            q->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            expect(!q->getHpdgLink().isConnected());
        }

        hpdg.reset();
        file.deleteFile();
    }
};

static LinkTests linkTests;
} // namespace hamlo::test
