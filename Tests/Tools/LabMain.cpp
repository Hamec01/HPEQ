// HPEQ_Lab m2 --out <dir> [--commit <sha>]
//
// M2 experiment (01 §5): H1 "delta bands" (the engine, independent LR4 band selectors per node)
// versus H2 "complementary multiband" (Linkwitz-Riley tree with all-pass compensation, lab-only).
// Same signals, same processors, same metrics; writes m2_lab.json, SVG plots and WAV renders.

#include <chrono>
#include <cmath>
#include <memory>
#include <functional>
#include <iostream>
#include <algorithm>
#include <map>

#include <juce_dsp/juce_dsp.h>

#include "DSP/LayerEngine.h"
#include "DSP/Processors.h"
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
constexpr double kPreroll = 0.25; // seconds of silence so node fade-ins finish before the signal

struct Range { double low, high; };

struct Scenario
{
    juce::String id, description;
    std::vector<Range> nodes;
    bool contiguous = true; // H2 needs non-overlapping, touching ranges
};

std::vector<Scenario> scenarios()
{
    return {
        { "A_1node", "1 node 250-700 Hz", { { 250, 700 } } },
        { "B_2nodes", "2 contiguous nodes", { { 250, 700 }, { 700, 2000 } } },
        { "C_3nodes", "3 contiguous nodes", { { 150, 400 }, { 400, 1200 }, { 1200, 4000 } } },
        { "D_5nodes", "5 contiguous nodes", { { 80, 200 }, { 200, 500 }, { 500, 1200 }, { 1200, 3000 }, { 3000, 8000 } } },
        { "E_overlap", "2 overlapping nodes 250-700 + 450-1200", { { 250, 700 }, { 450, 1200 } }, false },
    };
}

using BandProcessor = std::function<void(int nodeIndex, juce::AudioBuffer<float>& band)>;

juce::AudioBuffer<float> withPreroll(const juce::AudioBuffer<float>& x)
{
    const int pre = static_cast<int>(kPreroll * kFs);
    juce::AudioBuffer<float> out(x.getNumChannels(), x.getNumSamples() + pre);
    out.clear();
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        out.copyFrom(ch, pre, x, ch, 0, x.getNumSamples());
    return out;
}

juce::AudioBuffer<float> withoutPreroll(const juce::AudioBuffer<float>& y)
{
    const int pre = static_cast<int>(kPreroll * kFs);
    juce::AudioBuffer<float> out(y.getNumChannels(), y.getNumSamples() - pre);
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
        out.copyFrom(ch, 0, y, ch, pre, out.getNumSamples());
    return out;
}

// --- The real engine (H3, order-free tree, DECISIONS D4) -------------------------------------------------------------------------
juce::AudioBuffer<float> renderEngine(const juce::AudioBuffer<float>& x, const std::vector<Range>& nodes,
                                  const std::vector<FxSlotParams>& perNodeSlot, int block = 512)
{
    LayerSnapshot snapshot;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        auto& n = snapshot.fx[i];
        n.active = perNodeSlot[i].active;
        n.lowHz = static_cast<float>(nodes[i].low);
        n.highHz = static_cast<float>(nodes[i].high);
        n.mix = 1.0f;
        n.numSlots = 1;
        n.slots[0] = perNodeSlot[i];
    }
    auto enginePtr = std::make_unique<LayerEngine>(); // ~115 KB: keep it off the stack
    auto& engine = *enginePtr;
    engine.prepare(kFs, block);
    engine.publish(snapshot);
    auto y = withPreroll(x);
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* chunk[kMaxChannels] {};
        for (int ch = 0; ch < y.getNumChannels(); ++ch)
            chunk[ch] = y.getWritePointer(ch) + start;
        engine.process(chunk, y.getNumChannels(), juce::jmin(block, y.getNumSamples() - start));
    }
    return withoutPreroll(y);
}

