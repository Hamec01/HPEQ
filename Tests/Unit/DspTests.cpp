// M2: one PRE-EQ + frequency FX nodes. T01, T02, MIX-0, T04, T10, chain semantics (D2), delta
// math against an independent reference, automation smoothness, compressor law, PRE-EQ -> FX order.
// Thresholds below are fixed here, before the runs (04 §5.2).

#include <algorithm>
#include <cmath>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/LayerEngine.h"
#include "DSP/Processors.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginProcessor.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;

EqBandParams bell(float hz, float db, float q = 1.0f)
{
    EqBandParams b;
    b.active = true;
    b.shape = EqShape::Bell;
    b.frequencyHz = hz;
    b.gainDb = db;
    b.q = q;
    return b;
}

FxSlotParams gainSlot(float db, float mix = 1.0f)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Gain;
    s.mix = mix;
    s.values[slot::kGainDb] = db;
    return s;
}

FxSlotParams compSlot(float thresholdDb, float ratio, float kneeDb, float attackMs, float releaseMs, float makeupDb = 0.0f, float mix = 1.0f)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Compressor;
    s.mix = mix;
    s.values[slot::kThresholdDb] = thresholdDb;
    s.values[slot::kRatio] = ratio;
    s.values[slot::kKneeDb] = kneeDb;
    s.values[slot::kAttackMs] = attackMs;
    s.values[slot::kReleaseMs] = releaseMs;
    s.values[slot::kMakeupDb] = makeupDb;
    return s;
}

FxNodeParams node(float low, float high, std::initializer_list<FxSlotParams> chain, float mix = 1.0f, bool active = true)
{
    FxNodeParams n;
    n.active = active;
    n.lowHz = low;
    n.highHz = high;
    n.mix = mix;
    for (const auto& s : chain)
        n.slots[static_cast<size_t>(n.numSlots++)] = s;
    return n;
}

struct RenderResult
{
    juce::AudioBuffer<float> audio;
    std::uint64_t allocations = 0;
};

// Renders `input` through a fresh engine with `snapshot`, in blocks of `block`.
RenderResult render(const LayerSnapshot& snapshot, const juce::AudioBuffer<float>& input, int block, double fs = kFs)
{
    LayerEngine engine;
    engine.prepare(fs, block);
    engine.publish(snapshot);
    RenderResult r { input, 0 };
    for (int start = 0; start < input.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, input.getNumSamples() - start);
        float* chunk[kMaxChannels] {};
        for (int ch = 0; ch < input.getNumChannels(); ++ch)
            chunk[ch] = r.audio.getWritePointer(ch) + start;
        ScopedAllocationProbe probe;
        engine.process(chunk, input.getNumChannels(), n);
        r.allocations += probe.counts().allocations;
    }
    return r;
}

juce::AudioBuffer<float> musicLikeInput(double fs, double seconds)
{
    auto x = makeSyntheticDrums({ fs, 2, seconds, kFixtureSeed });
    const auto noise = makePinkNoise({ fs, 2, seconds, kFixtureSeed + 9 }, -26.0);
    for (int ch = 0; ch < 2; ++ch)
        x.addFrom(ch, 0, noise, ch, 0, x.getNumSamples());
    return x;
}

juce::AudioBuffer<float> slice(const juce::AudioBuffer<float>& a, int start, int length = -1)
{
    juce::AudioBuffer<float> out(a.getNumChannels(), length < 0 ? a.getNumSamples() - start : length);
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        out.copyFrom(ch, 0, a, ch, start, out.getNumSamples());
    return out;
}

} // namespace

// Independent references for H3 (plain cascades, no suffix trick, double precision).
juce::AudioBuffer<float> allPassReference(const juce::AudioBuffer<float>& x, std::vector<double> edges, double fs)
{
    auto y = x; // one all-pass per node edge, duplicates included (as in the engine)
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
    {
        std::vector<Biquad> chain(edges.size());
        for (size_t e = 0; e < edges.size(); ++e)
            chain[e].setCoefficients(design::allPass(fs, edges[e], design::kButterworthQ));
        for (int i = 0; i < x.getNumSamples(); ++i)
        {
            double v = x.getSample(ch, i);
            for (auto& f : chain)
                v = f.processSample(0, v);
            y.setSample(ch, i, static_cast<float>(v));
        }
    }
    return y;
}

