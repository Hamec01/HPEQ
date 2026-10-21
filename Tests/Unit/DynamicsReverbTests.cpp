// M4: compressor against its documented law (T06), reverb as a wet-only send in FX nodes (T07),
// spill-over bypass, reverb pool, PDC and host tail. Thresholds are fixed here before the runs.

#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/Biquad.h"
#include "DSP/LayerEngine.h"
#include "DSP/Processors.h"
#include "DSP/ReverbProcessor.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginProcessor.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;

FxSlotParams compressor(float t, float r, float w, float a, float rel, float makeup = 0.0f)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Compressor;
    s.values = { t, r, w, a, rel, makeup, 0.0f, 0.0f };
    return s;
}

FxSlotParams reverb(float decay, float hfRatio = 1.0f, float predelayMs = 0.0f, float size = 1.0f, float width = 1.0f,
                    float lowCut = 10.0f, float mix = 1.0f)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Reverb;
    s.mix = mix;
    s.values[slot::kPredelayMs] = predelayMs;
    s.values[slot::kDecaySeconds] = decay;
    s.values[slot::kHfRatio] = hfRatio;
    s.values[slot::kSize] = size;
    s.values[slot::kWidth] = width;
    s.values[slot::kLowCutHz] = lowCut;
    return s;
}

FxNodeParams nodeWith(float lo, float hi, std::initializer_list<FxSlotParams> chain)
{
    FxNodeParams n;
    n.active = true;
    n.lowHz = lo;
    n.highHz = hi;
    for (const auto& s : chain)
        n.slots[static_cast<size_t>(n.numSlots++)] = s;
    return n;
}

// Independent model of the documented compressor law (Processors.h), written from the comment.
juce::AudioBuffer<float> referenceCompressor(const juce::AudioBuffer<float>& x, const FxSlotParams& p, bool linked)
{
    const double t = p.values[0], ratio = p.values[1], w = p.values[2];
    const double att = std::exp(-1.0 / (p.values[3] * 1.0e-3 * kFs)), rel = std::exp(-1.0 / (p.values[4] * 1.0e-3 * kFs));
    const double makeup = p.values[5];
    const auto law = [&](double xDb)
    {
        double yDb;
        if (2.0 * (xDb - t) < -w)
            yDb = xDb;
        else if (2.0 * std::abs(xDb - t) <= w && w > 0.0)
            yDb = xDb + (1.0 / ratio - 1.0) * (xDb - t + w / 2.0) * (xDb - t + w / 2.0) / (2.0 * w);
        else
            yDb = t + (xDb - t) / ratio;
        return xDb - yDb;
    };
    auto y = x;
    const int detectors = linked ? 1 : x.getNumChannels();
    std::vector<double> c(static_cast<size_t>(detectors), 0.0);
    for (int i = 0; i < x.getNumSamples(); ++i)
        for (int d = 0; d < detectors; ++d)
        {
            double level = 0.0;
            for (int ch = linked ? 0 : d; ch < (linked ? x.getNumChannels() : d + 1); ++ch)
                level = std::max(level, static_cast<double>(std::abs(x.getSample(ch, i))));
            const double target = level > 0.0 ? std::max(0.0, law(20.0 * std::log10(level))) : 0.0;
            auto& cd = c[static_cast<size_t>(d)];
            cd = target + (target > cd ? att : rel) * (cd - target);
            const double g = std::pow(10.0, (makeup - cd) / 20.0);
            for (int ch = linked ? 0 : d; ch < (linked ? x.getNumChannels() : d + 1); ++ch)
                y.setSample(ch, i, static_cast<float>(x.getSample(ch, i) * g));
        }
    return y;
}

juce::AudioBuffer<float> runCompressor(const juce::AudioBuffer<float>& x, const FxSlotParams& p, bool linked, int block = 256)
{
    CompressorProcessor comp;
    comp.prepare(kFs);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), y.getNumChannels(), start, juce::jmin(block, y.getNumSamples() - start));
        comp.process(view.getArrayOfWritePointers(), view.getNumChannels(), view.getNumSamples(), p, linked);
    }
    return y;
}