// --- H2: Linkwitz-Riley tree with all-pass compensation (lab only) --------------------------------
// edges e_0 < ... < e_{E-1}; band k (k < E) = LP_k(rest_k) passed through AP_{k+1..E-1}; band E = rest.
// Sum of all bands = prod AP_k (x): flat magnitude, not a time-domain identity.
juce::AudioBuffer<float> renderH2(const juce::AudioBuffer<float>& xIn, const std::vector<Range>& nodes, const BandProcessor& process,
                                  bool preroll = true)
{
    std::vector<double> edges;
    for (const auto& r : nodes)
    {
        if (edges.empty() || std::abs(edges.back() - r.low) > 1e-9)
            edges.push_back(r.low);
        edges.push_back(r.high);
    }
    const int numEdges = static_cast<int>(edges.size());
    const auto x = preroll ? withPreroll(xIn) : xIn;
    const int channels = x.getNumChannels(), length = x.getNumSamples();

    std::vector<juce::AudioBuffer<float>> bands(static_cast<size_t>(numEdges + 1), juce::AudioBuffer<float>(channels, length));
    for (int ch = 0; ch < channels; ++ch)
    {
        struct Split { Biquad lp1, lp2, hp1, hp2; };
        std::vector<Split> splits(static_cast<size_t>(numEdges));
        std::vector<std::vector<Biquad>> allPasses(static_cast<size_t>(numEdges));
        for (int k = 0; k < numEdges; ++k)
        {
            auto& s = splits[static_cast<size_t>(k)];
            const auto lp = design::lowPass(kFs, edges[static_cast<size_t>(k)], design::kButterworthQ);
            const auto hp = design::highPass(kFs, edges[static_cast<size_t>(k)], design::kButterworthQ);
            s.lp1.setCoefficients(lp); s.lp2.setCoefficients(lp);
            s.hp1.setCoefficients(hp); s.hp2.setCoefficients(hp);
            for (int j = k + 1; j < numEdges; ++j)
            {
                Biquad ap;
                ap.setCoefficients(design::allPass(kFs, edges[static_cast<size_t>(j)], design::kButterworthQ));
                allPasses[static_cast<size_t>(k)].push_back(ap);
            }
        }
        for (int i = 0; i < length; ++i)
        {
            double rest = x.getSample(ch, i);
            for (int k = 0; k < numEdges; ++k)
            {
                auto& s = splits[static_cast<size_t>(k)];
                double low = s.lp2.processSample(0, s.lp1.processSample(0, rest));
                rest = s.hp2.processSample(0, s.hp1.processSample(0, rest));
                for (auto& ap : allPasses[static_cast<size_t>(k)])
                    low = ap.processSample(0, low);
                bands[static_cast<size_t>(k)].setSample(ch, i, static_cast<float>(low));
            }
            bands[static_cast<size_t>(numEdges)].setSample(ch, i, static_cast<float>(rest));
        }
    }

    // Node j covers the band whose lower edge is nodes[j].low.
    for (size_t j = 0; j < nodes.size(); ++j)
    {
        const auto it = std::find_if(edges.begin(), edges.end(), [&](double e) { return std::abs(e - nodes[j].low) < 1e-9; });
        process(static_cast<int>(j), bands[static_cast<size_t>(std::distance(edges.begin(), it) + 1)]);
    }

    juce::AudioBuffer<float> y(channels, length);
    y.clear();
    for (const auto& b : bands)
        for (int ch = 0; ch < channels; ++ch)
            y.addFrom(ch, 0, b, ch, 0, length);
    return preroll ? withoutPreroll(y) : y;
}

// H3: one phase-coherent Linkwitz-Riley tree over the union of all node edges (overlap allowed).
// Elementary bands e_k sum to AP(x). Node j's band M_j = sum of elementary bands inside [low_j, high_j];
// y = AP(x) + sum_j (P_j(M_j) - M_j). For touching, non-overlapping nodes this equals H2.
juce::AudioBuffer<float> renderH3(const juce::AudioBuffer<float>& xIn, const std::vector<Range>& nodes, const BandProcessor& process)
{
    std::vector<double> edges;
    for (const auto& r : nodes) { edges.push_back(r.low); edges.push_back(r.high); }
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end(), [](double a, double b) { return std::abs(a - b) < 1e-9; }), edges.end());
    const int numEdges = static_cast<int>(edges.size());

    // Elementary split via renderH2's tree on contiguous pseudo-nodes, capturing each band.
    std::vector<Range> elementary;
    for (int k = 0; k + 1 < numEdges; ++k)
        elementary.push_back({ edges[static_cast<size_t>(k)], edges[static_cast<size_t>(k + 1)] });
    std::vector<juce::AudioBuffer<float>> bands(elementary.size());
    const auto apDry = renderH2(xIn, elementary, [&](int k, juce::AudioBuffer<float>& band) { bands[static_cast<size_t>(k)] = withoutPreroll(band); });

    auto y = apDry;
    for (size_t j = 0; j < nodes.size(); ++j)
    {
        juce::AudioBuffer<float> m(y.getNumChannels(), y.getNumSamples());
        m.clear();
        for (size_t k = 0; k < elementary.size(); ++k)
            if (elementary[k].low >= nodes[j].low - 1e-9 && elementary[k].high <= nodes[j].high + 1e-9)
                for (int ch = 0; ch < m.getNumChannels(); ++ch)
                    m.addFrom(ch, 0, bands[k], ch, 0, m.getNumSamples());
        auto processed = m;
        process(static_cast<int>(j), processed);
        for (int ch = 0; ch < y.getNumChannels(); ++ch)
        {
            y.addFrom(ch, 0, processed, ch, 0, y.getNumSamples());
            y.addFrom(ch, 0, m, ch, 0, y.getNumSamples(), -1.0f);
        }
    }
    return y;
}