juce::AudioBuffer<float> singleNodeReference(const juce::AudioBuffer<float>& x, double lo, double hi, double gainDb, double fs)
{
    const double g = std::pow(10.0, gainDb / 20.0);
    auto y = x;
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
    {
        Biquad apLo, apHi, hp1, hp2, lp1, lp2;
        apLo.setCoefficients(design::allPass(fs, lo, design::kButterworthQ));
        apHi.setCoefficients(design::allPass(fs, hi, design::kButterworthQ));
        hp1.setCoefficients(design::highPass(fs, lo, design::kButterworthQ));
        hp2.setCoefficients(design::highPass(fs, lo, design::kButterworthQ));
        lp1.setCoefficients(design::lowPass(fs, hi, design::kButterworthQ));
        lp2.setCoefficients(design::lowPass(fs, hi, design::kButterworthQ));
        for (int i = 0; i < x.getNumSamples(); ++i)
        {
            const double v = x.getSample(ch, i);
            const double dry = apHi.processSample(0, apLo.processSample(0, v));
            const double band = lp2.processSample(0, lp1.processSample(0, hp2.processSample(0, hp1.processSample(0, v))));
            y.setSample(ch, i, static_cast<float>(dry + (g - 1.0) * band));
        }
    }
    return y;
}

class DspTests final : public juce::UnitTest
{
public:
    DspTests() : juce::UnitTest("DSP M2", "HPEQ") {}

