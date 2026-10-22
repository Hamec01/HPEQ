// HPEQ_Lab m6 --out <dir> [--commit <sha>]
//
// M6 evidence: T08 spectra of a saturated sine at 1x / 2x / 4x oversampling, saturator transfer curves,
// limiter peak vs ceiling, gate state changes, delay echo positions and tails, Monsta block-size nulls,
// latency (PDC) per configuration, and CPU per processor and for full layers.

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>

#include <juce_dsp/juce_dsp.h>

#include "DSP/LayerEngine.h"
#include "DSP/ProcessorsM6.h"
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
const char* kColours[] { "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b" };
constexpr int kFftOrder = 16, kFftSize = 1 << kFftOrder, kSineBin = 9601;

FxSlotParams slotOf(FxType type, std::initializer_list<std::pair<int, float>> values, float mix = 1.0f)
{
    FxSlotParams s;
    s.active = true;
    s.type = type;
    s.mix = mix;
    for (const auto& [index, value] : values)
        s.values[static_cast<size_t>(index)] = value;
    return s;
}

FxSlotParams saturator(float driveDb, int curve, int os, float bias = 0.0f)
{
    return slotOf(FxType::Saturator, { { slot::kDriveDb, driveDb }, { slot::kCurve, static_cast<float>(curve) }, { slot::kBias, bias },
                                       { slot::kOversampling, static_cast<float>(os) }, { slot::kAutoGain, 1.0f } });
}

FxSlotParams limiter(float ceilingDb, float releaseMs, float lookaheadMs)
{
    return slotOf(FxType::Limiter, { { slot::kCeilingDb, ceilingDb }, { slot::kLimiterReleaseMs, releaseMs }, { slot::kLookaheadMs, lookaheadMs } });
}

FxSlotParams gateSlot(float hysteresis)
{
    return slotOf(FxType::GateExpander, { { slot::kGateThresholdDb, -30.0f }, { slot::kGateRatio, 4.0f }, { slot::kGateRangeDb, 30.0f },
                                          { slot::kGateAttackMs, 0.5f }, { slot::kGateHoldMs, 0.0f }, { slot::kGateReleaseMs, 50.0f },
                                          { slot::kGateHysteresisDb, hysteresis } });
}

FxSlotParams delaySlot(int sync, float timeMs, float feedback, bool pingPong, float mix = 1.0f)
{
    return slotOf(FxType::Delay, { { slot::kDelayTimeMs, timeMs }, { slot::kDelaySync, static_cast<float>(sync) }, { slot::kFeedback, feedback },
                                   { slot::kPingPong, pingPong ? 1.0f : 0.0f }, { slot::kDampingHz, 8000.0f }, { slot::kDelayLowCutHz, 120.0f },
                                   { slot::kDelayWidth, 1.0f } }, mix);
}

FxSlotParams monsta(float chance, float seed, int division, float crush, float gate)
{
    return slotOf(FxType::MonstaFx, { { slot::kDivision, static_cast<float>(division) }, { slot::kChance, chance }, { slot::kSeed, seed },
                                      { slot::kCrushBits, crush }, { slot::kGateDepth, gate }, { slot::kReverseShare, 0.3f } });
}

FxSlotParams compressor()
{
    return slotOf(FxType::Compressor, { { 0, -30.0f }, { 1, 4.0f }, { 2, 6.0f }, { 3, 5.0f }, { 4, 100.0f } });
}

FxNodeParams node(float lo, float hi, std::initializer_list<FxSlotParams> chain)
{
    FxNodeParams n;
    n.active = true;
    n.lowHz = lo;
    n.highHz = hi;
    for (const auto& s : chain)
        n.slots[static_cast<size_t>(n.numSlots++)] = s;
    return n;
}

juce::AudioBuffer<float> runInPlace(const juce::AudioBuffer<float>& x, int block, const std::function<void(float* const*, int, int, int)>& process)
{
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        float* chunk[kMaxChannels] {};
        for (int ch = 0; ch < y.getNumChannels(); ++ch)
            chunk[ch] = y.getWritePointer(ch) + start;
        process(chunk, y.getNumChannels(), n, start);
    }
    return y;
}

