// HamloUdarnik_Lab <material dir> [report.md] [label] [classic]
// docs/UDARNIK_V2.md: targets U1 .. U8 measured through the plug-in (latency alignment, MIX, ON included) on the
// owner's clips. The same code makes the "before" and the "after" run; parameters that do not exist yet
// (`engine`, `protect`) are skipped. Exit code = checks that fail.

#include <cmath>
#include <functional>
#include <iostream>
#include <vector>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloUdarnikProcessor.h"

namespace
{
using hamlo::udarnik::HamloUdarnikProcessor;
constexpr double kFs = 48000.0;
constexpr int kBlock = 512;
using KV = std::vector<std::pair<const char*, double>>;

double rmsOf(const juce::AudioBuffer<float>& x, int from, int to)
{
    from = juce::jlimit(0, x.getNumSamples(), from);
    to = juce::jlimit(from, x.getNumSamples(), to);
    if (to <= from)
        return 0.0;
    double s = 0.0;
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        for (int i = from; i < to; ++i)
            s += static_cast<double>(x.getSample(ch, i)) * x.getSample(ch, i);
    return std::sqrt(s / (x.getNumChannels() * static_cast<double>(to - from)));
}

double activeRms(const juce::AudioBuffer<float>& x)
{
    double s = 0.0;
    int n = 0;
    for (int start = 0; start + kBlock <= x.getNumSamples(); start += kBlock)
    {
        const double r = rmsOf(x, start, start + kBlock);
        if (r > std::pow(10.0, -50.0 / 20.0))
        {
            s += r * r;
            ++n;
        }
    }
    return n > 0 ? std::sqrt(s / n) : 0.0;
}

juce::AudioBuffer<float> load(const juce::File& file, double rmsDb, double seconds = 12.0)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(file));
    if (r == nullptr)
        return {};
    const int n = static_cast<int>(std::min<juce::int64>(r->lengthInSamples, static_cast<juce::int64>(seconds * kFs)));
    juce::AudioBuffer<float> x(2, n);
    r->read(&x, 0, n, 0, true, true);
    x.applyGain(static_cast<float>(std::pow(10.0, rmsDb / 20.0) / std::max(1.0e-9, activeRms(x))));
    return x;
}

std::vector<int> onsets(const juce::AudioBuffer<float>& x);
bool gClassic = false; // `classic`: engine 0, no protection = the HPEQ laws, bit-identical to HamloUdarnik 0.1.0

juce::AudioBuffer<float> render(const juce::AudioBuffer<float>& x, const KV& values, int& latency)
{
    HamloUdarnikProcessor p;
    for (const auto& [id, v] : values)
        if (p.parameter(id) != nullptr)
            p.setParameterValue(id, v);
    if (gClassic && p.parameter("engine") != nullptr)
    {
        p.setParameterValue("engine", 0.0);
        p.setParameterValue("protect", 0.0);
    }
    p.prepareToPlay(kFs, kBlock);
    latency = p.getLatencySamples();
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += kBlock)
    {
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(kBlock, y.getNumSamples() - start));
        p.processBlock(view, midi);
    }
    if (juce::SystemStats::getEnvironmentVariable("UDARNIK_LAB_DEBUG", {}).isNotEmpty() && p.shaper().v2Active())
        std::cout << "  DEBUG v2 hits " << p.shaper().engine2().meters().events << ", lab onsets " << onsets(x).size() << std::endl;
    return y;
}

// Onsets: the 1 ms RMS rises over 2.5 x its 50 ms mean and above -45 dB re the clip's peak; 60 ms apart at least.
std::vector<int> onsets(const juce::AudioBuffer<float>& x)
{
    std::vector<int> out;
    const double floorLevel = std::pow(10.0, -45.0 / 20.0) * x.getMagnitude(0, x.getNumSamples());
    const int w = static_cast<int>(0.001 * kFs), longW = static_cast<int>(0.050 * kFs), gap = static_cast<int>(0.060 * kFs);
    int last = -gap;
    for (int i = longW; i + w < x.getNumSamples(); i += w / 2)
    {
        const double now = rmsOf(x, i, i + w), before = rmsOf(x, i - longW, i);
        if (now > 2.5 * before && now > floorLevel && i - last >= gap)
        {
            out.push_back(i);
            last = i;
        }
    }
    return out;
}