    void runTest() override
    {
        const auto input = musicLikeInput(kFs, 1.0);

        beginTest("T01: 'FX off' is bit-exact; a passive running node equals the documented all-pass result");
        {
            struct Case { const char* name; LayerSnapshot snapshot; bool bitExact; };
            std::vector<Case> cases;
            cases.push_back({ "empty layer", {}, true });
            {
                LayerSnapshot s;
                s.fx[0] = node(250, 700, { compSlot(-30, 4, 6, 5, 80) }, 1.0f, false);
                cases.push_back({ "node disabled", s, true });
            }
            {
                LayerSnapshot s;
                s.eq[0] = bell(1000, 0.0f);
                cases.push_back({ "bell at 0 dB", s, true });
            }
            {
                LayerSnapshot s;
                s.fx[0] = node(250, 700, { compSlot(-30, 4, 6, 5, 80) }, 0.0f);
                cases.push_back({ "node mix 0", s, false });
            }
            {
                LayerSnapshot s;
                auto inactive = compSlot(-30, 4, 6, 5, 80);
                inactive.active = false;
                s.fx[0] = node(250, 700, { inactive, inactive });
                cases.push_back({ "all slots bypassed", s, false });
            }
            {
                LayerSnapshot s;
                s.fx[0] = node(250, 700, { compSlot(-30, 4, 6, 5, 80, 0, 0.0f) });
                cases.push_back({ "slot mix 0", s, false });
            }
            {
                LayerSnapshot s;
                s.fx[0] = node(250, 700, { gainSlot(0.0f) });
                cases.push_back({ "gain 0 dB", s, false });
            }
            {
                LayerSnapshot s;
                s.fx[0] = node(250, 700, { compSlot(0, 4, 0, 5, 80) }); // threshold above any band level
                cases.push_back({ "compressor never above threshold", s, false });
            }

            double worstAllPassDb = kSilenceDb;
            for (const auto& c : cases)
                for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
                    for (int block : { 1, 64, 512 })
                    {
                        const auto x = musicLikeInput(fs, 0.3);
                        const auto r = render(c.snapshot, x, block, fs);
                        const auto label = juce::String(c.name) + " fs " + juce::String(fs) + " block " + juce::String(block);
                        expectEquals(static_cast<int>(r.allocations), 0);
                        if (c.bitExact)
                        {
                            expect(compareAligned(r.audio, x).bitExact, label);
                            continue;
                        }
                        // 04 §3: compare to the documented expected all-pass result, after the 60 ms
                        // edge birth glide from 1 Hz at start-up.
                        const int settled = static_cast<int>(0.1 * fs);
                        const auto expected = allPassReference(x, { 250.0, 700.0 }, fs);
                        const auto null = compareAligned(slice(r.audio, settled), slice(expected, settled));
                        worstAllPassDb = std::max(worstAllPassDb, null.errorDb);
                        expect(null.errorDb <= -100.0, label + ": " + juce::String(null.errorDb) + " dB");
                    }
            logMessage("  passive running node vs AP reference, worst: " + juce::String(worstAllPassDb, 1) + " dB");
        }

        beginTest("T02: 2-8 overlapping nodes - disabled: bit-exact PRE-EQ; mix 0 / bypassed: AP(PRE-EQ)");
        {
            LayerSnapshot eqOnly;
            eqOnly.eq[0] = bell(900, 6.0f, 0.8f);
            eqOnly.eq[1] = bell(120, -4.0f, 1.2f);
            const auto reference = render(eqOnly, input, 256).audio;
            expect(!compareAligned(reference, input).bitExact, "the PRE-EQ must actually change the signal");

            const float ranges[8][2] { { 250, 700 }, { 450, 1200 }, { 300, 900 }, { 600, 2000 },
                                       { 100, 400 }, { 1000, 5000 }, { 200, 3000 }, { 500, 650 } };
            const int settled = static_cast<int>(0.1 * kFs);
            double worstDb = kSilenceDb, worstRmsChange = 0.0, worstWindowShift = 0.0;
            for (int count = 2; count <= 8; ++count)
                for (int variant = 0; variant < 3; ++variant)
                {
                    auto s = eqOnly;
                    std::vector<double> edges;
                    for (int i = 0; i < count; ++i)
                    {
                        auto inactiveSlot = compSlot(-40, 8, 0, 1, 50);
                        inactiveSlot.active = variant != 2;
                        s.fx[static_cast<size_t>(i)] = node(ranges[i][0], ranges[i][1], { inactiveSlot, gainSlot(-12, variant == 2 ? 1.0f : 0.5f) },
                                                            variant == 1 ? 0.0f : 1.0f, variant != 0);
                        if (variant == 2)
                            s.fx[static_cast<size_t>(i)].slots[1].active = false;
                        edges.push_back(ranges[i][0]);
                        edges.push_back(ranges[i][1]);
                    }
                    const auto r = render(s, input, 256);
                    const auto label = juce::String(count) + " nodes, variant " + juce::String(variant);
                    if (variant == 0)
                    {
                        expect(compareAligned(r.audio, reference).bitExact, label);
                        continue;
                    }
                    const auto expected = allPassReference(reference, edges, kFs);
                    const auto null = compareAligned(slice(r.audio, settled), slice(expected, settled));
                    worstDb = std::max(worstDb, null.errorDb);
                    expect(null.errorDb <= -100.0, label + ": " + juce::String(null.errorDb) + " dB");
                    // No dry doubling: the level equals the all-pass reference's level (a doubled dry would be
                    // +6 dB). Versus the PRE-EQ output itself the windowed RMS differs slightly because an
                    // all-pass moves energy in time across the window edges; that is logged, not asserted.
                    const double rmsChange = measureAll(slice(r.audio, settled)).rmsDbfs - measureAll(slice(expected, settled)).rmsDbfs;
                    worstRmsChange = std::max(worstRmsChange, std::abs(rmsChange));
                    expect(std::abs(rmsChange) < 0.01, label + ": level change " + juce::String(rmsChange) + " dB");
                    worstWindowShift = std::max(worstWindowShift, std::abs(measureAll(slice(r.audio, settled)).rmsDbfs
                                                                           - measureAll(slice(reference, settled)).rmsDbfs));
                }
            logMessage("  overlapping passive nodes vs AP reference, worst: " + juce::String(worstDb, 1)
                       + " dB; RMS vs AP reference " + juce::String(worstRmsChange, 4) + " dB; windowed RMS vs PRE-EQ output "
                       + juce::String(worstWindowShift, 3) + " dB");
        }

        beginTest("delta math: y = AP_hi(AP_lo(x)) + (g - 1) * LP4_hi(HP4_lo(x)), independent reference");
        {
            LayerSnapshot s;
            s.fx[0] = node(250, 700, { gainSlot(-6.0f) });
            const auto r = render(s, input, 128);
            const auto expected = singleNodeReference(input, 250.0, 700.0, -6.0, kFs);
            // Edge birth glide (60 ms) and presence fade-in (20 ms) are excluded.
            const int settled = static_cast<int>(0.1 * kFs);
            const auto null = compareAligned(slice(r.audio, settled), slice(expected, settled));
            expect(null.errorDb < -120.0, "error " + juce::String(null.errorDb) + " dB");
            logMessage("  engine vs independent H3 reference: " + juce::String(null.errorDb, 1) + " dB");
        }

        beginTest("MIX-0: after mixing a running compressor out, output equals the passive-node render exactly");
        {
            LayerEngine engine;
            engine.prepare(kFs, 256);
            LayerSnapshot s;
            s.eq[0] = bell(2000, 3.0f);
            s.fx[0] = node(250, 700, { compSlot(-40, 10, 0, 1, 2000) }); // long release: state stays "hot"
            engine.publish(s);
            auto y = input;
            const int half = input.getNumSamples() / 2;
            const auto processRange = [&](int from, int to)
            {
                for (int start = from; start < to; start += 256)
                {
                    float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    engine.process(chunk, 2, juce::jmin(256, to - start));
                }
            };
            processRange(0, half);
            s.fx[0].mix = 0.0f;
            engine.publish(s);
            processRange(half, input.getNumSamples());
            expect(engine.getNode(0).getGainReductionDb(0) > 1.0f, "compressor state must still be active");

            auto passive = s;
            passive.fx[0].slots[0].active = false;
            passive.fx[0].mix = 1.0f;
            const auto reference = render(passive, input, 256).audio;
            const int afterRamp = half + static_cast<int>(0.025 * kFs);
            expect(compareAligned(slice(y, afterRamp), slice(reference, afterRamp)).bitExact);
        }

        beginTest("FX off again: disabling the last node returns to the bit-exact PRE-EQ output");
        {
            LayerEngine engine;
            engine.prepare(kFs, 256);
            LayerSnapshot s;
            s.eq[0] = bell(150, -3.0f);
            s.fx[0] = node(250, 700, { compSlot(-40, 6, 0, 1, 100) });
            s.fx[1] = node(450, 3000, { gainSlot(-6) });
            engine.publish(s);
            auto y = input;
            const int off = 24064;
            for (int start = 0; start < y.getNumSamples(); start += 256)
            {
                if (start == off)
                {
                    s.fx[0].active = false;
                    s.fx[1].active = false;
                    engine.publish(s);
                }
                float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                engine.process(chunk, 2, juce::jmin(256, y.getNumSamples() - start));
            }
            LayerSnapshot eqOnly;
            eqOnly.eq[0] = s.eq[0];
            const auto reference = render(eqOnly, input, 256).audio;
            // node fade-out 20 ms + next block + edges parking glide 60 ms + next block
            const int settled = off + static_cast<int>(0.1 * kFs);
            expect(compareAligned(slice(y, settled), slice(reference, settled)).bitExact, "not back to bit-exact");
            expect(!engine.isTreeActive());
        }

        beginTest("topology changes (first node on, second node added, edge order change, all off) are smooth");
        {
            // Thresholds fixed before the run: HF products > 5 kHz below -60 dB (as for edge glides) and
            // no short-term level dip deeper than 3 dB below the quieter steady state around a change.
            const auto sine = makeSine({ kFs, 2, 1.2, kFixtureSeed }, 1000.0, -6.0);
            LayerEngine engine;
            engine.prepare(kFs, 256);
            LayerSnapshot s;
            auto y = sine;
            const int changes[] { 12032, 24064, 36096, 48128 };
            for (int start = 0; start < y.getNumSamples(); start += 256)
            {
                if (start == changes[0]) { s.fx[0] = node(250, 700, { gainSlot(-6) }); engine.publish(s); }
                if (start == changes[1]) { s.fx[1] = node(2000, 5000, { gainSlot(-6) }); engine.publish(s); }
                if (start == changes[2]) { s.fx[0].highHz = 1500.0f; engine.publish(s); }
                if (start == changes[3]) { s.fx[0].active = false; s.fx[1].active = false; engine.publish(s); }
                float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                engine.process(chunk, 2, juce::jmin(256, y.getNumSamples() - start));
            }
            const double hf = highBandRelativeDb(y, kFs, 5000.0, 0, 0.1);
            const double floor = highBandRelativeDb(sine, kFs, 5000.0, 0, 0.1);
            expect(hf < -60.0, "HF products " + juce::String(hf, 1) + " dB");

            const int window = static_cast<int>(0.0025 * kFs);
            const auto rmsAt = [&](int from)
            {
                double sum = 0.0;
                for (int i = from; i < from + window; ++i)
                    sum += static_cast<double>(y.getSample(0, i)) * y.getSample(0, i);
                return 10.0 * std::log10(sum / window + 1e-30);
            };
            double worstDip = 0.0;
            for (int c : changes)
            {
                const double before = rmsAt(c - 2 * window), after = rmsAt(c + static_cast<int>(0.15 * kFs));
                double lowest = 1e9;
                for (int i = c; i < c + static_cast<int>(0.12 * kFs); i += window / 2)
                    lowest = std::min(lowest, rmsAt(i));
                worstDip = std::max(worstDip, std::min(before, after) - lowest);
            }
            expect(worstDip < 3.0, "level dip " + juce::String(worstDip, 2) + " dB");
            logMessage("  topology changes: HF > 5 kHz " + juce::String(hf, 1) + " dB (floor " + juce::String(floor, 1)
                       + " dB), worst dip " + juce::String(worstDip, 2) + " dB");
        }

        beginTest("D2 chain: order, bypass of a middle slot, two gains compose");
        {
            LayerSnapshot one, two;
            one.fx[0] = node(300, 3000, { gainSlot(-6.0f) });
            two.fx[0] = node(300, 3000, { gainSlot(-3.0f), gainSlot(-3.0f) });
            // Compared after the 20 ms slot fade-ins: two slots ramping together are not one ramp.
            const int settledChain = static_cast<int>(0.05 * kFs);
            const auto a = slice(render(one, input, 256).audio, settledChain);
            const auto b = slice(render(two, input, 256).audio, settledChain);
            const auto chainNull = compareAligned(b, a);
            expect(chainNull.errorDb < -120.0, "-3 then -3 must equal -6: " + juce::String(chainNull.errorDb) + " dB");

            LayerSnapshot compThenGain, gainThenComp;
            compThenGain.fx[0] = node(100, 2000, { compSlot(-30, 6, 0, 2, 100), gainSlot(-12) });
            gainThenComp.fx[0] = node(100, 2000, { gainSlot(-12), compSlot(-30, 6, 0, 2, 100) });
            const auto c1 = render(compThenGain, input, 256).audio;
            const auto c2 = render(gainThenComp, input, 256).audio;
            expect(compareAligned(c1, c2).errorDb > -60.0, "slot order must matter for a compressor");

            LayerSnapshot three, withoutMiddle;
            auto bypassed = compSlot(-50, 20, 0, 1, 10);
            bypassed.active = false;
            three.fx[0] = node(100, 2000, { gainSlot(-2), bypassed, compSlot(-30, 4, 6, 5, 100) });
            withoutMiddle.fx[0] = node(100, 2000, { gainSlot(-2), compSlot(-30, 4, 6, 5, 100) });
            // The slot index differs, but the processed result must not.
            const auto t = render(three, input, 256).audio;
            const auto w = render(withoutMiddle, input, 256).audio;
            expect(compareAligned(t, w).bitExact, "bypassed middle slot changed the result");

            LayerSnapshot six;
            six.fx[0] = node(100, 8000, { gainSlot(-1), compSlot(-30, 2, 6, 5, 100), gainSlot(1), compSlot(-20, 3, 0, 1, 50),
                                          gainSlot(-2), compSlot(-25, 4, 12, 10, 200) });
            const auto r6 = render(six, input, 256);
            expect(measureAll(r6.audio).allFinite);
            expectEquals(static_cast<int>(r6.allocations), 0);
        }

        beginTest("T10: identical render for block sizes 1, 32, 127, 256, 1024");
        {
            LayerSnapshot s;
            s.eq[0] = bell(150, -5.0f, 0.9f);
            s.eq[1] = bell(3500, 4.0f, 1.5f);
            s.fx[0] = node(250, 700, { compSlot(-30, 4, 6, 3, 90) });
            s.fx[1] = node(450, 1200, { gainSlot(-4.0f), compSlot(-24, 3, 0, 1, 40, 2.0f) }, 0.7f);
            s.fx[2] = node(3000, 9000, { compSlot(-36, 8, 3, 0.5f, 20) }, 0.5f);
            const auto reference = render(s, input, 1).audio;
            for (int block : { 32, 127, 256, 1024 })
            {
                const auto r = render(s, input, block);
                const auto null = compareAligned(r.audio, reference);
                expect(null.bitExact, "block " + juce::String(block) + ": " + juce::String(null.errorDb) + " dB");
                expectEquals(static_cast<int>(r.allocations), 0);
            }
        }

        beginTest("T04: 100k random mutations, random blocks 1..2048, no NaN/Inf, bounded, no allocations");
        {
            LayerEngine engine;
            engine.prepare(kFs, 512); // blocks above 512 exercise the internal chunking
            DeterministicRandom random(kFixtureSeed + 4);
            const auto uni = [&](double lo, double hi) { return lo + (hi - lo) * (random.nextBipolar() * 0.5 + 0.5); };
            const auto source = makeWhiteNoise({ kFs, 2, 2.0, kFixtureSeed }, -12.0);
            juce::AudioBuffer<float> block(2, 2048);
            LayerSnapshot s;
            bool finite = true;
            double worstPeak = 0.0;
            std::uint64_t allocations = 0;
            int position = 0, published = 0, blocks = 0;
            for (int mutation = 0; mutation < 100000; ++mutation)
            {
                // Mutate one random field-group per step. Gains bounded so the worst stack is finite:
                // 4 bands x +12 dB, nodes with up to +12 dB per slot.
                const int what = static_cast<int>(uni(0, 6));
                auto& b = s.eq[static_cast<size_t>(uni(0, 4))];
                auto& n = s.fx[static_cast<size_t>(uni(0, kMaxFxNodesPerLayer))];
                auto& sl = n.slots[static_cast<size_t>(uni(0, kMaxFxPerNode))];
                switch (what)
                {
                    case 0: b = bell(static_cast<float>(uni(5, 30000)), static_cast<float>(uni(-12, 12)), static_cast<float>(uni(0.1, 20))); b.active = uni(0, 1) > 0.3; break;
                    case 1: n.active = uni(0, 1) > 0.4; n.mix = static_cast<float>(uni(0, 1)); break;
                    case 2: n.lowHz = static_cast<float>(uni(5, 30000)); n.highHz = static_cast<float>(uni(5, 30000)); break;
                    case 3: n.numSlots = static_cast<int>(uni(0, kMaxFxPerNode + 1)); break;
                    case 4: sl = uni(0, 1) > 0.5 ? gainSlot(static_cast<float>(uni(-60, 12)), static_cast<float>(uni(0, 1)))
                                                 : compSlot(static_cast<float>(uni(-60, 0)), static_cast<float>(uni(1, 100)), static_cast<float>(uni(0, 24)),
                                                            static_cast<float>(uni(0.05, 500)), static_cast<float>(uni(1, 5000)), static_cast<float>(uni(-24, 12)),
                                                            static_cast<float>(uni(0, 1)));
                            break;
                    default: sl.active = uni(0, 1) > 0.5; sl.type = uni(0, 1) > 0.8 ? FxType::Reverb : sl.type; break;
                }
                if (mutation % 5 == 0)
                {
                    engine.publish(s);
                    ++published;
                }
                if (mutation % 10 != 0)
                    continue;
                const int count = static_cast<int>(uni(1, 2049));
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < count; ++i)
                        block.setSample(ch, i, source.getSample(ch, (position + i) % source.getNumSamples()));
                position += count;
                float* chunk[2] { block.getWritePointer(0), block.getWritePointer(1) };
                ScopedAllocationProbe probe;
                engine.process(chunk, 2, count);
                allocations += probe.counts().allocations;
                ++blocks;
                juce::AudioBuffer<float> view(block.getArrayOfWritePointers(), 2, 0, count);
                const auto stats = measureAll(view);
                finite = finite && stats.allFinite;
                worstPeak = std::max(worstPeak, stats.peak);
            }
            expect(finite, "non-finite output");
            expect(worstPeak < 1.0e4, "runaway level " + juce::String(worstPeak));
            expectEquals(static_cast<int>(allocations), 0);
            logMessage("  " + juce::String(published) + " snapshots, " + juce::String(blocks) + " blocks, worst peak "
                       + juce::String(juce::Decibels::gainToDecibels(worstPeak), 1) + " dBFS");
        }