// Offline H1 for constant gains: y = x + sum (g_i - 1) * B_i(x), B_i = LR(2k) high-pass(low) * LR(2k)
// low-pass(high), each side Butterworth order k squared. k = 2 must reproduce the engine (cross-check);
// k = 4 (LR8, 48 dB/oct) is the steeper-edge variant.
juce::AudioBuffer<float> renderH1Offline(const juce::AudioBuffer<float>& x, const std::vector<Range>& nodes,
                                         const std::vector<double>& gainsDb, int butterworthOrder)
{
    const std::vector<double> qs = butterworthOrder == 2 ? std::vector<double> { 0.70710678118654752 }
                                                         : std::vector<double> { 0.54119610014619698, 1.3065629648763766 };
    auto y = x;
    for (size_t j = 0; j < nodes.size(); ++j)
    {
        const double g = std::pow(10.0, gainsDb[j] / 20.0);
        if (g == 1.0)
            continue;
        for (int ch = 0; ch < x.getNumChannels(); ++ch)
        {
            std::vector<Biquad> chain;
            for (int pass = 0; pass < 2; ++pass)
                for (double q : qs)
                {
                    Biquad hp, lp;
                    hp.setCoefficients(design::highPass(kFs, nodes[j].low, q));
                    lp.setCoefficients(design::lowPass(kFs, nodes[j].high, q));
                    chain.push_back(hp);
                    chain.push_back(lp);
                }
            for (int i = 0; i < x.getNumSamples(); ++i)
            {
                double b = x.getSample(ch, i);
                for (auto& f : chain)
                    b = f.processSample(0, b);
                y.setSample(ch, i, static_cast<float>(y.getSample(ch, i) + (g - 1.0) * b));
            }
        }
    }
    return y;
}

// Offline H1 with an arbitrary processor on one band: y = x + (P(b) - b), b = LR4 band of x.
juce::AudioBuffer<float> renderH1OfflineBand(const juce::AudioBuffer<float>& x, Range range,
                                             const std::function<void(juce::AudioBuffer<float>&)>& process)
{
    auto band = x;
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
    {
        std::array<Biquad, 4> chain;
        chain[0].setCoefficients(design::highPass(kFs, range.low, design::kButterworthQ));
        chain[1].setCoefficients(design::highPass(kFs, range.low, design::kButterworthQ));
        chain[2].setCoefficients(design::lowPass(kFs, range.high, design::kButterworthQ));
        chain[3].setCoefficients(design::lowPass(kFs, range.high, design::kButterworthQ));
        for (int i = 0; i < x.getNumSamples(); ++i)
        {
            double v = x.getSample(ch, i);
            for (auto& f : chain)
                v = f.processSample(0, v);
            band.setSample(ch, i, static_cast<float>(v));
        }
    }
    auto processed = band;
    process(processed);
    auto y = x;
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
    {
        y.addFrom(ch, 0, processed, ch, 0, y.getNumSamples());
        y.addFrom(ch, 0, band, ch, 0, y.getNumSamples(), -1.0f);
    }
    return y;
}

// Offline order-free H3 for constant gains, plain RBJ cascades (independent of the engine's SVFs):
// dry = prod_all AP(x);  band_j = prod_{other edges} AP( LP4_hi( HP4_lo(x) ) );  y = dry + sum (g-1) band_j.
juce::AudioBuffer<float> renderH3Offline(const juce::AudioBuffer<float>& x, const std::vector<Range>& nodes,
                                         const std::vector<double>& gainsDb)
{
    const auto cascade = [&](int ch, const std::vector<BiquadCoefficients>& sections)
    {
        juce::AudioBuffer<float> out(1, x.getNumSamples());
        std::vector<Biquad> f(sections.size());
        for (size_t k = 0; k < sections.size(); ++k)
            f[k].setCoefficients(sections[k]);
        for (int i = 0; i < x.getNumSamples(); ++i)
        {
            double v = x.getSample(ch, i);
            for (auto& b : f)
                v = b.processSample(0, v);
            out.setSample(0, i, static_cast<float>(v));
        }
        return out;
    };
    auto y = x;
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
    {
        std::vector<BiquadCoefficients> all;
        for (const auto& r : nodes)
        {
            all.push_back(design::allPass(kFs, r.low, design::kButterworthQ));
            all.push_back(design::allPass(kFs, r.high, design::kButterworthQ));
        }
        auto sum = cascade(ch, all);
        for (size_t j = 0; j < nodes.size(); ++j)
        {
            const double g = std::pow(10.0, gainsDb[j] / 20.0);
            std::vector<BiquadCoefficients> sections {
                design::highPass(kFs, nodes[j].low, design::kButterworthQ), design::highPass(kFs, nodes[j].low, design::kButterworthQ),
                design::lowPass(kFs, nodes[j].high, design::kButterworthQ), design::lowPass(kFs, nodes[j].high, design::kButterworthQ) };
            for (size_t k = 0; k < nodes.size(); ++k)
                if (k != j)
                {
                    sections.push_back(design::allPass(kFs, nodes[k].low, design::kButterworthQ));
                    sections.push_back(design::allPass(kFs, nodes[k].high, design::kButterworthQ));
                }
            const auto band = cascade(ch, sections);
            sum.addFrom(0, 0, band, 0, 0, sum.getNumSamples(), static_cast<float>(g - 1.0));
        }
        y.copyFrom(ch, 0, sum, 0, 0, y.getNumSamples());
    }
    return y;
}