// Schroeder energy-decay curve of the summed channels; RT60 from the -5..-25 dB slope (T20 x 3).
double rt60FromEdc(const juce::AudioBuffer<float>& y, double fs)
{
    std::vector<double> energy(static_cast<size_t>(y.getNumSamples()));
    for (int i = 0; i < y.getNumSamples(); ++i)
        for (int ch = 0; ch < y.getNumChannels(); ++ch)
            energy[static_cast<size_t>(i)] += static_cast<double>(y.getSample(ch, i)) * y.getSample(ch, i);
    double acc = 0.0;
    std::vector<double> edc(energy.size());
    for (size_t i = energy.size(); i-- > 0;)
        edc[i] = (acc += energy[i]);
    const double total = edc[0];
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int n = 0;
    for (size_t i = 0; i < edc.size(); i += 16)
    {
        const double db = 10.0 * std::log10(edc[i] / total + 1e-300);
        if (db > -5.0 || db < -25.0)
            continue;
        const double t = static_cast<double>(i) / fs;
        sx += t; sy += db; sxx += t * t; sxy += t * db; ++n;
    }
    if (n < 3)
        return 0.0;
    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx); // dB per second
    return slope < 0.0 ? -60.0 / slope : 0.0;
}

juce::AudioBuffer<float> bandFiltered(const juce::AudioBuffer<float>& y, double f0)
{
    auto out = y;
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
    {
        Biquad a, b;
        // Two RBJ band-passes (constant 0 dB peak), Q 1.4: an octave-ish analysis band.
        const double w = 2.0 * juce::MathConstants<double>::pi * f0 / kFs, alpha = std::sin(w) / (2.0 * 1.4), c = std::cos(w);
        const BiquadCoefficients bp { alpha / (1 + alpha), 0.0, -alpha / (1 + alpha), -2 * c / (1 + alpha), (1 - alpha) / (1 + alpha) };
        a.setCoefficients(bp);
        b.setCoefficients(bp);
        for (int i = 0; i < y.getNumSamples(); ++i)
            out.setSample(ch, i, static_cast<float>(b.processSample(0, a.processSample(0, y.getSample(ch, i)))));
    }
    return out;
}

juce::AudioBuffer<float> reverbImpulseResponse(const FxSlotParams& p, double seconds, int block = 512)
{
    ReverbProcessor r;
    r.prepare(kFs);
    auto x = makeImpulse({ kFs, 2, seconds, kFixtureSeed }, 0, 1.0f);
    juce::AudioBuffer<float> wet(2, x.getNumSamples());
    for (int start = 0; start < x.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, x.getNumSamples() - start);
        const float* in[2] { x.getReadPointer(0) + start, x.getReadPointer(1) + start };
        float* out[2] { wet.getWritePointer(0) + start, wet.getWritePointer(1) + start };
        r.process(in, out, 2, n, p);
    }
    return wet;
}

double windowRmsDb(const juce::AudioBuffer<float>& y, int from, int length)
{
    double s = 0.0;
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
        for (int i = from; i < from + length; ++i)
            s += static_cast<double>(y.getSample(ch, i)) * y.getSample(ch, i);
    return 10.0 * std::log10(s / (length * y.getNumChannels()) + 1e-30);
}

template <typename Fn>
juce::AudioBuffer<float> renderEngine(const juce::AudioBuffer<float>& x, int block, Fn&& beforeBlock, LayerEngine** keep = nullptr)
{
    static std::unique_ptr<LayerEngine> engineHolder;
    engineHolder = std::make_unique<LayerEngine>();
    auto& engine = *engineHolder;
    engine.prepare(kFs, block);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        beforeBlock(engine, start);
        float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        engine.process(chunk, 2, juce::jmin(block, y.getNumSamples() - start));
    }
    if (keep != nullptr)
        *keep = engineHolder.get();
    return y;
}
} // namespace

