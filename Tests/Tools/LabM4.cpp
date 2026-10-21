// HPEQ_Lab m4 --out <dir> [--commit <sha>]
//
// M4 evidence: compressor static curve and gain-reduction envelope, reverb energy-decay curves and
// RT60 per setting/band, impulse responses (WAV), the "compressor 250-700 + reverb 3-8 kHz" render on
// drums, and CPU per processor.

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>

#include "DSP/Biquad.h"
#include "DSP/LayerEngine.h"
#include "DSP/Processors.h"
#include "DSP/ReverbProcessor.h"
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

FxSlotParams compressorParams(float t, float r, float w, float a, float rel)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Compressor;
    s.values = { t, r, w, a, rel, 0.0f, 0.0f, 0.0f };
    return s;
}

FxSlotParams reverbParams(float decay, float hf, float predelay = 0.0f, float size = 1.0f, float width = 1.0f, float lowCut = 10.0f, float mix = 1.0f)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Reverb;
    s.mix = mix;
    s.values[slot::kPredelayMs] = predelay;
    s.values[slot::kDecaySeconds] = decay;
    s.values[slot::kHfRatio] = hf;
    s.values[slot::kSize] = size;
    s.values[slot::kWidth] = width;
    s.values[slot::kLowCutHz] = lowCut;
    return s;
}

juce::AudioBuffer<float> impulseResponse(const FxSlotParams& p, double seconds)
{
    ReverbProcessor r;
    r.prepare(kFs);
    const auto x = makeImpulse({ kFs, 2, seconds, kFixtureSeed }, 0, 1.0f);
    juce::AudioBuffer<float> wet(2, x.getNumSamples());
    for (int start = 0; start < x.getNumSamples(); start += 512)
    {
        const int n = juce::jmin(512, x.getNumSamples() - start);
        const float* in[2] { x.getReadPointer(0) + start, x.getReadPointer(1) + start };
        float* out[2] { wet.getWritePointer(0) + start, wet.getWritePointer(1) + start };
        r.process(in, out, 2, n, p);
    }
    return wet;
}

std::vector<double> edcDb(const juce::AudioBuffer<float>& y)
{
    std::vector<double> e(static_cast<size_t>(y.getNumSamples()));
    for (int i = 0; i < y.getNumSamples(); ++i)
        for (int ch = 0; ch < y.getNumChannels(); ++ch)
            e[static_cast<size_t>(i)] += static_cast<double>(y.getSample(ch, i)) * y.getSample(ch, i);
    double acc = 0.0;
    std::vector<double> edc(e.size());
    for (size_t i = e.size(); i-- > 0;)
        edc[i] = (acc += e[i]);
    const double total = edc[0];
    for (auto& v : edc)
        v = 10.0 * std::log10(v / total + 1e-300);
    return edc;
}

double rt60(const std::vector<double>& edc)
{
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int n = 0;
    for (size_t i = 0; i < edc.size(); i += 16)
        if (edc[i] <= -5.0 && edc[i] >= -25.0)
        {
            const double t = static_cast<double>(i) / kFs;
            sx += t; sy += edc[i]; sxx += t * t; sxy += t * edc[i]; ++n;
        }
    if (n < 3)
        return 0.0;
    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    return slope < 0 ? -60.0 / slope : 0.0;
}

juce::AudioBuffer<float> octave(const juce::AudioBuffer<float>& y, double f0)
{
    auto out = y;
    const double w = 2.0 * juce::MathConstants<double>::pi * f0 / kFs, alpha = std::sin(w) / (2.0 * 1.4), c = std::cos(w);
    const BiquadCoefficients bp { alpha / (1 + alpha), 0.0, -alpha / (1 + alpha), -2 * c / (1 + alpha), (1 - alpha) / (1 + alpha) };
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
    {
        Biquad a, b;
        a.setCoefficients(bp);
        b.setCoefficients(bp);
        for (int i = 0; i < y.getNumSamples(); ++i)
            out.setSample(ch, i, static_cast<float>(b.processSample(0, a.processSample(0, y.getSample(ch, i)))));
    }
    return out;
}

juce::var timing(const std::function<void(juce::AudioBuffer<float>&)>& process)
{
    const auto source = makePinkNoise({ kFs, 2, 10.0, kFixtureSeed }, -18.0);
    juce::AudioBuffer<float> work(2, 512);
    std::vector<double> us;
    for (int start = 0; start + 512 <= source.getNumSamples(); start += 512)
    {
        for (int ch = 0; ch < 2; ++ch)
            work.copyFrom(ch, 0, source, ch, start, 512);
        const auto t0 = std::chrono::steady_clock::now();
        process(work);
        us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
    }
    const auto t = summarise(std::move(us));
    return JsonObject().set("p50_us", t.p50).set("p99_us", t.p99).set("p99_budget_fraction", t.p99 / (1.0e6 * 512 / kFs));
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
} // namespace