FxSlotParams gainSlot(double db, bool active = true)
{
    FxSlotParams s;
    s.active = active;
    s.type = FxType::Gain;
    s.values[slot::kGainDb] = static_cast<float>(db);
    return s;
}

FxSlotParams compressorSlot()
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Compressor;
    s.values[slot::kThresholdDb] = -36.0f;
    s.values[slot::kRatio] = 6.0f;
    s.values[slot::kKneeDb] = 6.0f;
    s.values[slot::kAttackMs] = 3.0f;
    s.values[slot::kReleaseMs] = 80.0f;
    return s;
}

std::vector<double> logGrid(int count = 240)
{
    std::vector<double> grid;
    for (int i = 0; i < count; ++i)
        grid.push_back(20.0 * std::pow(1000.0, static_cast<double>(i) / (count - 1)));
    return grid;
}

struct Response
{
    std::vector<double> frequency, magnitudeDb, groupDelayMs;
};

Response measureResponse(const juce::AudioBuffer<float>& y, const juce::AudioBuffer<float>& x)
{
    Response r;
    r.frequency = logGrid();
    const auto points = measureTransfer(y, x, kFs, r.frequency, 0, -90.0);
    std::vector<double> phase;
    for (const auto& p : points)
    {
        r.magnitudeDb.push_back(p.magnitudeDb.value_or(std::nan("")));
        phase.push_back(p.phaseRadians.value_or(0.0));
    }
    // Unwrap and differentiate: tau = -dphi/domega.
    for (size_t i = 1; i < phase.size(); ++i)
        while (phase[i] - phase[i - 1] > juce::MathConstants<double>::pi) phase[i] -= juce::MathConstants<double>::twoPi;
    for (size_t i = 1; i < phase.size(); ++i)
        while (phase[i] - phase[i - 1] < -juce::MathConstants<double>::pi) phase[i] += juce::MathConstants<double>::twoPi;
    for (size_t i = 0; i < phase.size(); ++i)
    {
        const size_t a = i == 0 ? 0 : i - 1, b = i + 1 < phase.size() ? i + 1 : i;
        const double dw = juce::MathConstants<double>::twoPi * (r.frequency[b] - r.frequency[a]);
        r.groupDelayMs.push_back(dw > 0 ? -(phase[b] - phase[a]) / dw * 1000.0 : 0.0);
    }
    return r;
}

double magnitudeAt(const Response& r, double f)
{
    size_t best = 0;
    for (size_t i = 0; i < r.frequency.size(); ++i)
        if (std::abs(std::log(r.frequency[i] / f)) < std::abs(std::log(r.frequency[best] / f)))
            best = i;
    return r.magnitudeDb[best];
}

// Max |dB| over the grid where `inside` returns true.
double maxAbsWhere(const Response& r, const std::function<bool(double)>& inside, double offset = 0.0)
{
    double worst = 0.0;
    for (size_t i = 0; i < r.frequency.size(); ++i)
        if (inside(r.frequency[i]) && std::isfinite(r.magnitudeDb[i]))
            worst = std::max(worst, std::abs(r.magnitudeDb[i] - offset));
    return worst;
}

std::vector<std::pair<double, double>> curve(const Response& r, const std::vector<double>& values)
{
    std::vector<std::pair<double, double>> pts;
    for (size_t i = 0; i < r.frequency.size(); ++i)
        pts.emplace_back(r.frequency[i], values[i]);
    return pts;
}

// Mean power in [lo, hi] Hz of channel 0, dB, via one FFT of the whole signal.
double bandPowerDb(const juce::AudioBuffer<float>& a, double lo, double hi)
{
    int order = 1;
    while ((1 << order) < a.getNumSamples()) ++order;
    juce::dsp::FFT fft(order);
    std::vector<float> data(static_cast<size_t>(2 << order), 0.0f);
    std::copy(a.getReadPointer(0), a.getReadPointer(0) + a.getNumSamples(), data.begin());
    fft.performFrequencyOnlyForwardTransform(data.data());
    const double binHz = kFs / (1 << order);
    double sum = 0.0;
    int count = 0;
    for (int k = static_cast<int>(lo / binHz); k <= static_cast<int>(hi / binHz); ++k, ++count)
        sum += static_cast<double>(data[static_cast<size_t>(k)]) * data[static_cast<size_t>(k)];
    return 10.0 * std::log10(std::max(sum / std::max(count, 1), 1e-30));
}

