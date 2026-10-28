// HamloComp_Lab [report.md] [label]
// Measures the compressor against the targets of docs/COMPRESSOR_MODELS.md (T2 .. T14) and prints / writes
// one table: check, settings, measured value, target, pass. The same code makes the "before" run (today's
// compressor, which ignores MODEL / SOURCE) and the "after" run. Exit code = number of checks that fail.

#include <cmath>
#include <functional>
#include <iostream>
#include <vector>

#include <juce_core/juce_core.h>

#include "DSP/Compressor.h"

namespace
{
using hamlo::dsp::CompressorProcessor;
using hamlo::dsp::CompressorSettings;
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

enum Model { Classic, Vca, Fet, Opto, Tube };
enum Source { All, Drums, Samples, Voc };

CompressorSettings settings(int model, float attack, float release, float ratio = 4.0f, float knee = 0.0f, int source = All, float detector = 0.0f)
{
    CompressorSettings s;
    s.thresholdDb = -30.0f;
    s.ratio = ratio;
    s.kneeDb = knee;
    s.attackMs = attack;
    s.releaseMs = release;
    s.makeupDb = 0.0f;
    s.detector = detector;
    s.character = 0.0f; // Clean: the CLASSIC baseline
    s.autoMakeup = 0.0f;
    s.model = static_cast<float>(model);
    s.source = static_cast<float>(source);
    return s;
}

// A signal described per sample: amplitude envelope (linear) times a carrier (+-1 square or a sine).
std::vector<float> make(int n, const std::function<double(int)>& amplitude, double hz, bool square)
{
    std::vector<float> x(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
    {
        const double c = std::sin(2.0 * kPi * hz * i / kFs);
        x[static_cast<size_t>(i)] = static_cast<float>(amplitude(i) * (square ? (c >= 0.0 ? 1.0 : -1.0) : c));
    }
    return x;
}

std::vector<float> run(const CompressorSettings& s, const std::vector<float>& x)
{
    CompressorProcessor c;
    c.prepare(kFs);
    std::vector<float> l = x, r = x;
    for (size_t start = 0; start < x.size(); start += 64)
    {
        float* ch[2] { l.data() + start, r.data() + start };
        c.process(ch, 2, static_cast<int>(std::min<size_t>(64, x.size() - start)), s, true);
    }
    return l;
}

double db(double v) { return 20.0 * std::log10(std::max(v, 1.0e-12)); }
double lin(double d) { return std::pow(10.0, d / 20.0); }

// Gain reduction (dB, >= 0) per sample of a square signal: |x| / |y|.
std::vector<double> grOf(const std::vector<float>& x, const std::vector<float>& y)
{
    std::vector<double> g(x.size());
    for (size_t i = 0; i < x.size(); ++i)
        g[i] = std::abs(x[i]) > 0.0f ? db(std::abs(x[i]) / std::max(1.0e-12f, std::abs(y[i]))) : 0.0;
    return g;
}

double rms(const std::vector<float>& x, size_t from, size_t to)
{
    double s = 0.0;
    for (size_t i = from; i < to; ++i)
        s += static_cast<double>(x[i]) * x[i];
    return std::sqrt(s / static_cast<double>(to - from));
}

// Steady gain (dB, out / in) of a held signal at `levelDb` (peak), over its last 0.5 s.
double steadyGainDb(const CompressorSettings& s, double levelDb, double hz, bool square)
{
    const int n = static_cast<int>(2.0 * kFs);
    const auto x = make(n, [&](int) { return lin(levelDb); }, hz, square);
    const auto y = run(s, x);
    return db(rms(y, x.size() / 2 * 3 / 2, x.size()) / rms(x, x.size() / 2 * 3 / 2, x.size()));
}

// Effective ratio around `overDb` over the threshold: 1 / slope of the static output curve (square signal).
double effectiveRatio(const CompressorSettings& s, double overDb)
{
    const double a = s.thresholdDb + overDb - 1.0, b = s.thresholdDb + overDb + 1.0;
    const double ya = a + steadyGainDb(s, a, 1000.0, true), yb = b + steadyGainDb(s, b, 1000.0, true);
    return 2.0 / std::max(1.0e-9, yb - ya);
}

// A burst at -6 dBFS of `seconds`, then a -50 dBFS probe: returns GR(t) after the burst (dB, per sample)
// and the GR at the burst's end.
std::vector<double> afterBurst(const CompressorSettings& s, double seconds, double& grAtEnd)
{
    const int burst = static_cast<int>(seconds * kFs) + static_cast<int>(0.2 * kFs), n = burst + static_cast<int>(20.0 * kFs);
    const int start = static_cast<int>(0.2 * kFs);
    const auto x = make(n, [&](int i) { return i >= start && i < burst ? lin(-6.0) : lin(-50.0); }, 1000.0, true);
    const auto g = grOf(x, run(s, x));
    grAtEnd = g[static_cast<size_t>(burst - 1)];
    return { g.begin() + burst, g.end() };
}

double timeTo(const std::vector<double>& g, const std::function<bool(double)>& reached)
{
    for (size_t i = 0; i < g.size(); ++i)
        if (reached(g[i]))
            return static_cast<double>(i) / kFs;
    return 1.0e9;
}

// Step from -50 to -10 dBFS: time (s) to reach `fraction` of the final GR (measured 0.5 s after the step).
double attackTime(const CompressorSettings& s, double fraction)
{
    const int step = static_cast<int>(0.2 * kFs), n = step + static_cast<int>(0.6 * kFs);
    const auto x = make(n, [&](int i) { return i >= step ? lin(-10.0) : lin(-50.0); }, 1000.0, true);
    const auto g = grOf(x, run(s, x));
    const double final = g[static_cast<size_t>(step + static_cast<int>(0.5 * kFs))];
    const std::vector<double> after(g.begin() + step, g.end());
    return timeTo(after, [&](double v) { return v >= fraction * final; });
}

struct Harmonics
{
    double levelDb = 0, grDb = 0, h2 = -200, h3 = -200, thdPct = 0;
};

// The 1 kHz sine level that gives ~6 dB steady GR, then H2 / H3 and THD (H2..H5) of the output over 0.5 s.
Harmonics colourAt6dB(const CompressorSettings& s)
{
    double lo = -30.0, hi = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        (-steadyGainDb(s, mid, 1000.0, false) < 6.0 ? lo : hi) = mid;
    }
    Harmonics h;
    h.levelDb = 0.5 * (lo + hi);
    h.grDb = -steadyGainDb(s, h.levelDb, 1000.0, false);
    const int n = static_cast<int>(2.0 * kFs);
    const auto y = run(s, make(n, [&](int) { return lin(h.levelDb); }, 1000.0, false));
    double amp[6] {};
    for (int k = 1; k <= 5; ++k)
    {
        double re = 0, im = 0;
        for (int i = n - 24000; i < n; ++i)
        {
            const double t = 2.0 * kPi * 1000.0 * k * i / kFs;
            re += y[static_cast<size_t>(i)] * std::cos(t);
            im += y[static_cast<size_t>(i)] * std::sin(t);
        }
        amp[k] = std::sqrt(re * re + im * im);
    }
    h.h2 = db(amp[2] / amp[1]);
    h.h3 = db(amp[3] / amp[1]);
    h.thdPct = 100.0 * std::sqrt(amp[2] * amp[2] + amp[3] * amp[3] + amp[4] * amp[4] + amp[5] * amp[5]) / amp[1];
    return h;
}

// Steady GR (dB) of a sine at -10 dBFS and `hz`.
double grAt(const CompressorSettings& s, double hz)
{
    return -steadyGainDb(s, -10.0, hz, false);
}

struct Row
{
    juce::String id, check, measured, target;
    bool pass;
};
} // namespace

