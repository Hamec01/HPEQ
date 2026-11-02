// R4 (docs/REVOLUTION_V2.md section 9): the SIMPLE generator (V8, V10, V11), GROOVE from HPDG (V9), following a
// linked HPDG (V12), V3 with a groove, V13 the mode and SIMPLE state, the TOPS band, the SIMPLE page.

#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/Revolution.h"
#include "Core/RevolutionSimple.h"
#include "Link/HamloLinkClient.h"
#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include "UI/HamloRevolutionPanel.h"
#include "UI/HamloRevolutionSimplePage.h"

namespace hamlo::test
{
namespace
{
using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;
namespace rev = hpeq::revolution;
namespace simple = hpeq::revolution::simple;
using State = ModulePluginProcessor::RevolutionState;
constexpr double kPi = 3.14159265358979323846;

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

juce::AudioBuffer<float> play(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x, int block = 512, int prepared = 512)
{
    Head head;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, prepared);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        head.ppq = static_cast<double>(start) * 2.0 / kFs;
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
    }
    p.setPlayHead(nullptr);
    return y;
}

// An HPDG pattern, 2 bars: kick on 1 and 11, snare + clap on 5 and 13 (+20 ticks), hats on the eighths, the odd
// 1/16s 40 ticks late (its swing). Every event at a 1/16 has the same offset, so that 1/16 has one hit time.
constexpr int kSnareLate = 20, kSwingLate = 40;
int offsetAt(int step) { return step % 2 == 1 ? kSwingLate : (step == 4 || step == 12) ? kSnareLate : 0; }
std::unique_ptr<link::Snapshot> hpdgPattern(std::uint64_t id, int genre, int generation)
{
    auto s = std::make_unique<link::Snapshot>();
    link::initialise(*s);
    s->instanceId = id;
    juce::String("BEAT ONE").copyToUTF8(s->instanceName, sizeof(s->instanceName));
    s->bpm = 120.0f;
    s->swingPercent = 58.333332f; // the odd 1/16s 40 ticks late, as the events
    s->genre = genre;
    s->keyRoot = 9;
    s->bars = 2;
    s->generationCounter = generation;
    int n = 0;
    const auto add = [&](std::uint8_t lane, int step)
    {
        for (int bar = 0; bar < 2; ++bar)
            s->events[n++] = { lane, 0, 100, 0, (bar * 16 + step) * 240 + offsetAt(step), 120 };
    };
    for (int st : { 0, 10 })
        add(link::Kick, st);
    for (int st : { 4, 12 })
    {
        add(link::Snare, st);
        add(link::Clap, st);
    }
    for (int st = 0; st < 16; st += 2)
        add(link::HiHat, st);
    add(link::HiHat, 15);
    s->eventCount = n;
    return s;
}

std::uint64_t testId()
{
    juce::Random r;
    return 0x51e9000000000000ULL | (static_cast<std::uint64_t>(r.nextInt64()) & 0xffffffffffffULL);
}

// The values a lane plays bar by bar (Core's rule = the engine's): bars x (its steps per bar at 120 BPM, 1/16 grid).
std::vector<std::vector<int>> barsOf(const rev::Pattern& pattern, const rev::Lane& lane, int l, std::uint32_t seed, int bars)
{
    float first[4] {}, packed[rev::kLaneFloats] {};
    for (int q = 0; q < 4; ++q)
        first[q] = l < rev::kEngines ? rev::packQuad(pattern.steps[static_cast<size_t>(l)], q) : rev::packQuad(lane.first, q);
    rev::packLane(lane, packed);
    bool chance = false;
    for (int q = 0; q < 8; ++q)
        chance = chance || packed[7 + q] != 0.0f;
    static constexpr int perBar[] { 16, 4, 8, 16, 32, 6, 12, 24, 48 }; // FOLLOW (1/16), 1/4 .. 1/32, 1/4T .. 1/32T
    const int steps = perBar[juce::jlimit(0, 8, lane.rate)];
    const int length = lane.length > 0 ? lane.length : rev::kSteps;
    std::vector<std::vector<int>> out;
    for (int b = 0; b < bars; ++b)
    {
        std::vector<int> bar;
        for (long long k = static_cast<long long>(b) * steps; k < static_cast<long long>(b + 1) * steps; ++k)
            bar.push_back(rev::laneEventValue(first, &packed[3], &packed[7], chance, length, lane.condition, seed, l, k));
        out.push_back(std::move(bar));
    }
    return out;
}