juce::var timing(const std::function<void(juce::AudioBuffer<float>&)>& processBlock, int block)
{
    const auto source = makePinkNoise({ kFs, 2, 10.0, kFixtureSeed }, -18.0);
    juce::AudioBuffer<float> work(2, block);
    std::vector<double> us;
    for (int start = 0; start + block <= source.getNumSamples(); start += block)
    {
        for (int ch = 0; ch < 2; ++ch)
            work.copyFrom(ch, 0, source, ch, start, block);
        const auto t0 = std::chrono::steady_clock::now();
        processBlock(work);
        us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
    }
    const auto t = summarise(std::move(us));
    const double budget = 1.0e6 * block / kFs;
    return JsonObject().set("block", block).set("p50_us", t.p50).set("p95_us", t.p95).set("p99_us", t.p99)
        .set("max_us", t.max).set("p99_budget_fraction", t.p99 / budget);
}

juce::var engineTiming(const LayerSnapshot& snapshot, int block = 512)
{
    auto enginePtr = std::make_unique<LayerEngine>(); // ~115 KB: keep it off the stack
    auto& engine = *enginePtr;
    engine.prepare(kFs, block);
    engine.publish(snapshot);
    return timing([&](juce::AudioBuffer<float>& b) { engine.process(b.getArrayOfWritePointers(), 2, b.getNumSamples()); }, block);
}
} // namespace

int runM3Lab(const juce::File& out, const juce::String& commit); // LabM3.cpp
int runM4Lab(const juce::File& out, const juce::String& commit); // LabM4.cpp
int runM6Lab(const juce::File& out, const juce::String& commit); // LabM6.cpp
int runUiSnapshots(const juce::File& out); // LabUi.cpp
int runBench(const juce::File& out, const juce::String& commit, int reps, int blocks, bool cold); // LabBench.cpp

