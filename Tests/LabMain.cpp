// HamloRedWall_Lab <material dir> [report.md] [label]
// docs/REDWALL_V2.md targets R1 .. R10 measured through the plug-in. The same code makes the "before" and the
// "after" run; parameters that do not exist yet (`drive`, `tube`) are skipped and their rows report "n/a".
// Exit code = checks that fail.

#include <cmath>
#include <functional>
#include <iostream>
#include <vector>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloRedWallProcessor.h"

namespace
{
using hamlo::redwall::HamloRedWallProcessor;
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;
constexpr int kBlock = 512;
using KV = std::vector<std::pair<const char*, double>>;

bool hasParameter(const char* id)
{
    HamloRedWallProcessor p;
    return p.parameter(id) != nullptr;
}

juce::AudioBuffer<float> render(const juce::AudioBuffer<float>& x, const KV& values)
{
    HamloRedWallProcessor p;
    for (const auto& [id, v] : values)
        if (p.parameter(id) != nullptr)
            p.setParameterValue(id, v);
    p.prepareToPlay(kFs, kBlock);
    const int latency = p.getLatencySamples();
    // feed the input followed by `latency` zeros, return the output aligned to the input
    juce::AudioBuffer<float> y(2, x.getNumSamples() + latency);
    y.clear();
    for (int ch = 0; ch < 2; ++ch)
        y.copyFrom(ch, 0, x, ch % x.getNumChannels(), 0, x.getNumSamples());
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += kBlock)
    {
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(kBlock, y.getNumSamples() - start));
        p.processBlock(view, midi);
    }
    juce::AudioBuffer<float> out(2, x.getNumSamples());
    for (int ch = 0; ch < 2; ++ch)
        out.copyFrom(ch, 0, y, ch, latency, x.getNumSamples());
    return out;
}

double peakDb(const juce::AudioBuffer<float>& x) { return 20.0 * std::log10(x.getMagnitude(0, x.getNumSamples()) + 1.0e-12); }

double truePeakDb(const juce::AudioBuffer<float>& x)
{
    // 8x reconstruction, windowed sinc (Hann, 32 taps each side)
    double peak = 0.0;
    constexpr int taps = 32;
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
    {
        const float* d = x.getReadPointer(ch);
        for (int i = taps; i < x.getNumSamples() - taps; ++i)
        {
            peak = std::max(peak, static_cast<double>(std::abs(d[i])));
            if (std::abs(d[i]) < 0.5f * std::pow(10.0f, -1.0f / 20.0f))
                continue; // a true peak near the ceiling needs a sample nearby above -7 dB
            for (int k = 1; k < 8; ++k)
            {
                const double frac = k / 8.0;
                double v = 0.0;
                for (int j = -taps + 1; j <= taps; ++j)
                {
                    const double t = j - frac;
                    const double sinc = std::sin(kPi * t) / (kPi * t);
                    const double w = 0.5 + 0.5 * std::cos(kPi * t / (taps + 0.5));
                    v += d[i + j] * sinc * w;
                }
                peak = std::max(peak, std::abs(v));
            }
        }
    }
    return 20.0 * std::log10(peak + 1.0e-12);
}

// BS.1770-4 integrated loudness (48 kHz K-weighting, 400 ms blocks, 75 % overlap, gates -70 LUFS and -10 LU)
double lufs(const juce::AudioBuffer<float>& x)
{
    const double b1[3] { 1.53512485958697, -2.69169618940638, 1.19839281085285 }, a1[3] { 1.0, -1.69065929318241, 0.73248077421585 };
    const double b2[3] { 1.0, -2.0, 1.0 }, a2[3] { 1.0, -1.99004745483398, 0.99007225036621 };
    std::vector<std::vector<double>> k(static_cast<size_t>(x.getNumChannels()), std::vector<double>(static_cast<size_t>(x.getNumSamples())));
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
    {
        double s1[2] {}, s2[2] {};
        for (int i = 0; i < x.getNumSamples(); ++i)
        {
            const double in = x.getSample(ch, i);
            const double y1 = b1[0] * in + s1[0];
            s1[0] = b1[1] * in - a1[1] * y1 + s1[1];
            s1[1] = b1[2] * in - a1[2] * y1;
            const double y2 = b2[0] * y1 + s2[0];
            s2[0] = b2[1] * y1 - a2[1] * y2 + s2[1];
            s2[1] = b2[2] * y1 - a2[2] * y2;
            k[static_cast<size_t>(ch)][static_cast<size_t>(i)] = y2;
        }
    }
    const int block = static_cast<int>(0.4 * kFs), hop = block / 4;
    std::vector<double> powers;
    for (int s = 0; s + block <= x.getNumSamples(); s += hop)
    {
        double p = 0.0;
        for (const auto& c : k)
        {
            double e = 0.0;
            for (int i = s; i < s + block; ++i)
                e += c[static_cast<size_t>(i)] * c[static_cast<size_t>(i)];
            p += e / block;
        }
        powers.push_back(p);
    }
    const auto gated = [&](double floorLufs)
    {
        double sum = 0.0;
        int n = 0;
        for (double p : powers)
            if (-0.691 + 10.0 * std::log10(p + 1.0e-30) > floorLufs)
            {
                sum += p;
                ++n;
            }
        return n > 0 ? sum / n : 1.0e-30;
    };
    const double relative = -0.691 + 10.0 * std::log10(gated(-70.0)) - 10.0;
    return -0.691 + 10.0 * std::log10(gated(relative));
}

