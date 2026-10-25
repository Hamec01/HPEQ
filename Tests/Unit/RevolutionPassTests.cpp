// Module pass 10 (owner): HamloRevolution - the Monsta FX slot as TIME / GLITCH / MOTION with six orders,
// an explicit 16-step pattern per engine and Generate -> Edit -> Lock. Thresholds fixed before the runs
// (HamloRevolution task, section 18: A01-A16, A19).

#include <cmath>
#include <functional>
#include <memory>

#include "Core/Revolution.h"
#include "DSP/ProcessorsM6.h"
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

        beginTest("A08: HALF reads the run's first half at half speed (linear input: exact within 1e-6), never ahead of the writer");
        {
            const auto p = revSlot([](FxSlotState&, rev::Pattern& pat) { setStep(pat, rev::Time, 1, rev::Half); setStep(pat, rev::Time, 2, rev::Half); });
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
            panel.onPatternChange = [&](const rev::Pattern& p, juce::int64 seed, const juce::String& w) { sent = p; sentSeed = seed; what = w; };
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
} // namespace hpeq::test
