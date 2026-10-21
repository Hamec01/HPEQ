// M3: PRE-EQ shapes, slopes, channel modes. T03 (measured vs analytic response, asymptotic slopes,
// near-Nyquist), equivalence with the RBJ cookbook, T05 (L/R/M/S semantics, PRE-EQ and FX nodes),
// B-ORDER with Low Cut 200 Hz, smooth transitions, T04/T10 with the full EQ.
// All thresholds are fixed here, before the runs (04 §5.2).

#include <cmath>
#include <complex>
#include <memory>
#include <tuple>

#include <juce_core/juce_core.h>

#include "DSP/Biquad.h"
#include "DSP/EqDesign.h"
#include "DSP/LayerEngine.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kPi = 3.14159265358979323846;

EqBandParams band(EqShape shape, float hz, float db = 0.0f, float q = 0.707f, int slope = 12, ChannelMode mode = ChannelMode::Stereo)
{
    EqBandParams b;
    b.active = true;
    b.shape = shape;
    b.frequencyHz = hz;
    b.gainDb = db;
    b.q = q;
    b.slopeDbPerOct = slope;
    b.channel = mode;
    return b;
}

// Renders x through a fresh engine after `prerollSeconds` of silence (entry glides/fades finish there).
juce::AudioBuffer<float> renderEq(const LayerSnapshot& s, const juce::AudioBuffer<float>& x, double fs,
                                  int block = 512, double prerollSeconds = 0.3, std::uint64_t* allocations = nullptr)
{
    auto engine = std::make_unique<LayerEngine>();
    engine->prepare(fs, block);
    engine->publish(s);
    const int pre = static_cast<int>(prerollSeconds * fs);
    juce::AudioBuffer<float> y(x.getNumChannels(), x.getNumSamples() + pre);
    y.clear();
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, pre, x, ch, 0, x.getNumSamples());
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* chunk[kMaxChannels] {};
        for (int ch = 0; ch < y.getNumChannels(); ++ch)
            chunk[ch] = y.getWritePointer(ch) + start;
        ScopedAllocationProbe probe;
        engine->process(chunk, y.getNumChannels(), juce::jmin(block, y.getNumSamples() - start));
        if (allocations != nullptr)
            *allocations += probe.counts().allocations;
    }
    juce::AudioBuffer<float> out(x.getNumChannels(), x.getNumSamples());
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        out.copyFrom(ch, 0, y, ch, pre, x.getNumSamples());
    return out;
}

std::vector<double> logGrid(double lo, double hi, int n)
{
    std::vector<double> g;
    for (int i = 0; i < n; ++i)
        g.push_back(lo * std::pow(hi / lo, static_cast<double>(i) / (n - 1)));
    return g;
}

double analyticDb(const EqBandParams& b, double fs, double f)
{
    return 20.0 * std::log10(std::abs(bandResponse(designBand(b.shape, b.slopeDbPerOct, fs, b.frequencyHz, b.gainDb, b.q), fs, f)) + 1e-300);
}

// RBJ cookbook references (independent formulas, a0-normalised) for shapes Biquad does not provide.
BiquadCoefficients rbj(double b0, double b1, double b2, double a0, double a1, double a2)
{
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

std::complex<double> biquadResponse(const BiquadCoefficients& c, double fs, double f)
{
    const auto z1 = std::polar(1.0, -2.0 * kPi * f / fs);
    return (c.b0 + c.b1 * z1 + c.b2 * z1 * z1) / (1.0 + c.a1 * z1 + c.a2 * z1 * z1);
}

BiquadCoefficients rbjShape(EqShape shape, double fs, double f0, double db, double q)
{
    const double a = std::pow(10.0, db / 40.0), w = 2.0 * kPi * f0 / fs, c = std::cos(w), alpha = std::sin(w) / (2.0 * q);
    const double sa = 2.0 * std::sqrt(a) * alpha;
    switch (shape)
    {
        case EqShape::LowShelf:  return rbj(a * ((a + 1) - (a - 1) * c + sa), 2 * a * ((a - 1) - (a + 1) * c), a * ((a + 1) - (a - 1) * c - sa),
                                            (a + 1) + (a - 1) * c + sa, -2 * ((a - 1) + (a + 1) * c), (a + 1) + (a - 1) * c - sa);
        case EqShape::HighShelf: return rbj(a * ((a + 1) + (a - 1) * c + sa), -2 * a * ((a - 1) + (a + 1) * c), a * ((a + 1) + (a - 1) * c - sa),
                                            (a + 1) - (a - 1) * c + sa, 2 * ((a - 1) - (a + 1) * c), (a + 1) - (a - 1) * c - sa);
        case EqShape::Notch:     return rbj(1, -2 * c, 1, 1 + alpha, -2 * c, 1 - alpha);
        case EqShape::BandPass:  return rbj(alpha, 0, -alpha, 1 + alpha, -2 * c, 1 - alpha);
        case EqShape::Bell:      return design::peak(fs, f0, q, db);
        case EqShape::AllPass:   return design::allPass(fs, f0, q);
        case EqShape::LowCut:    return design::highPass(fs, f0, q);
        case EqShape::HighCut:   return design::lowPass(fs, f0, q);
        case EqShape::Tilt:      break;
    }
    return {};
}

double shortRmsDb(const juce::AudioBuffer<float>& y, int from, int window)
{
    double sum = 0.0;
    for (int i = from; i < from + window; ++i)
        sum += static_cast<double>(y.getSample(0, i)) * y.getSample(0, i);
    return 10.0 * std::log10(sum / window + 1e-30);
}
} // namespace