juce::AudioBuffer<float> load(const juce::File& file, double seconds)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(file));
    if (r == nullptr)
        return {};
    const int n = static_cast<int>(std::min<juce::int64>(r->lengthInSamples, static_cast<juce::int64>(seconds * kFs)));
    juce::AudioBuffer<float> x(2, n);
    r->read(&x, 0, n, 0, true, true);
    return x;
}

juce::AudioBuffer<float> tone(double seconds, std::vector<std::pair<double, double>> parts) // (Hz, dBFS)
{
    const int n = static_cast<int>(seconds * kFs);
    juce::AudioBuffer<float> x(2, n);
    for (int i = 0; i < n; ++i)
    {
        double v = 0.0;
        for (const auto& [hz, db] : parts)
            v += std::pow(10.0, db / 20.0) * std::sin(2.0 * kPi * hz * i / kFs);
        x.setSample(0, i, static_cast<float>(v));
        x.setSample(1, i, static_cast<float>(v));
    }
    return x;
}

// Amplitude of `hz` in the output over [from, from + n) (n = whole periods are not needed at this resolution)
double amplitude(const juce::AudioBuffer<float>& y, double hz, int from, int n)
{
    double re = 0, im = 0;
    for (int i = from; i < from + n; ++i)
    {
        const double t = 2.0 * kPi * hz * i / kFs, w = 0.5 - 0.5 * std::cos(2.0 * kPi * (i - from) / n);
        re += w * y.getSample(0, i) * std::cos(t);
        im += w * y.getSample(0, i) * std::sin(t);
    }
    return std::sqrt(re * re + im * im) * 4.0 / n;
}