bool laneUsed(const simple::Result& r, int l)
{
    const auto& lane = r.lanes[static_cast<size_t>(l)];
    for (int s = 0; s < rev::kSteps; ++s)
        if ((l < rev::kEngines ? r.pattern.steps[static_cast<size_t>(l)][static_cast<size_t>(s)] : lane.first[static_cast<size_t>(s)]) != 0
            || lane.extra[static_cast<size_t>(s)] != 0)
            return true;
    return false;
}

double dB(double ratio) { return 20.0 * std::log10(std::max(ratio, 1.0e-12)); }

// The first sample from `from` where a DC input of 1 has been pulled down (a GLITCH gap starting).
int gapStart(const juce::AudioBuffer<float>& y, int from)
{
    for (int i = from; i < y.getNumSamples(); ++i)
        if (y.getSample(0, i) < 0.999f)
            return i;
    return -1;
}
} // namespace

class SimpleTests final : public juce::UnitTest
{
public:
    SimpleTests() : juce::UnitTest("Simple", "HamloRevolution") {}

    void runTest() override
    {
        const auto hpdgHits = simple::hitsFromSnapshot(*hpdgPattern(1, link::Trap, 1));

        beginTest("Hits from an HPDG snapshot: the roles per 1/16 and the groove");
        {
            expectEquals(hpdgHits.snare[4], 1.0f);
            expectEquals(hpdgHits.kick[10], 1.0f);
            expectEquals(hpdgHits.hats[15], 1.0f);
            expectEquals(hpdgHits.snare[5], 0.0f);
            expectWithinAbsoluteError(hpdgHits.groove[4], static_cast<float>(kSnareLate / 960.0), 1.0e-6f);
            expectWithinAbsoluteError(hpdgHits.groove[15], static_cast<float>(kSwingLate / 960.0), 1.0e-6f);
            expectEquals(simple::resolveGenre(simple::AutoGenre, hpdgHits, 90.0), static_cast<int>(simple::Trap));
        }

        beginTest("V10: events never fall as AMOUNT rises; fills at the phrase end; TARGET HATS stays on HIGH");
        {
            int checked = 0, monotonic = 0, fills = 0, fillsAtEnd = 0, hatsCases = 0, hatsHigh = 0;
            for (int genre = simple::BoomBap; genre < simple::kGenres; ++genre)
                for (int intent = 0; intent < simple::kIntents; ++intent)
                    for (std::uint64_t seed : { 1ull, 7ull, 99ull })
                        for (const auto* hits : { &hpdgHits, static_cast<const simple::Hits*>(nullptr) })
                        {
                            const auto source = hits != nullptr ? *hits : simple::templateHits(genre);
                            simple::Settings s;
                            s.genre = genre;
                            s.intent = (intent + 0.5) / simple::kIntents;
                            s.seed = seed;
                            int last = -1;
                            bool rising = true;
                            for (int a = 0; a <= 10; ++a)
                            {
                                s.amount = a / 10.0;
                                const auto r = simple::generate(s, source, 120.0);
                                rising = rising && r.events >= last;
                                last = r.events;
                                for (int l = 0; l < rev::kLanes; ++l)
                                    for (int st = 0; st < 32; ++st)
                                        if (((r.fills[static_cast<size_t>(l)] >> st) & 1u) != 0)
                                        {
                                            ++fills;
                                            const int from = intent == simple::Breakdown && l == rev::Time ? 8 : 12;
                                            fillsAtEnd += st >= from && st < 16
                                                          && r.lanes[static_cast<size_t>(l)].condition == simple::fillCondition(s.fill);
                                        }
                            }
                            ++checked;
                            monotonic += rising;
                            s.target = simple::Hats;
                            s.amount = 0.7;
                            const auto h = simple::generate(s, source, 120.0);
                            for (int l = 0; l < rev::kLanes; ++l)
                                if (laneUsed(h, l))
                                {
                                    ++hatsCases;
                                    hatsHigh += h.lanes[static_cast<size_t>(l)].band == rev::High;
                                }
                        }
            logMessage("  " + juce::String(monotonic) + " / " + juce::String(checked) + " cases monotonic; fills at the phrase end " + juce::String(fillsAtEnd)
                       + " / " + juce::String(fills) + "; TARGET HATS lanes on HIGH " + juce::String(hatsHigh) + " / " + juce::String(hatsCases));
            expectEquals(monotonic, checked);
            expect(fills > 0 && fillsAtEnd >= 0.9 * fills);
            expect(hatsCases > 0 && hatsHigh == hatsCases);
        }

        beginTest("V8: on-hit events only where the TARGET's role hits (SNARE: 1/16s 5 and 13)");
        {
            int onHit = 0, wrong = 0;
            for (int genre = simple::BoomBap; genre < simple::kGenres; ++genre)
                for (int intent = 0; intent < simple::kIntents; ++intent)
                {
                    simple::Settings s;
                    s.genre = genre;
                    s.intent = (intent + 0.5) / simple::kIntents;
                    s.target = simple::Snare;
                    s.amount = 1.0;
                    const auto r = simple::generate(s, hpdgHits, 120.0);
                    for (int l = 0; l < rev::kLanes; ++l)
                        for (int st = 0; st < 32; ++st)
                            if (((r.onHit[static_cast<size_t>(l)] >> st) & 1u) != 0)
                            {
                                ++onHit;
                                wrong += hpdgHits.snare[static_cast<size_t>(st & 15)] < simple::kHit;
                            }
                }
            logMessage("  on-hit events " + juce::String(onHit) + ", off the snare's hits " + juce::String(wrong));
            expect(onHit > 0 && wrong == 0);
        }

        beginTest("V11: VARIATION 0 (FILL OFF): bar n = bar n + 1 for every lane; VARIATION 100 %: bars differ, rules hold");
        {
            int cases = 0, repeating = 0, varied = 0, varCases = 0, rulesHold = 0;
            for (int genre = simple::BoomBap; genre < simple::kGenres; ++genre)
                for (int intent = 0; intent < simple::kIntents; ++intent)
                    for (std::uint64_t seed : { 3ull, 11ull })
                    {
                        simple::Settings s;
                        s.genre = genre;
                        s.intent = (intent + 0.5) / simple::kIntents;
                        s.seed = seed;
                        s.amount = 0.6;
                        s.fill = simple::FillOff;
                        s.variation = 0.0;
                        const auto r = simple::generate(s, hpdgHits, 120.0);
                        bool same = true;
                        for (int l = 0; l < rev::kLanes; ++l)
                        {
                            const auto bars = barsOf(r.pattern, r.lanes[static_cast<size_t>(l)], l, static_cast<std::uint32_t>(seed & 0xffffff), 8);
                            for (size_t b = 1; b < bars.size(); ++b)
                                same = same && bars[b] == bars[0];
                        }
                        ++cases;
                        repeating += same;

                        s.variation = 1.0;
                        s.fill = simple::FillEvery4;
                        s.target = simple::Hats;
                        const auto v = simple::generate(s, hpdgHits, 120.0);
                        bool differs = false, high = true;
                        for (int l = 0; l < rev::kLanes; ++l)
                        {
                            const auto bars = barsOf(v.pattern, v.lanes[static_cast<size_t>(l)], l, static_cast<std::uint32_t>(seed & 0xffffff), 8);
                            for (size_t b = 1; b < bars.size(); ++b)
                                differs = differs || bars[b] != bars[0];
                            high = high && (!laneUsed(v, l) || v.lanes[static_cast<size_t>(l)].band == rev::High);
                        }
                        ++varCases;
                        varied += differs;
                        rulesHold += high;
                    }
            logMessage("  VARIATION 0: " + juce::String(repeating) + " / " + juce::String(cases) + " repeat bar for bar; VARIATION 100 %: "
                       + juce::String(varied) + " / " + juce::String(varCases) + " vary, rules hold in " + juce::String(rulesHold));
            expectEquals(repeating, cases);
            expectEquals(varied, varCases);
            expectEquals(rulesHold, varCases);
        }

        const auto id = testId();
        const auto file = link::slotFile(id);
        auto hpdg = link::SharedSlot::openForWriting(file);
        expect(hpdg != nullptr);
        if (hpdg == nullptr)
            return;
        const juce::int64 t0 = juce::Time::currentTimeMillis();
        hpdg->write(*hpdgPattern(id, link::Trap, 1), t0);

        beginTest("V12: linked, SIMPLE: GENRE AUTO = HPDG's genre; a new HPDG pattern regenerates; LOCK keeps; HPDG gone keeps");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            expect(p->getPage() == ModulePluginProcessor::Page::Simple, "a new instance opens SIMPLE");
            p->getHpdgLink().connect(id, false);
            p->getHpdgLink().pollOnce(t0 + 10);
            expect(p->followHpdg(), "the pattern taken");
            expectEquals(p->lastSimple().genre, static_cast<int>(simple::Trap));
            expect(p->lastSimple().linked);
            expectEquals(juce::roundToInt(p->parameterValue("throwANote")), 9);
            hpdg->write(*hpdgPattern(id, link::Techno, 2), t0 + 20);
            p->getHpdgLink().pollOnce(t0 + 30);
            expect(p->followHpdg());
            expectEquals(p->lastSimple().genre, static_cast<int>(simple::Techno));
            expect(!p->followHpdg(), "nothing new: no regeneration");
            p->setParameterValue("simpleLock", 1);
            const auto before = p->getRevolutionState();
            hpdg->write(*hpdgPattern(id, link::DnB, 3), t0 + 40);
            p->getHpdgLink().pollOnce(t0 + 50);
            p->followHpdg();
            expect(p->getRevolutionState().pattern == before.pattern && p->getRevolutionState().lanes == before.lanes, "LOCK keeps the scene");
            p->getHpdgLink().pollOnce(t0 + 5000); // no heartbeat for 5 s
            expect(p->getHpdgLink().getState() == link::Client::State::Lost);
            expect(p->getRevolutionState().lanes == before.lanes, "HPDG gone: the last pattern stays");
        }

