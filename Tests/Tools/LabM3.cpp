// HPEQ_Lab m3 --out <dir> [--commit <sha>]
//
// M3 experiments: PRE-EQ shapes and slopes (T03 evidence: measured vs analytic response, slope fits,
// group delay, impulse), and the automation-zipper comparison of filter structures.

#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <iostream>

#include "DSP/Biquad.h"
#include "DSP/EqDesign.h"
#include "DSP/LayerEngine.h"
#include "DSP/Ramp.h"
#include "Harness/AudioFileIO.h"
#include "Harness/AudioMetrics.h"
#include "Harness/Json.h"
#include "Harness/MachineInfo.h"
#include "Harness/SvgPlot.h"
#include "Harness/TestSignalFactory.h"

using namespace hpeq;
using namespace hpeq::dsp;
using namespace hpeq::test;

namespace
{
constexpr double kFs = 48000.0;

// The M0/M2 zipper sweep: 100 Hz sine -12 dBFS, bell 100 Hz Q 1, -12 -> +12 dB over 1 s, new target
// every 512 samples; parameters glide over 30 ms on a 32-sample control clock.
juce::AudioBuffer<float> zipperSignal() { return makeSine({ kFs, 1, 1.0, kFixtureSeed }, 100.0, -12.0); }

double targetGainAt(int start, int length) { return -12.0 + 24.0 * static_cast<double>(start) / length; }

double zipperEngine()
{
    auto y = zipperSignal();
    auto enginePtr = std::make_unique<LayerEngine>(); // ~115 KB: keep it off the stack
    auto& engine = *enginePtr;
    engine.prepare(kFs, 512);
    LayerSnapshot s;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        s.eq[0].active = true;
        s.eq[0].shape = EqShape::Bell;
        s.eq[0].frequencyHz = 100.0f;
        s.eq[0].q = 1.0f;
        s.eq[0].gainDb = static_cast<float>(targetGainAt(start, y.getNumSamples()));
        engine.publish(s);
        float* chunk[1] { y.getWritePointer(0) + start };
        engine.process(chunk, 1, juce::jmin(512, y.getNumSamples() - start));
    }
    return highBandRelativeDb(y, kFs, 2000.0);
}

template <typename Filter>
double zipperGeneric(Filter&& filterFactory, int tick = kControlInterval)
{
    auto y = zipperSignal();
    LinearRamp gain;
    gain.reset(-12.0);
    const int steps = stepsFor(kParameterRampSeconds, kFs / tick);
    auto filter = filterFactory();
    filter.set(gain.value());
    for (int i = 0; i < y.getNumSamples(); ++i)
    {
        if (i % 512 == 0)
            gain.setTarget(targetGainAt(i, y.getNumSamples()), steps);
        if (i % tick == 0 && gain.isRamping())
            filter.set(gain.next());
        y.setSample(0, i, static_cast<float>(filter.process(y.getSample(0, i))));
    }
    return highBandRelativeDb(y, kFs, 2000.0);
}

struct RbjBell
{
    Biquad b;
    void set(double db) { b.setCoefficients(design::peak(kFs, 100.0, 1.0, db)); }
    double process(double x) { return b.processSample(0, x); }
};

// TPT SVF with arbitrary (k, m0, m1, m2), as the engine's sections.
struct Svf
{
    double g = 0, k = 1, m0 = 1, m1 = 0, m2 = 0, s1 = 0, s2 = 0;
    double process(double x)
    {
        const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const double v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2 * v1 - s1;
        s2 = 2 * v2 - s2;
        return m0 * x + m1 * v1 + m2 * v2;
    }
};

struct SvfBellProportional // Simper/RBJ-equivalent: damping k = 1/(Q A) follows the gain
{
    Svf f;
    void set(double db)
    {
        const double a = std::pow(10.0, db / 40.0);
        f.g = std::tan(juce::MathConstants<double>::pi * 100.0 / kFs);
        f.k = 1.0 / a;
        f.m1 = f.k * (a * a - 1.0);
    }
    double process(double x) { return f.process(x); }
};

struct SvfBellConstantDamping // damping fixed at 1/Q, only the output mix follows the gain
{
    Svf f;
    void set(double db)
    {
        const double a2 = std::pow(10.0, db / 20.0);
        f.g = std::tan(juce::MathConstants<double>::pi * 100.0 / kFs);
        f.k = 1.0;
        f.m1 = f.k * (a2 - 1.0);
    }
    double process(double x) { return f.process(x); }
};