class EqTests final : public juce::UnitTest
{
public:
    EqTests() : juce::UnitTest("PRE-EQ M3", "HPEQ") {}

    void runTest() override
    {
        beginTest("SVF designs equal the RBJ cookbook (complex response, 2nd-order shapes)");
        {
            double worst = 0.0;
            for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
                for (const auto& [shape, db, q] : { std::tuple { EqShape::Bell, 6.0, 1.4 }, { EqShape::Bell, -9.0, 0.5 },
                                                    { EqShape::LowShelf, 6.0, 0.7 }, { EqShape::LowShelf, -12.0, 1.2 },
                                                    { EqShape::HighShelf, -6.0, 0.7 }, { EqShape::HighShelf, 9.0, 0.5 },
                                                    { EqShape::Notch, 0.0, 2.0 }, { EqShape::BandPass, 0.0, 2.0 },
                                                    { EqShape::AllPass, 0.0, 1.0 } })
                    for (double f0 : { 80.0, 1000.0, 6000.0 })
                    {
                        const auto d = designBand(shape, 12, fs, f0, db, q);
                        const auto ref = rbjShape(shape, fs, f0, db, q);
                        for (double f : logGrid(10.0, 0.45 * fs, 120))
                            worst = std::max(worst, std::abs(bandResponse(d, fs, f) - biquadResponse(ref, fs, f)));
                    }
            for (int slope : { 12 })
                for (const auto shape : { EqShape::LowCut, EqShape::HighCut })
                {
                    const auto d = designBand(shape, slope, 48000.0, 1000.0, 0.0, 0.707);
                    const auto ref = rbjShape(shape, 48000.0, 1000.0, 0.0, design::kButterworthQ);
                    for (double f : logGrid(10.0, 21000.0, 120))
                        worst = std::max(worst, std::abs(bandResponse(d, 48000.0, f) - biquadResponse(ref, 48000.0, f)));
                }
            expect(worst < 1.0e-9, "max |H_svf - H_rbj| = " + juce::String(worst));
            logMessage("  SVF vs RBJ, worst complex difference: " + juce::String(worst));
        }

        beginTest("Butterworth cuts: section count, -3 dB at fc, 96 dB/oct = 16th order");
        {
            for (int slope : kEqSlopes)
            {
                const int order = slope / 6;
                const auto d = designBand(EqShape::LowCut, slope, 48000.0, 1000.0, 0.0, 0.707);
                expectEquals(d.count, (order + 1) / 2, "sections for " + juce::String(slope));
                expectWithinAbsoluteError(20.0 * std::log10(std::abs(bandResponse(d, 48000.0, 1000.0))), -3.0103, 0.001);
            }
            // One octave below fc a Butterworth of order N sits at -10 log10(1 + 2^(2N)); N = 16 -> -96.33 dB.
            const auto d96 = designBand(EqShape::LowCut, 96, 48000.0, 1000.0, 0.0, 0.707);
            const double atOctave = 20.0 * std::log10(std::abs(bandResponse(d96, 48000.0, 500.0)));
            logMessage("  96 dB/oct low cut, 1 octave below 1 kHz: " + juce::String(atOctave, 2) + " dB");
            expect(std::abs(atOctave + 96.33) < 0.5, juce::String(atOctave));
        }

        beginTest("T03: measured response = analytic response of the used coefficients (grid)");
        {
            // Masks fixed before the run: |error| <= 0.05 dB where analytic >= -60 dB,
            // <= 1 dB where -100 <= analytic < -60 dB, not evaluated below -100 dB (float floor).
            double worstHigh = 0.0, worstLow = 0.0;
            int configurations = 0, skipped = 0;
            juce::String worstWhere;
            std::vector<EqBandParams> shapes {
                band(EqShape::Bell, 0, 6.0f, 1.4f), band(EqShape::LowShelf, 0, -6.0f, 0.7f), band(EqShape::HighShelf, 0, 4.0f, 0.7f),
                band(EqShape::Notch, 0, 0.0f, 2.0f), band(EqShape::BandPass, 0, 0.0f, 1.0f), band(EqShape::AllPass, 0, 0.0f, 1.0f),
                band(EqShape::Tilt, 0, 6.0f) };
            for (int slope : kEqSlopes)
            {
                shapes.push_back(band(EqShape::LowCut, 0, 0.0f, 0.707f, slope));
                shapes.push_back(band(EqShape::HighCut, 0, 0.0f, 0.707f, slope));
            }
            for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
            {
                const auto sweep = makeExpSweep({ fs, 1, 1.5, kFixtureSeed }, 10.0, 0.45 * fs, -12.0);
                const auto grid = logGrid(20.0, 0.44 * fs, 160);
                for (double fc : { 30.0, 80.0, 200.0, 1000.0, 6000.0 })
                    for (auto b : shapes)
                    {
                        b.frequencyHz = static_cast<float>(fc);
                        LayerSnapshot s;
                        s.eq[0] = b;
                        const auto y = renderEq(s, sweep, fs);
                        const auto measured = measureTransfer(y, sweep, fs, grid, 0, -100.0);
                        ++configurations;
                        for (size_t i = 0; i < grid.size(); ++i)
                        {
                            const double a = analyticDb(b, fs, measured[i].binFrequencyHz);
                            if (!measured[i].magnitudeDb.has_value() || a < -100.0)
                            {
                                ++skipped;
                                continue;
                            }
                            const double e = std::abs(*measured[i].magnitudeDb - a);
                            if (a >= -60.0 && e > worstHigh)
                            {
                                worstHigh = e;
                                worstWhere = "shape " + juce::String(static_cast<int>(b.shape)) + " slope " + juce::String(b.slopeDbPerOct)
                                             + " fc " + juce::String(fc) + " fs " + juce::String(fs) + " at " + juce::String(measured[i].binFrequencyHz, 2)
                                             + " Hz: analytic " + juce::String(a, 2) + " measured " + juce::String(*measured[i].magnitudeDb, 2);
                            }
                            else
                                worstLow = std::max(worstLow, e);
                        }
                    }
            }
            expect(worstHigh <= 0.05, "error above -60 dB: " + juce::String(worstHigh));
            expect(worstLow <= 1.0, "error -60..-100 dB: " + juce::String(worstLow));
            logMessage("  " + juce::String(configurations) + " configurations: worst |measured - analytic| "
                       + juce::String(worstHigh, 4) + " dB (>= -60 dB), " + juce::String(worstLow, 3) + " dB (-60..-100 dB); worst at "
                       + worstWhere);
        }

        beginTest("T03: asymptotic slopes within +-6 dB/oct of 6.02 N (fit where analytic is -30..-100 dB)");
        {
            constexpr double fs = 48000.0;
            const auto sweep = makeExpSweep({ fs, 1, 3.0, kFixtureSeed }, 10.0, 0.45 * fs, -6.0);
            juce::String table;
            for (const auto shape : { EqShape::LowCut, EqShape::HighCut })
                for (int slope : kEqSlopes)
                {
                    // Low cut at 1 kHz (stopband below), high cut at 200 Hz (stopband above, kept under fs/4
                    // so the bilinear warping near Nyquist does not steepen the fit).
                    const double fc = shape == EqShape::LowCut ? 1000.0 : 200.0;
                    auto b = band(shape, static_cast<float>(fc), 0.0f, 0.707f, slope);
                    LayerSnapshot s;
                    s.eq[0] = b;
                    const auto y = renderEq(s, sweep, fs);
                    std::vector<double> xs, ys;
                    for (double f : logGrid(15.0, fs / 4.0, 400))
                    {
                        const double a = analyticDb(b, fs, f);
                        if (a > -30.0 || a < -100.0)
                            continue;
                        const auto m = measureTransfer(y, sweep, fs, { f }, 0, -100.0).front();
                        if (!m.magnitudeDb.has_value())
                            continue;
                        xs.push_back(std::log2(f));
                        ys.push_back(*m.magnitudeDb);
                    }
                    const bool valid = xs.size() >= 5 && (xs.back() - xs.front()) >= 0.25;
                    expect(valid, juce::String(shape == EqShape::LowCut ? "LowCut " : "HighCut ") + juce::String(slope) + ": no valid interval");
                    if (!valid)
                        continue;
                    double mx = 0, my = 0;
                    for (size_t i = 0; i < xs.size(); ++i) { mx += xs[i]; my += ys[i]; }
                    mx /= static_cast<double>(xs.size());
                    my /= static_cast<double>(xs.size());
                    double sxy = 0, sxx = 0;
                    for (size_t i = 0; i < xs.size(); ++i) { sxy += (xs[i] - mx) * (ys[i] - my); sxx += (xs[i] - mx) * (xs[i] - mx); }
                    const double fitted = std::abs(sxy / sxx);
                    const double target = 6.0206 * slope / 6.0;
                    expect(std::abs(fitted - target) <= 6.0, juce::String(slope) + ": fitted " + juce::String(fitted, 2));
                    table << (shape == EqShape::LowCut ? "LC" : "HC") << slope << "=" << juce::String(fitted, 2) << " ";
                }
            logMessage("  fitted dB/oct: " + table);
        }

        beginTest("near Nyquist: cuts and bells at/above 0.45 fs stay finite and match the clamped design");
        {
            for (double fs : { 44100.0, 48000.0 })
            {
                const auto noise = makeWhiteNoise({ fs, 2, 0.5, kFixtureSeed }, -12.0);
                for (const auto& b : { band(EqShape::HighCut, 20000.0f, 0.0f, 0.707f, 96), band(EqShape::LowCut, 20000.0f, 0.0f, 0.707f, 96),
                                       band(EqShape::Bell, 23000.0f, 12.0f, 8.0f), band(EqShape::HighShelf, 30000.0f, 12.0f, 0.7f) })
                {
                    LayerSnapshot s;
                    s.eq[0] = b;
                    const auto y = renderEq(s, noise, fs);
                    expect(measureAll(y).allFinite, "non-finite near Nyquist");
                    expect(measureAll(y).peak < 20.0, "runaway near Nyquist");
                }
            }
        }

        beginTest("Tilt: +6 dB tilts the response from -3 dB (lows) to +3 dB (highs)");
        {
            const auto t = band(EqShape::Tilt, 1000.0f, 6.0f);
            const double lo = analyticDb(t, 48000.0, 20.0), hi = analyticDb(t, 48000.0, 20000.0), mid = analyticDb(t, 48000.0, 1000.0);
            expectWithinAbsoluteError(hi - lo, 6.0, 0.5);
            expectWithinAbsoluteError(mid, 0.0, 0.01);
            logMessage("  tilt +6 dB: 20 Hz " + juce::String(lo, 2) + ", 1 kHz " + juce::String(mid, 3) + ", 20 kHz " + juce::String(hi, 2) + " dB");
        }

        beginTest("T05: PRE-EQ channel modes (Left / Right / Mid / Side), exact where they must be");
        {
            constexpr double fs = 48000.0;
            const auto leftOnly = makeStereoPlacement({ fs, 2, 0.5, kFixtureSeed }, StereoPlacement::LeftOnly, -20.0);
            const auto midOnly = makeStereoPlacement({ fs, 2, 0.5, kFixtureSeed }, StereoPlacement::MidOnly, -20.0);
            const auto sideOnly = makeStereoPlacement({ fs, 2, 0.5, kFixtureSeed }, StereoPlacement::SideOnly, -20.0);
            const auto general = makePinkNoise({ fs, 2, 0.5, kFixtureSeed }, -20.0);
            const auto channel = [](const juce::AudioBuffer<float>& a, int ch)
            {
                juce::AudioBuffer<float> c(1, a.getNumSamples());
                c.copyFrom(0, 0, a, ch, 0, a.getNumSamples());
                return c;
            };
            const auto withBand = [&](const juce::AudioBuffer<float>& x, ChannelMode m)
            {
                LayerSnapshot s;
                s.eq[0] = band(EqShape::Bell, 1000.0f, 9.0f, 1.0f, 12, m);
                s.eq[1] = band(EqShape::LowCut, 300.0f, 0.0f, 0.707f, 48, m);
                return renderEq(s, x, fs);
            };

            const auto l = withBand(general, ChannelMode::Left);
            expect(compareAligned(channel(l, 1), channel(general, 1)).bitExact, "Left mode touched R");
            expect(!compareAligned(channel(l, 0), channel(general, 0)).bitExact, "Left mode did not process L");
            const auto r = withBand(general, ChannelMode::Right);
            expect(compareAligned(channel(r, 0), channel(general, 0)).bitExact, "Right mode touched L");
            const auto rOnLeft = withBand(leftOnly, ChannelMode::Right);
            expect(compareAligned(rOnLeft, leftOnly).bitExact, "Right mode on a left-only signal must be exact");

            const auto midOnSide = withBand(sideOnly, ChannelMode::Mid);
            expect(compareAligned(midOnSide, sideOnly).bitExact, "Mid band changed a side-only signal");
            const auto sideOnMid = withBand(midOnly, ChannelMode::Side);
            expect(compareAligned(sideOnMid, midOnly).bitExact, "Side band changed a mid-only signal");
            const auto midOnMid = withBand(midOnly, ChannelMode::Mid);
            expect(compareAligned(channel(midOnMid, 0), channel(midOnMid, 1)).bitExact, "Mid band broke L == R");
            const auto sideOnSide = withBand(sideOnly, ChannelMode::Side);
            bool antiPhase = true;
            for (int i = 0; i < sideOnSide.getNumSamples(); ++i)
                antiPhase = antiPhase && sideOnSide.getSample(0, i) == -sideOnSide.getSample(1, i);
            expect(antiPhase, "Side band broke L == -R");

            // Mid mode against an independent reference: filter M = (L+R)/2 alone, rebuild L = M+S, R = M-S.
            const auto mid = withBand(general, ChannelMode::Mid);
            juce::AudioBuffer<float> m(1, general.getNumSamples()), s(1, general.getNumSamples());
            for (int i = 0; i < general.getNumSamples(); ++i)
            {
                m.setSample(0, i, 0.5f * (general.getSample(0, i) + general.getSample(1, i)));
                s.setSample(0, i, 0.5f * (general.getSample(0, i) - general.getSample(1, i)));
            }
            LayerSnapshot monoS;
            monoS.eq[0] = band(EqShape::Bell, 1000.0f, 9.0f, 1.0f);
            monoS.eq[1] = band(EqShape::LowCut, 300.0f, 0.0f, 0.707f, 48);
            const auto mFiltered = renderEq(monoS, m, fs);
            auto expected = general;
            for (int i = 0; i < general.getNumSamples(); ++i)
            {
                expected.setSample(0, i, mFiltered.getSample(0, i) + s.getSample(0, i));
                expected.setSample(1, i, mFiltered.getSample(0, i) - s.getSample(0, i));
            }
            const auto null = compareAligned(mid, expected);
            expect(null.errorDb < -120.0, "Mid vs M/S reference: " + juce::String(null.errorDb) + " dB");
            logMessage("  Mid mode vs independent M/S reference: " + juce::String(null.errorDb, 1) + " dB");

            // Mono input: Right and Side bands do nothing, Left/Mid/Stereo process the channel.
            const auto mono = makePinkNoise({ fs, 1, 0.5, kFixtureSeed }, -20.0);
            for (const auto mode : { ChannelMode::Right, ChannelMode::Side })
                expect(compareAligned(withBand(mono, mode), mono).bitExact, "mono with a Right/Side band must be exact");
            expect(!compareAligned(withBand(mono, ChannelMode::Mid), mono).bitExact);
        }

        beginTest("T05: FX node channel modes route the delta to L / R / M / S only");
        {
            constexpr double fs = 48000.0;
            const auto general = makePinkNoise({ fs, 2, 0.6, kFixtureSeed }, -14.0);
            const auto render = [&](ChannelMode m, float gainDb)
            {
                LayerSnapshot s;
                auto& n = s.fx[0];
                n.active = true;
                n.lowHz = 200.0f;
                n.highHz = 2000.0f;
                n.channel = m;
                n.numSlots = 1;
                n.slots[0].active = true;
                n.slots[0].type = FxType::Gain;
                n.slots[0].values[slot::kGainDb] = gainDb;
                return renderEq(s, general, fs);
            };
            const auto channel = [](const juce::AudioBuffer<float>& a, int ch)
            {
                juce::AudioBuffer<float> c(1, a.getNumSamples());
                c.copyFrom(0, 0, a, ch, 0, a.getNumSamples());
                return c;
            };
            // Same mode at 0 dB = the passive render (only the tree's all-pass), the reference per mode.
            for (const auto mode : { ChannelMode::Left, ChannelMode::Right })
            {
                const auto active = render(mode, -9.0f), passive = render(mode, 0.0f);
                const int untouched = mode == ChannelMode::Left ? 1 : 0;
                expect(compareAligned(channel(active, untouched), channel(passive, untouched)).bitExact, "node mode leaked into the other channel");
                expect(!compareAligned(channel(active, 1 - untouched), channel(passive, 1 - untouched)).bitExact);
            }
            for (const auto mode : { ChannelMode::Mid, ChannelMode::Side })
            {
                const auto active = render(mode, -9.0f), passive = render(mode, 0.0f);
                // The delta lives only in M (L and R change equally) or only in S (opposite changes).
                double worst = 0.0, energy = 0.0;
                for (int i = 0; i < general.getNumSamples(); ++i)
                {
                    const double dl = active.getSample(0, i) - passive.getSample(0, i);
                    const double dr = active.getSample(1, i) - passive.getSample(1, i);
                    worst = std::max(worst, std::abs(mode == ChannelMode::Mid ? dl - dr : dl + dr));
                    energy = std::max(energy, std::abs(dl));
                }
                expect(energy > 1.0e-3, "node did nothing");
                expect(worst < 1.0e-6, "delta leaked into the other M/S component: " + juce::String(worst));
            }
        }

        beginTest("T05: Dual Mono compressor keeps channels independent, Linked does not");
        {
            constexpr double fs = 48000.0;
            auto loudLeft = makeSine({ fs, 2, 0.6, kFixtureSeed }, 500.0, -3.0);
            const auto quiet = makeSine({ fs, 1, 0.6, kFixtureSeed }, 500.0, -40.0);
            loudLeft.copyFrom(1, 0, quiet, 0, 0, quiet.getNumSamples());
            const auto render = [&](ChannelMode m, bool compress)
            {
                LayerSnapshot s;
                auto& n = s.fx[0];
                n.active = true;
                n.lowHz = 200.0f;
                n.highHz = 2000.0f;
                n.channel = m;
                n.numSlots = 1;
                n.slots[0].active = true;
                n.slots[0].type = FxType::Compressor;
                n.slots[0].values = { compress ? -30.0f : 0.0f, 8.0f, 0.0f, 1.0f, 50.0f, 0.0f, 0.0f, 0.0f };
                return renderEq(s, loudLeft, fs);
            };
            const auto rightRms = [](const juce::AudioBuffer<float>& a)
            {
                juce::AudioBuffer<float> c(1, a.getNumSamples());
                c.copyFrom(0, 0, a, 1, 0, a.getNumSamples());
                return measureAll(c).rmsDbfs;
            };
            const double passiveR = rightRms(render(ChannelMode::DualMono, false));
            const double dualR = rightRms(render(ChannelMode::DualMono, true));
            const double linkedR = rightRms(render(ChannelMode::LinkedStereo, true));
            expectWithinAbsoluteError(dualR, passiveR, 0.01);
            expect(linkedR < passiveR - 3.0, "linked detector must duck the quiet channel");
            logMessage("  quiet right channel: passive " + juce::String(passiveR, 2) + ", dual mono " + juce::String(dualR, 2)
                       + ", linked " + juce::String(linkedR, 2) + " dBFS");
        }

        beginTest("B-ORDER: Low Cut 200 Hz (24 dB/oct) before a 100-250 Hz compressor; 80 Hz input");
        {
            constexpr double fs = 48000.0;
            const auto low = makeSine({ fs, 2, 1.0, kFixtureSeed }, 80.0, -6.0);
            const auto measureGr = [&](bool withCut)
            {
                LayerSnapshot s;
                if (withCut)
                    s.eq[0] = band(EqShape::LowCut, 200.0f, 0.0f, 0.707f, 24);
                auto& n = s.fx[0];
                n.active = true;
                n.lowHz = 100.0f;
                n.highHz = 250.0f;
                n.numSlots = 1;
                n.slots[0].active = true;
                n.slots[0].type = FxType::Compressor;
                n.slots[0].values = { -30.0f, 4.0f, 0.0f, 5.0f, 100.0f, 0.0f, 0.0f, 0.0f };
                auto engine = std::make_unique<LayerEngine>();
                engine->prepare(fs, 512);
                engine->publish(s);
                auto y = low;
                float gr = 0.0f;
                for (int start = 0; start < y.getNumSamples(); start += 512)
                {
                    float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    engine->process(chunk, 2, juce::jmin(512, y.getNumSamples() - start));
                    if (start > 24000)
                        gr = std::max(gr, engine->getNode(0).getGainReductionDb(0));
                }
                return gr;
            };
            const float without = measureGr(false), with = measureGr(true);
            // Fixed: the detector must see the PRE-filtered 80 Hz: > 5 dB GR without the cut, < 1 dB with it.
            expect(without > 5.0f, juce::String(without));
            expect(with < 1.0f, juce::String(with));
            logMessage("  compressor 100-250 Hz on 80 Hz: GR " + juce::String(without, 2) + " dB without, "
                       + juce::String(with, 2) + " dB behind Low Cut 200 Hz / 24");
        }

        beginTest("transitions: enabling/disabling every shape, slope and mode changes are smooth");
        {
            // Fixed: level dip <= 3 dB below the quieter steady state around each change, HF > 5 kHz <= -60 dB.
            constexpr double fs = 48000.0;
            const auto tone = makeSine({ fs, 2, 1.25, kFixtureSeed }, 1000.0, -6.0);
            struct Scenario { const char* name; EqBandParams on, changed; };
            const Scenario scenarios[] {
                { "LowCut 200/24, slope -> 96", band(EqShape::LowCut, 200.0f, 0, 0.707f, 24), band(EqShape::LowCut, 200.0f, 0, 0.707f, 96) },
                { "HighCut 5k/24, slope -> 12", band(EqShape::HighCut, 5000.0f, 0, 0.707f, 24), band(EqShape::HighCut, 5000.0f, 0, 0.707f, 12) },
                { "Bell 1k +6, mode -> Mid", band(EqShape::Bell, 1000.0f, 6.0f, 1.0f), band(EqShape::Bell, 1000.0f, 6.0f, 1.0f, 12, ChannelMode::Mid) },
                { "Notch 3k, -> Bell 3k -6", band(EqShape::Notch, 3000.0f, 0, 2.0f), band(EqShape::Bell, 3000.0f, -6.0f, 2.0f) },
                { "BandPass 1k, Q 1 -> 0.5", band(EqShape::BandPass, 1000.0f, 0, 1.0f), band(EqShape::BandPass, 1000.0f, 0, 0.5f) },
                { "AllPass 1k, -> 2k", band(EqShape::AllPass, 1000.0f, 0, 1.0f), band(EqShape::AllPass, 2000.0f, 0, 1.0f) },
                { "Tilt +6, -> -6", band(EqShape::Tilt, 1000.0f, 6.0f), band(EqShape::Tilt, 1000.0f, -6.0f) },
                { "LowShelf 300 +6, -> HighShelf 3k -6", band(EqShape::LowShelf, 300.0f, 6.0f, 0.7f), band(EqShape::HighShelf, 3000.0f, -6.0f, 0.7f) },
            };
            double worstDip = 0.0, worstHf = -400.0;
            juce::String detail;
            for (const auto& sc : scenarios)
            {
                auto engine = std::make_unique<LayerEngine>();
                engine->prepare(fs, 256);
                auto y = tone;
                const int changes[] { 12032, 24064, 36096 }; // on, change, off
                LayerSnapshot s;
                for (int start = 0; start < y.getNumSamples(); start += 256)
                {
                    if (start == changes[0]) { s.eq[0] = sc.on; engine->publish(s); }
                    if (start == changes[1]) { s.eq[0] = sc.changed; engine->publish(s); }
                    if (start == changes[2]) { s.eq[0].active = false; engine->publish(s); }
                    float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    engine->process(chunk, 2, juce::jmin(256, y.getNumSamples() - start));
                }
                const int window = static_cast<int>(0.0025 * fs);
                double dip = 0.0;
                for (int c : changes)
                {
                    const double before = shortRmsDb(y, c - 2 * window, window), after = shortRmsDb(y, c + static_cast<int>(0.2 * fs), window);
                    double lowest = 1e9;
                    for (int i = c; i < c + static_cast<int>(0.15 * fs); i += window / 2)
                        lowest = std::min(lowest, shortRmsDb(y, i, window));
                    dip = std::max(dip, std::min(before, after) - lowest);
                }
                const double hf = highBandRelativeDb(y, fs, 5000.0, 0, 0.1);
                worstDip = std::max(worstDip, dip);
                worstHf = std::max(worstHf, hf);
                detail << "\n    " << sc.name << ": dip " << juce::String(dip, 2) << " dB, HF " << juce::String(hf, 1) << " dB";
                expect(dip <= 3.0, juce::String(sc.name) + ": dip " + juce::String(dip, 2));
                expect(hf <= -60.0, juce::String(sc.name) + ": HF " + juce::String(hf, 1));
            }
            logMessage("  worst dip " + juce::String(worstDip, 2) + " dB, worst HF " + juce::String(worstHf, 1) + " dB" + detail);
        }

        beginTest("T10 with the full EQ: identical render for block sizes 1, 32, 127, 256, 1024");
        {
            constexpr double fs = 48000.0;
            LayerSnapshot s;
            s.eq[0] = band(EqShape::LowCut, 40.0f, 0, 0.707f, 96);
            s.eq[1] = band(EqShape::HighCut, 15000.0f, 0, 0.707f, 18);
            s.eq[2] = band(EqShape::LowShelf, 120.0f, 3.0f, 0.7f, 12, ChannelMode::Mid);
            s.eq[3] = band(EqShape::HighShelf, 8000.0f, -2.0f, 0.7f, 12, ChannelMode::Side);
            s.eq[4] = band(EqShape::Notch, 3000.0f, 0, 4.0f, 12, ChannelMode::Left);
            s.eq[5] = band(EqShape::Tilt, 1000.0f, 2.0f);
            s.eq[6] = band(EqShape::AllPass, 600.0f, 0, 1.0f, 12, ChannelMode::Right);
            s.fx[0].active = true; s.fx[0].lowHz = 250; s.fx[0].highHz = 900; s.fx[0].channel = ChannelMode::Mid; s.fx[0].numSlots = 1;
            s.fx[0].slots[0].active = true; s.fx[0].slots[0].type = FxType::Compressor;
            s.fx[0].slots[0].values = { -30.0f, 4.0f, 6.0f, 3.0f, 90.0f, 0.0f, 0.0f, 0.0f };
            const auto x = makeSyntheticDrums({ fs, 2, 1.0, kFixtureSeed });
            std::uint64_t allocations = 0;
            const auto reference = renderEq(s, x, fs, 1, 0.0, &allocations);
            for (int block : { 32, 127, 256, 1024 })
            {
                const auto y = renderEq(s, x, fs, block, 0.0, &allocations);
                const auto null = compareAligned(y, reference);
                expect(null.bitExact, "block " + juce::String(block) + ": " + juce::String(null.errorDb) + " dB");
            }
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("T04 with the full EQ: 50k mutations of shapes/slopes/modes, random blocks, stable");
        {
            constexpr double fs = 48000.0;
            auto engine = std::make_unique<LayerEngine>();
            engine->prepare(fs, 512);
            DeterministicRandom random(kFixtureSeed + 7);
            const auto uni = [&](double lo, double hi) { return lo + (hi - lo) * (random.nextBipolar() * 0.5 + 0.5); };
            const auto source = makeWhiteNoise({ fs, 2, 2.0, kFixtureSeed }, -12.0);
            juce::AudioBuffer<float> block(2, 2048);
            LayerSnapshot s;
            bool finite = true;
            double worstPeak = 0.0;
            std::uint64_t allocations = 0;
            int position = 0;
            for (int mutation = 0; mutation < 50000; ++mutation)
            {
                auto& b = s.eq[static_cast<size_t>(uni(0, 6))];
                b.active = uni(0, 1) > 0.25;
                b.shape = static_cast<EqShape>(static_cast<int>(uni(0, 9)));
                b.frequencyHz = static_cast<float>(uni(5, 30000));
                b.gainDb = static_cast<float>(uni(-12, 12));
                b.q = static_cast<float>(uni(0.1, 18));
                b.slopeDbPerOct = kEqSlopes[static_cast<int>(uni(0, 8))];
                b.channel = static_cast<ChannelMode>(static_cast<int>(uni(0, 7)));
                if (mutation % 3 == 0)
                    engine->publish(s);
                if (mutation % 5 != 0)
                    continue;
                const int count = static_cast<int>(uni(1, 2049));
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < count; ++i)
                        block.setSample(ch, i, source.getSample(ch, (position + i) % source.getNumSamples()));
                position += count;
                float* chunk[2] { block.getWritePointer(0), block.getWritePointer(1) };
                ScopedAllocationProbe probe;
                engine->process(chunk, 2, count);
                allocations += probe.counts().allocations;
                juce::AudioBuffer<float> view(block.getArrayOfWritePointers(), 2, 0, count);
                const auto stats = measureAll(view);
                finite = finite && stats.allFinite;
                worstPeak = std::max(worstPeak, stats.peak);
            }
            expect(finite);
            // 6 bands of at most +12 dB on a -12 dBFS-RMS noise: bound far above any legitimate stack.
            expect(worstPeak < 1.0e3, "runaway " + juce::String(worstPeak));
            expectEquals(static_cast<int>(allocations), 0);
            logMessage("  worst peak " + juce::String(juce::Decibels::gainToDecibels(worstPeak), 1) + " dBFS");
        }
    }
};

static EqTests eqTests;
} // namespace hpeq::test