        beginTest("V9: GROOVE 100 % linked: a GLITCH gap starts at HPDG's hit time within 1 ms; GROOVE 0: on the straight grid");
        {
            hpdg->write(*hpdgPattern(id, link::Trap, 4), t0 + 60);
            const auto render = [&](int step, double groove)
            {
                auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
                p->getHpdgLink().connect(id, false);
                p->getHpdgLink().pollOnce(t0 + 70);
                p->followHpdg(); // the groove table from HPDG
                auto s = p->getRevolutionState();
                s.pattern = {};
                s.pattern.steps[rev::Glitch][static_cast<size_t>(step)] = rev::Gap;
                s.lanes = {};
                p->setRevolutionState(s);
                for (const char* lane : { "timeOn", "motionOn", "shapeAOn", "shapeBOn", "throwAOn", "throwBOn" })
                    p->setParameterValue(lane, 0);
                p->setParameterValue("glitchMix", 1);
                p->setParameterValue("glitchDepth", 1);
                p->setParameterValue("simpleGroove", groove);
                juce::AudioBuffer<float> x(2, static_cast<int>(2.2 * kFs));
                for (int ch = 0; ch < 2; ++ch)
                    juce::FloatVectorOperations::fill(x.getWritePointer(ch), 1.0f, x.getNumSamples());
                const auto y = play(*p, x);
                return gapStart(y, static_cast<int>((step - 0.5) * 0.125 * kFs));
            };
            for (int step : { 4, 5, 12 })
            {
                const int straight = render(step, 0.0), grooved = render(step, 1.0);
                const double expected = offsetAt(step) / 960.0 * 0.5 * kFs; // ticks -> quarter notes -> seconds at 120 BPM -> samples
                const double ms = (grooved - straight - expected) / kFs * 1000.0;
                logMessage("  step " + juce::String(step + 1) + ": straight at " + juce::String(straight) + " (grid " + juce::String(step * 6000)
                           + "), grooved " + juce::String(grooved) + ", HPDG's offset " + juce::String(expected, 1) + " samples: off by "
                           + juce::String(ms, 3) + " ms");
                expect(std::abs(ms) <= 1.0, juce::String(ms));
                expect(std::abs(straight - step * 6000) <= 96, "GROOVE 0: the straight grid (the gap's 2 ms fade)");
            }
        }
        hpdg.reset();
        file.deleteFile();

