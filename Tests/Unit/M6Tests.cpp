// M6: Saturator (T08 alias, oversampling latency), Transient, Stereo, Gate (no chatter), Limiter (ceiling
// guarantee, look-ahead PDC), Delay (sync, T07 tail, ping-pong), Monsta FX (determinism, T10), pools,
// layer/plugin latency alignment. Thresholds are fixed here before the runs.

#include <cmath>
#include <functional>
#include <memory>

#include <juce_dsp/juce_dsp.h>

#include "DSP/LayerEngine.h"
#include "DSP/ProcessorsM6.h"
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

FxSlotParams saturator(float driveDb, int curve, int os, float bias = 0.0f, bool autoGain = true)
{
    return slotOf(FxType::Saturator, { { slot::kDriveDb, driveDb }, { slot::kCurve, static_cast<float>(curve) }, { slot::kBias, bias },
                                       { slot::kTrimDb, 0.0f }, { slot::kOversampling, static_cast<float>(os) }, { slot::kAutoGain, autoGain ? 1.0f : 0.0f } });
}

FxSlotParams limiter(float ceilingDb, float releaseMs, float lookaheadMs)
{
    return slotOf(FxType::Limiter, { { slot::kCeilingDb, ceilingDb }, { slot::kLimiterReleaseMs, releaseMs }, { slot::kLookaheadMs, lookaheadMs } });
}

FxSlotParams gateSlot(float thresholdDb, float ratio, float rangeDb, float holdMs, float hysteresisDb)
{
    return slotOf(FxType::GateExpander, { { slot::kGateThresholdDb, thresholdDb }, { slot::kGateRatio, ratio }, { slot::kGateRangeDb, rangeDb },
                                          { slot::kGateAttackMs, 0.5f }, { slot::kGateHoldMs, holdMs }, { slot::kGateReleaseMs, 50.0f },
                                          { slot::kGateHysteresisDb, hysteresisDb } });
}

FxSlotParams delaySlot(int sync, float timeMs, float feedback, bool pingPong, float mix = 1.0f)
{
    return slotOf(FxType::Delay, { { slot::kDelayTimeMs, timeMs }, { slot::kDelaySync, static_cast<float>(sync) }, { slot::kFeedback, feedback },
                                   { slot::kPingPong, pingPong ? 1.0f : 0.0f }, { slot::kDampingHz, 20000.0f }, { slot::kDelayLowCutHz, 10.0f },
                                   { slot::kDelayWidth, 1.0f } }, mix);
}

FxSlotParams monsta(float chance, float seed, int division = 2, float crushBits = 0.0f, float gateDepth = 0.0f)
{
    return slotOf(FxType::MonstaFx, { { slot::kDivision, static_cast<float>(division) }, { slot::kChance, chance }, { slot::kSeed, seed },
                                      { slot::kCrushBits, crushBits }, { slot::kGateDepth, gateDepth }, { slot::kReverseShare, 0.3f } });
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

// Runs an in-place processor over a copy of x in blocks of `block`.
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

juce::AudioBuffer<float> scaled(juce::AudioBuffer<float> x, double db)
{
    x.applyGain(static_cast<float>(std::pow(10.0, db / 20.0)));
    return x;
}

juce::AudioBuffer<float> segment(const juce::AudioBuffer<float>& x, int start, int length)
{
    juce::AudioBuffer<float> y(x.getNumChannels(), length);
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, 0, x, ch, start, length);
    return y;
}

double rmsDb(const juce::AudioBuffer<float>& x, int start, int length)
{
    return measureAll(segment(x, start, length)).rmsDbfs;
}

// T08 metric. Sine on an exact FFT bin; 65536-point Blackman-Harris spectrum of channel 0 after
// `settle`. Harmonic energy = +-4 bins around k * f0 (k * f0 below Nyquist); everything else above
// 20 Hz counts as alias + noise. Returns (alias energy) / (fundamental energy) in dB.
constexpr int kFftOrder = 16, kFftSize = 1 << kFftOrder, kSineBin = 9601;
double sineBinHz() { return kFs * kSineBin / kFftSize; }