int main(int argc, char* argv[])
{
    std::vector<Row> rows;
    const auto add = [&](const char* id, const juce::String& check, const juce::String& measured, const juce::String& target, bool pass)
    {
        rows.push_back({ id, check, measured, target, pass });
        std::cout << id << "  " << check << ": " << measured << "  (target " << target << ")" << (pass ? "" : "  FAIL") << std::endl;
    };
    const auto f = [](double v, int d = 2) { return juce::String(v, d); };

    {
        const auto s = settings(Vca, 5, 100);
        const double gain = steadyGainDb(s, -10.0, 1000.0, true), law = -CompressorProcessor::staticGainReductionDb(-10.0, -30.0, 4.0, 0.0);
        add("T2", "VCA static gain at -10 dBFS (square, -30 dB, 4:1)", f(gain) + " dB (law " + f(law) + ")", "within 0.5 dB", std::abs(gain - law) <= 0.5);
    }
    {
        const auto s = settings(Vca, 1, 100);
        double e1 = 0, e2 = 0;
        const auto shortBurst = afterBurst(s, 0.03, e1), longBurst = afterBurst(s, 3.0, e2);
        const double a = shortBurst[static_cast<size_t>(0.3 * kFs)], b = longBurst[static_cast<size_t>(0.3 * kFs)];
        add("T3", "VCA GR left 300 ms after a 3 s vs a 30 ms burst (release 100 ms)", f(b) + " vs " + f(a) + " dB", "long >= 2 x short", b >= 2.0 * a && b > 0.05);
    }
    {
        const auto h = colourAt6dB(settings(Vca, 10, 200));
        add("T4", "VCA THD at 6 dB GR", f(h.thdPct, 3) + " % (H2 " + f(h.h2, 1) + ", H3 " + f(h.h3, 1) + " dB)", "<= 0.05 %", h.thdPct <= 0.05);
    }
    {
        const double t = attackTime(settings(Fet, 0.1f, 100), 0.9);
        add("T5", "FET attack 0.1 ms: time to 90 % of the GR (+20 dB step)", f(t * 1000.0, 2) + " ms", "<= 2 ms", t <= 0.002);
    }
    {
        const auto h = colourAt6dB(settings(Fet, 10, 200));
        add("T6", "FET THD at 6 dB GR", f(h.thdPct, 3) + " % (H2 " + f(h.h2, 1) + ", H3 " + f(h.h3, 1) + " dB)", "0.3 .. 3 %, H2 and H3 >= -70 dB",
            h.thdPct >= 0.3 && h.thdPct <= 3.0 && h.h2 >= -70.0 && h.h3 >= -70.0);
    }
    {
        const auto s = settings(Opto, 10, 60);
        double e1 = 0, e2 = 0;
        const auto shortBurst = afterBurst(s, 0.03, e1), longBurst = afterBurst(s, 3.0, e2);
        const double half = timeTo(shortBurst, [&](double v) { return v <= 0.5 * e1; });
        const double short90 = timeTo(shortBurst, [&](double v) { return v <= 0.1 * e1; }), long90 = timeTo(longBurst, [&](double v) { return v <= 0.1 * e2; });
        add("T7", "OPTO 30 ms burst: time to recover 50 % (release 60 ms)", f(half * 1000.0, 0) + " ms (GR " + f(e1, 1) + " dB)", "40 .. 120 ms", half >= 0.04 && half <= 0.12);
        add("T8", "OPTO 3 s burst: time to recover 90 %, vs after 30 ms", f(long90, 2) + " s vs " + f(short90, 2) + " s", ">= 1 s and >= 4 x", long90 >= 1.0 && long90 >= 4.0 * short90);
        const double t63 = attackTime(s, 0.63);
        add("T9", "OPTO attack 10 ms: time to 63 % of the GR", f(t63 * 1000.0, 1) + " ms", "5 .. 20 ms", t63 >= 0.005 && t63 <= 0.02);
    }
    {
        const auto s = settings(Tube, 5, 200, 8.0f);
        const double low = effectiveRatio(s, 4.0), high = effectiveRatio(s, 20.0);
        add("T10", "TUBE effective ratio 4 dB vs 20 dB over the threshold (ratio 8)", f(low) + " vs " + f(high), "low <= 0.6 x high", low <= 0.6 * high);
        const auto h = colourAt6dB(settings(Tube, 10, 200, 8.0f));
        add("T11", "TUBE colour at 6 dB GR", f(h.thdPct, 3) + " % (H2 " + f(h.h2, 1) + ", H3 " + f(h.h3, 1) + " dB)", "H2 >= H3 + 6 dB, 0.2 .. 3 %",
            h.h2 >= h.h3 + 6.0 && h.thdPct >= 0.2 && h.thdPct <= 3.0);
    }
    {
        const auto s = settings(Vca, 5, 300, 4.0f, 0.0f, Drums);
        const double low = grAt(s, 50.0), mid = grAt(s, 1000.0);
        add("T12", "DRUMS: GR from 50 Hz vs 1 kHz (-10 dBFS)", f(low, 1) + " vs " + f(mid, 1) + " dB", "50 Hz >= 8 dB less", mid - low >= 8.0);
    }
    {
        const auto s = settings(Vca, 5, 300, 4.0f, 0.0f, Voc);
        const double hi = grAt(s, 3000.0), mid = grAt(s, 1000.0), low = grAt(s, 80.0);
        add("T13", "VOC: GR from 3 kHz / 1 kHz / 80 Hz", f(hi, 1) + " / " + f(mid, 1) + " / " + f(low, 1) + " dB", "3 kHz >= 2 dB more; 80 Hz >= 6 dB less",
            hi - mid >= 2.0 && mid - low >= 6.0);
    }
    {
        const auto s = settings(Vca, 5, 300, 4.0f, 0.0f, Samples);
        const double low = grAt(s, 30.0), mid = grAt(s, 1000.0);
        add("T14", "SAMPLES: GR from 30 Hz vs 1 kHz", f(low, 1) + " vs " + f(mid, 1) + " dB", "30 Hz >= 4 dB less", mid - low >= 4.0);
    }

    int failed = 0;
    juce::String table = "| # | Check | Measured | Target | Pass |\n|---|---|---|---|---|\n";
    for (const auto& r : rows)
    {
        failed += r.pass ? 0 : 1;
        table << "| " << r.id << " | " << r.check << " | " << r.measured << " | " << r.target << " | " << (r.pass ? "yes" : "**no**") << " |\n";
    }
    const juce::String label = argc > 2 ? juce::String(argv[2]) : juce::String("run");
    std::cout << rows.size() - static_cast<size_t>(failed) << " / " << rows.size() << " checks pass" << std::endl;
    if (argc > 1)
        juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]).replaceWithText("# HamloComp lab: " + label + "\n\n"
                                                                                        + juce::String(static_cast<int>(rows.size()) - failed) + " / "
                                                                                        + juce::String(static_cast<int>(rows.size())) + " checks pass\n\n" + table);
    return failed;
}