// Every attack, also one riding on a previous hit's tail (1 ms RMS over 2 x its 10 ms mean, above the floor,
// 15 ms apart): the body window stops before the next of these. Added after the first runs on the drum bus,
// where a second attack 25 .. 100 ms after a hit counted as its body (docs/reports/V2_REPORT.md).
std::vector<int> attacks(const juce::AudioBuffer<float>& x)
{
    std::vector<int> out;
    const double floorLevel = std::pow(10.0, -45.0 / 20.0) * x.getMagnitude(0, x.getNumSamples());
    const int w = static_cast<int>(0.001 * kFs), shortW = static_cast<int>(0.010 * kFs), gap = static_cast<int>(0.015 * kFs);
    int last = -gap;
    for (int i = shortW; i + w < x.getNumSamples(); i += w / 2)
    {
        const double now = rmsOf(x, i, i + w), before = rmsOf(x, i - shortW, i);
        if (now > 2.0 * before && now > floorLevel && i - last >= gap)
        {
            out.push_back(i);
            last = i;
        }
    }
    return out;
}

// Mean attack contrast (dB) over the hits: (out A - in A) - (out B - in B), A = 0 .. 10 ms, B = 25 .. 100 ms.
double attackContrast(const juce::AudioBuffer<float>& x, const juce::AudioBuffer<float>& y, int latency)
{
    const auto hits = onsets(x);
    const auto every = attacks(x);
    const int a = static_cast<int>(0.010 * kFs), b0 = static_cast<int>(0.025 * kFs), b1 = static_cast<int>(0.100 * kFs), guard = static_cast<int>(0.005 * kFs);
    double sum = 0.0;
    int n = 0;
    for (size_t k = 0; k < hits.size(); ++k)
    {
        const int t = hits[k];
        int end = std::min(t + b1, (k + 1 < hits.size() ? hits[k + 1] - guard : x.getNumSamples()));
        for (const int a2 : every)
            if (a2 > t + a && a2 - guard < end)
                end = a2 - guard;
        if (end - (t + b0) < static_cast<int>(0.015 * kFs) || t + b1 + latency >= x.getNumSamples())
            continue;
        const double inA = rmsOf(x, t, t + a), inB = rmsOf(x, t + b0, end);
        const double outA = rmsOf(y, t + latency, t + a + latency), outB = rmsOf(y, t + b0 + latency, end + latency);
        if (inA <= 0.0 || inB <= 1.0e-6 || outA <= 0.0 || outB <= 0.0)
            continue;
        sum += 20.0 * std::log10(outA / inA) - 20.0 * std::log10(outB / inB);
        if (juce::SystemStats::getEnvironmentVariable("UDARNIK_LAB_HITS", {}).isNotEmpty())
            std::cout << "    hit " << t / kFs << " s: A " << 20.0 * std::log10(outA / inA) << " dB, B " << 20.0 * std::log10(outB / inB) << " dB, in A/B " << 20.0 * std::log10(inA / inB)
                      << " dB, next " << (k + 1 < hits.size() ? (hits[k + 1] - t) / kFs * 1000.0 : -1.0) << " ms" << std::endl;
        ++n;
    }
    return n > 0 ? sum / n : 0.0;
}