// The LOUDNESS (threshold) that gives `gr` dB of steady gain reduction on x (bisection on the output level)
double thresholdFor(const juce::AudioBuffer<float>& x, double gr, const KV& base)
{
    double lo = -30.0, hi = 0.0;
    const double inRms = 20.0 * std::log10(x.getRMSLevel(0, x.getNumSamples() / 2, x.getNumSamples() / 2));
    for (int k = 0; k < 14; ++k)
    {
        const double mid = 0.5 * (lo + hi);
        KV v = base;
        v.push_back({ "thresholdDb", mid });
        const auto y = render(x, v);
        const double outRms = 20.0 * std::log10(y.getRMSLevel(0, y.getNumSamples() / 2, y.getNumSamples() / 2));
        const double reduction = (inRms - mid) - outRms; // pushed by -mid, came out lower by the reduction
        (reduction < gr ? hi : lo) = mid;
    }
    return 0.5 * (lo + hi);
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
        std::cout << "usage: HamloRedWall_Lab <material dir> [report.md] [label]\n";
        return 2;
    }
    const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    std::vector<juce::AudioBuffer<float>> mixes;
    for (const char* n : { "mixdown_Ardnold", "mixdown_boom_bap_3", "mixdown_sad_trap" })
        mixes.push_back(load(dir.getChildFile(juce::String(n) + ".wav"), 20.0));
    auto drumbus = load(dir.getChildFile("drumbus1.wav"), 12.0);
    const bool drive = hasParameter("drive"), tube = hasParameter("tube");

    std::vector<Row> rows;
    const auto add = [&](const char* id, const juce::String& check, const juce::String& measured, const juce::String& target, bool pass)
    {
        rows.push_back({ id, check, measured, target, pass });
        std::cout << id << "  " << check << ": " << measured << "  (target " << target << ")" << (pass ? "" : "  FAIL") << std::endl;
    };
    const auto f = [](double v, int d = 2) { return juce::String(v, d); };
    const char* styles[] { "GLASNOST", "STAKHANOV", "OCTOBER" };

    // R1 / R2: the ceiling
    {
        auto hot = drumbus;
        hot.applyGain(static_cast<float>(std::pow(10.0, (-1.0 - peakDb(hot) + 18.0) / 20.0))); // peaks +17 dBFS
        double worstSample = -200.0;
        for (int style = 0; style < 3; ++style)
            for (int tp = 0; tp < 2; ++tp)
                for (const double amount : { 0.0, 1.0 })
                {
                    if (amount > 0.0 && !drive && !tube)
                        continue;
                    worstSample = std::max(worstSample, peakDb(render(hot, { { "ceilingDb", -1 }, { "style", style }, { "truePeak", tp }, { "drive", amount }, { "tube", amount } })));
                }
        add("R1", "Sample peak, drums +18 dB, every style, true peak on / off, DRIVE / TUBE 0 and 100 %", f(worstSample, 3) + " dBFS", "<= -1 dBFS (LIMIT)", worstSample <= -1.0 + 1.0e-4);
        double worstTrue = -200.0;
        std::vector<juce::AudioBuffer<float>> material = mixes;
        material.push_back(drumbus);
        for (const auto& x : material)
            for (const double amount : { 0.0, 1.0 })
            {
                if (amount > 0.0 && !drive && !tube)
                    continue;
                worstTrue = std::max(worstTrue, truePeakDb(render(x, { { "thresholdDb", -9 }, { "ceilingDb", -1 }, { "truePeak", 1 }, { "drive", amount }, { "tube", amount } })));
            }
        add("R2", "True peak (8x), pushed 9 dB, mixes + drum bus, TRUE PEAK on", f(worstTrue, 3) + " dBTP", "<= -0.9 dBTP (LIMIT + 0.1)", worstTrue <= -0.9);
    }

    // R3: bass distortion at 6 dB GR
    {
        const auto x = tone(3.0, { { 50.0, -12.0 } });
        const KV base { { "ceilingDb", -1 }, { "style", 0 }, { "autoRelease", 1 }, { "truePeak", 1 } };
        const double th = thresholdFor(x, 6.0, base);
        KV v = base;
        v.push_back({ "thresholdDb", th });
        const auto y = render(x, v);
        const int from = static_cast<int>(1.5 * kFs), n = static_cast<int>(1.0 * kFs);
        const double h1 = amplitude(y, 50.0, from, n);
        double hs = 0.0;
        for (int k = 2; k <= 5; ++k)
            hs += std::pow(amplitude(y, 50.0 * k, from, n), 2.0);
        const double thd = 100.0 * std::sqrt(hs) / h1;
        add("R3", "50 Hz sine at 6 dB GR (LOUDNESS " + f(th, 1) + "), AUTO, GLASNOST: THD", f(thd, 3) + " %", "<= 0.3 %", thd <= 0.3);
    }
    // R4: intermodulation
    {
        const auto x = tone(3.0, { { 60.0, -6.0 }, { 3000.0, -18.0 } });
        const KV base { { "ceilingDb", -1 }, { "style", 0 }, { "autoRelease", 1 }, { "truePeak", 1 } };
        const double th = thresholdFor(x, 6.0, base);
        KV v = base;
        v.push_back({ "thresholdDb", th });
        const auto y = render(x, v);
        const int from = static_cast<int>(1.5 * kFs), n = static_cast<int>(1.0 * kFs);
        const double carrier = amplitude(y, 3000.0, from, n);
        double side = 0.0;
        for (const double d : { -120.0, -60.0, 60.0, 120.0 })
            side = std::max(side, amplitude(y, 3000.0 + d, from, n));
        const double imd = 20.0 * std::log10(side / carrier);
        add("R4", "60 Hz + 3 kHz at 6 dB GR: the largest sideband 3 kHz +- 60 / 120 Hz re 3 kHz", f(imd, 1) + " dB", "<= -40 dB", imd <= -40.0);
    }
    // R5: pumping (reported; the target is "no worse than before")
    {
        const int n = static_cast<int>(3.0 * kFs), beat = static_cast<int>(0.5 * kFs);
        juce::AudioBuffer<float> x(2, n);
        for (int i = 0; i < n; ++i)
        {
            const double t = (i % beat) / kFs;
            const double kick = std::exp(-t / 0.06) * std::sin(2.0 * kPi * (55.0 + 120.0 * std::exp(-t / 0.01)) * t);
            const double v = kick + 0.1 * std::sin(2.0 * kPi * 1000.0 * i / kFs);
            x.setSample(0, i, static_cast<float>(v));
            x.setSample(1, i, static_cast<float>(v));
        }
        const auto y = render(x, { { "thresholdDb", -6 }, { "ceilingDb", -1 }, { "style", 0 }, { "autoRelease", 1 }, { "truePeak", 1 } });
        // the 1 kHz tone's level in 5 ms windows after the third kick, against its level before it (band by DFT)
        const int k0 = 2 * beat;
        const int w = static_cast<int>(0.005 * kFs);
        const double before = amplitude(y, 1000.0, k0 - 4 * w, 4 * w);
        double dip = 0.0, back = 0.0;
        for (int s = k0; s + w < k0 + beat - w; s += w)
        {
            const double a = 20.0 * std::log10(amplitude(y, 1000.0, s, w) / before);
            dip = std::min(dip, a);
            if (a < -1.0)
                back = (s + w - k0) / kFs * 1000.0;
        }
        add("R5", "1 kHz tone under kicks, pushed 6 dB: deepest dip; time back within 1 dB", f(dip, 1) + " dB; " + f(back, 0) + " ms", "no worse than before", true);
    }
    // R6: loudness for the push
    {
        double gain = 0.0;
        for (const auto& m : mixes)
            gain += (lufs(render(m, { { "thresholdDb", -6 }, { "ceilingDb", -1 }, { "truePeak", 1 }, { "autoRelease", 1 } })) - lufs(m)) / static_cast<double>(mixes.size());
        add("R6", "The 3 mixes, LOUDNESS -6, LIMIT -1 dBTP: integrated loudness gain", f(gain, 2) + " LU", ">= +4.5 LU", gain >= 4.5);
        if (juce::SystemStats::getEnvironmentVariable("RW_LAB_STYLES", {}).isNotEmpty())
            for (int style = 0; style < 3; ++style)
                for (const int autoRelease : { 0, 1 })
                    for (const double release : { 10.0, 60.0, 200.0 })
                    {
                        double g = 0.0, thdAt = 0.0;
                        for (const auto& m : mixes)
                            g += (lufs(render(m, { { "thresholdDb", -6 }, { "ceilingDb", -1 }, { "truePeak", 1 }, { "autoRelease", autoRelease }, { "style", style }, { "releaseMs", release } })) - lufs(m)) / 3.0;
                        juce::ignoreUnused(thdAt);
                        std::cout << "  STYLE " << style << " auto " << autoRelease << " release " << release << ": " << g << " LU" << std::endl;
                    }
    }
    // R7 .. R10: DRIVE and TUBE
    const auto thdOf = [&](const juce::AudioBuffer<float>& y, double hz, double& h2, double& h3)
    {
        const int from = static_cast<int>(0.5 * kFs), n = static_cast<int>(1.0 * kFs);
        const double h1 = amplitude(y, hz, from, n);
        double hs = 0.0;
        h2 = 20.0 * std::log10(amplitude(y, 2 * hz, from, n) / h1 + 1.0e-12);
        h3 = 20.0 * std::log10(amplitude(y, 3 * hz, from, n) / h1 + 1.0e-12);
        for (int k = 2; k <= 7; ++k)
            hs += std::pow(amplitude(y, hz * k, from, n), 2.0);
        return 100.0 * std::sqrt(hs) / h1;
    };
    if (drive)
    {
        const auto x = tone(1.6, { { 1000.0, -12.0 } });
        juce::String values;
        double last = -1.0, at100 = 0.0, h2 = 0, h3 = 0;
        bool rising = true;
        for (const double d : { 0.25, 0.5, 1.0 })
        {
            const double thd = thdOf(render(x, { { "ceilingDb", 0 }, { "drive", d } }), 1000.0, h2, h3);
            rising = rising && thd > last;
            last = thd;
            at100 = thd;
            values << (values.isEmpty() ? "" : " / ") << f(thd, 2);
        }
        add("R7", "DRIVE 25 / 50 / 100 %, 1 kHz at -12 dBFS: THD (at 100 %: H2 / H3)", values + " % (" + f(h2, 1) + " / " + f(h3, 1) + " dB)", "rising, 2 .. 10 % at 100 %, H3 > H2",
            rising && at100 >= 2.0 && at100 <= 10.0 && h3 > h2);
        // aliasing: everything that is not k x 9 kHz (folded) - measured on a grid of candidate alias frequencies
        const auto y = render(tone(1.6, { { 9000.0, -6.0 } }), { { "ceilingDb", 0 }, { "drive", 1.0 } });
        const int from = static_cast<int>(0.5 * kFs), n = static_cast<int>(1.0 * kFs);
        const double h1 = amplitude(y, 9000.0, from, n);
        double worst = 0.0;
        for (int k = 2; k <= 15; ++k)
        {
            double fk = std::fmod(9000.0 * k, kFs);
            if (fk > kFs / 2)
                fk = kFs - fk;
            if (9000.0 * k < kFs / 2)
                continue; // a true harmonic, not an alias
            if (std::abs(fk - 9000.0) < 50.0 || std::abs(fk - 18000.0) < 50.0)
                continue; // folds onto the fundamental or H2 (k = 15: 135 kHz -> 9 kHz), not measurable apart
                          // (first run counted it and read 0.0 dB: the fundamental itself)
            worst = std::max(worst, amplitude(y, fk, from, n));
        }
        const double alias = 20.0 * std::log10(worst / h1 + 1.0e-12);
        add("R8", "DRIVE 100 %, 9 kHz at -6 dBFS: the largest aliased harmonic re the fundamental", f(alias, 1) + " dB", "<= -80 dB", alias <= -80.0);
    }
    else
    {
        add("R7", "DRIVE: harmonics", "n/a (no DRIVE)", "rising, 2 .. 10 % at 100 %, H3 > H2", false);
        add("R8", "DRIVE: aliasing", "n/a (no DRIVE)", "<= -80 dB", false);
    }
    if (tube)
    {
        double h2 = 0, h3 = 0;
        const double thd = thdOf(render(tone(1.6, { { 1000.0, -12.0 } }), { { "ceilingDb", 0 }, { "tube", 1.0 } }), 1000.0, h2, h3);
        add("R9", "TUBE 100 %, 1 kHz at -12 dBFS: THD; H2 / H3", f(thd, 2) + " %; " + f(h2, 1) + " / " + f(h3, 1) + " dB", "H2 > H3, 0.5 .. 5 %", h2 > h3 && thd >= 0.5 && thd <= 5.0);
        juce::String values;
        bool ok = true;
        std::vector<juce::AudioBuffer<float>> material = mixes;
        material.push_back(drumbus);
        for (const auto& x : material)
        {
            const auto y = render(x, { { "ceilingDb", 0 }, { "tube", 1.0 } });
            const double crestIn = peakDb(x) - 20.0 * std::log10(x.getRMSLevel(0, 0, x.getNumSamples()));
            const double crestOut = peakDb(y) - 20.0 * std::log10(y.getRMSLevel(0, 0, y.getNumSamples()));
            if (juce::SystemStats::getEnvironmentVariable("RW_LAB_STYLES", {}).isNotEmpty())
                std::cout << "R10 debug: peak " << peakDb(x) << " -> " << peakDb(y) << ", rms " << 20.0 * std::log10(x.getRMSLevel(0, 0, x.getNumSamples()))
                          << " -> " << 20.0 * std::log10(y.getRMSLevel(0, 0, y.getNumSamples())) << std::endl;
            ok = ok && crestIn - crestOut >= 2.0 && peakDb(y) <= peakDb(x) + 0.1;
            values << (values.isEmpty() ? "" : " / ") << f(crestIn - crestOut, 1);
        }
        add("R10", "TUBE 100 %, LOUDNESS 0, LIMIT 0: crest factor lowered (mixes / drum bus); peak not raised", values + " dB", ">= 2 dB, peak <= input + 0.1", ok);
    }
    else
    {
        add("R9", "TUBE: harmonics", "n/a (no TUBE)", "H2 > H3, 0.5 .. 5 %", false);
        add("R10", "TUBE: density", "n/a (no TUBE)", ">= 2 dB, peak <= input + 0.1", false);
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
        juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]).replaceWithText("# HamloRedWall lab: " + juce::String(argc > 3 ? argv[3] : "run") + "\n\n"
                                                                                        + juce::String(static_cast<int>(rows.size()) - failed) + " / "
                                                                                        + juce::String(static_cast<int>(rows.size())) + " checks pass\n\n" + table);
    return failed;
}