int main(int argc, char* argv[])
{
    juce::StringArray args;
    for (int i = 1; i < argc; ++i)
        args.add(argv[i]);
    if (args.contains("ui"))
    {
        const auto o = args.indexOf("--out");
        return runUiSnapshots(juce::File::getCurrentWorkingDirectory().getChildFile(o >= 0 ? args[o + 1] : "hpeq_ui"));
    }
    if (args.contains("bench"))
    {
        const auto o = args.indexOf("--out"), c = args.indexOf("--commit"), r = args.indexOf("--reps"), b = args.indexOf("--blocks");
        return runBench(juce::File::getCurrentWorkingDirectory().getChildFile(o >= 0 ? args[o + 1] : "bench.json"),
                        c >= 0 ? args[c + 1] : juce::String("unknown"), r >= 0 ? args[r + 1].getIntValue() : 3,
                        b >= 0 ? args[b + 1].getIntValue() : 2000, args.contains("--cold"));
    }
    if (args.contains("m6"))
    {
        const auto o = args.indexOf("--out"), c = args.indexOf("--commit");
        return runM6Lab(juce::File::getCurrentWorkingDirectory().getChildFile(o >= 0 ? args[o + 1] : "hpeq_m6_lab"),
                        c >= 0 ? args[c + 1] : juce::String("unknown"));
    }
    if (args.contains("m4"))
    {
        const auto o = args.indexOf("--out"), c = args.indexOf("--commit");
        return runM4Lab(juce::File::getCurrentWorkingDirectory().getChildFile(o >= 0 ? args[o + 1] : "hpeq_m4_lab"),
                        c >= 0 ? args[c + 1] : juce::String("unknown"));
    }
    if (args.contains("m3"))
    {
        const auto o = args.indexOf("--out"), c = args.indexOf("--commit");
        return runM3Lab(juce::File::getCurrentWorkingDirectory().getChildFile(o >= 0 ? args[o + 1] : "hpeq_m3_lab"),
                        c >= 0 ? args[c + 1] : juce::String("unknown"));
    }
    const auto outIndex = args.indexOf("--out");
    const auto out = juce::File::getCurrentWorkingDirectory().getChildFile(outIndex >= 0 ? args[outIndex + 1] : "hpeq_m2_lab");
    const auto commitIndex = args.indexOf("--commit");
    const auto commit = commitIndex >= 0 ? args[commitIndex + 1] : juce::String("unknown");

    const auto sweep = makeExpSweep({ kFs, 1, 4.0, kFixtureSeed }, 10.0, 22000.0, -12.0);
    auto results = jsonArray();

    for (const auto& sc : scenarios())
    {
        std::cout << "scenario " << sc.id << "\n";
        double minLow = 1e9, maxHigh = 0;
        for (const auto& r : sc.nodes) { minLow = std::min(minLow, r.low); maxHigh = std::max(maxHigh, r.high); }
        const auto farOutside = [&](double f) { return f < minLow / 2.0 || f > maxHigh * 2.0; };
        auto entry = JsonObject().set("id", sc.id).set("description", sc.description);

        for (const juce::String hyp : { "H1", "H1_LR8", "H2", "H3_tree", "H3_engine" })
        {
            if (hyp == "H2" && !sc.contiguous)
            {
                entry.set("H2", JsonObject().set("status", "N/A").set("reason", "H2 cannot represent overlapping ranges"));
                continue;
            }
            const auto run = [&](const std::vector<FxSlotParams>& slots)
            {
                std::vector<double> gainsDb;
                for (const auto& sl : slots)
                    gainsDb.push_back(sl.active ? static_cast<double>(sl.values[slot::kGainDb]) : 0.0);
                if (hyp == "H1")
                    return renderH1Offline(sweep, sc.nodes, gainsDb, 2);
                if (hyp == "H3_engine")
                    return renderEngine(sweep, sc.nodes, slots);
                if (hyp == "H3_tree")
                    return renderH3(sweep, sc.nodes, [&](int j, juce::AudioBuffer<float>& band)
                    {
                        const auto& sl = slots[static_cast<size_t>(j)];
                        if (sl.active)
                            band.applyGain(std::pow(10.0f, sl.values[slot::kGainDb] / 20.0f));
                    });
                if (hyp == "H1_LR8")
                    return renderH1Offline(sweep, sc.nodes, gainsDb, 4);
                return renderH2(sweep, sc.nodes, [&](int j, juce::AudioBuffer<float>& band)
                {
                    const auto& s = slots[static_cast<size_t>(j)];
                    if (s.active)
                        band.applyGain(std::pow(10.0f, s.values[slot::kGainDb] / 20.0f));
                });
            };

            const std::vector<FxSlotParams> bypass(sc.nodes.size(), gainSlot(0.0, false));
            const std::vector<FxSlotParams> all(sc.nodes.size(), gainSlot(-6.0));
            std::vector<FxSlotParams> one(sc.nodes.size(), gainSlot(0.0, false));
            const size_t target = sc.nodes.size() / 2;
            one[target] = gainSlot(-6.0);

            const auto yBypass = run(bypass);
            const auto rBypass = measureResponse(yBypass, sweep);
            const auto rAll = measureResponse(run(all), sweep);
            const auto rOne = measureResponse(run(one), sweep);
            const auto null = compareAligned(yBypass, sweep);
            // Passive nodes: present and running, processing at 0 dB (H3: the dry all-pass is heard).
            const auto yPassive = run(std::vector<FxSlotParams>(sc.nodes.size(), gainSlot(0.0)));
            const auto rPassive = measureResponse(yPassive, sweep);
            double passiveGd = 0.0;
            for (double g : rPassive.groupDelayMs)
                passiveGd = std::max(passiveGd, std::abs(g));

            // Impulse: where does the bypass response peak (time smearing of H2).
            const auto impulse = makeImpulse({ kFs, 1, 0.5, kFixtureSeed }, 0, 1.0f);
            const auto impulseOut = hyp == "H2"        ? renderH2(impulse, sc.nodes, [](int, juce::AudioBuffer<float>&) {})
                                  : hyp == "H3_tree"   ? renderH3(impulse, sc.nodes, [](int, juce::AudioBuffer<float>&) {})
                                  : hyp == "H3_engine" ? renderEngine(impulse, sc.nodes, bypass)
                                                       : impulse; // H1 at bypass is an exact identity
            int peakIndex = 0;
            for (int i = 0; i < impulseOut.getNumSamples(); ++i)
                if (std::abs(impulseOut.getSample(0, i)) > std::abs(impulseOut.getSample(0, peakIndex)))
                    peakIndex = i;

            auto depths = jsonArray();
            for (const auto& r : sc.nodes)
                depths.append(magnitudeAt(rAll, std::sqrt(r.low * r.high)));
            const auto& t = sc.nodes[target];
            double neighbourWorst = 0.0;
            for (size_t j = 0; j < sc.nodes.size(); ++j)
                if (j != target)
                    neighbourWorst = std::max(neighbourWorst, std::abs(magnitudeAt(rOne, std::sqrt(sc.nodes[j].low * sc.nodes[j].high))));
            double gdMax = 0.0;
            for (double g : rBypass.groupDelayMs)
                gdMax = std::max(gdMax, std::abs(g));

            entry.set(hyp, JsonObject()
                               .set("bypass_bit_exact", null.bitExact)
                               .set("bypass_null_error_db", null.errorDb)
                               .set("bypass_magnitude_max_abs_db", maxAbsWhere(rBypass, [](double) { return true; }))
                               .set("bypass_group_delay_max_ms", gdMax)
                               .set("bypass_impulse_peak_sample", peakIndex)
                               .set("passive_null_error_db", compareAligned(yPassive, sweep).errorDb)
                               .set("passive_magnitude_max_abs_db", maxAbsWhere(rPassive, [](double) { return true; }))
                               .set("passive_group_delay_max_ms", passiveGd)
                               .set("all_minus6_depth_at_node_centres_db", depths)
                               .set("all_minus6_inside_union_max_dev_from_-6.02_db",
                                    maxAbsWhere(rAll, [&](double f) { return f > minLow * 1.25 && f < maxHigh / 1.25; }, -6.0206))
                               .set("all_minus6_far_outside_max_abs_db", maxAbsWhere(rAll, farOutside))
                               .set("one_minus6_target_centre_db", magnitudeAt(rOne, std::sqrt(t.low * t.high)))
                               .set("one_minus6_neighbour_centres_max_abs_db", neighbourWorst)
                               .set("one_minus6_far_outside_max_abs_db", maxAbsWhere(rOne, farOutside))
                               .set("all_minus6_curve_db", [&] { auto a = jsonArray(); for (double v : rAll.magnitudeDb) a.append(std::isfinite(v) ? v : 0.0); return a; }())
                               .set("one_minus6_curve_db", [&] { auto a = jsonArray(); for (double v : rOne.magnitudeDb) a.append(std::isfinite(v) ? v : 0.0); return a; }()));

            // Plots for the single-node, 5-node and overlap cases.
            if (sc.id == "A_1node" || sc.id == "D_5nodes" || sc.id == "E_overlap")
            {
                const auto colour = hyp == "H1" ? juce::String("#1f77b4") : hyp == "H2" ? juce::String("#d62728")
                                  : hyp == "H3_tree" ? juce::String("#ff7f0e")
                                  : hyp == "H3_engine" ? juce::String("#9467bd") : juce::String("#2ca02c");
                static std::map<juce::String, std::vector<PlotSeries>> magnitudePlots, delayPlots;
                magnitudePlots[sc.id].push_back({ hyp + " all nodes -6 dB", colour, curve(rAll, rAll.magnitudeDb) });
                magnitudePlots[sc.id].push_back({ hyp + " middle node -6 dB", colour, curve(rOne, rOne.magnitudeDb), true });
                delayPlots[sc.id].push_back({ hyp + " bypass", colour, curve(rBypass, rBypass.groupDelayMs) });
                PlotSpec mag { sc.description + ": magnitude, gain -6 dB per node", "Frequency (Hz)", "dB" };
                mag.yMin = -9.0; mag.yMax = 3.0; mag.yStep = 1.5;
                writeSvgPlot(out.getChildFile("plots/" + sc.id + "_magnitude.svg"), mag, magnitudePlots[sc.id]);
                PlotSpec gd { sc.description + ": group delay at bypass", "Frequency (Hz)", "ms" };
                gd.yMin = -1.0; gd.yMax = 6.0; gd.yStep = 1.0;
                writeSvgPlot(out.getChildFile("plots/" + sc.id + "_bypass_group_delay.svg"), gd, delayPlots[sc.id]);
            }
        }
        {
            // Cross-check: the engine against the offline order-free H3 formula (RBJ cascades), all nodes -6 dB.
            const std::vector<FxSlotParams> all(sc.nodes.size(), gainSlot(-6.0));
            const auto engine = renderEngine(sweep, sc.nodes, all);
            const auto offline = renderH3Offline(sweep, sc.nodes, std::vector<double>(sc.nodes.size(), -6.0));
            entry.set("engine_vs_offline_H3_error_db", compareAligned(engine, offline).errorDb);
        }
        results.append(entry);
    }

    // Compressor on a region, music-like input: in-band reduction vs out-of-band preservation.
    std::cout << "compressor region test\n";
    auto drums = makeSyntheticDrums({ kFs, 2, 4.0, kFixtureSeed });
    const std::vector<Range> regionA { { 250, 700 } };
    CompressorProcessor h1Compressor;
    h1Compressor.prepare(kFs);
    const auto h1Comp = renderH1OfflineBand(drums, regionA.front(), [&](juce::AudioBuffer<float>& band)
    {
        h1Compressor.process(band.getArrayOfWritePointers(), band.getNumChannels(), band.getNumSamples(), compressorSlot());
    });
    const auto engineComp = renderEngine(drums, regionA, { compressorSlot() });
    CompressorProcessor h2Compressor;
    h2Compressor.prepare(kFs);
    const auto h2Comp = renderH2(drums, regionA, [&](int, juce::AudioBuffer<float>& band)
    {
        h2Compressor.process(band.getArrayOfWritePointers(), band.getNumChannels(), band.getNumSamples(), compressorSlot());
    });
    CompressorProcessor h3Compressor;
    h3Compressor.prepare(kFs);
    const auto h3Comp = renderH3(drums, regionA, [&](int, juce::AudioBuffer<float>& band)
    {
        h3Compressor.process(band.getArrayOfWritePointers(), band.getNumChannels(), band.getNumSamples(), compressorSlot());
    });
    const auto bandDelta = [&](const juce::AudioBuffer<float>& y, double lo, double hi)
    {
        return bandPowerDb(y, lo, hi) - bandPowerDb(drums, lo, hi);
    };
    auto compressor = JsonObject();
    for (const auto& [name, y] : { std::pair<const char*, const juce::AudioBuffer<float>*> { "H1", &h1Comp }, { "H2", &h2Comp }, { "H3_tree", &h3Comp }, { "H3_engine", &engineComp } })
        compressor.set(name, JsonObject()
                                 .set("power_change_40_100hz_db", bandDelta(*y, 40, 100))
                                 .set("power_change_300_600hz_db", bandDelta(*y, 300, 600))
                                 .set("power_change_3k_8k_db", bandDelta(*y, 3000, 8000))
                                 .set("null_vs_input_db", compareAligned(*y, drums).errorDb));
    writeFloatWav(out.getChildFile("wav/drums_input.wav"), drums, kFs);
    writeFloatWav(out.getChildFile("wav/drums_H1_compressor_250-700.wav"), h1Comp, kFs);
    writeFloatWav(out.getChildFile("wav/drums_H2_compressor_250-700.wav"), h2Comp, kFs);
    writeFloatWav(out.getChildFile("wav/drums_H3_tree_compressor_250-700.wav"), h3Comp, kFs);
    writeFloatWav(out.getChildFile("wav/drums_H3_engine_compressor_250-700.wav"), engineComp, kFs);

    // CPU on this machine (48 kHz stereo, block 512).
    std::cout << "timing\n";
    auto cpu = JsonObject();
    {
        LayerSnapshot one;
        one.fx[0].active = true; one.fx[0].lowHz = 250; one.fx[0].highHz = 700; one.fx[0].numSlots = 1; one.fx[0].slots[0] = compressorSlot();
        cpu.set("H3_engine_1node_compressor", engineTiming(one));

        LayerSnapshot five;
        const auto d = scenarios()[3].nodes; // copy: scenarios() returns a temporary
        for (size_t i = 0; i < d.size(); ++i)
        {
            auto& n = five.fx[i];
            n.active = true; n.lowHz = static_cast<float>(d[i].low); n.highHz = static_cast<float>(d[i].high); n.numSlots = 1; n.slots[0] = compressorSlot();
        }
        cpu.set("H3_engine_5nodes_compressor", engineTiming(five));

        LayerSnapshot worst; // 8 nodes x 6 compressors + 24 bells: the per-layer ceiling of D2
        for (int i = 0; i < kMaxEqNodesPerLayer; ++i)
        {
            auto& b = worst.eq[static_cast<size_t>(i)];
            b.active = true; b.frequencyHz = 40.0f * std::pow(1.3f, static_cast<float>(i)); b.gainDb = 1.0f; b.q = 1.0f;
        }
        for (int i = 0; i < kMaxFxNodesPerLayer; ++i)
        {
            auto& n = worst.fx[static_cast<size_t>(i)];
            n.active = true; n.lowHz = 60.0f * std::pow(2.0f, static_cast<float>(i)); n.highHz = n.lowHz * 2.5f; n.numSlots = kMaxFxPerNode;
            for (auto& s : n.slots) s = compressorSlot();
        }
        cpu.set("H3_engine_worst_24bells_8nodes_x6compressors", engineTiming(worst));

        cpu.set("H2_5nodes_gain_split_only", timing([&](juce::AudioBuffer<float>& b)
        {
            b = renderH2(b, d, [](int, juce::AudioBuffer<float>& band) { band.applyGain(0.5f); }, false);
        }, 512));
    }

    auto grid = jsonArray();
    for (double f : logGrid())
        grid.append(f);
    const auto report = JsonObject()
                            .set("milestone", "M2")
                            .set("frequency_grid_hz", grid)
                            .set("commit", commit)
                            .set("machine", describeMachine())
                            .set("sample_rate", kFs)
                            .set("scenarios", results)
                            .set("compressor_region_250_700", compressor)
                            .set("cpu", cpu)
                            .set("note_H2_timing", "lab implementation allocates per call (not RT code); split cost only, indicative");
    writeJson(out.getChildFile("m2_lab.json"), report);
    std::cout << "lab -> " << out.getFullPathName() << "\n";
    return 0;
}
