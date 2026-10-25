// Module pass 10 (owner): HamloRevolution - the Monsta FX slot as TIME / GLITCH / MOTION with six orders,
// an explicit 16-step pattern per engine and Generate -> Edit -> Lock. Thresholds fixed before the runs
// (HamloRevolution task, section 18: A01-A16, A19).

#include <chrono>
#include <cmath>
#include <functional>
#include <memory>

#include <juce_dsp/juce_dsp.h>

#include "Core/Revolution.h"
#include "DSP/ProcessorsM6.h"
#include "DSP/RevolutionFft.h"
#include "DSP/ProjectEngine.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "UI/HamloRevolutionPanel.h"
#include "UI/Theme.h"

namespace hpeq::test
{
using namespace hpeq::dsp;
namespace rev = hpeq::revolution;

namespace
{
constexpr double kFs = 48000.0;

// The slot as the project hands it to the engine (FxSlotState -> snapshot), from a cleared pattern.
FxSlotParams revSlot(const std::function<void(FxSlotState&, rev::Pattern&)>& edit = {})
{
    auto m = defaultModule(FxType::MonstaFx);
    rev::Pattern pattern;
    if (edit)
        edit(m, pattern);
    rev::writePattern(m.parameters, pattern);
    LayerState layer;
    FxNodeState node;
    node.id = 1;
    node.chain = { m };
    layer.fxNodes = { node };
    assignEngineSlots(layer);
    return makeSnapshot(layer).fx[static_cast<size_t>(layer.fxNodes[0].engineSlot)].slots[0];
}

struct RunOptions
{
    int block = 512;
    bool playing = true;
    double bpm = 120.0;
    std::function<void(FxSlotParams&, int start)> change;
    std::function<double(int start)> ppqAt; // default: start * bpm / (60 fs)
    std::uint64_t* allocations = nullptr;
    double fs = kFs;
};

juce::AudioBuffer<float> run(const juce::AudioBuffer<float>& x, FxSlotParams p, const RunOptions& o = {}, MonstaProcessor* keep = nullptr)
{
    auto own = std::make_unique<MonstaProcessor>();
    auto& m = keep != nullptr ? *keep : *own;
    if (keep == nullptr)
        m.prepare(o.fs);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += o.block)
    {
        const int n = juce::jmin(o.block, y.getNumSamples() - start);
        if (o.change)
            o.change(p, start);
        Transport t;
        t.bpm = o.bpm;
        t.playing = o.playing;
        t.hasPosition = o.playing;
        t.ppqAtBlockStart = o.ppqAt ? o.ppqAt(start) : start * o.bpm / (60.0 * o.fs);
        float* chunk[2] { y.getWritePointer(0) + start, y.getNumChannels() > 1 ? y.getWritePointer(1) + start : nullptr };
        ScopedAllocationProbe probe;
        m.process(chunk, y.getNumChannels(), n, p, t, 0);
        if (o.allocations != nullptr)
            *o.allocations += probe.counts().allocations;
    }
    return y;
}

juce::AudioBuffer<float> ramp(int samples, int channels = 2)
{
    juce::AudioBuffer<float> b(channels, samples);
    for (int ch = 0; ch < channels; ++ch)
        for (int i = 0; i < samples; ++i)
            b.setSample(ch, i, static_cast<float>(i) * 1.0e-5f * (ch == 0 ? 1.0f : -1.0f));
    return b;
}

juce::AudioBuffer<float> constant(int samples, float v)
{
    juce::AudioBuffer<float> b(2, samples);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < samples; ++i)
            b.setSample(ch, i, v);
    return b;
}

bool identical(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    if (a.getNumSamples() != b.getNumSamples() || a.getNumChannels() != b.getNumChannels())
        return false;
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        if (std::memcmp(a.getReadPointer(ch), b.getReadPointer(ch), sizeof(float) * static_cast<size_t>(a.getNumSamples())) != 0)
            return false;
    return true;
}

bool finite(const juce::AudioBuffer<float>& a)
{
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        for (int i = 0; i < a.getNumSamples(); ++i)
            if (!std::isfinite(a.getSample(ch, i)))
                return false;
    return true;
}

void setStep(rev::Pattern& p, int engine, int step, int value)
{
    p.steps[static_cast<size_t>(engine)][static_cast<size_t>(step)] = static_cast<std::uint8_t>(value);
}
} // namespace

class RevolutionPassTests final : public juce::UnitTest
{
public:
    RevolutionPassTests() : juce::UnitTest("HamloRevolution (module pass 10)", "HPEQ") {}