// Renders x through a fresh engine after 0.3 s of silence (entry glides finish there).
juce::AudioBuffer<float> renderBand(const EqBandParams& b, const juce::AudioBuffer<float>& x)
{
    auto engine = std::make_unique<LayerEngine>();
    engine->prepare(kFs, 512);
    LayerSnapshot s;
    s.eq[0] = b;
    engine->publish(s);
    const int pre = static_cast<int>(0.3 * kFs);
    juce::AudioBuffer<float> y(x.getNumChannels(), x.getNumSamples() + pre);
    y.clear();
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, pre, x, ch, 0, x.getNumSamples());
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        float* chunk[kMaxChannels] {};
        for (int ch = 0; ch < y.getNumChannels(); ++ch)
            chunk[ch] = y.getWritePointer(ch) + start;
        engine->process(chunk, y.getNumChannels(), juce::jmin(512, y.getNumSamples() - start));
    }
    juce::AudioBuffer<float> out(x.getNumChannels(), x.getNumSamples());
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        out.copyFrom(ch, 0, y, ch, pre, x.getNumSamples());
    return out;
}

EqBandParams eqBand(EqShape shape, float hz, float db, float q, int slope)
{
    EqBandParams b;
    b.active = true;
    b.shape = shape;
    b.frequencyHz = hz;
    b.gainDb = db;
    b.q = q;
    b.slopeDbPerOct = slope;
    return b;
}

std::vector<double> grid()
{
    std::vector<double> g;
    for (int i = 0; i < 300; ++i)
        g.push_back(10.0 * std::pow(2160.0, i / 299.0)); // 10 Hz .. 21.6 kHz
    return g;
}

struct Measured
{
    std::vector<std::pair<double, double>> magnitude, groupDelayMs;
};

Measured measure(const juce::AudioBuffer<float>& y, const juce::AudioBuffer<float>& x)
{
    Measured m;
    const auto points = measureTransfer(y, x, kFs, grid(), 0, -110.0);
    std::vector<double> phase, freq;
    for (const auto& p : points)
    {
        m.magnitude.emplace_back(p.frequencyHz, p.magnitudeDb.value_or(std::nan("")));
        if (p.phaseRadians.has_value())
        {
            phase.push_back(*p.phaseRadians);
            freq.push_back(p.binFrequencyHz);
        }
    }
    for (size_t i = 1; i < phase.size(); ++i)
        while (phase[i] - phase[i - 1] > juce::MathConstants<double>::pi) phase[i] -= juce::MathConstants<double>::twoPi;
    for (size_t i = 1; i < phase.size(); ++i)
        while (phase[i] - phase[i - 1] < -juce::MathConstants<double>::pi) phase[i] += juce::MathConstants<double>::twoPi;
    for (size_t i = 1; i + 1 < phase.size(); ++i)
    {
        const double dw = juce::MathConstants<double>::twoPi * (freq[i + 1] - freq[i - 1]);
        if (dw > 0)
            m.groupDelayMs.emplace_back(freq[i], -(phase[i + 1] - phase[i - 1]) / dw * 1000.0);
    }
    return m;
}

// Impulse response facts: peak position and time until the envelope stays 60 dB below its peak.
juce::var impulseFacts(const EqBandParams& b)
{
    const auto impulse = makeImpulse({ kFs, 1, 1.0, kFixtureSeed }, 0, 1.0f);
    const auto y = renderBand(b, impulse);
    int peak = 0;
    for (int i = 0; i < y.getNumSamples(); ++i)
        if (std::abs(y.getSample(0, i)) > std::abs(y.getSample(0, peak)))
            peak = i;
    const double threshold = std::abs(y.getSample(0, peak)) * 1.0e-3;
    int last = 0;
    for (int i = 0; i < y.getNumSamples(); ++i)
        if (std::abs(y.getSample(0, i)) > threshold)
            last = i;
    return JsonObject().set("peak_sample", peak).set("ring_to_minus60db_ms", 1000.0 * last / kFs)
        .set("all_finite", measureAll(y).allFinite);
}