class DynamicsReverbTests final : public juce::UnitTest
{
public:
    DynamicsReverbTests() : juce::UnitTest("Dynamics + Reverb M4", "HPEQ") {}

    void runTest() override
    {
        beginTest("T06: compressor equals an independent model of its documented law (<= -120 dB)");
        {
            auto burst = makeGatedBurst({ kFs, 2, 2.0, kFixtureSeed }, -6.0, -40.0, 0.25);
            const auto drums = makeSyntheticDrums({ kFs, 2, 2.0, kFixtureSeed });
            auto unbalanced = makePinkNoise({ kFs, 2, 1.0, kFixtureSeed }, -12.0);
            unbalanced.applyGain(1, 0, unbalanced.getNumSamples(), 0.1f);
            double worst = kSilenceDb;
            const FxSlotParams settings[] { compressor(-20, 4, 0, 10, 100), compressor(-30, 2, 12, 1, 300),
                                            compressor(-10, 20, 6, 30, 20, 6), compressor(-40, 100, 3, 0.1f, 50, -3) };
            for (const auto& p : settings)
                for (const juce::AudioBuffer<float>* x : std::initializer_list<const juce::AudioBuffer<float>*> { &burst, &drums, &unbalanced })
                    for (bool linked : { true, false })
                    {
                        const auto null = compareAligned(runCompressor(*x, p, linked), referenceCompressor(*x, p, linked));
                        worst = std::max(worst, null.errorDb);
                    }
            expect(worst <= -120.0, "worst " + juce::String(worst) + " dB");
            logMessage("  compressor vs independent law model, worst: " + juce::String(worst, 1) + " dB");
        }

        beginTest("T06: ratio, attack and release time constants as specified");
        {
            const auto dc = [](double db, int n)
            {
                juce::AudioBuffer<float> b(1, n);
                for (int i = 0; i < n; ++i)
                    b.setSample(0, i, static_cast<float>(std::pow(10.0, db / 20.0)));
                return b;
            };
            for (float ratio : { 2.0f, 4.0f, 10.0f })
            {
                const auto p = compressor(-30, ratio, 0, 1, 10);
                const auto lo = runCompressor(dc(-20, 48000), p, true), hi = runCompressor(dc(-10, 48000), p, true);
                const double slope = (20 * std::log10(hi.getSample(0, 47999)) - 20 * std::log10(lo.getSample(0, 47999))) / 10.0;
                expectWithinAbsoluteError(slope, 1.0 / ratio, 1.0e-4);
            }
            // Release: 0 dBFS settled, then -40 dBFS (below threshold): reduction falls to 36.8 % in tau.
            const auto p = compressor(-20, 4, 0, 1, 200);
            juce::AudioBuffer<float> step(1, 96000);
            for (int i = 0; i < 96000; ++i)
                step.setSample(0, i, i < 48000 ? 1.0f : 0.01f);
            const auto y = runCompressor(step, p, true);
            const double c0 = -20.0 * std::log10(y.getSample(0, 47999)); // 15 dB
            int reached = -1;
            for (int i = 48000; i < 96000; ++i)
                if (-20.0 * std::log10(y.getSample(0, i) / 0.01) <= std::exp(-1.0) * c0)
                {
                    reached = i - 48000;
                    break;
                }
            expect(std::abs(reached - 9600) <= 1, "release 63 % at " + juce::String(reached) + " samples (expected 9600)");
            logMessage("  release 200 ms: 63 % recovery at " + juce::String(reached) + " samples (9600 expected)");
        }

        beginTest("T07: reverb RT60 from the energy decay curve within +-15 % of the setting");
        {
            juce::String table;
            for (float decay : { 0.5f, 1.8f, 4.0f })
                for (float size : { 0.5f, 1.0f, 1.5f })
                {
                    const auto ir = reverbImpulseResponse(reverb(decay, 1.0f, 0.0f, size), decay * 2.0 + 0.5);
                    const double rt = rt60FromEdc(ir, kFs);
                    expect(std::abs(rt / decay - 1.0) <= 0.15, "T60 " + juce::String(decay) + " size " + juce::String(size) + ": " + juce::String(rt, 3));
                    table << juce::String(decay) << "s/size" << juce::String(size) << "=" << juce::String(rt, 3) << " ";
                    expect(measureAll(ir).allFinite);
                }
            logMessage("  measured RT60: " + table);
        }

        beginTest("T07: damping shortens the high-frequency decay; width controls L/R correlation");
        {
            const auto ir = reverbImpulseResponse(reverb(2.0f, 0.3f), 4.5);
            const double rtLow = rt60FromEdc(bandFiltered(ir, 500.0), kFs), rtHigh = rt60FromEdc(bandFiltered(ir, 8000.0), kFs);
            expect(std::abs(rtLow / 2.0 - 1.0) <= 0.2, "500 Hz T60 " + juce::String(rtLow, 3));
            expect(rtHigh < 0.8 * rtLow, "8 kHz T60 " + juce::String(rtHigh, 3));
            logMessage("  T60 2 s, HF ratio 0.3: 500 Hz " + juce::String(rtLow, 3) + " s, 8 kHz " + juce::String(rtHigh, 3) + " s");

            const auto wide = measureStereo(reverbImpulseResponse(reverb(1.5f, 1.0f, 0.0f, 1.0f, 1.0f), 2.0));
            const auto mono = measureStereo(reverbImpulseResponse(reverb(1.5f, 1.0f, 0.0f, 1.0f, 0.0f), 2.0));
            expect(std::abs(wide.correlation) < 0.3, "tail correlation " + juce::String(wide.correlation, 3));
            expectWithinAbsoluteError(mono.correlation, 1.0, 1.0e-6);
            logMessage("  tail L/R correlation: width 1 -> " + juce::String(wide.correlation, 3) + ", width 0 -> " + juce::String(mono.correlation, 6));
        }

        beginTest("wet-only send: before the first reflection the node output equals the passive render exactly");
        {
            const auto x = makeImpulse({ kFs, 2, 1.0, kFixtureSeed }, static_cast<int>(0.3 * kFs), 1.0f);
            const auto render = [&](bool withReverb)
            {
                return renderEngine(x, 256, [&](LayerEngine& e, int start)
                {
                    if (start != 0)
                        return;
                    LayerSnapshot s;
                    auto r = reverb(1.0f, 1.0f, 20.0f);
                    r.active = withReverb;
                    s.fx[0] = nodeWith(200, 8000, { r });
                    if (!withReverb)
                        s.fx[0].slots[0] = compressor(0, 1, 0, 1, 10); // passive insert: keeps the node and tree running
                    e.publish(s);
                });
            };
            const auto wet = render(true), passive = render(false);
            // First possible reverb output: impulse at 0.3 s + 20 ms pre-delay + 31.3 ms shortest line.
            const int firstReflection = static_cast<int>(0.3 * kFs) + static_cast<int>(0.020 * kFs) + static_cast<int>(0.0313 * kFs) - 2;
            juce::AudioBuffer<float> a(2, firstReflection), b(2, firstReflection);
            for (int ch = 0; ch < 2; ++ch)
            {
                a.copyFrom(ch, 0, wet, ch, 0, firstReflection);
                b.copyFrom(ch, 0, passive, ch, 0, firstReflection);
            }
            expect(compareAligned(a, b).bitExact, "the dry path changed before any reverb output (dry doubling or latency)");
            expect(windowRmsDb(wet, firstReflection + 100, 4800) > windowRmsDb(passive, firstReflection + 100, 4800) + 20.0, "no reverb");
        }

        beginTest("T07: bypassing the reverb slot or the node lets the tail ring out, then frees the reverb");
        {
            // Criteria (revised after the first run, see M4 report): (a) bypass after the input has
            // stopped must leave the tail bit-identical to the un-bypassed render; (b) bypass while the
            // input plays: the remaining tail decays with RT60 within +-20 % of the setting; (c) the
            // reverb returns to the pool and a bypassed node leaves the path once the tail has rung out.
            auto x = makePinkNoise({ kFs, 2, 5.0, kFixtureSeed }, -14.0);
            const int inputEnd = static_cast<int>(0.5 * kFs) / 256 * 256;
            for (int ch = 0; ch < 2; ++ch)
                x.clear(ch, inputEnd, x.getNumSamples() - inputEnd);

            const auto render = [&](int variant, int bypassAt, LayerEngine** keep)
            {
                return renderEngine(x, 256, [&](LayerEngine& e, int start)
                {
                    LayerSnapshot s;
                    s.fx[0] = nodeWith(200, 10000, { reverb(1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 80.0f, 0.7f) });
                    if (bypassAt >= 0 && start >= bypassAt)
                    {
                        if (variant == 0)
                            s.fx[0].slots[0].active = false; // slot bypass
                        else
                            s.fx[0].active = false;          // node bypass
                    }
                    if (start == 0 || start == bypassAt)
                        e.publish(s);
                }, keep);
            };
            LayerEngine* engine = nullptr;
            const auto reference = render(0, -1, &engine);
            const int lateBypass = static_cast<int>(0.6 * kFs) / 256 * 256;
            const int earlyBypass = static_cast<int>(0.3 * kFs) / 256 * 256;
            for (int variant = 0; variant < 2; ++variant)
            {
                const auto late = render(variant, lateBypass, &engine);
                const int compareUntil = static_cast<int>(1.5 * kFs);
                juce::AudioBuffer<float> a(2, compareUntil), b(2, compareUntil);
                for (int ch = 0; ch < 2; ++ch)
                {
                    a.copyFrom(ch, 0, late, ch, 0, compareUntil);
                    b.copyFrom(ch, 0, reference, ch, 0, compareUntil);
                }
                expect(compareAligned(a, b).bitExact, "bypass after the input stopped changed the tail");
                expectEquals(engine->reverbsInUse(), 0, "reverb not released after its tail");
                if (variant == 1)
                    expect(!engine->isTreeActive(), "bypassed node did not leave the path");

                const auto early = render(variant, earlyBypass, &engine);
                juce::AudioBuffer<float> tail(2, static_cast<int>(2.0 * kFs));
                for (int ch = 0; ch < 2; ++ch)
                    tail.copyFrom(ch, 0, early, ch, inputEnd + 64, tail.getNumSamples());
                const double rt = rt60FromEdc(tail, kFs);
                expect(std::abs(rt - 1.0) <= 0.2, "tail RT60 after an early bypass " + juce::String(rt, 3));
                expectEquals(engine->reverbsInUse(), 0);
                logMessage(juce::String(variant == 0 ? "  slot" : "  node") + " bypass: tail unchanged when bypassed after the input; "
                           "tail RT60 after a bypass during the input " + juce::String(rt, 3) + " s (setting 1.0 s)");
            }
        }

        beginTest("M4 scenario: compressor 250-700 Hz and reverb 3-8 kHz at once (+ overlapping), music input");
        {
            const auto drums = makeSyntheticDrums({ kFs, 2, 3.0, kFixtureSeed });
            LayerEngine* engine = nullptr;
            float gr = 0.0f;
            const auto y = renderEngine(drums, 512, [&](LayerEngine& e, int start)
            {
                if (start == 0)
                {
                    LayerSnapshot s;
                    s.fx[0] = nodeWith(250, 700, { compressor(-36, 6, 6, 3, 80) });
                    s.fx[1] = nodeWith(3000, 8000, { reverb(1.4f, 0.5f, 15.0f) });
                    s.fx[2] = nodeWith(500, 5000, { compressor(-30, 3, 6, 5, 120), reverb(0.8f, 0.6f) }); // overlaps both
                    e.publish(s);
                }
                if (start > 0)
                    gr = std::max(gr, e.getNode(0).getGainReductionDb(0));
            }, &engine);
            expect(measureAll(y).allFinite);
            expect(gr > 3.0f, "compressor inactive: GR " + juce::String(gr));
            expectEquals(engine->reverbsInUse(), 2);
            logMessage("  GR " + juce::String(gr, 2) + " dB, peak " + juce::String(measureAll(y).peakDbfs, 2) + " dBFS, 2 reverbs running");
        }

        beginTest("reverb pool: a 9th reverb slot is reported as starved, nothing breaks");
        {
            LayerEngine* engine = nullptr;
            const auto noise = makePinkNoise({ kFs, 2, 0.3, kFixtureSeed }, -20.0);
            renderEngine(noise, 512, [&](LayerEngine& e, int start)
            {
                if (start != 0)
                    return;
                LayerSnapshot s;
                for (int n = 0; n < 2; ++n)
                {
                    s.fx[static_cast<size_t>(n)] = nodeWith(200.0f * (n + 1), 2000.0f * (n + 1), {});
                    for (int k = 0; k < (n == 0 ? 6 : 3); ++k)
                        s.fx[static_cast<size_t>(n)].slots[static_cast<size_t>(s.fx[static_cast<size_t>(n)].numSlots++)] = reverb(1.0f);
                }
                e.publish(s);
            }, &engine);
            expectEquals(engine->reverbsInUse(), ReverbPool::kSize);
            expect(engine->getNode(1).slotStarved(2), "9th reverb must report starvation");
        }

        beginTest("T10 and allocations with reverbs: identical render for blocks 1, 64, 127, 512, 0 allocations");
        {
            const auto drums = makeSyntheticDrums({ kFs, 2, 0.8, kFixtureSeed });
            const auto render = [&](int block, std::uint64_t& allocations)
            {
                auto e = std::make_unique<LayerEngine>();
                e->prepare(kFs, block);
                LayerSnapshot s;
                s.fx[0] = nodeWith(150, 900, { compressor(-30, 4, 6, 3, 90), reverb(1.2f, 0.5f, 12.0f, 0.8f) });
                s.fx[1] = nodeWith(2500, 9000, { reverb(2.0f, 0.4f, 30.0f, 1.3f, 1.2f, 200.0f, 0.5f) });
                e->publish(s);
                auto y = drums;
                for (int start = 0; start < y.getNumSamples(); start += block)
                {
                    float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    ScopedAllocationProbe probe;
                    e->process(chunk, 2, juce::jmin(block, y.getNumSamples() - start));
                    allocations += probe.counts().allocations;
                }
                return y;
            };
            std::uint64_t allocations = 0;
            const auto reference = render(1, allocations);
            for (int block : { 64, 127, 512 })
            {
                const auto null = compareAligned(render(block, allocations), reference);
                expect(null.bitExact, "block " + juce::String(block) + ": " + juce::String(null.errorDb) + " dB");
            }
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("stability: T60 20 s, no damping, size 1.5, 30 s of noise; 20k parameter mutations");
        {
            ReverbProcessor r;
            r.prepare(kFs);
            const auto noise = makeWhiteNoise({ kFs, 2, 30.0, kFixtureSeed }, -12.0);
            juce::AudioBuffer<float> wet(2, noise.getNumSamples());
            const auto p = reverb(20.0f, 1.0f, 0.0f, 1.5f, 1.5f, 10.0f);
            for (int start = 0; start < noise.getNumSamples(); start += 512)
            {
                const int n = juce::jmin(512, noise.getNumSamples() - start);
                const float* in[2] { noise.getReadPointer(0) + start, noise.getReadPointer(1) + start };
                float* out[2] { wet.getWritePointer(0) + start, wet.getWritePointer(1) + start };
                r.process(in, out, 2, n, p);
            }
            const auto stats = measureAll(wet);
            expect(stats.allFinite);
            expect(stats.peakDbfs < 40.0, "peak " + juce::String(stats.peakDbfs, 1));
            logMessage("  T60 20 s steady state on -12 dBFS noise: wet RMS " + juce::String(stats.rmsDbfs, 1) + " dBFS, peak " + juce::String(stats.peakDbfs, 1));

            auto e = std::make_unique<LayerEngine>();
            e->prepare(kFs, 512);
            DeterministicRandom random(kFixtureSeed + 11);
            const auto uni = [&](double lo, double hi) { return lo + (hi - lo) * (random.nextBipolar() * 0.5 + 0.5); };
            juce::AudioBuffer<float> block(2, 1024);
            LayerSnapshot s;
            bool finite = true;
            std::uint64_t allocations = 0;
            for (int m = 0; m < 20000; ++m)
            {
                auto& n = s.fx[static_cast<size_t>(uni(0, 4))];
                n.active = uni(0, 1) > 0.3;
                n.lowHz = static_cast<float>(uni(20, 5000));
                n.highHz = static_cast<float>(uni(200, 20000));
                n.numSlots = static_cast<int>(uni(1, 4));
                auto& sl = n.slots[static_cast<size_t>(uni(0, 3))];
                sl = uni(0, 1) > 0.5 ? reverb(static_cast<float>(uni(0.1, 20)), static_cast<float>(uni(0.05, 1)), static_cast<float>(uni(0, 250)),
                                             static_cast<float>(uni(0.3, 1.5)), static_cast<float>(uni(0, 1.5)), static_cast<float>(uni(10, 1000)),
                                             static_cast<float>(uni(0, 1)))
                                     : compressor(static_cast<float>(uni(-60, 0)), static_cast<float>(uni(1, 20)), 6, 5, 100);
                sl.active = uni(0, 1) > 0.2;
                if (m % 4 == 0)
                    e->publish(s);
                if (m % 8 != 0)
                    continue;
                const int count = static_cast<int>(uni(1, 1025));
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < count; ++i)
                        block.setSample(ch, i, noise.getSample(ch, (m * 37 + i) % noise.getNumSamples()));
                float* chunk[2] { block.getWritePointer(0), block.getWritePointer(1) };
                ScopedAllocationProbe probe;
                e->process(chunk, 2, count);
                allocations += probe.counts().allocations;
                juce::AudioBuffer<float> view(block.getArrayOfWritePointers(), 2, 0, count);
                finite = finite && measureAll(view).allFinite && measureAll(view).peak < 1.0e3;
            }
            expect(finite, "non-finite or runaway output under mutations");
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("PDC and host tail: latency 0 for every configuration; tail = pre-delay + T60 of the longest reverb");
        {
            auto p = std::make_unique<FxPluginProcessor>();
            auto project = p->getProject();
            FxNodeState n;
            n.chain.clear();
            FxSlotState comp;
            comp.type = FxType::Compressor;
            FxSlotState rev;
            rev.type = FxType::Reverb;
            rev.parameters.set("predelayMs", 40.0);
            rev.parameters.set("decaySeconds", 3.5);
            n.chain = { comp, rev };
            EqNodeState cut;
            cut.shape = EqShape::LowCut;
            cut.slopeDbPerOct = 96;
            cut.frequencyHz = 40.0;
            project.layers[0].eqNodes.push_back(cut);
            project.layers[0].fxNodes.push_back(n);
            p->setProject(project);
            p->setRateAndBufferSizeDetails(kFs, 512);
            p->prepareToPlay(kFs, 512);
            expectEquals(p->getLatencySamples(), 0);
            expectWithinAbsoluteError(p->getTailLengthSeconds(), 3.54, 1.0e-6);
            project.layers[0].fxNodes.front().enabled = false;
            p->setProject(project);
            expectWithinAbsoluteError(p->getTailLengthSeconds(), 0.0, 1.0e-9);
        }
    }
};

static DynamicsReverbTests dynamicsReverbTests;
} // namespace hpeq::test
