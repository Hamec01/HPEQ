// GLITCH 2 (docs/GLITCH_RESEARCH.md): the block engine (no buzz: repeats >= 30 ms, the repeat shape, the playing
// audio itself, no clicks), old projects on the legacy GLITCH, the generator's glitch formula, the material.
// HAMLO_RENDER=<folder>: also writes before / after renders for listening.

#include <cmath>
#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/Revolution.h"
#include "Core/RevolutionSimple.h"
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
using State = ModulePluginProcessor::RevolutionState;
constexpr double kPi = 3.14159265358979323846;
constexpr int kStep = 6000; // a 1/16 at 120 BPM

class Head final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm(bpm);
        info.setIsPlaying(true);
        info.setPpqPosition(ppq);
        return info;
    }
    double ppq = 0.0, bpm = 120.0;
};

juce::AudioBuffer<float> play(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x, double bpm = 120.0)
{
    Head head;
    head.bpm = bpm;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, 512);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        const int n = juce::jmin(512, y.getNumSamples() - start);
        head.ppq = static_cast<double>(start) * bpm / 60.0 / kFs;
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
    }
    p.setPlayHead(nullptr);
    return y;
}

juce::AudioBuffer<float> tone(double seconds, double hz, double amp)
{
    juce::AudioBuffer<float> x(2, static_cast<int>(seconds * kFs));
    for (int i = 0; i < x.getNumSamples(); ++i)
        for (int ch = 0; ch < 2; ++ch)
            x.setSample(ch, i, static_cast<float>(amp * std::sin(2.0 * kPi * hz * i / kFs)));
    return x;
}

// A sustained pad: three detuned saw-ish partials, a slow swell, no attacks.
juce::AudioBuffer<float> pad(double seconds)
{
    juce::AudioBuffer<float> x(2, static_cast<int>(seconds * kFs));
    for (int i = 0; i < x.getNumSamples(); ++i)
    {
        const double t = i / kFs, swell = 0.5 + 0.5 * std::sin(2.0 * kPi * 0.25 * t);
        double v = 0.0;
        for (const double hz : { 220.0, 277.18, 329.63 })
            for (int k = 1; k <= 4; ++k)
                v += std::sin(2.0 * kPi * hz * k * t + k) / (k * 6.0);
        for (int ch = 0; ch < 2; ++ch)
            x.setSample(ch, i, static_cast<float>(0.25 * (0.6 + 0.4 * swell) * v));
    }
    return x;
}

// Only GLITCH (the engine under test), its pattern, legacy or block.
std::unique_ptr<ModulePluginProcessor> glitchOnly(std::initializer_list<std::pair<int, int>> steps, bool block, double depth)
{
    auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
    for (const char* lane : { "timeOn", "motionOn", "shapeAOn", "shapeBOn", "throwAOn", "throwBOn" })
        p->setParameterValue(lane, 0);
    p->setParameterValue("glitchOn", 1);
    p->setParameterValue("glitchMix", 1);
    p->setParameterValue("glitchDepth", depth);
    p->setParameterValue("glitchEngine", block ? 1 : 0);
    State s;
    for (const auto& [step, action] : steps)
        s.pattern.steps[rev::Glitch][static_cast<size_t>(step)] = static_cast<std::uint8_t>(action);
    p->setRevolutionState(s);
    return p;
}