    void runTest() override
    {
        const auto drums = makeSyntheticDrums({ kFs, 2, 3.0, kFixtureSeed });

        beginTest("Legacy: a slot without \"mode\" is the M6 Monsta; its reset (now O(1)) renders as a fresh prepare");
        {
            FxSlotState legacy;
            legacy.type = FxType::MonstaFx;
            legacy.parameters.set("chance", 0.6);
            legacy.parameters.set("seed", 7);
            LayerState layer;
            FxNodeState node;
            node.id = 1;
            node.chain = { legacy };
            layer.fxNodes = { node };
            assignEngineSlots(layer);
            const auto p = makeSnapshot(layer).fx[static_cast<size_t>(layer.fxNodes[0].engineSlot)].slots[0];
            expectEquals(p.values[slot::kRevMode], 0.0f);
            auto m = std::make_unique<MonstaProcessor>();
            m->prepare(kFs);
            const auto first = run(drums, p, {}, m.get());
            run(makeWhiteNoise({ kFs, 2, 1.0, 5 }, -6.0), p, {}, m.get()); // fill the history with something else
            m->reset();
            expect(identical(run(drums, p, {}, m.get()), first), "reset does not behave as a cleared history");
            expect(!identical(first, drums), "legacy Monsta had no effect");
        }

        beginTest("A01: an empty pattern is bit-exact (every order, engines on, mix 1; blocks 1 / 64 / 512)");
        {
            for (int order = 0; order < rev::kOrders; ++order)
                for (int block : { 1, 64, 512 })
                {
                    const auto p = revSlot([order](FxSlotState& m, rev::Pattern&) { m.parameters.set("order", order); });
                    RunOptions o;
                    o.block = block;
                    expect(identical(run(drums, p, o), drums), "order " + juce::String(order) + " block " + juce::String(block));
                }
            // stopped transport, free mode, swing and humanize: still nothing happens without events
            const auto p = revSlot([](FxSlotState& m, rev::Pattern&) { m.parameters.set("free", 1); m.parameters.set("swing", 0.7); m.parameters.set("humanize", 1.0); });
            RunOptions o;
            o.playing = false;
            expect(identical(run(drums, p, o), drums));
        }

        beginTest("A02: engines off or at mix 0 are bit-exact with a full pattern");
        {
            const auto full = [](rev::Pattern& pat)
            {
                for (int s = 0; s < rev::kSteps; ++s)
                {
                    setStep(pat, rev::Time, s, 1 + s % 5);
                    setStep(pat, rev::Glitch, s, 1 + s % 7);
                    setStep(pat, rev::Motion, s, 15);
                }
            };
            const auto off = revSlot([&](FxSlotState& m, rev::Pattern& pat) { full(pat); for (auto* k : { "timeOn", "glitchOn", "motionOn" }) m.parameters.set(k, 0); });
            const auto zero = revSlot([&](FxSlotState& m, rev::Pattern& pat) { full(pat); for (auto* k : { "timeMix", "glitchMix", "motionMix" }) m.parameters.set(k, 0.0); });
            const auto onFull = revSlot([&](FxSlotState&, rev::Pattern& pat) { full(pat); });
            expect(identical(run(drums, off), drums), "engines off");
            expect(identical(run(drums, zero), drums), "mix 0");
            expect(!identical(run(drums, onFull), drums), "the full pattern had no effect");
        }

        beginTest("A03: block-size invariance (Destruction pattern, all engines, swing + humanize; 1/16/64/127/512/2048 + random)");
        {
            const auto p = revSlot([](FxSlotState& m, rev::Pattern& pat)
            {
                pat = rev::generate({}, 424242, rev::Destruction);
                m.parameters.set("swing", 0.4);
                m.parameters.set("humanize", 0.5);
                m.parameters.set("motionSmooth", 0.4);
                m.parameters.set("order", 3);
            });
            RunOptions o;
            o.block = 512;
            const auto reference = run(drums, p, o);
            expect(!identical(reference, drums));
            for (int block : { 1, 16, 64, 127, 2048 })
            {
                o.block = block;
                expect(identical(run(drums, p, o), reference), "block " + juce::String(block));
            }
            // random partition: the processor sees uneven blocks
            auto m = std::make_unique<MonstaProcessor>();
            m->prepare(kFs);
            auto y = drums;
            juce::Random r(9);
            for (int start = 0; start < y.getNumSamples();)
            {
                const int n = juce::jmin(1 + r.nextInt(700), y.getNumSamples() - start);
                Transport t;
                t.playing = t.hasPosition = true;
                t.ppqAtBlockStart = start * 120.0 / (60.0 * kFs);
                float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                m->process(chunk, 2, n, p, t, 0);
                start += n;
            }
            expect(identical(y, reference), "random partition");
        }

        beginTest("A05: grid - GAP on step 5 (1/16 at 120 BPM = 6000 samples) starts at sample 24000; swing moves odd steps");
        {
            const int fade = static_cast<int>(2.0e-3 * kFs);
            const auto gapAt = [](int step, double swing)
            {
                return revSlot([=](FxSlotState& m, rev::Pattern& pat) { setStep(pat, rev::Glitch, step, rev::Gap); m.parameters.set("swing", swing); });
            };
            const auto y = run(constant(60000, 0.5f), gapAt(4, 0.0));
            expectEquals(y.getSample(0, 23999), 0.5f);
            expectEquals(y.getSample(0, 24000 + fade), 0.0f);
            expectEquals(y.getSample(0, 29999 - fade), 0.0f);
            expectEquals(y.getSample(0, 30000 + fade), 0.5f);
            // swing 1 (75 %): step 2 (index 1) starts at 0.75 x 12000 = 9000
            const auto s = run(constant(30000, 0.5f), gapAt(1, 1.0));
            expectEquals(s.getSample(0, 8999), 0.5f);
            expectEquals(s.getSample(0, 9000 + fade), 0.0f);
            expectEquals(s.getSample(0, 11999 - fade), 0.0f);
            // the same at 44.1 kHz, 93 BPM (a step of 7112.9 samples): the gap begins within one sample
            const double fs = 44100.0, step = 0.25 * 60.0 / 93.0 * fs;
            RunOptions o;
            o.fs = fs;
            o.bpm = 93.0;
            const auto z = run(constant(60000, 0.5f), gapAt(4, 0.0), o);
            const int expected = static_cast<int>(std::ceil(4.0 * step));
            int first = -1;
            for (int i = 20000; i < 40000 && first < 0; ++i)
                if (z.getSample(0, i) < 0.5f)
                    first = i;
            logMessage("  93 BPM: gap fade starts at " + juce::String(first) + ", step boundary " + juce::String(4.0 * step, 2));
            expect(std::abs(first - expected) <= 1);
        }

        beginTest("A07: REVERSE plays the window before the run exactly backwards (ramp input)");
        {
            const auto p = revSlot([](FxSlotState&, rev::Pattern& pat) { setStep(pat, rev::Time, 1, rev::TimeReverse); });
            const auto x = ramp(24000);
            const auto y = run(x, p);
            const int fade = static_cast<int>(2.0e-3 * kFs);
            bool exact = true;
            for (int j = fade + 1; j < 5900; ++j)
                exact = exact && y.getSample(0, 6000 + j) == x.getSample(0, 5999 - j) && y.getSample(1, 6000 + j) == x.getSample(1, 5999 - j);
            expect(exact);
        }

        beginTest("A08: TAPE HALF reads the run's first half at half speed (linear input: exact within 1e-6), never ahead of the writer");
        {
            const auto p = revSlot([](FxSlotState& m, rev::Pattern& pat) { m.parameters.set("timeStretch", 0); setStep(pat, rev::Time, 1, rev::Half); setStep(pat, rev::Time, 2, rev::Half); });
            const auto x = ramp(30000);
            const auto y = run(x, p);
            double worst = 0.0;
            bool causal = true;
            for (int j = 200; j < 11800; ++j)
            {
                const double expected = (6000.0 + 0.5 * j - 2.0) * 1.0e-5; // age 0.5 u + 2
                worst = std::max(worst, std::abs(y.getSample(0, 6000 + j) - expected));
                causal = causal && y.getSample(0, 6000 + j) <= x.getSample(0, 6000 + j);
            }
            logMessage("  HALF worst error on the ramp: " + juce::String(worst, 9));
            expect(worst < 1.0e-6 && causal);
        }

        beginTest("A09: TAPE STOP ends silent (no DC hold); FREEZE loops what came before; the run then returns to dry");
        {
            const auto stop = revSlot([](FxSlotState&, rev::Pattern& pat) { for (int s = 4; s < 8; ++s) setStep(pat, rev::Time, s, rev::TapeStop); });
            const auto y = run(constant(60000, 0.5f), stop);
            float tail = 0.0f;
            for (int i = 47900; i < 47990; ++i)
                tail = std::max(tail, std::abs(y.getSample(0, i)));
            logMessage("  TAPE STOP: largest of the last 100 samples of the run " + juce::String(tail, 5));
            expect(tail < 0.01f);
            expectEquals(y.getSample(0, 48000 + 200), 0.5f);

            // FREEZE on step 2 (depth 1: loop = half a step, 3000 samples) of a ramp: values repeat with period 3000
            const auto freeze = revSlot([](FxSlotState&, rev::Pattern& pat) { setStep(pat, rev::Time, 1, rev::Freeze); });
            const auto f = run(ramp(24000), freeze);
            bool periodic = true;
            for (int j = 200; j < 2700; ++j)
                periodic = periodic && f.getSample(0, 6000 + j) == f.getSample(0, 9000 + j);
            expect(periodic);
            expect(f.getSample(0, 6000 + 200) == ramp(24000).getSample(0, 3000 + 200), "freeze does not loop the material before the run");
        }

        beginTest("Causality: after a transport jump the old history is never read (REPEAT on every step, silence after the jump)");
        {
            const auto p = revSlot([](FxSlotState&, rev::Pattern& pat) { for (int s = 0; s < rev::kSteps; ++s) setStep(pat, rev::Glitch, s, rev::Repeat); });
            static constexpr int jump = 94 * 512; // the seek lands on a block start (the host reports positions per block)
            auto x = makeWhiteNoise({ kFs, 2, 2.0, 3 }, -6.0);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = jump; i < x.getNumSamples(); ++i)
                    x.setSample(ch, i, 0.0f);
            RunOptions o;
            o.ppqAt = [](int start) { return start < jump ? start * 120.0 / (60.0 * kFs) : 64.0 + (start - jump) * 120.0 / (60.0 * kFs); };
            const auto y = run(x, p, o);
            float after = 0.0f;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = jump; i < y.getNumSamples(); ++i)
                    after = std::max(after, std::abs(y.getSample(ch, i)));
            expectEquals(after, 0.0f);
        }

        beginTest("Orders: TIME -> GLITCH and GLITCH -> TIME differ; an order change mid-play fades (no jump > the signal's own)");
        {
            const auto both = [](int order)
            {
                return revSlot([order](FxSlotState& m, rev::Pattern& pat)
                {
                    m.parameters.set("order", order);
                    for (int s = 0; s < rev::kSteps; s += 2)
                        setStep(pat, rev::Time, s, rev::Half);
                    for (int s = 0; s < rev::kSteps; s += 3)
                        setStep(pat, rev::Glitch, s, rev::Stutter);
                });
            };
            const auto tg = run(drums, both(0)), gt = run(drums, both(2));
            expect(!identical(tg, gt));
            RunOptions o;
            o.change = [](FxSlotParams& p, int start) { p.values[slot::kRevOrder] = start >= 48000 ? 4.0f : 0.0f; };
            const auto sw = run(drums, both(0), o);
            expect(finite(sw));
            float jump = 0.0f, inputJump = 0.0f;
            for (int i = 47000; i < 52000; ++i)
            {
                jump = std::max(jump, std::abs(sw.getSample(0, i) - sw.getSample(0, i - 1)));
                inputJump = std::max(inputJump, std::abs(drums.getSample(0, i) - drums.getSample(0, i - 1)));
            }
            logMessage("  order change: largest step " + juce::String(jump, 4) + " (input " + juce::String(inputJump, 4) + ")");
            expect(jump <= inputJump * 1.5f + 1.0e-3f);
        }

        beginTest("MOTION: volume level 15 at depth 1 silences the step; low pass at level 0 is exact; width 15 collapses to mono");
        {
            const auto vol = revSlot([](FxSlotState& m, rev::Pattern& pat) { m.parameters.set("motionTarget", 0); m.parameters.set("motionDepth", 1.0); m.parameters.set("motionSmooth", 0.0); setStep(pat, rev::Motion, 2, 15); });
            const auto y = run(constant(30000, 0.5f), vol);
            expectEquals(y.getSample(0, 12000 + 200), 0.0f);
            expectEquals(y.getSample(0, 6000), 0.5f);
            const auto lp = revSlot([](FxSlotState& m, rev::Pattern&) { m.parameters.set("motionTarget", 1); });
            expect(identical(run(drums, lp), drums), "low pass at level 0");
            const auto width = revSlot([](FxSlotState& m, rev::Pattern& pat) { m.parameters.set("motionTarget", 3); m.parameters.set("motionDepth", 1.0); m.parameters.set("motionSmooth", 0.0); setStep(pat, rev::Motion, 0, 15); });
            const auto w = run(drums, width);
            float side = 0.0f;
            for (int i = 300; i < 5800; ++i)
                side = std::max(side, std::abs(w.getSample(0, i) - w.getSample(1, i)));
            expect(side < 1.0e-6f);
        }

        beginTest("A10 / A14: 44.1 / 96 / 192 kHz, mono, silence, large input, an injected NaN: finite output, empty pattern still exact");
        {
            for (double fs : { 44100.0, 96000.0, 192000.0 })
            {
                RunOptions o;
                o.fs = fs;
                const auto x = makeSyntheticDrums({ fs, 2, 1.0, kFixtureSeed });
                expect(identical(run(x, revSlot(), o), x));
                const auto p = revSlot([](FxSlotState&, rev::Pattern& pat) { pat = rev::generate({}, 99, rev::Destruction); });
                expect(finite(run(x, p, o)), juce::String(fs));
            }
            juce::AudioBuffer<float> mono(1, 48000);
            for (int i = 0; i < 48000; ++i)
                mono.setSample(0, i, drums.getSample(0, i));
            const auto p = revSlot([](FxSlotState&, rev::Pattern& pat) { pat = rev::generate({}, 5, rev::Experimental); });
            expect(finite(run(mono, p)));
            auto loud = drums;
            loud.applyGain(1000.0f);
            expect(finite(run(loud, p)));
            auto bad = drums;
            bad.setSample(0, 1000, std::numeric_limits<float>::quiet_NaN());
            auto m = std::make_unique<MonstaProcessor>();
            m->prepare(kFs);
            run(bad, p, {}, m.get());
            m->reset(); // a reset (pool reuse, seek) recovers
            expect(finite(run(drums, p, {}, m.get())));
        }

        beginTest("A12: no allocation on the audio thread (all engines, order changes, seek)");
        {
            const auto p = revSlot([](FxSlotState&, rev::Pattern& pat) { pat = rev::generate({}, 31337, rev::Destruction); });
            std::uint64_t allocations = 0;
            RunOptions o;
            o.block = 64;
            o.allocations = &allocations;
            o.change = [](FxSlotParams& q, int start) { q.values[slot::kRevOrder] = static_cast<float>((start / 24000) % 6); };
            o.ppqAt = [](int start) { return (start % 50000) * 120.0 / (60.0 * kFs); };
            run(drums, p, o);
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("A16: Generate on 10000 seeds x 4 characters: locks kept, forbidden actions absent, event limits, deterministic, locks do not shift other steps");
        {
            int violations = 0, lockChanges = 0, forbidden = 0, overLimit = 0, nonDeterministic = 0, shifted = 0;
            rev::Pattern base;
            setStep(base, rev::Time, 3, rev::Freeze);
            setStep(base, rev::Glitch, 7, rev::Gap);
            setStep(base, rev::Motion, 0, 9);
            base.locks = { 1u << 3, 1u << 7, 1u << 0 };
            base.allow[rev::Time] = static_cast<std::uint16_t>(0xffff & ~(1u << rev::Double));
            base.allow[rev::Glitch] = static_cast<std::uint16_t>(0xffff & ~(1u << rev::Scratch) & ~(1u << rev::Jump));
            for (std::uint64_t seed = 1; seed <= 10000; ++seed)
                for (int c = 0; c < rev::kCharacters; ++c)
                {
                    const auto g = rev::generate(base, seed, c);
                    nonDeterministic += g == rev::generate(base, seed, c) ? 0 : 1;
                    lockChanges += g.steps[rev::Time][3] == rev::Freeze && g.steps[rev::Glitch][7] == rev::Gap && g.steps[rev::Motion][0] == 9 ? 0 : 1;
                    for (int s = 0; s < rev::kSteps; ++s)
                    {
                        forbidden += g.steps[rev::Time][static_cast<size_t>(s)] == rev::Double ? 1 : 0;
                        forbidden += g.steps[rev::Glitch][static_cast<size_t>(s)] == rev::Scratch || g.steps[rev::Glitch][static_cast<size_t>(s)] == rev::Jump ? 1 : 0;
                        violations += g.steps[rev::Time][static_cast<size_t>(s)] >= rev::kTimeActions || g.steps[rev::Glitch][static_cast<size_t>(s)] >= rev::kGlitchActions ? 1 : 0;
                    }
                    for (int e = 0; e < 2; ++e)
                    {
                        int events = 0;
                        for (auto v : g.steps[static_cast<size_t>(e)])
                            events += v != 0 ? 1 : 0;
                        overLimit += events > rev::maxEvents(e, c) ? 1 : 0;
                    }
                    if (c == rev::Destruction) // no limit cuts at 16: locking one more step must leave every other step as it was
                    {
                        auto more = base;
                        more.locks[rev::Glitch] = static_cast<std::uint16_t>(more.locks[rev::Glitch] | (1u << 2));
                        const auto h = rev::generate(more, seed, c);
                        for (int e = 0; e < rev::kEngines; ++e)
                            for (int s = 0; s < rev::kSteps; ++s)
                                if (!(e == rev::Glitch && s == 2))
                                    shifted += h.steps[static_cast<size_t>(e)][static_cast<size_t>(s)] == g.steps[static_cast<size_t>(e)][static_cast<size_t>(s)] ? 0 : 1;
                    }
                }
            logMessage("  violations " + juce::String(violations) + ", lock changes " + juce::String(lockChanges) + ", forbidden " + juce::String(forbidden)
                       + ", over limit " + juce::String(overLimit) + ", non-deterministic " + juce::String(nonDeterministic) + ", shifted " + juce::String(shifted));
            expect(violations == 0 && lockChanges == 0 && forbidden == 0 && overLimit == 0 && nonDeterministic == 0 && shifted == 0);
            // the characters really differ in density
            double density[rev::kCharacters] {};
            for (std::uint64_t seed = 1; seed <= 500; ++seed)
                for (int c = 0; c < rev::kCharacters; ++c)
                    for (auto v : rev::generate({}, seed, c).steps[rev::Glitch])
                        density[c] += v != 0 ? 1.0 : 0.0;
            logMessage("  GLITCH events per pattern: " + juce::String(density[0] / 500, 2) + " / " + juce::String(density[1] / 500, 2) + " / "
                       + juce::String(density[2] / 500, 2) + " / " + juce::String(density[3] / 500, 2));
            expect(density[0] < density[1] && density[1] < density[2] && density[2] < density[3]);
        }

        beginTest("A15: pattern, locks, allow masks and the seed round-trip through the saved state; the default module is HamloRevolution");
        {
            FxProjectState project;
            FxNodeState node;
            node.id = 1;
            auto m = defaultModule(FxType::MonstaFx);
            auto pat = rev::generate({}, 77, rev::Experimental);
            pat.locks = { 0x00f0, 0x1234, 0x8001 };
            pat.allow = { 0x003e, 0x00fe, 0xffff };
            rev::writePattern(m.parameters, pat);
            m.parameters.set("revSeed", static_cast<juce::int64>(98765432));
            node.chain = { m };
            project.layers[0].fxNodes = { node };
            project.nextNodeId = 2;
            sanitize(project);
            const auto xml = toValueTree(project).createXml();
            const auto back = fromValueTree(juce::ValueTree::fromXml(*xml));
            expect(back.has_value());
            if (back)
            {
                const auto& restored = back->layers[0].fxNodes[0].chain[0];
                expect(rev::readPattern(restored.parameters) == pat);
                expectEquals(static_cast<juce::int64>(restored.parameters["revSeed"]), static_cast<juce::int64>(98765432));
                expectEquals(static_cast<int>(restored.parameters["mode"]), 1);
            }
            expect(ui::fxName(FxType::MonstaFx) == juce::String("HamloRevolution"));
            expectEquals(FxNodeProcessor::slotLatency(revSlot(), kFs), 0); // nothing for the plug-in delay compensation
        }

        beginTest("In the project: a HamloRevolution region renders, finite; module mix 0 is the region with the module off");
        {
            const auto render = [&](float mix, bool enabled)
            {
                FxProjectState project;
                FxNodeState node;
                node.id = 1;
                node.lowHz = 200.0;
                node.highHz = 5000.0;
                auto m = defaultModule(FxType::MonstaFx);
                m.mix = mix;
                m.enabled = enabled;
                node.chain = { m };
                project.layers[0].fxNodes = { node };
                project.nextNodeId = 2;
                sanitize(project);
                auto e = std::make_unique<ProjectEngine>();
                e->prepare(kFs, 512);
                e->publish(project);
                auto y = drums;
                for (int start = 0; start < y.getNumSamples(); start += 512)
                {
                    float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    e->process(c, 2, juce::jmin(512, y.getNumSamples() - start));
                }
                return y;
            };
            const auto wet = render(1.0f, true), dry = render(0.0f, true), off = render(1.0f, false);
            expect(finite(wet));
            expect(!identical(wet, dry));
            const double error = compareAligned(dry, off).errorDb;
            logMessage("  mix 0 vs module off: " + juce::String(error, 1) + " dB");
            expect(error < -120.0, "module mix 0 changes the band");
        }

        beginTest("Page: GENERATE / NEW SEED / step clicks / locks send the pattern; UPGRADE on a legacy slot");
        {
            ui::HamloRevolutionPanel panel;
            panel.setSize(1180, 700);
            rev::Pattern sent;
            juce::int64 sentSeed = 0;
            juce::String what;
            panel.onPatternChange = [&](const rev::Pattern& p, juce::int64 seed, const rev::Feel&, const juce::String& w) { sent = p; sentSeed = seed; what = w; };
            auto v = panel.getValues();
            v.seed = 123;
            v.switches[ui::HamloRevolutionPanel::Character] = rev::Musical;
            panel.setValues(v);
            panel.pressGenerate();
            expect(sent == rev::generate(v.pattern, 123, rev::Musical) && sentSeed == 123 && what == "Generate");
            panel.pressNewSeed();
            expect(sentSeed == static_cast<juce::int64>(rev::nextSeed(123)));
            const auto before = panel.getValues().pattern.steps[rev::Time][5];
            panel.clickStep(rev::Time, 5);
            expectEquals(static_cast<int>(sent.steps[rev::Time][5]), (before + 1) % rev::kTimeActions);
            panel.clickStep(rev::Glitch, 9, true);
            expect(((sent.locks[rev::Glitch] >> 9) & 1) == 1);
            panel.setMotionLevel(3, 12);
            expectEquals(static_cast<int>(sent.steps[rev::Motion][3]), 12);
            bool upgraded = false;
            panel.onUpgrade = [&] { upgraded = true; };
            v = panel.getValues();
            v.legacy = true;
            panel.setValues(v);
            panel.mouseDown(juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), { 590.0f, 565.0f }, juce::ModifierKeys(), 0.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, &panel, &panel, juce::Time::getCurrentTime(), { 590.0f, 565.0f }, juce::Time::getCurrentTime(), 1, false));
            expect(upgraded);
        }
    }
};