        beginTest("V3: blocks 1, 64, 127, 4096 after prepare(256) bit-identical with a SIMPLE pattern and GROOVE 100 %");
        {
            const auto x = drums(2.0);
            const auto render = [&](int block)
            {
                auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
                p->setParameterValue("simpleGenre", simple::BoomBap);
                p->setParameterValue("simpleIntent", 0.5);
                p->setParameterValue("simpleAmount", 0.8);
                p->generateSimple(false);
                p->setParameterValue("simpleGroove", 1.0);
                return play(*p, x, block, 256);
            };
            const auto expected = render(256);
            expect(differenceDb(expected, x) > -40.0, "the pattern does something");
            for (int block : { 1, 64, 127, 4096 })
                expect(bitExact(render(block), expected), "block " + juce::String(block));
        }

        beginTest("V13: the page, SIMPLE settings and the groove round trip; an R3 state opens ADVANCED unchanged");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setParameterValue("simpleGenre", simple::Techno);
            p->setParameterValue("simpleVariation", 0.8);
            p->setParameterValue("simpleTarget", simple::Hats);
            p->generateSimple(true);
            auto s = p->getRevolutionState();
            s.groove[3] = 0.05f;
            p->setRevolutionState(s);
            juce::MemoryBlock saved;
            p->getStateInformation(saved);
            auto q = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            q->setPage(ModulePluginProcessor::Page::Advanced);
            q->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            expect(q->getPage() == ModulePluginProcessor::Page::Simple);
            expect(q->getRevolutionState().groove == s.groove, "groove");
            expect(q->getRevolutionState().lanes == s.lanes, "lanes");
            expectEquals(juce::roundToInt(q->parameterValue("simpleGenre")), static_cast<int>(simple::Techno));
            expect(q->currentSlot(true).values == p->currentSlot(true).values, "the slots");