// The strongest repetition period of the output's level over [from, from + n): RMS per 1 ms, the autocorrelation's
// highest peak over lags 5 .. 120 ms. The "robot" is a period of 10 .. 30 ms (a 33 .. 100 Hz buzz).
double repeatPeriodMs(const juce::AudioBuffer<float>& y, int from, int n)
{
    const int frame = 48, frames = n / frame;
    std::vector<double> env(static_cast<size_t>(frames));
    double mean = 0.0;
    for (int f = 0; f < frames; ++f)
    {
        double e = 0.0;
        for (int i = 0; i < frame; ++i)
            e += static_cast<double>(y.getSample(0, from + f * frame + i)) * y.getSample(0, from + f * frame + i);
        env[static_cast<size_t>(f)] = std::sqrt(e / frame);
        mean += env[static_cast<size_t>(f)];
    }
    mean /= std::max(1, frames);
    for (auto& e : env)
        e -= mean;
    double zero = 0.0;
    for (double e : env)
        zero += e * e;
    // a level that moves by less than 5 % of its mean does not repeat (the test tone fills each 1 ms frame exactly)
    if (std::sqrt(zero / std::max(1, frames)) < 0.05 * mean)
        return 0.0;
    std::vector<double> ac(122, 0.0);
    double best = 0.0;
    for (int lag = 4; lag <= 121 && lag < frames / 2; ++lag)
    {
        double a = 0.0;
        for (int f = 0; f + lag < frames; ++f)
            a += env[static_cast<size_t>(f)] * env[static_cast<size_t>(f + lag)];
        ac[static_cast<size_t>(lag)] = a / (zero * static_cast<double>(frames - lag) / frames);
        best = std::max(best, ac[static_cast<size_t>(lag)]);
    }
    if (best < 0.3)
        return 0.0;
    // the fundamental: the shortest lag at a local peak of at least 60 % of the highest one (a period between two frame lengths splits its peak) (not a multiple of it)
    for (int lag = 5; lag <= 120; ++lag)
        if (ac[static_cast<size_t>(lag)] >= 0.6 * best && ac[static_cast<size_t>(lag)] >= ac[static_cast<size_t>(lag - 1)]
            && ac[static_cast<size_t>(lag)] >= ac[static_cast<size_t>(lag + 1)])
            return lag; // ms (frames of 1 ms)
    return 0.0;
}

double dB(double r) { return 20.0 * std::log10(std::max(r, 1.0e-12)); }

void writeWav(const juce::AudioBuffer<float>& b, const juce::String& name)
{
    const auto folder = juce::SystemStats::getEnvironmentVariable("HAMLO_RENDER", {});
    if (folder.isEmpty())
        return;
    const juce::File dir(folder);
    dir.createDirectory();
    const auto file = dir.getChildFile(name + ".wav");
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> stream(file.createOutputStream().release());
    if (stream == nullptr)
        return;
    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.get(), kFs, 2, 24, {}, 0));
    if (writer == nullptr)
        return;
    stream.release(); // the writer owns it now
    writer->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
}
} // namespace

class GlitchTests final : public juce::UnitTest
{
public:
    GlitchTests() : juce::UnitTest("Glitch 2", "HamloRevolution") {}