static RevolutionPassTests revolutionPassTests;
// Pass 10b (owner: "доделываем"): pitch-preserving TIME (RHYTHM = WSOLA, CLEAN = phase vocoder), C-ATD
// splices, the attack analysis per step and Generate with LISTEN, CPU per callback. Thresholds fixed
// before the runs (task section 18: A17, A18-A20, A12, A16, A22; section 17.3 CPU).
namespace
{
juce::AudioBuffer<float> sine(double hz, int samples, double phaseR = 0.0, float amp = 0.5f, double fs = kFs)
{
    juce::AudioBuffer<float> b(2, samples);
    for (int i = 0; i < samples; ++i)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * hz * i / fs;
        b.setSample(0, i, amp * static_cast<float>(std::sin(w)));
        b.setSample(1, i, amp * static_cast<float>(std::sin(w + phaseR)));
    }
    return b;
}

// Frequency from interpolated rising zero crossings between `from` and `to` (channel 0).
double crossingHz(const juce::AudioBuffer<float>& y, int from, int to, double fs = kFs)
{
    double first = -1.0, last = -1.0;
    int count = 0;
    for (int i = from + 1; i < to; ++i)
    {
        const float a = y.getSample(0, i - 1), b = y.getSample(0, i);
        if (a < 0.0f && b >= 0.0f)
        {
            const double t = (i - 1) + a / (a - b);
            if (first < 0.0)
                first = t;
            last = t;
            ++count;
        }
    }
    return count > 1 ? (count - 1) * fs / (last - first) : 0.0;
}