// Blackman-Harris magnitude spectrum (dB relative to the fundamental) of channel 0 after `settle`.
std::vector<double> spectrumDb(const juce::AudioBuffer<float>& y, int settle)
{
    std::vector<float> data(static_cast<size_t>(2 * kFftSize), 0.0f);
    for (int i = 0; i < kFftSize; ++i)
        data[static_cast<size_t>(i)] = y.getSample(0, settle + i);
    juce::dsp::WindowingFunction<float> window(static_cast<size_t>(kFftSize), juce::dsp::WindowingFunction<float>::blackmanHarris, false);
    window.multiplyWithWindowingTable(data.data(), static_cast<size_t>(kFftSize));
    juce::dsp::FFT(kFftOrder).performFrequencyOnlyForwardTransform(data.data());
    const double reference = data[static_cast<size_t>(kSineBin)];
    std::vector<double> db(static_cast<size_t>(kFftSize / 2 + 1));
    for (size_t b = 0; b < db.size(); ++b)
        db[b] = 20.0 * std::log10(std::max(1.0e-20, static_cast<double>(data[b])) / reference);
    return db;
}

double aliasRatioDb(const std::vector<double>& db)
{
    double fundamental = 0.0, alias = 0.0;
    const int minBin = static_cast<int>(std::ceil(20.0 * kFftSize / kFs));
    for (int b = minBin; b < static_cast<int>(db.size()); ++b)
    {
        const double power = std::pow(10.0, db[static_cast<size_t>(b)] / 10.0);
        bool harmonic = false;
        for (int k = 1; k * kSineBin <= kFftSize / 2 + 4; ++k)
            if (std::abs(b - k * kSineBin) <= 4)
            {
                harmonic = true;
                if (k == 1)
                    fundamental += power;
            }
        if (!harmonic)
            alias += power;
    }
    return 10.0 * std::log10((alias + 1.0e-30) / (fundamental + 1.0e-30));
}

juce::var timing(const std::function<void(float* const*, int)>& process)
{
    const auto source = makePinkNoise({ kFs, 2, 10.0, kFixtureSeed }, -12.0);
    juce::AudioBuffer<float> work(2, 512);
    std::vector<double> us;
    for (int start = 0; start + 512 <= source.getNumSamples(); start += 512)
    {
        for (int ch = 0; ch < 2; ++ch)
            work.copyFrom(ch, 0, source, ch, start, 512);
        const auto t0 = std::chrono::steady_clock::now();
        process(work.getArrayOfWritePointers(), 512);
        us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
    }
    const auto t = summarise(std::move(us));
    return JsonObject().set("p50_us", t.p50).set("p99_us", t.p99).set("p99_budget_fraction", t.p99 / (1.0e6 * 512 / kFs));
}

juce::var engineTiming(const LayerSnapshot& snapshot)
{
    auto e = std::make_unique<LayerEngine>();
    e->prepare(kFs, 512);
    e->publish(snapshot);
    Transport t;
    t.bpm = 120.0;
    return timing([&](float* const* c, int n) { e->process(c, 2, n, t); });
}
} // namespace