int runM4Lab(const juce::File& out, const juce::String& commit)
{
    // Compressor static curve (DC levels) for three knees, and the GR envelope on a gated burst.
    std::cout << "compressor\n";
    std::vector<PlotSeries> curves;
    int colour = 0;
    for (float knee : { 0.0f, 6.0f, 12.0f })
    {
        PlotSeries series { "T -20, R 4, knee " + juce::String(knee) + " dB", kColours[colour++] };
        for (double inDb = -40.0; inDb <= 0.01; inDb += 0.5)
        {
            CompressorProcessor c;
            c.prepare(kFs);
            juce::AudioBuffer<float> dc(1, 4800);
            for (int i = 0; i < 4800; ++i)
                dc.setSample(0, i, static_cast<float>(std::pow(10.0, inDb / 20.0)));
            float* ch[1] { dc.getWritePointer(0) };
            c.process(ch, 1, 4800, compressorParams(-20, 4, knee, 1, 10));
            series.points.emplace_back(inDb, 20.0 * std::log10(dc.getSample(0, 4799)));
        }
        curves.push_back(series);
    }
    PlotSpec staticSpec { "Compressor static curve (measured on DC levels)", "Input level (dBFS)", "Output level (dBFS)" };
    staticSpec.logX = false; staticSpec.xMin = -40; staticSpec.xMax = 0; staticSpec.xTicks = { -40, -35, -30, -25, -20, -15, -10, -5, 0 };
    staticSpec.yMin = -40; staticSpec.yMax = 0; staticSpec.yStep = 5;
    writeSvgPlot(out.getChildFile("plots/compressor_static_curve.svg"), staticSpec, curves);

    {
        auto burst = makeGatedBurst({ kFs, 1, 1.0, kFixtureSeed }, -6.0, -40.0, 0.25);
        const auto input = burst;
        CompressorProcessor c;
        c.prepare(kFs);
        float* ch[1] { burst.getWritePointer(0) };
        c.process(ch, 1, burst.getNumSamples(), compressorParams(-20, 4, 0, 10, 100));
        PlotSeries gr { "gain (dB), attack 10 ms, release 100 ms", "#d62728" };
        for (int i = 0; i < burst.getNumSamples(); i += 48)
            if (std::abs(input.getSample(0, i)) > 1e-4f)
                gr.points.emplace_back(1000.0 * i / kFs, 20.0 * std::log10(std::abs(burst.getSample(0, i) / input.getSample(0, i))));
        PlotSpec spec { "Compressor gain on a -40/-6 dBFS 1 kHz burst (T -20, R 4)", "Time (ms)", "Gain (dB)" };
        spec.logX = false; spec.xMin = 0; spec.xMax = 1000; spec.xTicks = { 0, 250, 500, 750, 1000 };
        spec.yMin = -12; spec.yMax = 1; spec.yStep = 2;
        writeSvgPlot(out.getChildFile("plots/compressor_gain_envelope.svg"), spec, { gr });
    }

    // Reverb: RT60 table, EDC plot, octave-band decay, impulse responses.
    std::cout << "reverb\n";
    auto rtTable = jsonArray();
    std::vector<PlotSeries> edcPlot;
    colour = 0;
    for (float decay : { 0.5f, 1.8f, 4.0f })
        for (float hf : { 1.0f, 0.5f, 0.2f })
        {
            const auto ir = impulseResponse(reverbParams(decay, hf), decay * 2.0 + 0.5);
            const auto edc = edcDb(ir);
            rtTable.append(JsonObject()
                               .set("decay_setting_s", decay)
                               .set("hf_ratio", hf)
                               .set("rt60_broadband_s", rt60(edc))
                               .set("rt60_125hz_s", rt60(edcDb(octave(ir, 125.0))))
                               .set("rt60_500hz_s", rt60(edcDb(octave(ir, 500.0))))
                               .set("rt60_2khz_s", rt60(edcDb(octave(ir, 2000.0))))
                               .set("rt60_8khz_s", rt60(edcDb(octave(ir, 8000.0))))
                               .set("tail_correlation", measureStereo(ir).correlation));
            if (hf == 0.5f)
            {
                PlotSeries series { "T60 " + juce::String(decay) + " s, HF 0.5", kColours[colour++] };
                for (size_t i = 0; i < edc.size(); i += 240)
                    series.points.emplace_back(static_cast<double>(i) / kFs, edc[i]);
                edcPlot.push_back(series);
            }
            if (decay == 1.8f)
                writeFloatWav(out.getChildFile("wav/reverb_ir_T60_1.8_hf_" + juce::String(hf) + ".wav"), ir, kFs);
        }
    PlotSpec edcSpec { "Reverb energy-decay curves (Schroeder), measured", "Time (s)", "dB" };
    edcSpec.logX = false; edcSpec.xMin = 0; edcSpec.xMax = 8; edcSpec.xTicks = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    edcSpec.yMin = -90; edcSpec.yMax = 0; edcSpec.yStep = 10;
    writeSvgPlot(out.getChildFile("plots/reverb_edc.svg"), edcSpec, edcPlot);

    // The M4 scenario on drums: compressor 250-700 Hz + reverb 3-8 kHz (+ an overlapping node).
    std::cout << "scenario\n";
    auto drums = makeSyntheticDrums({ kFs, 2, 4.0, kFixtureSeed });
    for (int ch = 0; ch < 2; ++ch)
        drums.clear(ch, static_cast<int>(3.0 * kFs), drums.getNumSamples() - static_cast<int>(3.0 * kFs)); // 1 s of tail
    const auto renderScenario = [&](const LayerSnapshot& s)
    {
        auto engine = std::make_unique<LayerEngine>();
        engine->prepare(kFs, 512);
        engine->publish(s);
        auto y = drums;
        for (int start = 0; start < y.getNumSamples(); start += 512)
        {
            float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
            engine->process(chunk, 2, juce::jmin(512, y.getNumSamples() - start));
        }
        return y;
    };
    LayerSnapshot scenario;
    scenario.fx[0] = node(250, 700, { compressorParams(-36, 6, 6, 3, 80) });
    scenario.fx[1] = node(3000, 8000, { reverbParams(1.4f, 0.5f, 15.0f, 1.0f, 1.0f, 200.0f, 0.8f) });
    const auto processed = renderScenario(scenario);
    writeFloatWav(out.getChildFile("wav/drums_input.wav"), drums, kFs);
    writeFloatWav(out.getChildFile("wav/drums_comp250-700_reverb3k-8k.wav"), processed, kFs);
    auto scenarioJson = JsonObject()
                            .set("input_rms_dbfs", measureAll(drums).rmsDbfs)
                            .set("output_rms_dbfs", measureAll(processed).rmsDbfs)
                            .set("output_peak_dbfs", measureAll(processed).peakDbfs);

    // CPU per configuration (engine, 48 kHz stereo, block 512).
    std::cout << "timing\n";
    const auto engineTiming = [&](const LayerSnapshot& s)
    {
        auto engine = std::make_unique<LayerEngine>();
        engine->prepare(kFs, 512);
        engine->publish(s);
        return timing([&](juce::AudioBuffer<float>& b) { engine->process(b.getArrayOfWritePointers(), 2, b.getNumSamples()); });
    };
    LayerSnapshot oneReverb, eightReverbs;
    oneReverb.fx[0] = node(200, 10000, { reverbParams(1.8f, 0.5f) });
    for (int i = 0; i < 8; ++i)
        eightReverbs.fx[static_cast<size_t>(i)] = node(60.0f * std::pow(2.0f, static_cast<float>(i)), 150.0f * std::pow(2.0f, static_cast<float>(i)), { reverbParams(2.0f, 0.5f) });
    auto cpu = JsonObject()
                   .set("one_reverb_node", engineTiming(oneReverb))
                   .set("m4_scenario_comp_plus_reverb", engineTiming(scenario))
                   .set("eight_reverb_nodes_pool_full", engineTiming(eightReverbs));
    {
        ReverbPool pool;
        pool.prepare(kFs);
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 100; ++i)
        {
            const int index = pool.acquire();
            pool.release(index);
        }
        cpu.set("reverb_acquire_reset_us", std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 100.0);
    }
    std::cout << juce::JSON::toString(cpu) << "\n";

    const auto report = JsonObject()
                            .set("milestone", "M4")
                            .set("commit", commit)
                            .set("machine", describeMachine())
                            .set("reverb_rt60", rtTable)
                            .set("scenario", scenarioJson)
                            .set("cpu", cpu);
    writeJson(out.getChildFile("m4_lab.json"), report);
    return 0;
}