struct Row
{
    juce::String id, check, measured, target;
    bool pass;
};
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
    {
        std::cout << "usage: HamloUdarnik_Lab <material dir> [report.md] [label]\n";
        return 2;
    }
    const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    gClassic = argc > 4 && juce::String(argv[4]) == "classic";
    const char* names[] { "kick", "snare", "hihat", "perc", "drumbus1" };
    std::vector<juce::AudioBuffer<float>> clips;
    for (const auto* n : names)
        clips.push_back(load(dir.getChildFile(juce::String(n) + ".wav"), -18.0));

    std::vector<Row> rows;
    const auto add = [&](const char* id, const juce::String& check, const juce::String& measured, const juce::String& target, bool pass)
    {
        rows.push_back({ id, check, measured, target, pass });
        std::cout << id << "  " << check << ": " << measured << "  (target " << target << ")" << (pass ? "" : "  FAIL") << std::endl;
    };
    const auto f = [](double v, int d = 1) { return juce::String(v, d); };
    const char* modes[] { "PRAVDA", "UDARNIK", "PERESTROIKA" };

    // U1 .. U4: contrast per clip and mode
    struct Case
    {
        const char* id;
        const char* knob;
        double value;
        bool body;
        std::function<bool(double)> ok;
        const char* target;
    };
    const std::vector<Case> cases { { "U1", "attackDb", 12, false, [](double c) { return c >= 8.0; }, ">= +8 dB" },
                                    { "U2", "attackDb", 6, false, [](double c) { return c >= 4.0; }, ">= +4 dB" },
                                    { "U3", "attackDb", -12, false, [](double c) { return c <= -8.0; }, "<= -8 dB" },
                                    { "U4", "sustainDb", -12, true, [](double c) { return c <= -8.0; }, "<= -8 dB" },
                                    { "U4", "sustainDb", 12, true, [](double c) { return c >= 8.0; }, ">= +8 dB" } };
    for (const auto& c : cases)
        for (int mode = 0; mode < 3; ++mode)
        {
            juce::String values;
            bool all = true;
            double worst = c.ok(1.0e9) ? 1.0e9 : -1.0e9;
            for (size_t k = 0; k < clips.size(); ++k)
            {
                int latency = 0;
                const auto y = render(clips[k], { { c.knob, c.value }, { "style", mode }, { "lookahead", 1 }, { "engine", 1 }, { "protect", 0 } }, latency);
                const double contrast = attackContrast(clips[k], y, latency) * (c.body ? -1.0 : 1.0);
                all = all && c.ok(contrast);
                values << (values.isEmpty() ? "" : " / ") << f(contrast);
            }
            juce::ignoreUnused(worst);
            add(c.id, juce::String(c.body ? "BODY " : "SNAP ") + (c.value > 0 ? "+" : "") + juce::String(c.value, 0) + " dB, " + modes[mode]
                          + ": contrast kick / snare / hats / perc / bus",
                values + " dB", c.target, all);
        }

    {
        auto loud = clips[4], quiet = clips[4];
        loud.applyGain(static_cast<float>(std::pow(10.0, 12.0 / 20.0)));
        quiet.applyGain(static_cast<float>(std::pow(10.0, -12.0 / 20.0)));
        int l1 = 0, l2 = 0;
        const KV v { { "attackDb", 9 }, { "lookahead", 1 }, { "engine", 1 }, { "protect", 0 } };
        const auto yLoud = render(loud, v, l1), yQuiet = render(quiet, v, l2); // (the latency is known only after the render)
        const double a = attackContrast(loud, yLoud, l1), b = attackContrast(quiet, yQuiet, l2);
        add("U5", "Level independence: drum bus at -6 vs -30 dBFS RMS, SNAP +9 (PRAVDA): contrast", f(a, 2) + " vs " + f(b, 2) + " dB", "within 1 dB", std::abs(a - b) <= 1.0);
    }
    {
        const int n = static_cast<int>(4.0 * kFs);
        juce::AudioBuffer<float> x(2, n);
        for (int i = 0; i < n; ++i)
        {
            const double v = 0.09 * (std::sin(2.0 * juce::MathConstants<double>::pi * 220.0 * i / kFs) + std::sin(2.0 * juce::MathConstants<double>::pi * 1000.0 * i / kFs));
            x.setSample(0, i, static_cast<float>(v));
            x.setSample(1, i, static_cast<float>(v));
        }
        for (int mode = 0; mode < 3; ++mode)
        {
            int latency = 0;
            const auto y = render(x, { { "attackDb", 12 }, { "sustainDb", -12 }, { "style", mode }, { "lookahead", 1 }, { "engine", 1 }, { "protect", 1 } }, latency);
            const int from = static_cast<int>(1.0 * kFs);
            double r = 0.0, in2 = 0.0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = from; i < n - latency; ++i)
                {
                    const double e = y.getSample(ch, i + latency) - x.getSample(ch, i);
                    r += e * e;
                    in2 += static_cast<double>(x.getSample(ch, i)) * x.getSample(ch, i);
                }
            const double level = 20.0 * std::log10(rmsOf(y, from + latency, n) / rmsOf(x, from, n - latency));
            const double residual = 10.0 * std::log10((r + 1.0e-30) / in2);
            add("U6", juce::String("Steady 220 Hz + 1 kHz, SNAP +12 / BODY -12, ") + modes[mode] + ": level change; residual",
                f(level, 2) + " dB; " + f(residual) + " dB", "<= 0.5 dB; <= -50 dB", std::abs(level) <= 0.5 && residual <= -50.0);
        }
    }
    {
        auto bus = clips[4];
        bus.applyGain(static_cast<float>(std::pow(10.0, -1.0 / 20.0) / bus.getMagnitude(0, bus.getNumSamples())));
        for (int mode = 0; mode < 3; ++mode)
        {
            int latency = 0;
            const auto y = render(bus, { { "attackDb", 12 }, { "style", mode }, { "lookahead", 1 }, { "engine", 1 }, { "protect", 1 } }, latency);
            const double peak = 20.0 * std::log10(y.getMagnitude(0, y.getNumSamples()));
            add("U7", juce::String("SNAP +12 on the drum bus at -1 dBFS peak, protection on, ") + modes[mode] + ": output peak", f(peak, 2) + " dBFS", "<= 0 dBFS", peak <= 0.0);
        }
    }
    {
        const auto& kick = clips[0];
        const auto first = [](const juce::AudioBuffer<float>& b, int from)
        {
            const float level = 0.25f * b.getMagnitude(0, b.getNumSamples());
            for (int i = from; i < b.getNumSamples(); ++i)
                if (std::abs(b.getSample(0, i)) >= level)
                    return i;
            return -1;
        };
        for (int mode = 0; mode < 3; ++mode)
        {
            int latency = 0;
            const auto y = render(kick, { { "attackDb", 12 }, { "style", mode }, { "lookahead", 1 }, { "engine", 1 }, { "protect", 0 } }, latency);
            const double shift = (first(y, 0) - latency - first(kick, 0)) / kFs * 1000.0;
            add("U8", juce::String("Onset (first sample at 25 % of the peak), SNAP +12, ") + modes[mode] + ": output - input - latency", f(shift, 3) + " ms", "within 0.1 ms",
                std::abs(shift) <= 0.1);
        }
    }

    int failed = 0;
    juce::String table = "| # | Check | Measured | Target | Pass |\n|---|---|---|---|---|\n";
    for (const auto& r : rows)
    {
        failed += r.pass ? 0 : 1;
        table << "| " << r.id << " | " << r.check << " | " << r.measured << " | " << r.target << " | " << (r.pass ? "yes" : "**no**") << " |\n";
    }
    std::cout << rows.size() - static_cast<size_t>(failed) << " / " << rows.size() << " checks pass" << std::endl;
    if (argc > 2)
        juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]).replaceWithText("# HamloUdarnik lab: " + juce::String(argc > 3 ? argv[3] : "run") + "\n\n"
                                                                                        + juce::String(static_cast<int>(rows.size()) - failed) + " / "
                                                                                        + juce::String(static_cast<int>(rows.size())) + " checks pass\n\n" + table);
    return failed;
}