// Phase of the `hz` component of a channel over [from, to) (projection on sin / cos).
double phaseAt(const juce::AudioBuffer<float>& y, int ch, double hz, int from, int to)
{
    double s = 0.0, c = 0.0;
    for (int i = from; i < to; ++i)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * hz * i / kFs;
        s += y.getSample(ch, i) * std::sin(w);
        c += y.getSample(ch, i) * std::cos(w);
    }
    return std::atan2(c, s);
}

double rmsOf(const juce::AudioBuffer<float>& y, int from, int to)
{
    double e = 0.0;
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
        for (int i = from; i < to; ++i)
            e += static_cast<double>(y.getSample(ch, i)) * y.getSample(ch, i);
    return std::sqrt(e / (y.getNumChannels() * static_cast<double>(to - from)));
}

double cents(double hz, double reference) { return 1200.0 * std::log2(hz / reference); }
} // namespace

class RevolutionStretchTests final : public juce::UnitTest
{
public:
    RevolutionStretchTests() : juce::UnitTest("HamloRevolution 10b (stretch, joins, listen)", "HPEQ") {}

    void runTest() override
    {
        const auto halfRun = [](int stretch, int action = rev::Half)
        {
            return revSlot([=](FxSlotState& m, rev::Pattern& pat)
            {
                m.parameters.set("timeStretch", stretch);
                for (int s = 4; s < 12; ++s) // the run starts after half a second of history
                    setStep(pat, rev::Time, s, action);
            });
        };

        beginTest("A17: HALF keeps the pitch with RHYTHM and CLEAN (440 Hz: <= 5 cents, level +-1 dB); TAPE drops an octave");
        {
            const auto x = sine(440.0, 96000, 0.0);
            for (int stretch : { 1, 2 })
            {
                const auto y = run(x, halfRun(stretch));
                expect(!identical(y, x), "HALF played dry");
                const double hz = crossingHz(y, 36000, 60000), level = 20.0 * std::log10(rmsOf(y, 36000, 60000) / rmsOf(x, 36000, 60000));
                logMessage("  " + juce::String(stretch == 1 ? "RHYTHM" : "CLEAN") + ": " + juce::String(cents(hz, 440.0), 2) + " cents, level "
                           + juce::String(level, 2) + " dB");
                expect(std::abs(cents(hz, 440.0)) <= 5.0 && std::abs(level) <= 1.0, juce::String(stretch));
            }
            const auto tape = run(x, halfRun(0));
            const double hz = crossingHz(tape, 36000, 60000);
            logMessage("  TAPE: " + juce::String(cents(hz, 440.0), 1) + " cents");
            expect(std::abs(cents(hz, 440.0) + 1200.0) <= 5.0);
        }

        beginTest("A17: 2X keeps the pitch with CLEAN and RHYTHM (<= 5 cents)");
        {
            // 2X on steps 13..16: the run plays the window before it (steps 9..12) and itself at twice the speed
            const auto x = sine(330.0, 100000, 0.0);
            for (int stretch : { 1, 2 })
            {
                const auto p = revSlot([stretch](FxSlotState& m, rev::Pattern& pat)
                {
                    m.parameters.set("timeStretch", stretch);
                    for (int s = 12; s < 16; ++s)
                        setStep(pat, rev::Time, s, rev::Double);
                });
                const auto y = run(x, p);
                expect(!identical(y, x), "2X played dry");
                const double c = cents(crossingHz(y, 76000, 94000), 330.0);
                logMessage("  2X " + juce::String(stretch == 1 ? "RHYTHM" : "CLEAN") + ": " + juce::String(c, 2) + " cents");
                expect(std::abs(c) <= 5.0, juce::String(stretch));
            }
        }

        beginTest("A19: CLEAN keeps the channels' phase difference (30 degrees: +-5)");
        {
            const double diff = juce::MathConstants<double>::pi / 6.0;
            const auto x = sine(440.0, 96000, diff);
            const auto y = run(x, halfRun(2));
            double measured = phaseAt(y, 1, 440.0, 44000, 54000) - phaseAt(y, 0, 440.0, 44000, 54000);
            measured = std::atan2(std::sin(measured), std::cos(measured));
            logMessage("  phase difference out: " + juce::String(measured * 180.0 / juce::MathConstants<double>::pi, 2) + " degrees");
            expect(std::abs(measured - diff) * 180.0 / juce::MathConstants<double>::pi <= 5.0);
        }

        beginTest("Stretch consumes the source at half speed: clicks every 6000 samples come out every 12000 (+-2 %)");
        {
            juce::AudioBuffer<float> clicks(2, 96000);
            clicks.clear();
            for (int i = 0; i < 96000; i += 6000)
                for (int ch = 0; ch < 2; ++ch)
                    for (int k = 0; k < 24; ++k) // a short decaying burst, so WSOLA has something to align
                        clicks.setSample(ch, i + k, 0.8f * std::exp(-0.15f * k) * (k % 2 == 0 ? 1.0f : -0.6f));
            for (int stretch : { 1, 2 })
            {
                const auto y = run(clicks, halfRun(stretch));
                std::vector<int> peaks;
                for (int i = 28000; i < 71000; ++i)
                {
                    float m = 0.0f;
                    for (int k = -300; k <= 300; ++k)
                        m = std::max(m, std::abs(y.getSample(0, juce::jlimit(0, 95999, i + k))));
                    if (std::abs(y.getSample(0, i)) == m && m > 0.2f && (peaks.empty() || i - peaks.back() > 3000))
                        peaks.push_back(i);
                }
                double spacing = 0.0;
                for (size_t i = 1; i < peaks.size(); ++i)
                    spacing += peaks[i] - peaks[i - 1];
                spacing /= std::max<size_t>(1, peaks.size() - 1);
                logMessage("  " + juce::String(stretch == 1 ? "RHYTHM" : "CLEAN") + ": " + juce::String(static_cast<int>(peaks.size())) + " clicks, spacing "
                           + juce::String(spacing, 1));
                expect(peaks.size() >= 3 && std::abs(spacing - 12000.0) <= 240.0, juce::String(stretch));
            }
        }

        beginTest("A03 / A12 / A14: RHYTHM and CLEAN are block-size invariant, allocation-free and finite (all actions, order changes, seeks)");
        {
            const auto drums = makeSyntheticDrums({ kFs, 2, 3.0, kFixtureSeed });
            for (int stretch : { 1, 2 })
            {
                const auto p = revSlot([stretch](FxSlotState& m, rev::Pattern& pat)
                {
                    pat = rev::generate({}, 2718, rev::Destruction);
                    for (int s = 0; s < rev::kSteps; s += 3)
                        setStep(pat, rev::Time, s, s % 2 == 0 ? rev::Half : rev::Double);
                    m.parameters.set("timeStretch", stretch);
                });
                RunOptions o;
                o.block = 512;
                const auto reference = run(drums, p, o);
                expect(finite(reference));
                for (int block : { 1, 64, 127 })
                {
                    o.block = block;
                    expect(identical(run(drums, p, o), reference), "stretch " + juce::String(stretch) + " block " + juce::String(block));
                }
                std::uint64_t allocations = 0;
                RunOptions a;
                a.block = 64;
                a.allocations = &allocations;
                a.change = [](FxSlotParams& q, int start) { q.values[slot::kRevOrder] = static_cast<float>((start / 30000) % 6); };
                a.ppqAt = [](int start) { return (start % 70000) * 120.0 / (60.0 * kFs); };
                expect(finite(run(drums, p, a)));
                expectEquals(static_cast<int>(allocations), 0);
            }
        }

        beginTest("C-ATD: a splice between uncorrelated heads keeps its level (dip <= 1 dB; plain fade shows >= 2 dB); equal heads stay exact");
        {
            const auto noise = makeWhiteNoise({ kFs, 2, 20.0, 11 }, -12.0);
            const auto jumps = [](int smart)
            {
                return revSlot([smart](FxSlotState& m, rev::Pattern& pat)
                {
                    m.parameters.set("smartJoin", smart);
                    m.parameters.set("fadeMs", 8.0);
                    for (int s = 1; s < rev::kSteps; s += 2)
                        setStep(pat, rev::Glitch, s, rev::Jump);
                });
            };
            const int fade = static_cast<int>(8.0e-3 * kFs);
            const auto dip = [&](const juce::AudioBuffer<float>& y)
            {
                std::vector<double> profile(static_cast<size_t>(fade), 0.0);
                double steady = 0.0;
                int transitions = 0, steadyCount = 0;
                for (int t = 24000; t + 6000 < y.getNumSamples(); t += 6000, ++transitions)
                {
                    for (int i = 0; i < fade; ++i)
                        for (int ch = 0; ch < 2; ++ch)
                            profile[static_cast<size_t>(i)] += static_cast<double>(y.getSample(ch, t + i)) * y.getSample(ch, t + i);
                    for (int i = 2000; i < 4000; ++i, ++steadyCount)
                        for (int ch = 0; ch < 2; ++ch)
                            steady += static_cast<double>(y.getSample(ch, t + i)) * y.getSample(ch, t + i);
                }
                steady /= steadyCount;
                double worst = 0.0;
                for (int i = 16; i + 16 < fade; ++i)
                {
                    double e = 0.0;
                    for (int k = -16; k < 16; ++k)
                        e += profile[static_cast<size_t>(i + k)];
                    e /= 32.0 * transitions;
                    worst = std::min(worst, 10.0 * std::log10(e / steady));
                }
                return worst;
            };
            const double plain = dip(run(noise, jumps(0))), smart = dip(run(noise, jumps(1)));
            logMessage("  deepest level in the splice: plain " + juce::String(plain, 2) + " dB, C-ATD " + juce::String(smart, 2) + " dB");
            expect(plain <= -2.0 && smart >= -1.0);

            const auto dc = run(constant(60000, 0.5f), revSlot([](FxSlotState& m, rev::Pattern& pat)
            {
                m.parameters.set("smartJoin", 1);
                for (int s = 1; s < rev::kSteps; s += 2)
                    setStep(pat, rev::Glitch, s, rev::Repeat);
            }));
            float worst = 0.0f;
            for (int i = 12000; i < 60000; ++i)
                worst = std::max(worst, std::abs(dc.getSample(0, i) - 0.5f));
            logMessage("  equal heads (DC through REPEAT splices): largest error " + juce::String(worst, 8));
            expect(worst <= 1.0e-6f);
        }

        beginTest("LISTEN analysis: hits on steps 1, 5, 9, 13 read >= 0.8 there and <= 0.1 elsewhere after four cycles");
        {
            juce::AudioBuffer<float> hits(2, 4 * 96000 + 1000); // the first hit (sample 0) has no history to stand out from
            hits.clear();
            for (int i = 0; i < hits.getNumSamples() - 600; i += 24000)
                for (int ch = 0; ch < 2; ++ch)
                    for (int k = 0; k < 500; ++k)
                        hits.setSample(ch, i + k, 0.7f * std::exp(-0.01f * k) * static_cast<float>(std::sin(0.3 * k)));
            auto m = std::make_unique<MonstaProcessor>();
            m->prepare(kFs);
            run(hits, revSlot(), {}, m.get());
            std::array<float, 16> feel {};
            m->getRevolution().getFeel(feel);
            juce::String shown;
            bool good = true;
            for (int s = 0; s < 16; ++s)
            {
                shown << juce::String(feel[static_cast<size_t>(s)], 2) << " ";
                good = good && (s % 4 == 0 ? feel[static_cast<size_t>(s)] >= 0.8f : feel[static_cast<size_t>(s)] <= 0.1f);
            }
            logMessage("  feel: " + shown);
            expect(good);
        }

        beginTest("A16 with LISTEN (2000 seeds x 4 characters): constraints hold; repeats land on the hits, gaps avoid them");
        {
            rev::Feel feel {};
            for (int s = 0; s < 16; s += 4)
                feel[static_cast<size_t>(s)] = 15;
            rev::Pattern base;
            setStep(base, rev::Glitch, 6, rev::Scratch);
            base.locks[rev::Glitch] = 1u << 6;
            base.allow[rev::Glitch] = static_cast<std::uint16_t>(0xffff & ~(1u << rev::Jump));
            int violations = 0;
            double alignedSmart = 0.0, alignedPlain = 0.0, repeatsSmart = 0.0, repeatsPlain = 0.0, gapsOnHits = 0.0, gaps = 0.0;
            for (std::uint64_t seed = 1; seed <= 2000; ++seed)
                for (int c = 0; c < rev::kCharacters; ++c)
                {
                    const auto g = rev::generateFollowing(base, seed, c, feel);
                    const auto plain = rev::generate(base, seed, c);
                    violations += g == rev::generateFollowing(base, seed, c, feel) ? 0 : 1;
                    violations += g.steps[rev::Glitch][6] == rev::Scratch ? 0 : 1;
                    for (int e = 0; e < 2; ++e)
                    {
                        int events = 0;
                        for (auto v : g.steps[static_cast<size_t>(e)])
                            events += v != 0 ? 1 : 0;
                        violations += events > rev::maxEvents(e, c) ? 1 : 0;
                    }
                    for (int s = 0; s < 16; ++s)
                    {
                        const auto v = g.steps[rev::Glitch][static_cast<size_t>(s)], w = plain.steps[rev::Glitch][static_cast<size_t>(s)];
                        violations += v == rev::Jump || v >= rev::kGlitchActions ? 1 : 0;
                        const bool hit = s % 4 == 0;
                        const auto isRepeat = [](int a) { return a == rev::Repeat || a == rev::Stutter || a == rev::Ratchet; };
                        repeatsSmart += isRepeat(v) ? 1.0 : 0.0;
                        alignedSmart += isRepeat(v) && hit ? 1.0 : 0.0;
                        repeatsPlain += isRepeat(w) ? 1.0 : 0.0;
                        alignedPlain += isRepeat(w) && hit ? 1.0 : 0.0;
                        gaps += v == rev::Gap ? 1.0 : 0.0;
                        gapsOnHits += v == rev::Gap && hit ? 1.0 : 0.0;
                    }
                }
            const double smart = alignedSmart / std::max(1.0, repeatsSmart), plainShare = alignedPlain / std::max(1.0, repeatsPlain);
            const double gapShare = gapsOnHits / std::max(1.0, gaps);
            logMessage("  violations " + juce::String(violations) + "; repeats on hits: LISTEN " + juce::String(smart, 3) + " vs plain "
                       + juce::String(plainShare, 3) + "; gaps on hits " + juce::String(gapShare, 3));
            expect(violations == 0 && smart >= plainShare + 0.2 && gapShare <= 0.1);
        }

        beginTest("A15 / A22: the LISTEN snapshot is saved with the pattern; the page sends it and regenerates the same pattern from it");
        {
            rev::Feel feel {};
            for (int s = 0; s < 16; ++s)
                feel[static_cast<size_t>(s)] = static_cast<std::uint8_t>((s * 7) % 16);
            juce::NamedValueSet set;
            rev::writeFeel(set, feel);
            expect(rev::readFeel(set) == feel);

            ui::HamloRevolutionPanel panel;
            panel.setSize(1180, 700);
            rev::Pattern sent;
            rev::Feel sentFeel {};
            panel.onPatternChange = [&](const rev::Pattern& p, juce::int64, const rev::Feel& f, const juce::String&) { sent = p; sentFeel = f; };
            auto v = panel.getValues();
            v.switches[ui::HamloRevolutionPanel::Listen] = 1;
            v.seed = 4242;
            panel.setValues(v);
            ui::HamloRevolutionPanel::Status s;
            for (int i = 0; i < 16; i += 4)
                s.feel[static_cast<size_t>(i)] = 1.0f;
            panel.readStatus = [s] { return s; };
            panel.pressGenerate();
            expect(sentFeel == rev::feelFrom(s.feel) && sent == rev::generateFollowing(v.pattern, 4242, v.switches[ui::HamloRevolutionPanel::Character], sentFeel));
        }

        beginTest("P6: the own FFT (CLEAN) matches juce::dsp::FFT (|diff| <= 1e-3 on 2048 points) and inverts (<= 1e-5)");
        {
            juce::dsp::FFT reference(11);
            RevolutionFft own;
            own.prepare(2048);
            std::vector<std::complex<float>> x(2048), a(2048), b(2048), back(2048);
            juce::Random r(3);
            for (auto& v : x)
                v = { r.nextFloat() - 0.5f, r.nextFloat() - 0.5f };
            reference.perform(x.data(), a.data(), false);
            own.perform(x.data(), b.data(), false);
            own.perform(b.data(), back.data(), true);
            double diff = 0.0, trip = 0.0;
            for (size_t i = 0; i < x.size(); ++i)
            {
                diff = std::max(diff, static_cast<double>(std::abs(a[i] - b[i])));
                trip = std::max(trip, static_cast<double>(std::abs(back[i] - x[i])));
            }
            logMessage("  own vs JUCE " + juce::String(diff, 7) + ", round trip " + juce::String(trip, 8));
            expect(diff <= 1.0e-3 && trip <= 1.0e-5);
        }

        beginTest("CPU per callback (48 kHz, 64 samples, deadline 1.333 ms): one HamloRevolution (CLEAN, all engines) p99 <= 25 %; eight p99 <= 100 %");
        {
            const auto drums = makeSyntheticDrums({ kFs, 2, 6.0, kFixtureSeed });
            const auto p = revSlot([](FxSlotState& m, rev::Pattern& pat)
            {
                pat = rev::generate({}, 99, rev::Destruction);
                for (int s = 0; s < rev::kSteps; s += 2)
                    setStep(pat, rev::Time, s, rev::Half);
                m.parameters.set("timeStretch", 2);
            });
            for (int instances : { 1, 8 })
            {
                std::vector<std::unique_ptr<MonstaProcessor>> chain;
                for (int i = 0; i < instances; ++i)
                {
                    chain.push_back(std::make_unique<MonstaProcessor>());
                    chain.back()->prepare(kFs);
                }
                auto y = drums;
                std::vector<double> ms;
                for (int start = 0; start + 64 <= y.getNumSamples(); start += 64)
                {
                    Transport t;
                    t.playing = t.hasPosition = true;
                    t.ppqAtBlockStart = start * 120.0 / (60.0 * kFs);
                    float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    const auto t0 = std::chrono::steady_clock::now();
                    for (auto& m : chain)
                        m->process(c, 2, 64, p, t, 0);
                    ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
                }
                std::sort(ms.begin(), ms.end());
                const double deadline = 64.0 / kFs * 1000.0, p50 = ms[ms.size() / 2], p99 = ms[ms.size() * 99 / 100], worst = ms.back();
                logMessage("  " + juce::String(instances) + " instance(s): p50 " + juce::String(100.0 * p50 / deadline, 1) + " %, p99 "
                           + juce::String(100.0 * p99 / deadline, 1) + " %, max " + juce::String(100.0 * worst / deadline, 1) + " % of the deadline");
                expect(p99 <= (instances == 1 ? 0.25 : 1.0) * deadline);
            }
        }
    }
};

static RevolutionStretchTests revolutionStretchTests;
} // namespace hpeq::test