// p50/p99 of LayerEngine::process for 10 s of stereo pink noise in blocks of 512 at 48 kHz.
// `automate`: publish a slightly different snapshot every block (all bands gliding all the time).
juce::var eqTiming(const std::function<LayerSnapshot(int block)>& snapshotFor, bool automate)
{
    auto engine = std::make_unique<LayerEngine>();
    engine->prepare(kFs, 512);
    engine->publish(snapshotFor(0));
    const auto source = makePinkNoise({ kFs, 2, 10.0, kFixtureSeed }, -18.0);
    juce::AudioBuffer<float> work(2, 512);
    std::vector<double> us;
    int index = 0;
    for (int start = 0; start + 512 <= source.getNumSamples(); start += 512, ++index)
    {
        for (int ch = 0; ch < 2; ++ch)
            work.copyFrom(ch, 0, source, ch, start, 512);
        if (automate)
            engine->publish(snapshotFor(index));
        const auto t0 = std::chrono::steady_clock::now();
        engine->process(work.getArrayOfWritePointers(), 2, 512);
        us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
    }
    const auto t = summarise(std::move(us));
    const double budget = 1.0e6 * 512 / kFs;
    return JsonObject().set("p50_us", t.p50).set("p99_us", t.p99).set("max_us", t.max).set("p99_budget_fraction", t.p99 / budget);
}

const char* kColours[] { "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#e377c2", "#17becf", "#7f7f7f" };
} // namespace