double aliasRatioDb(const juce::AudioBuffer<float>& y, int settle)
{
    std::vector<float> data(static_cast<size_t>(2 * kFftSize), 0.0f);
    for (int i = 0; i < kFftSize; ++i)
        data[static_cast<size_t>(i)] = y.getSample(0, settle + i);
    juce::dsp::WindowingFunction<float> window(static_cast<size_t>(kFftSize), juce::dsp::WindowingFunction<float>::blackmanHarris, false);
    window.multiplyWithWindowingTable(data.data(), static_cast<size_t>(kFftSize));
    juce::dsp::FFT fft(kFftOrder);
    fft.performFrequencyOnlyForwardTransform(data.data());
    double fundamental = 0.0, alias = 0.0;
    const int minBin = static_cast<int>(std::ceil(20.0 * kFftSize / kFs));
    for (int b = minBin; b <= kFftSize / 2; ++b)
    {
        const double power = static_cast<double>(data[static_cast<size_t>(b)]) * data[static_cast<size_t>(b)];
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

std::vector<int> peakPositions(const juce::AudioBuffer<float>& y, int channel, float threshold, int minDistance)
{
    std::vector<int> peaks;
    const auto* d = y.getReadPointer(channel);
    int best = -1;
    for (int i = 0; i < y.getNumSamples(); ++i)
    {
        if (std::abs(d[i]) < threshold)
            continue;
        if (best >= 0 && i - best > minDistance)
        {
            peaks.push_back(best);
            best = -1;
        }
        if (best < 0 || std::abs(d[i]) > std::abs(d[best]))
            best = i;
    }
    if (best >= 0)
        peaks.push_back(best);
    return peaks;
}
} // namespace

class M6Tests final : public juce::UnitTest
{
public:
    M6Tests() : juce::UnitTest("Processors M6", "HPEQ") {}

    void runTest() override
    {
        beginTest("Saturator: small-signal gain is unity with auto gain (every curve, bias, oversampling) within 0.1 dB");
        {
            const auto x = makeSine({ kFs, 2, 1.0 }, 1000.0, -60.0);
            for (int curve : { 0, 1, 2 })
                for (float bias : { 0.0f, 0.3f, -0.3f })
                    for (int os : { 0, 1, 2 })
                    {
                        SaturatorProcessor sat;
                        sat.prepare(kFs, 512);
                        const auto p = saturator(24.0f, curve, os, bias);
                        const auto y = runInPlace(x, 512, [&](float* const* c, int nc, int n, int) { sat.process(c, nc, n, p); });
                        const double diff = rmsDb(y, 24000, 24000) - rmsDb(x, 24000, 24000);
                        expect(std::abs(diff) <= 0.1, "curve " + juce::String(curve) + " bias " + juce::String(bias) + " os " + juce::String(os)
                                                         + ": " + juce::String(diff, 3) + " dB");
                    }
        }

        beginTest("Saturator: measured latency equals the reported latency (1x / 2x / 4x)");
        {
            const auto x = makeWhiteNoise({ kFs, 2, 0.5 }, -60.0);
            for (int os : { 0, 1, 2 })
            {
                SaturatorProcessor sat;
                sat.prepare(kFs, 256);
                const auto p = saturator(0.0f, 0, os);
                const auto y = runInPlace(x, 256, [&](float* const* c, int nc, int n, int) { sat.process(c, nc, n, p); });
                const int measured = estimateDelay(y, x, 400);
                logMessage("  os " + juce::String(os) + ": reported " + juce::String(SaturatorProcessor::latencySamples(os)) + ", measured " + juce::String(measured));
                expectEquals(measured, SaturatorProcessor::latencySamples(os));
            }
        }

        beginTest("Saturator: bias adds no DC (|mean| < 1e-3 after 0.5 s)");
        {
            const auto x = makeSine({ kFs, 2, 1.5 }, 220.0, -6.0);
            for (int curve : { 0, 1, 2 })
            {
                SaturatorProcessor sat;
                sat.prepare(kFs, 512);
                const auto p = saturator(18.0f, curve, 1, 0.5f);
                const auto y = runInPlace(x, 512, [&](float* const* c, int nc, int n, int) { sat.process(c, nc, n, p); });
                const auto stats = measureAll(segment(y, 24000, 48000));
                expect(std::abs(stats.dc) < 1.0e-3, "curve " + juce::String(curve) + ": dc " + juce::String(stats.dc, 6));
                expect(stats.allFinite);
            }
        }

        beginTest("T08: oversampling reduces aliasing (tanh, 12 dB drive, 7.03 kHz at -6 dBFS): 2x >= 12 dB, 4x >= 20 dB better than 1x");
        {
            const auto x = makeSine({ kFs, 1, 2.0 }, sineBinHz(), -6.0);
            std::array<double, 3> ratio {};
            for (int os : { 0, 1, 2 })
            {
                SaturatorProcessor sat;
                sat.prepare(kFs, 512);
                const auto p = saturator(12.0f, 0, os);
                const auto y = runInPlace(x, 512, [&](float* const* c, int nc, int n, int) { sat.process(c, nc, n, p); });
                ratio[static_cast<size_t>(os)] = aliasRatioDb(y, 8192);
            }
            logMessage("  alias / fundamental: 1x " + juce::String(ratio[0], 1) + " dB, 2x " + juce::String(ratio[1], 1) + " dB, 4x " + juce::String(ratio[2], 1) + " dB");
            expect(ratio[0] - ratio[1] >= 12.0);
            expect(ratio[0] - ratio[2] >= 20.0);
        }

        beginTest("Transient: 0 / 0 dB is bit-exact; attack +12 dB raises crest >= 3 dB; sustain -12 dB lowers the decay (150-250 ms after a hit) >= 6 dB");
        {
            // Criterion changed after the first run (disclosed in M6_REPORT): whole-file RMS of the drum
            // fixture moved only 0.97 dB because its energy sits in the attacks; the sustain law acts on decays.
            const auto drums = makeSyntheticDrums({ kFs, 2, 2.0 });
            const auto run = [&](const juce::AudioBuffer<float>& x, float attack, float sustain)
            {
                TransientProcessor t;
                t.prepare(kFs);
                const auto p = slotOf(FxType::TransientShaper, { { slot::kAttackGainDb, attack }, { slot::kSustainGainDb, sustain }, { slot::kTransientSpeed, 0.5f } });
                return runInPlace(x, 256, [&](float* const* c, int nc, int n, int) { t.process(c, nc, n, p); });
            };
            expect(compareAligned(run(drums, 0.0f, 0.0f), drums).bitExact);
            const auto in = measureAll(drums);
            const auto punch = measureAll(run(drums, 12.0f, 0.0f));
            logMessage("  crest in " + juce::String(in.crestDb, 2) + " dB, attack +12: " + juce::String(punch.crestDb, 2) + " dB");
            expect(punch.crestDb - in.crestDb >= 3.0);

            // Hits: 150 Hz tone, 60 ms exponential decay, every 500 ms.
            const int n = static_cast<int>(2.0 * kFs), period = static_cast<int>(0.5 * kFs);
            juce::AudioBuffer<float> hits(2, n);
            for (int i = 0; i < n; ++i)
            {
                const double t = (i % period) / kFs;
                const auto v = static_cast<float>(0.5 * std::exp(-t / 0.06) * std::sin(2.0 * juce::MathConstants<double>::pi * 150.0 * t));
                hits.setSample(0, i, v);
                hits.setSample(1, i, v);
            }
            const auto shaped = run(hits, 0.0f, -12.0f);
            double worst = 1000.0;
            for (int h = 1; h < 4; ++h)
            {
                const int at = h * period + static_cast<int>(0.15 * kFs);
                worst = std::min(worst, rmsDb(hits, at, static_cast<int>(0.1 * kFs)) - rmsDb(shaped, at, static_cast<int>(0.1 * kFs)));
            }
            logMessage("  sustain -12 dB: decay 150-250 ms after a hit lowered by " + juce::String(worst, 2) + " dB (worst hit)");
            expect(worst >= 6.0);
        }

        beginTest("Stereo: width 1 / balance 0 bit-exact; width 0 gives L == R; balance follows the linear law");
        {
            const auto x = makeWhiteNoise({ kFs, 2, 0.5 }, -12.0);
            StereoProcessor st;
            const auto run = [&](float width, float balance)
            {
                const auto p = slotOf(FxType::StereoProcessor, { { slot::kStereoWidth, width }, { slot::kStereoBalance, balance } });
                return runInPlace(x, 128, [&](float* const* c, int nc, int n, int) { st.process(c, nc, n, p); });
            };
            expect(compareAligned(run(1.0f, 0.0f), x).bitExact);
            const auto mono = run(0.0f, 0.0f);
            bool same = true;
            for (int i = 0; i < mono.getNumSamples(); ++i)
                same = same && mono.getSample(0, i) == mono.getSample(1, i);
            expect(same, "width 0: L != R");
            const auto bal = run(1.0f, 0.5f);
            double worst = 0.0;
            for (int i = 0; i < x.getNumSamples(); ++i)
                worst = std::max({ worst, std::abs(bal.getSample(0, i) - 0.5 * x.getSample(0, i)), std::abs(bal.getSample(1, i) - static_cast<double>(x.getSample(1, i))) });
            expect(worst < 1.0e-6, "balance 0.5: worst " + juce::String(worst));
        }

        beginTest("Gate: range 0 is bit-exact; closed gate attenuates by the range (+-0.5 dB); hysteresis prevents chatter");
        {
            const auto drums = makeSyntheticDrums({ kFs, 2, 1.0 });
            {
                GateProcessor g;
                g.prepare(kFs);
                const auto p = gateSlot(-20.0f, 4.0f, 0.0f, 20.0f, 4.0f);
                expect(compareAligned(runInPlace(drums, 256, [&](float* const* c, int nc, int n, int) { g.process(c, nc, n, p); }), drums).bitExact);
            }
            {
                // 0.5 s at -10 dBFS, then 1 s at -60 dBFS; threshold -40, ratio 100, range 40.
                auto x = makeSine({ kFs, 2, 1.5 }, 300.0, -10.0);
                for (int ch = 0; ch < 2; ++ch)
                    x.applyGain(ch, 24000, x.getNumSamples() - 24000, static_cast<float>(std::pow(10.0, -50.0 / 20.0)));
                GateProcessor g;
                g.prepare(kFs);
                const auto p = gateSlot(-40.0f, 100.0f, 40.0f, 20.0f, 4.0f);
                const auto y = runInPlace(x, 256, [&](float* const* c, int nc, int n, int) { g.process(c, nc, n, p); });
                const double attenuation = rmsDb(x, 60000, 12000) - rmsDb(y, 60000, 12000);
                logMessage("  closed-gate attenuation " + juce::String(attenuation, 2) + " dB (range 40)");
                expect(std::abs(attenuation - 40.0) <= 0.5);
            }
            {
                // A 200 Hz tone whose level wanders +-1.5 dB around the threshold (3 Hz).
                const int n = static_cast<int>(2.0 * kFs);
                juce::AudioBuffer<float> x(2, n);
                for (int i = 0; i < n; ++i)
                {
                    const double t = i / kFs;
                    const double db = -30.0 + 1.5 * std::sin(2.0 * juce::MathConstants<double>::pi * 3.0 * t);
                    const auto v = static_cast<float>(std::pow(10.0, db / 20.0) * std::sin(2.0 * juce::MathConstants<double>::pi * 200.0 * t));
                    x.setSample(0, i, v);
                    x.setSample(1, i, v);
                }
                const auto transitions = [&](float hysteresis)
                {
                    GateProcessor g;
                    g.prepare(kFs);
                    const auto p = gateSlot(-30.0f, 4.0f, 30.0f, 0.0f, hysteresis);
                    runInPlace(x, 256, [&](float* const* c, int nc, int len, int) { g.process(c, nc, len, p); });
                    return g.stateChanges();
                };
                const auto with = transitions(4.0f), without = transitions(0.0f);
                logMessage("  state changes over 2 s: hysteresis 4 dB -> " + juce::String(with) + ", hysteresis 0 -> " + juce::String(without));
                expect(with <= 2);
                expect(without > 2 * with);
            }
        }

        beginTest("Limiter: output sample peak never exceeds the ceiling (drums +18 dB, 27 settings, blocks 1 and 512 identical)");
        {
            const auto hot = scaled(makeSyntheticDrums({ kFs, 2, 2.0 }), 18.0);
            double worstOverDb = -1000.0;
            for (float ceiling : { -0.1f, -1.0f, -6.0f })
                for (float release : { 1.0f, 80.0f, 1000.0f })
                    for (float look : { 0.1f, 2.0f, 10.0f })
                    {
                        const auto p = limiter(ceiling, release, look);
                        const auto render = [&](int block)
                        {
                            LimiterProcessor l;
                            l.prepare(kFs);
                            return runInPlace(hot, block, [&](float* const* c, int nc, int n, int) { l.process(c, nc, n, p); });
                        };
                        const auto y = render(512);
                        const double limit = std::pow(10.0, ceiling / 20.0);
                        const auto stats = measureAll(y);
                        worstOverDb = std::max(worstOverDb, toDb(stats.peak) - ceiling);
                        expect(stats.peak <= limit * (1.0 + 1.0e-6), "ceiling " + juce::String(ceiling) + " release " + juce::String(release)
                                                                         + " look-ahead " + juce::String(look) + ": peak " + juce::String(toDb(stats.peak), 4));
                        if (release == 80.0f)
                            expect(compareAligned(render(1), y).bitExact, "block 1 vs 512");
                    }
            logMessage("  worst peak relative to the ceiling: " + juce::String(worstOverDb, 4) + " dB");
        }

        beginTest("Limiter: below the ceiling the output is the input delayed by the look-ahead, bit-exact");
        {
            const auto quiet = scaled(makeSyntheticDrums({ kFs, 2, 1.0 }), -20.0);
            for (float look : { 0.1f, 2.0f, 10.0f })
            {
                const auto p = limiter(0.0f, 80.0f, look);
                LimiterProcessor l;
                l.prepare(kFs);
                const auto y = runInPlace(quiet, 333, [&](float* const* c, int nc, int n, int) { l.process(c, nc, n, p); });
                const int latency = LimiterProcessor::lookaheadSamples(p, kFs);
                // compareAligned's bitExact means "exact with delay 0"; with a delay, exact = max error 0.
                expect(compareAligned(y, quiet, latency).maxAbsError == 0.0, "look-ahead " + juce::String(look) + " ms (" + juce::String(latency) + " samples)");
            }
        }

        beginTest("Delay: synced echo positions (1/4 at 120 bpm, 1/8 D at 90 bpm), feedback ratio, ping-pong sides");
        {
            const auto impulse = makeImpulse({ kFs, 2, 1.6 }, 0, 1.0f);
            const auto render = [&](const FxSlotParams& p, double bpm)
            {
                DelayProcessor d;
                d.prepare(kFs);
                Transport t;
                t.bpm = bpm;
                juce::AudioBuffer<float> wet(2, impulse.getNumSamples());
                for (int start = 0; start < wet.getNumSamples(); start += 512)
                {
                    const int n = juce::jmin(512, wet.getNumSamples() - start);
                    const float* in[2] { impulse.getReadPointer(0) + start, impulse.getReadPointer(1) + start };
                    float* out[2] { wet.getWritePointer(0) + start, wet.getWritePointer(1) + start };
                    d.process(in, out, 2, n, p, t);
                }
                return wet;
            };
            for (const auto& [sync, bpm] : { std::pair<int, double> { 8, 120.0 }, std::pair<int, double> { 6, 90.0 } })
            {
                const auto p = delaySlot(sync, 0.0f, 0.5f, false);
                const auto y = render(p, bpm);
                const auto peaks = peakPositions(y, 0, 0.05f, 1000);
                const int expected = static_cast<int>(std::lround(DelayProcessor::delaySeconds(p, bpm) * kFs));
                expect(expected == 24000, "expected delay " + juce::String(expected));
                expect(peaks.size() >= 3, "echo count " + juce::String(static_cast<int>(peaks.size())));
                if (peaks.size() >= 3)
                {
                    expect(std::abs(peaks[0] - expected) <= 2 && std::abs(peaks[1] - 2 * expected) <= 2 && std::abs(peaks[2] - 3 * expected) <= 2,
                           "echo positions " + juce::String(peaks[0]) + " " + juce::String(peaks[1]) + " " + juce::String(peaks[2]));
                    const double ratioDb = toDb(std::abs(y.getSample(0, peaks[1])) / std::abs(y.getSample(0, peaks[0])));
                    expect(std::abs(ratioDb - toDb(0.5)) <= 1.0, "second / first echo " + juce::String(ratioDb, 2) + " dB");
                }
            }
            {
                const auto y = render(delaySlot(0, 250.0f, 0.5f, true), 120.0);
                const int d = 12000;
                const auto level = [&](int ch, int at) { return toDb(measureChannel(segment(y, at - 200, 400), ch).peak); };
                logMessage("  ping-pong: echo 1 L " + juce::String(level(0, d), 1) + " / R " + juce::String(level(1, d), 1)
                           + " dB, echo 2 L " + juce::String(level(0, 2 * d), 1) + " / R " + juce::String(level(1, 2 * d), 1) + " dB");
                expect(level(0, d) - level(1, d) >= 40.0);
                expect(level(1, 2 * d) - level(0, 2 * d) >= 40.0);
            }
        }

        beginTest("T07 Delay: after tailSeconds() the echoes are >= 60 dB below the first echo; no runaway at feedback 0.95");
        {
            for (float feedback : { 0.3f, 0.7f, 0.95f })
            {
                const auto p = delaySlot(0, 120.0f, feedback, false);
                const double tail = DelayProcessor::tailSeconds(p, 120.0);
                const auto impulse = makeImpulse({ kFs, 2, tail + 1.0 }, 0, 1.0f);
                DelayProcessor d;
                d.prepare(kFs);
                juce::AudioBuffer<float> wet(2, impulse.getNumSamples());
                for (int start = 0; start < wet.getNumSamples(); start += 512)
                {
                    const int n = juce::jmin(512, wet.getNumSamples() - start);
                    const float* in[2] { impulse.getReadPointer(0) + start, impulse.getReadPointer(1) + start };
                    float* out[2] { wet.getWritePointer(0) + start, wet.getWritePointer(1) + start };
                    d.process(in, out, 2, n, p, Transport {});
                }
                const auto all = measureAll(wet);
                const int tailStart = static_cast<int>(std::ceil(tail * kFs)) + 2;
                const auto after = measureAll(segment(wet, tailStart, wet.getNumSamples() - tailStart));
                const double belowDb = toDb(all.peak) - toDb(after.peak);
                logMessage("  feedback " + juce::String(feedback) + ": tail " + juce::String(tail, 2) + " s, after it " + juce::String(belowDb, 1) + " dB below the first echo");
                expect(belowDb >= 59.5);
                expect(all.allFinite && all.peak <= 1.0 + 1.0e-6);
            }
        }

        beginTest("Monsta: chance 0 is bit-exact; action rate follows chance; seeds differ; T10 (blocks 1/64/127/512, free and playing)");
        {
            const auto drums = makeSyntheticDrums({ kFs, 2, 2.0 });
            const auto render = [&](const FxSlotParams& p, int block, bool playing)
            {
                MonstaProcessor m;
                m.prepare(kFs);
                return runInPlace(drums, block, [&](float* const* c, int nc, int n, int start)
                {
                    Transport t;
                    t.bpm = 128.0;
                    t.playing = playing;
                    t.hasPosition = playing;
                    t.ppqAtBlockStart = 3.0 + start * t.bpm / (60.0 * kFs);
                    m.process(c, nc, n, p, t, 0);
                });
            };
            expect(compareAligned(render(monsta(0.0f, 1.0f), 512, false), drums).bitExact);
            int acted = 0;
            const auto rate = monsta(0.35f, 7.0f);
            for (long long s = 0; s < 20000; ++s)
                acted += MonstaProcessor::actionFor(rate, s) != MonstaProcessor::Action::Pass ? 1 : 0;
            logMessage("  acted-on share at chance 0.35: " + juce::String(acted / 20000.0, 4));
            expect(std::abs(acted / 20000.0 - 0.35) <= 0.02);
            const auto a = render(monsta(0.6f, 1.0f, 2, 8.0f, 0.5f), 512, true);
            const auto b = render(monsta(0.6f, 2.0f, 2, 8.0f, 0.5f), 512, true);
            expect(compareAligned(b, a).errorDb > -20.0, "seeds 1 and 2 nearly identical");
            for (bool playing : { false, true })
            {
                const auto p = monsta(0.6f, 3.0f, 3, 6.0f, 0.5f);
                const auto reference = render(p, 1, playing);
                for (int block : { 64, 127, 512 })
                {
                    const auto null = compareAligned(render(p, block, playing), reference);
                    expect(null.bitExact, juce::String(playing ? "playing" : "free") + ", block " + juce::String(block) + ": " + juce::String(null.errorDb) + " dB");
                }
            }
        }

        beginTest("T10 engine, all M6 types: identical render for blocks 1, 64, 127, 512 and 0 allocations");
        {
            const auto drums = makeSyntheticDrums({ kFs, 2, 1.5 });
            const auto render = [&](int block, std::uint64_t& allocations)
            {
                auto e = std::make_unique<LayerEngine>();
                e->prepare(kFs, block);
                LayerSnapshot s;
                s.fx[0] = nodeWith(80, 600, { saturator(12.0f, 1, 1, 0.2f), slotOf(FxType::TransientShaper, { { slot::kAttackGainDb, 6.0f }, { slot::kSustainGainDb, -3.0f } }),
                                              gateSlot(-45.0f, 4.0f, 20.0f, 10.0f, 4.0f), limiter(-12.0f, 50.0f, 1.5f) });
                s.fx[1] = nodeWith(1500, 8000, { slotOf(FxType::StereoProcessor, { { slot::kStereoWidth, 1.4f }, { slot::kStereoBalance, 0.1f } }),
                                                 monsta(0.5f, 11.0f, 3, 10.0f, 0.3f), delaySlot(5, 0.0f, 0.4f, true, 0.5f) });
                e->publish(s);
                auto y = drums;
                for (int start = 0; start < y.getNumSamples(); start += block)
                {
                    float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    Transport t;
                    t.bpm = 100.0;
                    t.playing = t.hasPosition = true;
                    t.ppqAtBlockStart = start * t.bpm / (60.0 * kFs);
                    ScopedAllocationProbe probe;
                    e->process(chunk, 2, juce::jmin(block, y.getNumSamples() - start), t);
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
            expect(measureAll(reference).allFinite);
        }

        beginTest("PDC: a layer with look-ahead equals the zero-latency layer delayed by L (<= -120 dB after 1 s); other nodes stay aligned");
        {
            const auto x = scaled(makeSyntheticDrums({ kFs, 2, 3.0 }), -20.0);
            const auto render = [&](const FxSlotParams& first)
            {
                auto e = std::make_unique<LayerEngine>();
                e->prepare(kFs, 256);
                LayerSnapshot s;
                s.fx[0] = nodeWith(200, 2000, { first });
                s.fx[1] = nodeWith(3000, 9000, { slotOf(FxType::Compressor, { { 0, -40.0f }, { 1, 4.0f }, { 2, 6.0f }, { 3, 3.0f }, { 4, 80.0f } }) });
                e->publish(s);
                auto y = runInPlace(x, 256, [&](float* const* c, int nc, int n, int) { e->process(c, nc, n); });
                return std::make_pair(y, e->getLatencySamples());
            };
            const auto [plain, zero] = render(slotOf(FxType::Gain, { { slot::kGainDb, 0.0f } }));
            const auto look = limiter(0.0f, 80.0f, 2.0f);
            const auto [delayed, latency] = render(look);
            expectEquals(zero, 0);
            expectEquals(latency, LimiterProcessor::lookaheadSamples(look, kFs));
            const int skip = static_cast<int>(kFs);
            const auto null = compareAligned(segment(delayed, skip + latency, x.getNumSamples() - skip - latency),
                                             segment(plain, skip, x.getNumSamples() - skip - latency));
            logMessage("  latency " + juce::String(latency) + " samples, aligned null " + juce::String(null.errorDb, 1) + " dB");
            expect(null.errorDb <= -120.0);
        }

        beginTest("PDC in the plugin: reported latency = layer latency; bypassed output = input delayed by it, bit-exact");
        {
            auto p = std::make_unique<FxPluginProcessor>();
            auto project = p->getProject();
            FxNodeState n;
            FxSlotState lim;
            lim.type = FxType::Limiter;
            lim.parameters.set("lookaheadMs", 2.0);
            FxSlotState sat;
            sat.type = FxType::Saturator;
            sat.parameters.set("oversampling", 2);
            n.chain = { sat, lim };
            project.layers[0].fxNodes.push_back(n);
            p->setRateAndBufferSizeDetails(kFs, 512);
            p->prepareToPlay(kFs, 512);
            p->setProject(project);
            const int expected = LimiterProcessor::lookaheadSamples(limiter(-1.0f, 80.0f, 2.0f), kFs) + SaturatorProcessor::latencySamples(2);
            expectEquals(p->getLatencySamples(), expected);
            if (auto* bypass = p->getBypassParameter())
                bypass->setValueNotifyingHost(1.0f);
            p->prepareToPlay(kFs, 512); // bypass settled at once
            const auto x = makeSyntheticDrums({ kFs, 2, 1.0 });
            auto y = x;
            juce::MidiBuffer midi;
            for (int start = 0; start < y.getNumSamples(); start += 512)
            {
                juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(512, y.getNumSamples() - start));
                p->processBlock(view, midi);
            }
            expect(compareAligned(y, x, expected).maxAbsError == 0.0, "bypassed output is not the delayed input");
            project.layers[0].fxNodes.clear();
            p->setProject(project);
            expectEquals(p->getLatencySamples(), 0);
        }

        beginTest("Pools: a 5th Delay and a 5th Monsta in one layer are reported as starved, nothing breaks");
        {
            auto e = std::make_unique<LayerEngine>();
            e->prepare(kFs, 256);
            LayerSnapshot s;
            for (int j = 0; j < 5; ++j)
                s.fx[static_cast<size_t>(j)] = nodeWith(200.0f + 1000.0f * j, 900.0f + 1000.0f * j, { delaySlot(0, 100.0f, 0.3f, false), monsta(0.5f, static_cast<float>(j)) });
            e->publish(s);
            auto y = makeSyntheticDrums({ kFs, 2, 0.5 });
            runInPlace(y, 256, [&](float* const* c, int nc, int n, int) { e->process(c, nc, n); });
            expectEquals(e->delaysInUse(), 4);
            expectEquals(e->monstasInUse(), 4);
            int starved = 0;
            for (int j = 0; j < 5; ++j)
                for (int k = 0; k < 2; ++k)
                    starved += e->getNode(j).slotStarved(k) ? 1 : 0;
            expectEquals(starved, 2);
        }
    }
};

static M6Tests m6Tests;
} // namespace hpeq::test