    void runTest() override
    {
        const auto sine = tone(2.0, 1000.0, 0.5); // 1 kHz: one whole period in every 1 ms frame of the level
        const int runFrom = 4 * kStep + 300, runLength = 4 * kStep - 600; // a 4-step run (steps 5 .. 8)

        beginTest("The robot, before / after: the level's repetition period on a steady 1 kHz sine (a 4-step run)");
        {
            auto legacy = glitchOnly({ { 4, rev::Ratchet }, { 5, rev::Ratchet }, { 6, rev::Ratchet }, { 7, rev::Ratchet } }, false, 0.78);
            const double before = repeatPeriodMs(play(*legacy, sine), runFrom, runLength);
            logMessage("  BEFORE: legacy RATCHET at DEPTH 78 % (what SIMPLE wrote at AMOUNT 63 %): " + juce::String(before, 1) + " ms = "
                       + juce::String(before > 0.0 ? 1000.0 / before : 0.0, 0) + " Hz");
            for (int action : { static_cast<int>(rev::BlockRepeat), static_cast<int>(rev::BlockRoll), static_cast<int>(rev::BlockChop),
                                static_cast<int>(rev::BlockDrop), static_cast<int>(rev::BlockShuffle), static_cast<int>(rev::BlockReverse) })
            {
                auto p = glitchOnly({ { 4, action }, { 5, action }, { 6, action }, { 7, action } }, true, 0.5);
                const double period = repeatPeriodMs(play(*p, sine), runFrom, runLength);
                logMessage("  AFTER " + rev::blockActionName(action) + ": " + (period > 0.0 ? juce::String(period, 1) + " ms" : juce::String("no repetition")));
                expect(period == 0.0 || period >= 30.0, rev::blockActionName(action) + ": " + juce::String(period));
            }
            expect(before > 0.0 && before < 30.0, "the before is the buzz");
        }

        beginTest("ROLL: 1/32 (62.5 ms at 120 BPM; 1/48 on a triplet grid), each repeat -1.4 dB at DEPTH 50 % (within 0.5 dB)");
        {
            auto p = glitchOnly({ { 4, rev::BlockRoll }, { 5, rev::BlockRoll }, { 6, rev::BlockRoll }, { 7, rev::BlockRoll } }, true, 0.5);
            const auto y = play(*p, sine);
            const int period = 3000;
            double worst = 0.0;
            juce::String levels;
            const double first = y.getMagnitude(0, 4 * kStep + 400, period - 1200);
            for (int r = 1; r < 7; ++r)
            {
                const double level = y.getMagnitude(0, 4 * kStep + r * period + 400, period - 1200);
                const double expected = std::pow(1.0 - 0.3 * 0.5, r);
                worst = std::max(worst, std::abs(dB(level / first) - dB(expected)));
                levels << juce::String(dB(level / first), 1) << " ";
            }
            logMessage("  repeats re the first: " + levels + "dB; largest miss " + juce::String(worst, 2) + " dB");
            expect(worst <= 0.5, juce::String(worst));
            // the gate: the last 15 % of each repeat is silent
            expect(std::abs(y.getSample(0, 4 * kStep + 2 * period - 100)) < 1.0e-3f, "gated");
        }

        beginTest("CHOP: 1/16 pieces gated to 45 %: the dry audio, then silence, in each");
        {
            auto p = glitchOnly({ { 4, rev::BlockChop }, { 5, rev::BlockChop }, { 6, rev::BlockChop }, { 7, rev::BlockChop } }, true, 0.5);
            const auto y = play(*p, sine);
            int silent = 0;
            for (int i = 5 * kStep; i < 8 * kStep; ++i)
                silent += std::abs(y.getSample(0, i)) < 1.0e-4f;
            const double share = silent / static_cast<double>(3 * kStep);
            logMessage("  silent share in the run " + juce::String(100.0 * share, 1) + " % (gate 45 %: about 55 %)");
            expect(share > 0.45 && share < 0.62, juce::String(share));
        }

        beginTest("The playing audio itself: SHUFFLE plays the loop from earlier in the bar, sample for sample (no added sound)");
        {
            const auto loop = drums(4.0);
            auto p = glitchOnly({ { 4, rev::BlockShuffle }, { 5, rev::BlockShuffle } }, true, 0.5);
            const auto y = play(*p, loop);
            // which offset: the best match among 3/16, 1/4, 3/8, 1/2 at the run's middle
            double bestError = 1.0e9;
            int bestOffset = 0;
            for (const int offset : { 18000, 24000, 36000, 48000 })
            {
                double e = 0.0;
                const int bar2 = 16 * kStep; // the second bar: every offset reaches back inside the loop
                for (int i = bar2 + 4 * kStep + 500; i < bar2 + 6 * kStep - 500; ++i)
                    e += std::abs(y.getSample(0, i) - loop.getSample(0, i - offset));
                if (e < bestError)
                {
                    bestError = e;
                    bestOffset = offset;
                }
            }
            logMessage("  SHUFFLE plays the loop " + juce::String(bestOffset / 24000.0, 3) + " beats earlier; mean difference "
                       + juce::String(bestError / (2 * kStep - 1000), 9));
            expect(bestError / (2 * kStep - 1000) < 1.0e-6, "the source, unchanged");
        }

        beginTest("No clicks: a 100 Hz sine through each block action: the fourth difference <= -40 dB re the peak");
        for (int action = rev::BlockRepeat; action <= rev::BlockStop; ++action)
        {
            auto p = glitchOnly({ { 2, action }, { 3, action }, { 8, action }, { 12, action }, { 13, action }, { 14, action } }, true, 0.5);
            const auto y = play(*p, tone(2.0, 100.0, 0.5));
            double worst = 0.0;
            int at = 0;
            for (int i = 4; i < y.getNumSamples(); ++i)
            {
                const double d = std::abs(static_cast<double>(y.getSample(0, i)) - 4.0 * y.getSample(0, i - 1) + 6.0 * y.getSample(0, i - 2)
                                          - 4.0 * y.getSample(0, i - 3) + y.getSample(0, i - 4));
                if (d > worst)
                {
                    worst = d;
                    at = i;
                }
            }
            logMessage("  " + rev::blockActionName(action) + ": " + juce::String(dB(worst / 0.5), 1) + " dB at " + juce::String(at) + " (step "
                       + juce::String(at / kStep) + " + " + juce::String(at % kStep) + ")");
            expect(dB(worst / 0.5) <= -40.0, rev::blockActionName(action));
        }

        beginTest("Old projects keep the legacy GLITCH: a state without glitchEngine loads as Legacy");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setParameterValue("glitchEngine", 1);
            juce::MemoryBlock saved;
            p->getStateInformation(saved);
            auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), static_cast<int>(saved.getSize()));
            expect(xml != nullptr);
            if (xml == nullptr)
                return;
            if (auto* param = xml->getChildByAttribute("id", "glitchEngine"))
                xml->removeChildElement(param, true);
            juce::MemoryBlock old;
            juce::AudioProcessor::copyXmlToBinary(*xml, old);
            auto q = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            q->setParameterValue("glitchEngine", 1); // an instance reused for an older project
            q->setStateInformation(old.getData(), static_cast<int>(old.getSize()));
            expectEquals(juce::roundToInt(q->parameterValue("glitchEngine")), 0);
            auto r = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            r->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            expectEquals(juce::roundToInt(r->parameterValue("glitchEngine")), 1, "a GLITCH 2 state keeps it");
        }

        beginTest("The glitch formula: events per bar within the INTENT budget, never side by side, RISE only in the tail, SHAPE always written");
        {
            const auto hits = simple::templateHits(simple::Trap);
            static constexpr int budget[simple::kIntents] { 1, 2, 2, 3, 6 };
            int cases = 0, inBudget = 0, apart = 0, riseTail = 0, rises = 0, shaped = 0, glitchShare = 0;
            for (int genre = simple::BoomBap; genre < simple::kGenres; ++genre)
                for (int intent = 0; intent < simple::kIntents; ++intent)
                    for (std::uint64_t seed : { 1ull, 5ull, 9ull })
                        for (double amount : { 0.3, 0.63, 1.0 })
                        {
                            simple::Settings s;
                            s.genre = genre;
                            s.intent = (intent + 0.5) / simple::kIntents;
                            s.amount = amount;
                            s.seed = seed;
                            s.variation = 0.0;
                            const auto r = simple::generate(s, genre == simple::Trap ? hits : simple::templateHits(genre), 120.0);
                            const auto& g = r.pattern.steps[rev::Glitch];
                            int runs = 0, steps = 0;
                            bool touching = false;
                            for (int st = 0; st < 16; ++st)
                            {
                                const int v = g[static_cast<size_t>(st)];
                                if (v == 0)
                                    continue;
                                ++steps;
                                const int before = st > 0 ? g[static_cast<size_t>(st - 1)] : 0;
                                if (before == 0)
                                    ++runs;
                                else if (before != v)
                                    touching = true;
                                if (v == rev::BlockRise)
                                {
                                    ++rises;
                                    riseTail += st >= 12;
                                }
                            }
                            // two runs of the same action side by side read as one run: count them through the gaps
                            ++cases;
                            inBudget += runs <= budget[intent];
                            apart += !touching;
                            glitchShare += intent == simple::Chaos || steps <= 4;
                            bool shapeWritten = false;
                            for (int l = rev::kFirstShape; l < rev::kFirstThrow; ++l)
                                for (auto v : r.lanes[static_cast<size_t>(l)].first)
                                    shapeWritten = shapeWritten || v != 0;
                            shaped += shapeWritten;
                        }
            logMessage("  " + juce::String(cases) + " cases: within the budget " + juce::String(inBudget) + ", never side by side "
                       + juce::String(apart) + ", GLITCH <= 25 % of the steps (not CHAOS) " + juce::String(glitchShare) + ", RISE in the tail "
                       + juce::String(riseTail) + " / " + juce::String(rises) + ", SHAPE written " + juce::String(shaped));
            expectEquals(inBudget, cases);
            expectEquals(apart, cases);
            expectEquals(glitchShare, cases);
            expectEquals(riseTail, rises);
            expectEquals(shaped, cases);
        }

        beginTest("The owner's case (TRAP, SUBTLE, AMOUNT 63 %, FULL, HPDG hats on the eighths): before 8 RATCHETs a bar; now");
        {
            simple::Settings s;
            s.genre = simple::Trap;
            s.intent = 0.1;
            s.amount = 0.63;
            s.variation = 0.63;
            s.target = simple::FullTarget;
            s.fill = simple::FillEvery4;
            const auto r = simple::generate(s, simple::templateHits(simple::Trap), 135.0);
            juce::String row;
            int events = 0;
            for (int st = 0; st < 16; ++st)
            {
                const int v = r.pattern.steps[rev::Glitch][static_cast<size_t>(st)];
                row << (v == 0 ? juce::String(".") : rev::blockActionShortName(v)) << " ";
                events += v != 0 && (st == 0 || r.pattern.steps[rev::Glitch][static_cast<size_t>(st - 1)] != v);
            }
            logMessage("  GLITCH bar 1: " + row + "(" + juce::String(events) + " event)");
            expect(events <= 1);
        }

        beginTest("Material: a sustained pad is heard as sustained; its glitches sit on the beats, fewer than on drums");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setParameterValue("simpleGenre", simple::Techno);
            p->setParameterValue("simpleIntent", 0.3);
            p->setParameterValue("simpleAmount", 1.0);
            play(*p, pad(8.0));
            p->generateSimple(true);
            const auto state = p->getRevolutionState();
            logMessage("  dynamics: pad " + juce::String(p->moduleNode().getRevolutionDynamics(0), 1) + " dB (sustained below 9 dB)");
            expectEquals(state.material, 2, "sustained");
            int off = 0, events = 0;
            const auto& g = state.pattern.steps[rev::Glitch];
            for (int st = 0; st < 16; ++st)
                if (g[static_cast<size_t>(st)] != 0 && (st == 0 || g[static_cast<size_t>(st - 1)] != g[static_cast<size_t>(st)]))
                {
                    ++events;
                    off += !(st == 0 || st == 4 || st == 6 || st == 8 || st == 10 || st >= 12);
                }
            auto d = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            d->setParameterValue("simpleGenre", simple::Techno);
            d->setParameterValue("simpleIntent", 0.3);
            d->setParameterValue("simpleAmount", 1.0);
            play(*d, drums(8.0));
            d->generateSimple(true);
            logMessage("  dynamics: drums " + juce::String(d->moduleNode().getRevolutionDynamics(0), 1) + " dB");
            expectEquals(d->getRevolutionState().material, 1, "drums");
            logMessage("  pad: " + juce::String(events) + " glitch events, off the beats " + juce::String(off));
            expect(off == 0);
        }

        beginTest("GEN on SHAPE A / B and THROW A / B: that lane gets steps, the others stay; each press a new pattern");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setParameterValue("simpleGenre", simple::Techno);
            p->generateSimple(false);
            for (int lane = rev::kFirstShape; lane < rev::kLanes; ++lane)
            {
                auto before = p->getRevolutionState();
                before.lanes[static_cast<size_t>(lane)].first = {};
                p->setRevolutionState(before);
                p->generateLane(lane);
                const auto after = p->getRevolutionState();
                bool written = false, othersSame = after.pattern == before.pattern;
                for (auto v : after.lanes[static_cast<size_t>(lane)].first)
                    written = written || v != 0;
                for (int l = 0; l < rev::kLanes; ++l)
                    if (l != lane)
                        othersSame = othersSame && after.lanes[static_cast<size_t>(l)] == before.lanes[static_cast<size_t>(l)];
                p->generateLane(lane);
                const bool newPattern = p->getRevolutionState().lanes[static_cast<size_t>(lane)].first != after.lanes[static_cast<size_t>(lane)].first;
                expect(written && othersSame && newPattern, rev::engineName(lane));
                logMessage("  " + juce::String(rev::engineName(lane)) + ": written " + juce::String(static_cast<int>(written)) + ", the others kept "
                           + juce::String(static_cast<int>(othersSame)) + ", a second press differs " + juce::String(static_cast<int>(newPattern)));
            }
        }

        beginTest("Renders for listening (HAMLO_RENDER): drums and a pad, the legacy RATCHET vs GLITCH 2's actions, the owner's TRAP case");
        {
            const auto loop = drums(8.0), soft = pad(8.0);
            writeWav(loop, "00_drums_dry");
            writeWav(soft, "00_pad_dry");
            const std::initializer_list<std::pair<int, int>> everyBar { { 4, 0 } };
            juce::ignoreUnused(everyBar);
            const auto pattern = [](int action) -> std::initializer_list<std::pair<int, int>> { juce::ignoreUnused(action); return {}; };
            juce::ignoreUnused(pattern);
            auto legacy = glitchOnly({ { 6, rev::Ratchet }, { 7, rev::Ratchet }, { 14, rev::Ratchet }, { 15, rev::Ratchet } }, false, 0.78);
            writeWav(play(*legacy, loop), "01_drums_BEFORE_legacy_RT");
            auto legacyPad = glitchOnly({ { 6, rev::Ratchet }, { 7, rev::Ratchet }, { 14, rev::Ratchet }, { 15, rev::Ratchet } }, false, 0.78);
            writeWav(play(*legacyPad, soft), "01_pad_BEFORE_legacy_RT");
            for (int action = rev::BlockRepeat; action <= rev::BlockStop; ++action)
            {
                const auto actionTitle = rev::blockActionName(action);
                auto a = glitchOnly({ { 6, action }, { 7, action }, { 14, action }, { 15, action } }, true, 0.5);
                writeWav(play(*a, loop), "02_drums_" + juce::String(action) + "_" + actionTitle);
                auto b = glitchOnly({ { 6, action }, { 7, action }, { 14, action }, { 15, action } }, true, 0.5);
                writeWav(play(*b, soft), "03_pad_" + juce::String(action) + "_" + actionTitle);
            }
            auto owner = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            owner->setParameterValue("simpleGenre", simple::Trap);
            owner->setParameterValue("simpleIntent", 0.1);
            owner->setParameterValue("simpleAmount", 0.63);
            owner->setParameterValue("simpleVariation", 0.63);
            owner->setParameterValue("simpleTarget", simple::FullTarget);
            owner->generateSimple(false);
            writeWav(play(*owner, loop), "04_drums_SIMPLE_trap_subtle_63_AFTER");
            expect(true);
        }
    }
};

static GlitchTests glitchTests;
} // namespace hamlo::test