int runM3Lab(const juce::File& out, const juce::String& commit)
{
    std::cout << "zipper probe\n";
    const auto clean = zipperSignal();
    auto zipper = JsonObject()
                      .set("metric_floor_clean_sine_db", highBandRelativeDb(clean, kFs, 2000.0))
                      .set("hpdg_pattern_db", -95.0)
                      .set("engine_db", zipperEngine())
                      .set("rbj_tdf2_db", zipperGeneric([] { return RbjBell {}; }))
                      .set("svf_proportional_q_db", zipperGeneric([] { return SvfBellProportional {}; }))
                      .set("svf_constant_damping_db", zipperGeneric([] { return SvfBellConstantDamping {}; }))
                      .set("svf_proportional_q_per_sample_db", zipperGeneric([] { return SvfBellProportional {}; }, 1))
                      .set("svf_proportional_q_tick8_db", zipperGeneric([] { return SvfBellProportional {}; }, 8))
                      .set("rbj_tdf2_per_sample_db", zipperGeneric([] { return RbjBell {}; }, 1));
    std::cout << juce::JSON::toString(zipper) << "\n";

    std::cout << "slopes and shapes\n";
    const auto sweep = makeExpSweep({ kFs, 1, 4.0, kFixtureSeed }, 5.0, 0.45 * kFs, -6.0);
    auto slopes = jsonArray();
    std::vector<PlotSeries> lowCutMag, highCutMag, lowCutDelay;
    int colour = 0;
    for (int slope : kEqSlopes)
    {
        const auto lc = eqBand(EqShape::LowCut, 200.0f, 0, 0.707f, slope);
        const auto hc = eqBand(EqShape::HighCut, 2000.0f, 0, 0.707f, slope);
        const auto mLc = measure(renderBand(lc, sweep), sweep);
        const auto mHc = measure(renderBand(hc, sweep), sweep);
        const auto c = juce::String(kColours[colour++ % 9]);
        lowCutMag.push_back({ juce::String(slope) + " dB/oct", c, mLc.magnitude });
        highCutMag.push_back({ juce::String(slope) + " dB/oct", c, mHc.magnitude });
        lowCutDelay.push_back({ juce::String(slope) + " dB/oct", c, mLc.groupDelayMs });
        double gdAt400 = 0.0;
        for (const auto& [f, g] : mLc.groupDelayMs)
            if (f >= 400.0) { gdAt400 = g; break; }
        const double at100 = 20.0 * std::log10(std::abs(bandResponse(designBand(EqShape::LowCut, slope, kFs, 200.0, 0, 0.707), kFs, 100.0)));
        slopes.append(JsonObject()
                          .set("slope_db_per_oct", slope)
                          .set("sections", designBand(EqShape::LowCut, slope, kFs, 200.0, 0, 0.707).count)
                          .set("low_cut_200_analytic_at_100hz_db", at100)
                          .set("low_cut_200_group_delay_at_400hz_ms", gdAt400)
                          .set("low_cut_200_impulse", impulseFacts(lc))
                          .set("high_cut_2k_impulse", impulseFacts(hc)));
    }
    PlotSpec lcSpec { "Low Cut 200 Hz, all slopes (measured, 48 kHz)", "Frequency (Hz)", "dB" };
    lcSpec.xMin = 10; lcSpec.yMin = -120; lcSpec.yMax = 6; lcSpec.yStep = 12;
    writeSvgPlot(out.getChildFile("plots/low_cut_200_slopes.svg"), lcSpec, lowCutMag);
    PlotSpec hcSpec { "High Cut 2 kHz, all slopes (measured, 48 kHz)", "Frequency (Hz)", "dB" };
    hcSpec.xMin = 10; hcSpec.yMin = -120; hcSpec.yMax = 6; hcSpec.yStep = 12;
    writeSvgPlot(out.getChildFile("plots/high_cut_2k_slopes.svg"), hcSpec, highCutMag);
    PlotSpec gdSpec { "Low Cut 200 Hz: group delay (measured)", "Frequency (Hz)", "ms" };
    gdSpec.xMin = 100; gdSpec.yMin = 0; gdSpec.yMax = 12; gdSpec.yStep = 2;
    writeSvgPlot(out.getChildFile("plots/low_cut_200_group_delay.svg"), gdSpec, lowCutDelay);

    std::vector<PlotSeries> shapes;
    colour = 0;
    for (const auto& [name, b] : { std::pair<const char*, EqBandParams> { "Bell 1k +6 Q1.4", eqBand(EqShape::Bell, 1000, 6, 1.4f, 12) },
                                   { "Bell 3k -9 Q4", eqBand(EqShape::Bell, 3000, -9, 4, 12) },
                                   { "LowShelf 120 +6", eqBand(EqShape::LowShelf, 120, 6, 0.7f, 12) },
                                   { "HighShelf 8k -6", eqBand(EqShape::HighShelf, 8000, -6, 0.7f, 12) },
                                   { "Notch 60 Q8", eqBand(EqShape::Notch, 60, 0, 8, 12) },
                                   { "BandPass 500 Q2", eqBand(EqShape::BandPass, 500, 0, 2, 12) },
                                   { "Tilt 1k +6", eqBand(EqShape::Tilt, 1000, 6, 0.7f, 12) } })
        shapes.push_back({ name, kColours[colour++ % 9], measure(renderBand(b, sweep), sweep).magnitude });
    PlotSpec shSpec { "PRE-EQ shapes (measured, 48 kHz)", "Frequency (Hz)", "dB" };
    shSpec.xMin = 10; shSpec.yMin = -30; shSpec.yMax = 9; shSpec.yStep = 3;
    writeSvgPlot(out.getChildFile("plots/eq_shapes.svg"), shSpec, shapes);

    const auto ap = eqBand(EqShape::AllPass, 1000, 0, 1, 12);
    const auto mAp = measure(renderBand(ap, sweep), sweep);
    PlotSpec apSpec { "All Pass 1 kHz Q1: group delay (measured)", "Frequency (Hz)", "ms" };
    apSpec.xMin = 20; apSpec.yMin = 0; apSpec.yMax = 0.8; apSpec.yStep = 0.1;
    writeSvgPlot(out.getChildFile("plots/all_pass_1k_group_delay.svg"), apSpec, { { "All Pass 1k", "#9467bd", mAp.groupDelayMs } });

    std::cout << "timing\n";
    const auto bells = [](int block)
    {
        LayerSnapshot s;
        for (int i = 0; i < kMaxEqNodesPerLayer; ++i)
            s.eq[static_cast<size_t>(i)] = eqBand(EqShape::Bell, 40.0f * std::pow(1.3f, static_cast<float>(i)), 3.0f + 0.01f * static_cast<float>(block % 50), 1.0f, 12);
        return s;
    };
    const auto cuts = [](int)
    {
        LayerSnapshot s;
        for (int i = 0; i < kMaxEqNodesPerLayer; ++i)
            s.eq[static_cast<size_t>(i)] = eqBand(i % 2 == 0 ? EqShape::LowCut : EqShape::HighCut, i % 2 == 0 ? 20.0f + static_cast<float>(i) : 18000.0f - 100.0f * static_cast<float>(i), 0, 0.707f, 96);
        return s;
    };
    auto cpu = JsonObject()
                   .set("eq_24_bells_static", eqTiming(bells, false))
                   .set("eq_24_bells_automated_every_block", eqTiming(bells, true))
                   .set("eq_24_cuts_96db_static", eqTiming(cuts, false));
    std::cout << juce::JSON::toString(cpu) << "\n";

    const auto report = JsonObject().set("milestone", "M3").set("commit", commit).set("machine", describeMachine())
                            .set("zipper", zipper).set("slopes", slopes).set("all_pass_1k_impulse", impulseFacts(ap)).set("cpu", cpu);
    writeJson(out.getChildFile("m3_lab.json"), report);
    return 0;
}
