// R5 (docs/REVOLUTION_V2.md section 10): LISTEN's onset timing per 1/16, the heard groove in SIMPLE (V8, V9 from the
// input), TONAL following HPDG's key, the heard groove in the state.

#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/Revolution.h"
#include "Core/RevolutionSimple.h"
#include "Link/HamloLinkClient.h"
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"

namespace hamlo::test
{
namespace
{
using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;
namespace rev = hpeq::revolution;
namespace simple = hpeq::revolution::simple;
constexpr double kPi = 3.14159265358979323846;
constexpr int kSixteenth = 6000; // samples at 120 BPM, 48 kHz

class Head final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm(120.0);
        info.setIsPlaying(true);
        info.setPpqPosition(ppq);
        return info;
    }
    double ppq = 0.0;
};

juce::AudioBuffer<float> play(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x)
{
    Head head;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, 512);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        const int n = juce::jmin(512, y.getNumSamples() - start);
        head.ppq = static_cast<double>(start) * 2.0 / kFs;
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
    }
    p.setPlayHead(nullptr);
    return y;
}

// The loop's late samples per 1/16 (0-based), -1 = no hit: kicks 0, 8; snares 4, 12 (+10 ms); hats 2, 6, 10, 14 on the
// grid and the odd 1/16s +15 ms, except 9 and 11 (no hit there).
int lateness(int sixteenth)
{
    if (sixteenth == 9 || sixteenth == 11)
        return -1;
    if (sixteenth % 2 == 1)
        return 720;
    return sixteenth == 4 || sixteenth == 12 ? 480 : 0;
}

juce::AudioBuffer<float> loop(int bars)
{
    juce::AudioBuffer<float> x(2, bars * 16 * kSixteenth);
    x.clear();
    juce::Random rng(17);
    for (int b = 0; b < bars; ++b)
        for (int s = 0; s < 16; ++s)
        {
            const int late = lateness(s);
            if (late < 0)
                continue;
            const int start = (b * 16 + s) * kSixteenth + late;
            for (int i = 0; i < kSixteenth && start + i < x.getNumSamples(); ++i)
            {
                const double t = i / kFs;
                double v;
                if (s == 0 || s == 8)
                    v = 0.7 * std::exp(-t / 0.04) * std::sin(2.0 * kPi * (50.0 + 90.0 * std::exp(-t / 0.012)) * t);
                else if (s == 4 || s == 12)
                    v = 0.35 * std::exp(-t / 0.05) * (2.0 * rng.nextDouble() - 1.0);
                else
                    v = 0.3 * std::exp(-t / 0.012) * (2.0 * rng.nextDouble() - 1.0);
                for (int ch = 0; ch < 2; ++ch)
                    x.addSample(ch, start + i, static_cast<float>(v));
            }
        }
    return x;
}

// Only the analysis: every lane off (the engine still listens).
std::unique_ptr<ModulePluginProcessor> listener()
{
    auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
    for (const char* lane : { "timeOn", "glitchOn", "motionOn", "shapeAOn", "shapeBOn", "throwAOn", "throwBOn" })
        p->setParameterValue(lane, 0);
    return p;
}

int gapStart(const juce::AudioBuffer<float>& y, int from)
{
    for (int i = from; i < y.getNumSamples(); ++i)
        if (y.getSample(0, i) < 0.999f)
            return i;
    return -1;
}
} // namespace

class ListenTests final : public juce::UnitTest
{
public:
    ListenTests() : juce::UnitTest("Listen", "HamloRevolution") {}