            // an R3 state: no mode, no groove
            auto old = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            old->setCurrentProgram(3);
            juce::MemoryBlock r3;
            old->getStateInformation(r3);
            auto xml = juce::AudioProcessor::getXmlFromBinary(r3.getData(), static_cast<int>(r3.getSize()));
            expect(xml != nullptr);
            if (xml == nullptr)
                return;
            if (auto* e = xml->getChildByName("REVOLUTION"))
                e->removeAttribute("mode");
            juce::MemoryBlock stripped;
            juce::AudioProcessor::copyXmlToBinary(*xml, stripped);
            auto r = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            r->setStateInformation(stripped.getData(), static_cast<int>(stripped.getSize()));
            expect(r->getPage() == ModulePluginProcessor::Page::Advanced, "an R3 state opens ADVANCED");
            expect(r->currentSlot(true).values == old->currentSlot(true).values, "nothing changed");
        }

        beginTest("TOPS: GLITCH GAP on TOPS over 60 Hz + 6 kHz: 60 Hz within 0.5 dB of dry, 6 kHz >= 30 dB down");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setRevolutionState(State {});
            for (const char* lane : { "timeOn", "motionOn" })
                p->setParameterValue(lane, 0);
            State s;
            s.pattern.steps[rev::Glitch][4] = rev::Gap;
            s.lanes[rev::Glitch].band = rev::Tops;
            p->setRevolutionState(s);
            juce::AudioBuffer<float> x(2, static_cast<int>(1.0 * kFs));
            for (int i = 0; i < x.getNumSamples(); ++i)
                for (int ch = 0; ch < 2; ++ch)
                    x.setSample(ch, i, static_cast<float>(0.3 * std::sin(2.0 * kPi * 60.0 * i / kFs) + 0.3 * std::sin(2.0 * kPi * 6000.0 * i / kFs)));
            const auto y = play(*p, x);
            const auto amp = [](const juce::AudioBuffer<float>& b, int from, int n, double hz)
            {
                double re = 0.0, im = 0.0;
                for (int i = 0; i < n; ++i)
                {
                    const double w = 2.0 * kPi * hz * (from + i) / kFs;
                    re += b.getSample(0, from + i) * std::cos(w);
                    im += b.getSample(0, from + i) * std::sin(w);
                }
                return 2.0 * std::sqrt(re * re + im * im) / n;
            };
            const int from = static_cast<int>(4.25 * 0.125 * kFs), n = static_cast<int>(0.05 * kFs);
            const double low = dB(amp(y, from, n, 60.0) / amp(x, from, n, 60.0)), high = dB(amp(y, from, n, 6000.0) / amp(x, from, n, 6000.0));
            logMessage("  in the gap: 60 Hz " + juce::String(low, 2) + " dB, 6 kHz " + juce::String(high, 1) + " dB");
            expect(std::abs(low) <= 0.5 && high <= -30.0);
        }

        beginTest("Page: SIMPLE shown for a new instance; GENERATE, GENRE and the page switch reach the processor");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(ed.get());
            expect(editor != nullptr);
            if (editor == nullptr)
                return;
            auto* page = editor->getSimplePage();
            auto* advanced = dynamic_cast<hpeq::ui::HamloRevolutionPanel*>(editor->getPanel());
            expect(page != nullptr && advanced != nullptr);
            if (page == nullptr || advanced == nullptr)
                return;
            expect(page->isVisible() && !advanced->isVisible());
            const auto seed = p->getRevolutionState().seed;
            page->pressGenerate();
            expect(p->getRevolutionState().seed != seed, "GENERATE: a new seed");
            expect(!p->lastSimple().parameters.empty());
            page->chooseGenre(simple::DnB);
            expectEquals(p->lastSimple().genre, static_cast<int>(simple::DnB));
            page->setKnob(hpeq::ui::HamloRevolutionSimplePage::Amount, 0.9);
            expectWithinAbsoluteError(p->parameterValue("simpleAmount"), 0.9, 1.0e-4);
            page->pressAdvanced();
            expect(p->getPage() == ModulePluginProcessor::Page::Advanced && advanced->isVisible() && !page->isVisible());
            advanced->onSimple();
            expect(p->getPage() == ModulePluginProcessor::Page::Simple && page->isVisible());
        }
    }
};

static SimpleTests simpleTests;
} // namespace hamlo::test