        beginTest("range move during playback is smooth (HF products < -60 dB)");
        {
            const auto sine = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 1000.0, -6.0);
            LayerEngine engine;
            engine.prepare(kFs, 256);
            LayerSnapshot s;
            s.fx[0] = node(250, 700, { gainSlot(-12.0f) });
            engine.publish(s);
            auto y = sine;
            for (int start = 0; start < y.getNumSamples(); start += 256)
            {
                if (start == 24064) // ~0.5 s: jump the range target across the tone
                {
                    s.fx[0].lowHz = 800.0f;
                    s.fx[0].highHz = 2000.0f;
                    engine.publish(s);
                }
                float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                engine.process(chunk, 2, juce::jmin(256, y.getNumSamples() - start));
            }
            const double hf = highBandRelativeDb(y, kFs, 5000.0, 0, 0.1);
            const double floor = highBandRelativeDb(sine, kFs, 5000.0, 0, 0.1);
            expect(floor < -80.0, "metric floor too high: " + juce::String(floor, 1) + " dB");
            expect(hf < -60.0, "HF products " + juce::String(hf, 1) + " dB");
            logMessage("  range jump 250-700 -> 800-2000 Hz: HF > 5 kHz at " + juce::String(hf, 1) + " dB (metric floor "
                       + juce::String(floor, 1) + " dB)");
        }

        beginTest("PRE-EQ automation vs HPDG baseline (-95.0 dB HF products, 8th-order metric)");
        {
            // Same signal and sweep as the M0 HPDG EQ audit: 100 Hz sine -12 dBFS, bell 100 Hz Q 1
            // swept -12 -> +12 dB over 1 s, new target every 512 samples; HF above 2 kHz.
            auto y = makeSine({ kFs, 1, 1.0, kFixtureSeed }, 100.0, -12.0);
            LayerEngine engine;
            engine.prepare(kFs, 512);
            LayerSnapshot s;
            for (int start = 0; start < y.getNumSamples(); start += 512)
            {
                s.eq[0] = bell(100.0f, -12.0f + 24.0f * static_cast<float>(start) / static_cast<float>(y.getNumSamples()), 1.0f);
                engine.publish(s);
                float* chunk[1] { y.getWritePointer(0) + start };
                engine.process(chunk, 1, juce::jmin(512, y.getNumSamples() - start));
            }
            // Baseline: HPDG pattern on the same sweep and metric = -95.0 dB (M0 audit, re-measured in M2).
            const double relDb = highBandRelativeDb(y, kFs, 2000.0);
            expect(relDb < -95.0, "HF products " + juce::String(relDb, 1) + " dB");
            logMessage("  HPEQ PRE-EQ sweep: " + juce::String(relDb, 1) + " dB (HPDG pattern: -95.0 dB)");
        }

        beginTest("compressor: static law and time constants as documented");
        {
            CompressorProcessor comp;
            comp.prepare(kFs);
            const auto p = compSlot(-20, 4, 0, 10, 100);
            // Constant-magnitude input makes the detector level exact.
            for (double levelDb : { -30.0, -20.0, -10.0, 0.0 })
            {
                comp.reset();
                juce::AudioBuffer<float> dc(1, static_cast<int>(kFs));
                for (int i = 0; i < dc.getNumSamples(); ++i)
                    dc.setSample(0, i, static_cast<float>(std::pow(10.0, levelDb / 20.0)));
                float* ch[1] { dc.getWritePointer(0) };
                comp.process(ch, 1, dc.getNumSamples(), p);
                const double outDb = 20.0 * std::log10(dc.getSample(0, dc.getNumSamples() - 1));
                const double expected = levelDb > -20.0 ? -20.0 + (levelDb + 20.0) / 4.0 : levelDb;
                expectWithinAbsoluteError(outDb, expected, 1.0e-4);
            }
            // Soft knee centre: x = T -> reduction (1 - 1/R) * W / 8.
            expectWithinAbsoluteError(CompressorProcessor::staticGainReductionDb(-20, -20, 4, 8), 0.75 * 8 / 8.0, 1.0e-12);

            // Attack: step -40 -> 0 dBFS; reduction reaches 63.2 % of 15 dB at 10 ms (+/- 1 sample).
            comp.reset();
            juce::AudioBuffer<float> step(1, static_cast<int>(0.2 * kFs));
            for (int i = 0; i < step.getNumSamples(); ++i)
                step.setSample(0, i, i < 4800 ? 0.01f : 1.0f);
            float* sp[1] { step.getWritePointer(0) };
            comp.process(sp, 1, step.getNumSamples(), p);
            int reached = -1;
            for (int i = 4800; i < step.getNumSamples(); ++i)
                if (-20.0 * std::log10(step.getSample(0, i)) >= 0.632 * 15.0)
                {
                    reached = i - 4800;
                    break;
                }
            expect(std::abs(reached - 480) <= 1, "attack 63% at " + juce::String(reached) + " samples, expected 480");
        }

        beginTest("decision B: PRE-EQ cut reaches the compressor detector (80 Hz, bell -18 dB)");
        {
            const auto low = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 80.0, -6.0);
            LayerSnapshot withoutEq, withEq;
            withoutEq.fx[0] = node(60, 120, { compSlot(-30, 4, 0, 5, 100) });
            withEq = withoutEq;
            withEq.eq[0] = bell(80.0f, -18.0f, 2.0f);
            const auto measureGr = [&](const LayerSnapshot& s)
            {
                LayerEngine engine;
                engine.prepare(kFs, 512);
                engine.publish(s);
                auto y = low;
                float maxGr = 0.0f;
                for (int start = 0; start < y.getNumSamples(); start += 512)
                {
                    float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    engine.process(chunk, 2, juce::jmin(512, y.getNumSamples() - start));
                    if (start > 24000)
                        maxGr = std::max(maxGr, engine.getNode(0).getGainReductionDb(0));
                }
                return maxGr;
            };
            const float grWithout = measureGr(withoutEq);
            const float grWith = measureGr(withEq);
            expect(grWith < grWithout - 3.0f, "GR with PRE-EQ cut " + juce::String(grWith) + " vs " + juce::String(grWithout));
            logMessage("  compressor GR without PRE-EQ " + juce::String(grWithout, 2) + " dB, with -18 dB bell "
                       + juce::String(grWith, 2) + " dB");
        }

        beginTest("plugin: bypass with active processing settles to the exact dry signal");
        {
            FxPluginProcessor p;
            auto project = p.getProject();
            FxNodeState n;
            n.chain.front().type = FxType::Gain;
            n.chain.front().parameters.set("gainDb", -9.0);
            project.layers[0].fxNodes.push_back(n);
            p.setProject(project);
            p.setRateAndBufferSizeDetails(kFs, 256);
            p.prepareToPlay(kFs, 256);
            auto* bypass = p.getParameters().getParameter(params::bypass.getParamID());
            auto y = input;
            juce::MidiBuffer midi;
            for (int start = 0; start < y.getNumSamples(); start += 256)
            {
                if (start == 12032)
                    bypass->setValueNotifyingHost(1.0f);
                juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(256, y.getNumSamples() - start));
                p.processBlock(view, midi);
            }
            expect(!compareAligned(slice(y, 4800, 7000), slice(input, 4800, 7000)).bitExact, "processing must be audible before bypass");
            const int settled = 12032 + 2048;
            expect(compareAligned(slice(y, settled), slice(input, settled)).bitExact, "bypass not exact");
        }
    }
};

static DspTests dspTests;
} // namespace hpeq::test