    void runTest() override
    {
        auto p = listener();
        play(*p, loop(4));

        beginTest("Timing: after 4 bars the offsets within 1 ms of the loop's; presence >= 0.5 on hits, < 0.5 without");
        {
            std::array<float, 16> offsets {}, presence {};
            expect(p->moduleNode().getRevolutionTiming(0, offsets, presence));
            double worst = 0.0;
            int right = 0;
            for (int s = 0; s < 16; ++s)
            {
                const int late = lateness(s);
                if (late >= 0)
                {
                    const double ms = (offsets[static_cast<size_t>(s)] / 0.25 * kSixteenth - late) / kFs * 1000.0;
                    worst = std::max(worst, std::abs(ms));
                    right += presence[static_cast<size_t>(s)] >= 0.5f;
                }
                else
                    right += presence[static_cast<size_t>(s)] < 0.5f;
            }
            logMessage("  largest timing error " + juce::String(worst, 3) + " ms; presence right on " + juce::String(right) + " / 16 sixteenths");
            expect(worst <= 1.0, juce::String(worst));
            expectEquals(right, 16);
        }

        beginTest("V8 (LISTEN): GENERATE unlinked captures the heard hits and timing; on-hit events only where the input hits");
        {
            p->setParameterValue("simpleTarget", simple::FullTarget);
            p->setParameterValue("simpleAmount", 1.0);
            p->generateSimple(true);
            const auto state = p->getRevolutionState();
            const auto r = p->lastSimple();
            expect(r.listened && !r.linked);
            int mask = 0;
            for (int s = 0; s < 16; ++s)
                mask |= lateness(s) >= 0 ? 1 << s : 0;
            expectEquals(static_cast<int>(state.heardMask), mask);
            int onHit = 0, wrong = 0;
            for (int l = 0; l < rev::kLanes; ++l)
                for (int st = 0; st < 32; ++st)
                    if (((r.onHit[static_cast<size_t>(l)] >> st) & 1u) != 0)
                    {
                        ++onHit;
                        wrong += state.feel[static_cast<size_t>(st & 15)] < 8; // feel 0 .. 15: a hit = >= 0.5
                    }
            logMessage("  heard mask " + juce::String::toHexString(state.heardMask) + "; on-hit events " + juce::String(onHit) + ", off the input's hits "
                       + juce::String(wrong));
            expect(onHit > 0 && wrong == 0);
            // knob moves regenerate from what was captured: the same groove
            p->setParameterValue("simpleIntent", 0.7);
            p->simpleEdited();
            expect(p->getRevolutionState().groove == state.groove, "the captured groove kept");
        }

        beginTest("V9 (LISTEN): GROOVE 100 %: a GLITCH gap on 1/16s 5 and 6 starts at the loop's hit time within 1 ms");
        {
            const auto captured = p->getRevolutionState();
            const auto render = [&](int step, double groove)
            {
                auto q = listener();
                auto s = captured;
                s.pattern = {};
                s.pattern.steps[rev::Glitch][static_cast<size_t>(step)] = rev::Gap;
                s.lanes = {};
                q->setRevolutionState(s);
                q->setParameterValue("glitchOn", 1);
                q->setParameterValue("glitchMix", 1);
                q->setParameterValue("glitchDepth", 1);
                q->setParameterValue("simpleGroove", groove);
                juce::AudioBuffer<float> x(2, static_cast<int>(1.0 * kFs));
                for (int ch = 0; ch < 2; ++ch)
                    juce::FloatVectorOperations::fill(x.getWritePointer(ch), 1.0f, x.getNumSamples());
                return gapStart(play(*q, x), (step - 1) * kSixteenth + kSixteenth / 2);
            };
            for (int step : { 4, 5 })
            {
                const int straight = render(step, 0.0), grooved = render(step, 1.0);
                const double ms = (grooved - straight - lateness(step)) / kFs * 1000.0;
                logMessage("  1/16 " + juce::String(step + 1) + ": the gap moved " + juce::String(grooved - straight) + " samples, the loop's hit "
                           + juce::String(lateness(step)) + " late: off by " + juce::String(ms, 3) + " ms");
                expect(std::abs(ms) <= 1.0, juce::String(ms));
            }
        }

        beginTest("V13: the heard timing and the feel round trip");
        {
            juce::MemoryBlock saved;
            p->getStateInformation(saved);
            auto q = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            q->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            const auto a = p->getRevolutionState(), b = q->getRevolutionState();
            expect(a.heardMask == b.heardMask && a.feel == b.feel, "mask, feel");
            double worst = 0.0;
            for (int s = 0; s < 16; ++s)
                worst = std::max(worst, static_cast<double>(std::abs(a.heard[static_cast<size_t>(s)] - b.heard[static_cast<size_t>(s)])));
            expect(worst < 1.0e-5, juce::String(worst));
        }

        beginTest("Key: linked, HPDG's key A then D: both throws' NOTE follow (ADVANCED too); LOCK keeps NOTE");
        {
            juce::Random r;
            const auto id = 0x6b65000000000000ULL | (static_cast<std::uint64_t>(r.nextInt64()) & 0xffffffffffffULL);
            const auto file = link::slotFile(id);
            auto hpdg = link::SharedSlot::openForWriting(file);
            expect(hpdg != nullptr);
            if (hpdg == nullptr)
                return;
            const auto snapshot = [id](int key, int generation)
            {
                auto s = std::make_unique<link::Snapshot>();
                link::initialise(*s);
                s->instanceId = id;
                s->bpm = 120.0f;
                s->genre = link::Techno;
                s->keyRoot = key;
                s->bars = 1;
                s->generationCounter = generation;
                s->eventCount = 1;
                s->events[0] = { link::Kick, 0, 100, 0, 0, 120 };
                return s;
            };
            const juce::int64 t0 = juce::Time::currentTimeMillis();
            auto q = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            q->setPage(ModulePluginProcessor::Page::Advanced);
            q->getHpdgLink().connect(id, false);
            hpdg->write(*snapshot(9, 1), t0);
            q->getHpdgLink().pollOnce(t0 + 10);
            expect(q->followHpdg());
            expectEquals(juce::roundToInt(q->parameterValue("throwANote")), 9);
            expectEquals(juce::roundToInt(q->parameterValue("throwBNote")), 9);
            hpdg->write(*snapshot(2, 2), t0 + 20);
            q->getHpdgLink().pollOnce(t0 + 30);
            q->followHpdg();
            expectEquals(juce::roundToInt(q->parameterValue("throwANote")), 2);
            expectEquals(juce::roundToInt(q->parameterValue("throwBNote")), 2);
            q->setParameterValue("simpleLock", 1);
            hpdg->write(*snapshot(5, 3), t0 + 40);
            q->getHpdgLink().pollOnce(t0 + 50);
            q->followHpdg();
            expectEquals(juce::roundToInt(q->parameterValue("throwANote")), 2);
            hpdg.reset();
            file.deleteFile();
        }
    }
};

static ListenTests listenTests;
} // namespace hamlo::test