int runM6Lab(const juce::File& out, const juce::String& commit)
{
    out.getChildFile("plots").createDirectory();

    // T08: spectra of a 7.03 kHz sine at -6 dBFS through tanh, 12 dB drive.
    std::cout << "T08\n";
    auto t08 = jsonArray();
    std::vector<PlotSeries> spectra;
    const auto sine = makeSine({ kFs, 1, 2.0, kFixtureSeed }, kFs * kSineBin / kFftSize, -6.0);
    for (int os : { 0, 1, 2 })
        for (int curve : { 0, 2 })
        {
            SaturatorProcessor sat;
            sat.prepare(kFs, 512);
            const auto p = saturator(12.0f, curve, os);
            const auto y = runInPlace(sine, 512, [&](float* const* c, int nc, int n, int) { sat.process(c, nc, n, p); });
            const auto db = spectrumDb(y, 8192);
            const double ratio = aliasRatioDb(db);
            t08.append(JsonObject().set("curve", curve == 0 ? "tanh" : "hard").set("oversampling", 1 << os).set("alias_to_fundamental_db", ratio));
            if (curve == 0)
            {
                PlotSeries s { juce::String(1 << os) + "x (alias " + juce::String(ratio, 1) + " dB)", kColours[os] };
                for (size_t b = 30; b < db.size(); b += 8)
                {
                    double peak = -300.0; // max over 8 bins keeps narrow alias lines visible
                    for (size_t k = b; k < std::min(db.size(), b + 8); ++k)
                        peak = std::max(peak, db[k]);
                    s.points.push_back({ b * kFs / kFftSize, std::max(-160.0, peak) });
                }
                spectra.push_back(std::move(s));
            }
        }
    PlotSpec spectrumSpec { "T08: tanh 12 dB drive, 7.03 kHz at -6 dBFS", "Hz", "dB re fundamental" };
    spectrumSpec.xMax = 24000.0;
    spectrumSpec.yMin = -160.0;
    spectrumSpec.yMax = 5.0;
    spectrumSpec.yStep = 20.0;
    writeSvgPlot(out.getChildFile("plots/t08_saturator_spectra.svg"), spectrumSpec, spectra);

    // Saturator transfer curves (static, 1x, auto gain off would hide the shape; drive 12 dB, auto gain on).
    {
        std::vector<PlotSeries> curves;
        int colour = 0;
        for (const auto& [curve, bias, name] : { std::tuple<int, float, const char*> { 0, 0.0f, "tanh" }, { 1, 0.0f, "tube" }, { 2, 0.0f, "hard" }, { 0, 0.4f, "tanh bias 0.4" } })
        {
            PlotSeries s { name, kColours[colour++] };
            const auto p = slotOf(FxType::Saturator, { { slot::kDriveDb, 12.0f }, { slot::kCurve, static_cast<float>(curve) }, { slot::kBias, bias }, { slot::kAutoGain, 0.0f } });
            const float g = std::pow(10.0f, 12.0f / 20.0f);
            for (double x = -1.0; x <= 1.0001; x += 0.02)
            {
                const auto u = static_cast<float>(g * x + bias);
                const auto f = [curve](float v) { return curve == 2 ? std::clamp(v, -1.0f, 1.0f) : curve == 1 && v < 0.0f ? std::tanh(1.4f * v) / 1.4f : std::tanh(v); };
                s.points.push_back({ x, f(u) - f(bias) });
            }
            juce::ignoreUnused(p);
            curves.push_back(std::move(s));
        }
        PlotSpec spec { "Saturator laws (drive 12 dB, before auto gain / DC blocker)", "input", "output" };
        spec.logX = false;
        spec.xMin = -1.0;
        spec.xMax = 1.0;
        spec.xTicks = { -1.0, -0.5, 0.0, 0.5, 1.0 };
        spec.yMin = -1.5;
        spec.yMax = 1.5;
        spec.yStep = 0.5;
        writeSvgPlot(out.getChildFile("plots/saturator_laws.svg"), spec, curves);
    }

    // Limiter: worst sample peak relative to the ceiling, drums +18 dB.
    std::cout << "limiter\n";
    auto limiterTable = jsonArray();
    auto hot = makeSyntheticDrums({ kFs, 2, 4.0, kFixtureSeed });
    hot.applyGain(static_cast<float>(std::pow(10.0, 18.0 / 20.0)));
    for (float ceiling : { -0.1f, -1.0f, -6.0f })
        for (float look : { 0.1f, 2.0f, 10.0f })
        {
            LimiterProcessor l;
            l.prepare(kFs);
            const auto p = limiter(ceiling, 80.0f, look);
            const auto y = runInPlace(hot, 512, [&](float* const* c, int nc, int n, int) { l.process(c, nc, n, p); });
            limiterTable.append(JsonObject().set("ceiling_db", ceiling).set("lookahead_ms", look)
                                    .set("input_peak_dbfs", measureAll(hot).peakDbfs).set("output_peak_dbfs", measureAll(y).peakDbfs)
                                    .set("latency_samples", LimiterProcessor::lookaheadSamples(p, kFs)));
        }

    // Gate chatter: tone wandering +-1.5 dB around the threshold.
    auto gateTable = jsonArray();
    {
        const int n = static_cast<int>(4.0 * kFs);
        juce::AudioBuffer<float> x(2, n);
        for (int i = 0; i < n; ++i)
        {
            const double t = i / kFs;
            const double db = -30.0 + 1.5 * std::sin(2.0 * juce::MathConstants<double>::pi * 3.0 * t);
            const auto v = static_cast<float>(std::pow(10.0, db / 20.0) * std::sin(2.0 * juce::MathConstants<double>::pi * 200.0 * t));
            x.setSample(0, i, v);
            x.setSample(1, i, v);
        }
        for (float hysteresis : { 0.0f, 1.0f, 2.0f, 4.0f })
        {
            GateProcessor g;
            g.prepare(kFs);
            const auto p = gateSlot(hysteresis);
            runInPlace(x, 256, [&](float* const* c, int nc, int len, int) { g.process(c, nc, len, p); });
            gateTable.append(JsonObject().set("hysteresis_db", hysteresis).set("state_changes_4s", static_cast<int>(g.stateChanges())));
        }
    }

    // Delay: synced echo times and tails.
    std::cout << "delay\n";
    auto delayTable = jsonArray();
    for (const auto& [sync, bpm, name] : { std::tuple<int, double, const char*> { 8, 120.0, "1/4 @ 120" }, { 6, 90.0, "1/8 D @ 90" }, { 4, 140.0, "1/8 T @ 140" }, { 0, 333.0, "333 ms free" } })
    {
        const auto p = delaySlot(sync, 333.0f, 0.6f, false);
        delayTable.append(JsonObject().set("setting", name).set("bpm", bpm).set("delay_ms", DelayProcessor::delaySeconds(p, bpm) * 1000.0)
                              .set("tail_seconds_minus60", DelayProcessor::tailSeconds(p, bpm)));
    }

    // Monsta: block-size nulls vs block 1, transport playing.
    std::cout << "monsta\n";
    auto monstaTable = jsonArray();
    {
        const auto drums = makeSyntheticDrums({ kFs, 2, 4.0, kFixtureSeed });
        const auto render = [&](int block)
        {
            MonstaProcessor m;
            m.prepare(kFs);
            const auto p = monsta(0.6f, 3.0f, 3, 6.0f, 0.5f);
            return runInPlace(drums, block, [&](float* const* c, int nc, int n, int start)
            {
                Transport t;
                t.bpm = 128.0;
                t.playing = t.hasPosition = true;
                t.ppqAtBlockStart = start * t.bpm / (60.0 * kFs);
                m.process(c, nc, n, p, t, 0);
            });
        };
        const auto reference = render(1);
        writeFloatWav(out.getChildFile("monsta_drums_seed3.wav"), reference, kFs);
        for (int block : { 64, 127, 512, 2048 })
        {
            const auto null = compareAligned(render(block), reference);
            monstaTable.append(JsonObject().set("block", block).set("bit_exact_vs_block_1", null.bitExact).set("null_db", null.errorDb));
        }
    }

    // Latency per configuration.
    auto latencyTable = jsonArray();
    for (int os : { 0, 1, 2 })
        latencyTable.append(JsonObject().set("config", "saturator " + juce::String(1 << os) + "x").set("samples", SaturatorProcessor::latencySamples(os)));
    for (float look : { 0.1f, 2.0f, 10.0f })
        latencyTable.append(JsonObject().set("config", "limiter " + juce::String(look) + " ms").set("samples", LimiterProcessor::lookaheadSamples(limiter(0, 80, look), kFs)));
    {
        LayerSnapshot s;
        s.fx[0] = node(100, 500, { saturator(12, 0, 2), limiter(-1, 80, 2) });
        s.fx[1] = node(2000, 8000, { saturator(6, 0, 1) });
        latencyTable.append(JsonObject().set("config", "layer: node A sat 4x + limiter 2 ms, node B sat 2x").set("samples", LayerEngine::latencyFor(s, kFs)));
    }

    // CPU (512-sample stereo blocks, 10 s of pink noise).
    std::cout << "cpu\n";
    auto cpu = JsonObject();
    for (int os : { 0, 1, 2 })
    {
        SaturatorProcessor sat;
        sat.prepare(kFs, 512);
        const auto p = saturator(12, 0, os);
        cpu.set("saturator_" + juce::String(1 << os) + "x", timing([&](float* const* c, int n) { sat.process(c, 2, n, p); }));
    }
    {
        TransientProcessor t;
        t.prepare(kFs);
        const auto p = slotOf(FxType::TransientShaper, { { slot::kAttackGainDb, 6.0f }, { slot::kSustainGainDb, -6.0f }, { slot::kTransientSpeed, 0.5f } });
        cpu.set("transient", timing([&](float* const* c, int n) { t.process(c, 2, n, p); }));
    }
    {
        StereoProcessor st;
        const auto p = slotOf(FxType::StereoProcessor, { { slot::kStereoWidth, 1.5f }, { slot::kStereoBalance, 0.2f } });
        cpu.set("stereo", timing([&](float* const* c, int n) { st.process(c, 2, n, p); }));
    }
    {
        GateProcessor g;
        g.prepare(kFs);
        const auto p = gateSlot(4.0f);
        cpu.set("gate", timing([&](float* const* c, int n) { g.process(c, 2, n, p); }));
    }
    {
        LimiterProcessor l;
        l.prepare(kFs);
        const auto p = limiter(-6.0f, 80.0f, 2.0f);
        cpu.set("limiter_2ms", timing([&](float* const* c, int n) { l.process(c, 2, n, p); }));
    }
    {
        DelayProcessor d;
        d.prepare(kFs);
        const auto p = delaySlot(8, 0, 0.5f, true);
        juce::AudioBuffer<float> wet(2, 512);
        cpu.set("delay", timing([&](float* const* c, int n) { d.process(c, wet.getArrayOfWritePointers(), 2, n, p, Transport {}); }));
    }
    {
        MonstaProcessor m;
        m.prepare(kFs);
        const auto p = monsta(0.6f, 3.0f, 3, 6.0f, 0.5f);
        cpu.set("monsta", timing([&](float* const* c, int n) { m.process(c, 2, n, p, Transport {}, 0); }));
    }
    {
        LayerSnapshot mixed;
        mixed.fx[0] = node(80, 600, { saturator(12, 1, 1, 0.2f), slotOf(FxType::TransientShaper, { { slot::kAttackGainDb, 6.0f } }), limiter(-12, 50, 1.5f) });
        mixed.fx[1] = node(1500, 8000, { slotOf(FxType::StereoProcessor, { { slot::kStereoWidth, 1.4f } }), monsta(0.5f, 11, 3, 10, 0.3f), delaySlot(5, 0, 0.4f, true, 0.5f) });
        cpu.set("layer_two_nodes_mixed_m6", engineTiming(mixed));

        // Heaviest non-pooled layer: 8 nodes x [sat 4x, transient, gate, compressor, stereo, limiter 2 ms].
        LayerSnapshot heavy;
        for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
        {
            const float lo = 60.0f * std::pow(2.0f, static_cast<float>(j));
            heavy.fx[static_cast<size_t>(j)] = node(lo, lo * 1.8f, { saturator(12, 0, 2), slotOf(FxType::TransientShaper, { { slot::kAttackGainDb, 6.0f } }), gateSlot(4.0f),
                                                                      compressor(), slotOf(FxType::StereoProcessor, { { slot::kStereoWidth, 1.2f } }), limiter(-6, 80, 2) });
        }
        cpu.set("layer_8_nodes_x_6_slots_sat4x", engineTiming(heavy));
    }
    std::cout << juce::JSON::toString(cpu) << "\n";

    const auto report = JsonObject()
                            .set("milestone", "M6")
                            .set("commit", commit)
                            .set("machine", describeMachine())
                            .set("t08", t08)
                            .set("limiter", limiterTable)
                            .set("gate", gateTable)
                            .set("delay", delayTable)
                            .set("monsta_t10", monstaTable)
                            .set("latency", latencyTable)
                            .set("cpu", cpu);
    writeJson(out.getChildFile("m6_lab.json"), report);
    return 0;
}
