// Module pass 3 (owner): HamloEcho — the Delay module after FabFilter Timeless's function set, in the
// HamloComp style. Thresholds fixed here before the runs (docs/fx-addon/DELAY_DESIGN.md §4).

#include <cmath>
#include <memory>

#include <juce_dsp/juce_dsp.h>

#include "DSP/LayerEngine.h"
#include "DSP/ProcessorsM6.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/HamloEchoPanel.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;

// A HamloEcho slot with everything neutral: no feedback, open filters, no drive, no wobble.
FxSlotParams echoParams(int character, float timeMs = 300.0f)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Delay;
    s.values[slot::kDelayTimeMs] = timeMs;
    s.values[slot::kDelaySync] = 0.0f;
    s.values[slot::kFeedback] = 0.0f;
    s.values[slot::kPingPong] = 0.0f;
    s.values[slot::kDampingHz] = 20000.0f;
    s.values[slot::kDelayLowCutHz] = 10.0f;
    s.values[slot::kDelayWidth] = 1.0f;
    s.values[slot::kDlyCharacter] = static_cast<float>(character);
    s.values[slot::kDlyTimeRMs] = timeMs;
    s.values[slot::kDlySyncR] = 0.0f;
    s.values[slot::kDlyLink] = 1.0f;
    s.values[slot::kDlyCross] = 0.0f;
    s.values[slot::kDlyDriveDb] = 0.0f;
    s.values[slot::kDlyModRate] = 0.5f;
    s.values[slot::kDlyModDepth] = 0.0f;
    s.values[slot::kDlyDiffusion] = 0.0f;
    s.values[slot::kDlyPitch] = 0.0f;
    s.values[slot::kDlyLowRes] = 0.0f;
    s.values[slot::kDlyHighRes] = 0.0f;
    s.values[slot::kDlyBandHz] = 1000.0f;
    s.values[slot::kDlyBandGainDb] = 0.0f;
    s.values[slot::kDlyBandQ] = 1.0f;
    s.values[slot::kDlyDuck] = 0.0f;
    s.values[slot::kDlyFreeze] = 0.0f;
    s.values[slot::kDlyLofi] = 0.0f;
    s.values[slot::kDlyTimeMode] = 0.0f;
    return s;
}

template <typename Fn>
juce::AudioBuffer<float> render(const juce::AudioBuffer<float>& x, int block, Fn&& paramsAt, const Transport& transport = {},
                                std::uint64_t* allocations = nullptr)
{
    auto d = std::make_unique<DelayProcessor>();
    d->prepare(kFs);
    juce::AudioBuffer<float> wet(x.getNumChannels(), x.getNumSamples());
    for (int start = 0; start < x.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, x.getNumSamples() - start);
        const float* in[2] { x.getReadPointer(0) + start, x.getReadPointer(x.getNumChannels() - 1) + start };
        float* out[2] { wet.getWritePointer(0) + start, wet.getWritePointer(x.getNumChannels() - 1) + start };
        const auto p = paramsAt(start);
        ScopedAllocationProbe probe;
        d->process(in, out, x.getNumChannels(), n, p, transport);
        if (allocations != nullptr)
            *allocations += probe.counts().allocations;
    }
    return wet;
}

juce::AudioBuffer<float> renderWith(const juce::AudioBuffer<float>& x, const FxSlotParams& p, const Transport& t = {})
{
    return render(x, 512, [&p](int) { return p; }, t);
}

int argmaxAbs(const juce::AudioBuffer<float>& y, int ch, int from, int to)
{
    int best = from;
    for (int i = from; i < to; ++i)
        if (std::abs(y.getSample(ch, i)) > std::abs(y.getSample(ch, best)))
            best = i;
    return best;
}

double windowEnergyDb(const juce::AudioBuffer<float>& y, int ch, int from, int length)
{
    double s = 0.0;
    for (int i = from; i < from + length; ++i)
        s += static_cast<double>(y.getSample(ch, i)) * y.getSample(ch, i);
    return 10.0 * std::log10(s + 1e-30);
}

// Power spectrum of y[ch][start .. start + 2^order) with a 4-term Blackman-Harris window (-92 dB sidelobes).
std::vector<double> powerSpectrum(const juce::AudioBuffer<float>& y, int ch, int start, int order)
{
    const int n = 1 << order;
    juce::dsp::FFT fft(order);
    std::vector<float> frame(2 * static_cast<size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i)
    {
        const double t = 2.0 * juce::MathConstants<double>::pi * i / n;
        const double w = 0.35875 - 0.48829 * std::cos(t) + 0.14128 * std::cos(2 * t) - 0.01168 * std::cos(3 * t);
        frame[static_cast<size_t>(i)] = static_cast<float>(y.getSample(ch, start + i) * w);
    }
    fft.performFrequencyOnlyForwardTransform(frame.data());
    std::vector<double> p(static_cast<size_t>(n / 2 + 1));
    for (size_t k = 0; k < p.size(); ++k)
        p[k] = static_cast<double>(frame[k]) * frame[k];
    return p;
}

// Energy outside +-guard Hz of f0 relative to the total, dB (THD + N with the fundamental removed).
double outsideDb(const std::vector<double>& p, double f0, double guardHz, int order)
{
    const double binHz = kFs / (1 << order);
    double inside = 0.0, total = 0.0;
    for (size_t k = 1; k < p.size(); ++k)
    {
        total += p[k];
        if (std::abs(k * binHz - f0) <= guardHz)
            inside += p[k];
    }
    return 10.0 * std::log10((total - inside) / (total + 1e-30) + 1e-30);
}

double peakFrequency(const std::vector<double>& p, int order)
{
    size_t best = 1;
    for (size_t k = 1; k < p.size(); ++k)
        if (p[k] > p[best])
            best = k;
    // parabolic interpolation on the log magnitude
    const double a = std::log(p[best - 1] + 1e-30), b = std::log(p[best] + 1e-30), c = std::log(p[best + 1] + 1e-30);
    const double offset = 0.5 * (a - c) / (a - 2 * b + c);
    return (static_cast<double>(best) + offset) * kFs / (1 << order);
}

template <typename T>
T* findChild(juce::Component* root)
{
    juce::Array<juce::Component*> stack { root };
    while (!stack.isEmpty())
    {
        auto* c = stack.removeAndReturn(stack.size() - 1);
        for (auto* child : c->getChildren())
            stack.add(child);
        if (auto* t = dynamic_cast<T*>(c))
            return t;
    }
    return nullptr;
}
} // namespace

class DelayPassTests final : public juce::UnitTest
{
public:
    DelayPassTests() : juce::UnitTest("Delay pass", "HPEQ") {}

    void runTest() override
    {
        beginTest("Classic (no character key): the module is the M6 delay bit-exact (stereo, mono, ping-pong)");
        {
            const auto noise = makeWhiteNoise({ kFs, 2, 3.0, kFixtureSeed }, -18.0);
            for (int channels : { 2, 1 })
                for (bool pp : { false, true })
                {
                    auto p = echoParams(echo::Classic, 180.0f);
                    p.values[slot::kFeedback] = 0.6f;
                    p.values[slot::kPingPong] = pp ? 1.0f : 0.0f;
                    p.values[slot::kDampingHz] = 6000.0f;
                    p.values[slot::kDelayLowCutHz] = 150.0f;
                    p.values[slot::kDelayWidth] = 0.8f;
                    juce::AudioBuffer<float> x(channels, noise.getNumSamples());
                    for (int ch = 0; ch < channels; ++ch)
                        x.copyFrom(ch, 0, noise, ch, 0, noise.getNumSamples());
                    ClassicDelay classic;
                    classic.prepare(kFs);
                    juce::AudioBuffer<float> expected(channels, x.getNumSamples());
                    Transport t;
                    for (int start = 0; start < x.getNumSamples(); start += 300)
                    {
                        const int n = juce::jmin(300, x.getNumSamples() - start);
                        const float* in[2] { x.getReadPointer(0) + start, x.getReadPointer(channels - 1) + start };
                        float* out[2] { expected.getWritePointer(0) + start, expected.getWritePointer(channels - 1) + start };
                        classic.process(in, out, channels, n, p, t);
                    }
                    const auto got = render(x, 300, [&p](int) { return p; });
                    const auto null = compareAligned(got, expected);
                    expect(null.bitExact, juce::String(channels) + " ch, ping-pong " + juce::String(pp ? 1 : 0) + ": " + juce::String(null.errorDb, 1) + " dB");
                }
            FxSlotState old;
            old.type = FxType::Delay;
            old.parameters.set("timeMs", 200.0);
            LayerState layer;
            FxNodeState node;
            node.chain = { old };
            layer.fxNodes = { node };
            expectEquals(static_cast<int>(makeSnapshot(layer).fx[0].slots[0].values[slot::kDlyCharacter]), 0, "a saved delay must stay Classic");
        }

        beginTest("Echo positions: L / R free (300 / 450 ms) and synced (1/4, 1/8 D at 120 BPM) within 1 sample; link copies L");
        {
            const auto x = makeImpulse({ kFs, 2, 1.0, kFixtureSeed }, 0, 0.01f);
            Transport t;
            t.bpm = 120.0;
            auto p = echoParams(echo::Clean, 300.0f);
            p.values[slot::kDlyLink] = 0.0f;
            p.values[slot::kDlyTimeRMs] = 450.0f;
            auto y = renderWith(x, p, t);
            const int l = argmaxAbs(y, 0, 0, y.getNumSamples()), r = argmaxAbs(y, 1, 0, y.getNumSamples());
            logMessage("  free: L " + juce::String(l) + ", R " + juce::String(r) + " samples");
            expect(std::abs(l - 14400) <= 1 && std::abs(r - 21600) <= 1);
            p.values[slot::kDelaySync] = 8.0f;  // 1/4
            p.values[slot::kDlySyncR] = 6.0f;   // 1/8 D
            y = renderWith(x, p, t);
            const int ls = argmaxAbs(y, 0, 0, y.getNumSamples()), rs = argmaxAbs(y, 1, 0, y.getNumSamples());
            logMessage("  synced: L " + juce::String(ls) + ", R " + juce::String(rs) + " samples");
            expect(std::abs(ls - 24000) <= 1 && std::abs(rs - 18000) <= 1);
            p.values[slot::kDlyLink] = 1.0f;
            y = renderWith(x, p, t);
            expectEquals(argmaxAbs(y, 1, 0, y.getNumSamples()), argmaxAbs(y, 0, 0, y.getNumSamples()));
        }

        beginTest("Feedback 0.5: each echo is -6.02 dB below the previous one +-0.5 dB (Clean, filters open)");
        {
            auto p = echoParams(echo::Clean, 100.0f);
            p.values[slot::kFeedback] = 0.5f;
            const auto y = renderWith(makeImpulse({ kFs, 2, 0.6, kFixtureSeed }, 0, 0.01f), p);
            for (int k = 1; k <= 3; ++k)
            {
                const int a = argmaxAbs(y, 0, 4800 * k - 100, 4800 * k + 400), b = argmaxAbs(y, 0, 4800 * (k + 1) - 100, 4800 * (k + 1) + 400);
                const double ratio = 20.0 * std::log10(std::abs(y.getSample(0, b)) / std::abs(y.getSample(0, a)));
                logMessage("  echo " + juce::String(k + 1) + " / " + juce::String(k) + ": " + juce::String(ratio, 3) + " dB");
                expect(std::abs(ratio + 6.0206) <= 0.5);
            }
        }

        beginTest("Cross 1: the echoes alternate sides (the second echo >= 30 dB stronger on the other side)");
        {
            auto p = echoParams(echo::Clean, 100.0f);
            p.values[slot::kFeedback] = 0.7f;
            p.values[slot::kDlyCross] = 1.0f;
            auto x = makeImpulse({ kFs, 2, 0.5, kFixtureSeed }, 0, 0.01f);
            x.clear(1, 0, x.getNumSamples());
            const auto y = renderWith(x, p);
            const double same = windowEnergyDb(y, 0, 9600 - 200, 1200), other = windowEnergyDb(y, 1, 9600 - 200, 1200);
            const double first = windowEnergyDb(y, 0, 4800 - 200, 1200) - windowEnergyDb(y, 1, 4800 - 200, 1200);
            logMessage("  echo 1: L - R " + juce::String(first, 1) + " dB; echo 2: R - L " + juce::String(other - same, 1) + " dB");
            expect(other - same >= 30.0 && first >= 30.0);
        }

        beginTest("Mono source, ping-pong off, cross 0 (owner's bug, BBD 1/8T, pitch -0.3, mod 42 %): both sides echo alike (L - R within 1 dB)");
        {
            for (int character : { echo::Clean, echo::Tape, echo::Bbd, echo::LoFi })
            {
                auto p = echoParams(character, 250.0f);
                p.values[slot::kFeedback] = 0.04f;
                p.values[slot::kDlyDriveDb] = 3.0f;
                p.values[slot::kDelayWidth] = 0.77f;
                p.values[slot::kDlyModRate] = 0.42f;
                p.values[slot::kDlyModDepth] = 0.42f;
                p.values[slot::kDlyPitch] = -0.3f;
                p.values[slot::kDelayLowCutHz] = 120.0f;
                p.values[slot::kDampingHz] = 8000.0f;
                juce::AudioBuffer<float> x(2, 96000);
                juce::Random rng(3);
                for (int i = 0; i < 48000; ++i)
                {
                    const float v = 0.1f * (2.0f * rng.nextFloat() - 1.0f) * std::sin(static_cast<float>(i) * 0.0004f);
                    x.setSample(0, i, v);
                    x.setSample(1, i, v);
                }
                const auto y = renderWith(x, p);
                const double l = windowEnergyDb(y, 0, 0, 96000), r = windowEnergyDb(y, 1, 0, 96000);
                logMessage("  character " + juce::String(character) + ": L " + juce::String(l, 2) + " dB, R " + juce::String(r, 2) + " dB");
                expect(std::abs(l - r) <= 1.0, "character " + juce::String(character));
            }
        }

        beginTest("Ping-pong at CROSS 0 (owner's bug): a mono source's echoes bounce L, R, L (each >= 20 dB stronger on its side)");
        {
            for (int character : { echo::Clean, echo::Tape, echo::Bbd })
            {
                auto p = echoParams(character, 100.0f);
                p.values[slot::kFeedback] = 0.6f;
                p.values[slot::kPingPong] = 1.0f;
                auto x = makeImpulse({ kFs, 2, 0.5, kFixtureSeed }, 0, 0.01f);
                x.copyFrom(1, 0, x, 0, 0, x.getNumSamples());
                const auto y = renderWith(x, p);
                double worst = 1.0e9;
                juce::String line;
                for (int k = 1; k <= 3; ++k)
                {
                    const int at = k * 4800 - 200;
                    const double d = windowEnergyDb(y, k % 2 == 1 ? 0 : 1, at, 1200) - windowEnergyDb(y, k % 2 == 1 ? 1 : 0, at, 1200);
                    worst = std::min(worst, d);
                    line << " echo " << k << ": " << juce::String(d, 1) << " dB";
                }
                logMessage("  character " + juce::String(character) + ":" + line);
                expect(worst >= 20.0, "character " + juce::String(character));
            }
        }

        beginTest("Loop filters: the first echo's response = the analytic low cut x band x high cut +-0.5 dB (100 Hz .. 10 kHz)");
        {
            auto p = echoParams(echo::Clean, 50.0f);
            p.values[slot::kDelayLowCutHz] = 400.0f;
            p.values[slot::kDlyLowRes] = 0.5f;
            p.values[slot::kDlyBandHz] = 2000.0f;
            p.values[slot::kDlyBandGainDb] = 6.0f;
            p.values[slot::kDlyBandQ] = 1.0f;
            p.values[slot::kDampingHz] = 6000.0f;
            p.values[slot::kDlyHighRes] = 0.3f;
            const auto y = renderWith(makeImpulse({ kFs, 2, 0.6, kFixtureSeed }, 0, 0.01f), p);
            static constexpr int order = 14;
            juce::AudioBuffer<float> ir(1, 1 << order);
            ir.clear();
            for (int i = 0; i < (1 << order); ++i)
                ir.setSample(0, i, 100.0f * y.getSample(0, 2400 + i)); // the echo of a 0.01 impulse, scaled to 1
            // rectangular window: the impulse response has decayed long before the frame ends
            juce::dsp::FFT fft(order);
            std::vector<std::complex<float>> spec(static_cast<size_t>(1 << order)), time(static_cast<size_t>(1 << order));
            for (int i = 0; i < (1 << order); ++i)
                time[static_cast<size_t>(i)] = { ir.getSample(0, i), 0.0f };
            fft.perform(time.data(), spec.data(), false);
            double worst = 0.0;
            for (double f : { 100.0, 250.0, 400.0, 700.0, 1000.0, 2000.0, 3000.0, 5000.0, 6000.0, 8000.0, 10000.0 })
            {
                const int k = static_cast<int>(std::lround(f / (kFs / (1 << order))));
                const double measured = 20.0 * std::log10(std::abs(spec[static_cast<size_t>(k)]) + 1e-30);
                const double analytic = echo::loopResponseDb(k * kFs / (1 << order), p, kFs);
                worst = std::max(worst, std::abs(measured - analytic));
                logMessage("  " + juce::String(f, 0) + " Hz: measured " + juce::String(measured, 2) + " dB, analytic " + juce::String(analytic, 2) + " dB");
            }
            expect(worst <= 0.5, "worst " + juce::String(worst, 3) + " dB");
            p.values[slot::kDelayLowCutHz] = 10.0f;
            p.values[slot::kDampingHz] = 20000.0f;
            p.values[slot::kDlyBandHz] = 1000.0f;
            const auto band = renderWith(makeSine({ kFs, 2, 1.0, kFixtureSeed }, 1000.0, -30.0), p);
            const auto dry = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 1000.0, -30.0);
            const double gain = windowEnergyDb(band, 0, 24000, 12000) - windowEnergyDb(dry, 0, 24000, 12000);
            logMessage("  band +6 dB at 1 kHz: " + juce::String(gain, 3) + " dB");
            expect(std::abs(gain - 6.0) <= 0.5);
        }

        beginTest("Clean at -20 dBFS: echo THD < -80 dB; Tape drive 18 dB at -6 dBFS: >= 30 dB more");
        {
            // Disclosed measurement fix (report §5): the first run used a 2^15 window, whose Blackman-Harris
            // main lobe (+-5.9 Hz) is wider than the +-5 Hz guard, so the tone's own skirt read as -67 dB.
            static constexpr int order = 16;
            const auto thd = [](const FxSlotParams& p, double level)
            {
                const auto y = renderWith(makeSine({ kFs, 2, 2.5, kFixtureSeed }, 1000.0, level), p);
                return outsideDb(powerSpectrum(y, 0, 40000, order), 1000.0, 5.0, order);
            };
            const double clean = thd(echoParams(echo::Clean), -20.0);
            auto tape = echoParams(echo::Tape);
            tape.values[slot::kDlyDriveDb] = 18.0f;
            tape.values[slot::kDampingHz] = 20000.0f;
            const double driven = thd(tape, -6.0);
            logMessage("  THD+N: Clean -20 dBFS " + juce::String(clean, 1) + " dB, Tape drive 18 at -6 dBFS " + juce::String(driven, 1) + " dB");
            expect(clean < -80.0);
            expect(driven - clean >= 30.0);
        }

        beginTest("Pitch +12 / -12: a 500 Hz echo comes back at 1000 / 250 Hz +-2 %");
        {
            static constexpr int order = 14;
            for (float st : { 12.0f, -12.0f })
            {
                auto p = echoParams(echo::Clean, 100.0f);
                p.values[slot::kDlyPitch] = st;
                const auto y = renderWith(makeSine({ kFs, 2, 1.5, kFixtureSeed }, 500.0, -18.0), p);
                const double f = peakFrequency(powerSpectrum(y, 0, 24000, order), order);
                const double expected = 500.0 * std::pow(2.0, st / 12.0);
                logMessage("  pitch " + juce::String(st, 0) + " st: " + juce::String(f, 1) + " Hz (expected " + juce::String(expected, 0) + ")");
                expect(std::abs(f / expected - 1.0) <= 0.02);
            }
        }

        beginTest("Modulation: a 1 kHz echo spreads (outside +-5 Hz >= -30 dB) at depth 1, stays clean (< -80 dB) at depth 0");
        {
            static constexpr int order = 16;
            const auto spread = [](float depth)
            {
                auto p = echoParams(echo::Clean, 100.0f);
                p.values[slot::kDlyModDepth] = depth;
                p.values[slot::kDlyModRate] = 1.0f;
                const auto y = renderWith(makeSine({ kFs, 2, 2.0, kFixtureSeed }, 1000.0, -18.0), p);
                return outsideDb(powerSpectrum(y, 0, 12000, order), 1000.0, 5.0, order);
            };
            const double still = spread(0.0f), moving = spread(1.0f);
            logMessage("  outside +-5 Hz: depth 0 " + juce::String(still, 1) + " dB, depth 1 " + juce::String(moving, 1) + " dB");
            expect(still < -80.0 && moving >= -30.0);
        }

        beginTest("Time change 300 -> 400 ms: FADE keeps the echo at 1 kHz +-1 %; TAPE bends it > 2 %");
        {
            static constexpr int order = 12;
            const auto during = [](int mode)
            {
                const auto y = render(makeSine({ kFs, 2, 2.0, kFixtureSeed }, 1000.0, -18.0), 64, [mode](int start)
                {
                    auto p = echoParams(echo::Clean, start < 48000 ? 300.0f : 400.0f);
                    p.values[slot::kDlyTimeMode] = static_cast<float>(mode);
                    return p;
                });
                return peakFrequency(powerSpectrum(y, 0, 48000 + 200, order), order);
            };
            const double fade = during(echo::FadeTime), tape = during(echo::TapeTime);
            logMessage("  during the change: FADE " + juce::String(fade, 1) + " Hz, TAPE " + juce::String(tape, 1) + " Hz");
            expect(std::abs(fade / 1000.0 - 1.0) <= 0.01);
            expect(std::abs(tape / 1000.0 - 1.0) > 0.02);
        }

        beginTest("Diffusion 1 lowers the crest factor of the first echo >= 10 dB");
        {
            const auto crest = [](float diffusion)
            {
                auto p = echoParams(echo::Clean, 200.0f);
                p.values[slot::kDlyDiffusion] = diffusion;
                const auto y = renderWith(makeImpulse({ kFs, 2, 0.5, kFixtureSeed }, 0, 0.01f), p);
                double peak = 0.0, s = 0.0;
                for (int i = 9600 - 100; i < 9600 + 4800; ++i)
                {
                    peak = std::max(peak, static_cast<double>(std::abs(y.getSample(0, i))));
                    s += static_cast<double>(y.getSample(0, i)) * y.getSample(0, i);
                }
                return 20.0 * std::log10(peak / std::sqrt(s / 4900.0));
            };
            const double c0 = crest(0.0f), c1 = crest(1.0f);
            logMessage("  crest factor: diffusion 0 " + juce::String(c0, 1) + " dB, 1 " + juce::String(c1, 1) + " dB");
            expect(c0 - c1 >= 10.0);
        }

        beginTest("Lo-Fi 1 (Clean): THD+N of a -12 dBFS 1 kHz echo > -40 dB");
        {
            static constexpr int order = 16;
            auto p = echoParams(echo::Clean);
            p.values[slot::kDlyLofi] = 1.0f;
            const auto y = renderWith(makeSine({ kFs, 2, 2.5, kFixtureSeed }, 1000.0, -12.0), p);
            const double thd = outsideDb(powerSpectrum(y, 0, 40000, order), 1000.0, 5.0, order);
            logMessage("  lo-fi 1: THD+N " + juce::String(thd, 1) + " dB");
            expect(thd > -40.0);
        }

        beginTest("Characters: the 6th echo of noise is darker in BBD than in Clean (centroid <= 0.7 x)");
        {
            const auto centroid = [](int character)
            {
                auto p = echoParams(character, 100.0f);
                p.values[slot::kFeedback] = 0.8f;
                auto x = makeWhiteNoise({ kFs, 2, 1.0, kFixtureSeed }, -24.0);
                for (int ch = 0; ch < 2; ++ch)
                    x.clear(ch, 2400, x.getNumSamples() - 2400); // a 50 ms burst
                const auto y = renderWith(x, p);
                juce::AudioBuffer<float> sixth(1, 2400);
                sixth.copyFrom(0, 0, y, 0, 6 * 4800, 2400);
                return spectralCentroid(sixth, kFs, 0);
            };
            const double clean = centroid(echo::Clean), bbd = centroid(echo::Bbd), tape = centroid(echo::Tape);
            logMessage("  6th echo centroid: Clean " + juce::String(clean, 0) + " Hz, Tape " + juce::String(tape, 0) + " Hz, BBD " + juce::String(bbd, 0) + " Hz");
            expect(bbd <= 0.7 * clean);
        }

        beginTest("Freeze: the frozen loop keeps its energy within +-1 dB from 2 to 11 s and ignores new input (Clean, Tape)");
        {
            const auto x = makeWhiteNoise({ kFs, 2, 12.0, kFixtureSeed }, -18.0);
            for (int character : { echo::Clean, echo::Tape })
            {
                const auto y = render(x, 512, [character](int start)
                {
                    auto p = echoParams(character, 350.0f);
                    p.values[slot::kFeedback] = 0.5f;
                    p.values[slot::kDlyModDepth] = 0.5f;
                    p.values[slot::kDlyDriveDb] = 6.0f;
                    p.values[slot::kDlyFreeze] = start >= static_cast<int>(1.0 * kFs) ? 1.0f : 0.0f;
                    return p;
                });
                const double a = windowEnergyDb(y, 0, static_cast<int>(2.0 * kFs), static_cast<int>(kFs));
                const double b = windowEnergyDb(y, 0, static_cast<int>(10.0 * kFs), static_cast<int>(kFs));
                logMessage("  " + juce::String(echo::characterName(character)) + ": frozen energy " + juce::String(a, 2) + " -> " + juce::String(b, 2) + " dB");
                expect(std::abs(b - a) <= 1.0 && measureAll(y).allFinite);
            }
        }

        beginTest("Ducking 100 %: the echoes drop >= 12 dB under -6 dBFS input and come back within 1.5 dB 0.5 s after it stops");
        {
            auto x = makeWhiteNoise({ kFs, 2, 3.0, kFixtureSeed }, -6.0);
            for (int ch = 0; ch < 2; ++ch)
                x.clear(ch, static_cast<int>(1.5 * kFs), x.getNumSamples() - static_cast<int>(1.5 * kFs));
            const auto with = [&x](float duck)
            {
                auto p = echoParams(echo::Clean, 250.0f);
                p.values[slot::kFeedback] = 0.8f;
                p.values[slot::kDlyDuck] = duck;
                return renderWith(x, p);
            };
            const auto free = with(0.0f), ducked = with(1.0f);
            const double during = windowEnergyDb(free, 0, 48000, 24000) - windowEnergyDb(ducked, 0, 48000, 24000);
            const double after = windowEnergyDb(free, 0, 96000, 24000) - windowEnergyDb(ducked, 0, 96000, 24000);
            logMessage("  ducking during " + juce::String(during, 2) + " dB, after " + juce::String(after, 2) + " dB");
            expect(during >= 12.0 && std::abs(after) <= 1.5);
        }

        beginTest("Engine change Classic -> Tape mid-stream: no sample above 1.5 x the steady peak");
        {
            const auto x = makeWhiteNoise({ kFs, 2, 3.0, kFixtureSeed }, -18.0);
            const int change = static_cast<int>(1.5 * kFs) + 33;
            const auto y = render(x, 64, [change](int start)
            {
                auto p = echoParams(start < change ? echo::Classic : echo::Tape, 200.0f);
                p.values[slot::kFeedback] = 0.5f;
                return p;
            });
            float steady = 0.0f, around = 0.0f;
            for (int i = 48000; i < change; ++i)
                steady = std::max(steady, std::abs(y.getSample(0, i)));
            for (int i = change; i < change + 2400; ++i)
                around = std::max(around, std::abs(y.getSample(0, i)));
            logMessage("  steady " + juce::String(steady, 4) + ", around the change " + juce::String(around, 4));
            expect(around <= 1.5f * steady);
        }

        beginTest("Tail: time + time ln(1e-3) / ln(feedback) + 0.1 s; 30 s or more while frozen");
        {
            auto p = echoParams(echo::Tape, 250.0f);
            p.values[slot::kFeedback] = 0.5f;
            expectWithinAbsoluteError(DelayProcessor::tailSeconds(p, 120.0), 0.25 + 0.25 * std::log(1.0e-3) / std::log(0.5) + 0.1, 1.0e-9);
            p.values[slot::kDlyFreeze] = 1.0f;
            expect(DelayProcessor::tailSeconds(p, 120.0) >= 30.0);
        }

        beginTest("Block sizes 1, 64, 127, 512 render identically (everything on); 0 allocations");
        {
            auto x = makePinkNoise({ kFs, 2, 1.5, kFixtureSeed }, -14.0);
            const auto params = [](int)
            {
                auto p = echoParams(echo::Tape, 180.0f);
                p.values[slot::kFeedback] = 0.7f;
                p.values[slot::kDlyLink] = 0.0f;
                p.values[slot::kDlySyncR] = 5.0f;
                p.values[slot::kDlyCross] = 0.4f;
                p.values[slot::kDlyDriveDb] = 9.0f;
                p.values[slot::kDlyModDepth] = 0.8f;
                p.values[slot::kDlyDiffusion] = 0.6f;
                p.values[slot::kDlyPitch] = 7.0f;
                p.values[slot::kDlyLofi] = 0.3f;
                p.values[slot::kDelayLowCutHz] = 200.0f;
                p.values[slot::kDlyLowRes] = 0.4f;
                p.values[slot::kDampingHz] = 5000.0f;
                p.values[slot::kDlyHighRes] = 0.6f;
                p.values[slot::kDlyBandGainDb] = 4.0f;
                p.values[slot::kDlyDuck] = 0.5f;
                p.values[slot::kDlyTimeMode] = 1.0f;
                return p;
            };
            Transport t;
            t.bpm = 97.0;
            std::uint64_t allocations = 0;
            const auto reference = render(x, 512, params, t, &allocations);
            for (int block : { 1, 64, 127 })
            {
                const auto y = render(x, block, params, t, &allocations);
                const auto null = compareAligned(y, reference);
                expect(null.bitExact, "block " + juce::String(block) + ": " + juce::String(null.errorDb, 1) + " dB");
            }
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("Stability: 20k random mutations of every delay setting in the engine: finite, bounded, 0 allocations");
        {
            auto e = std::make_unique<LayerEngine>();
            e->prepare(kFs, 512);
            const auto noise = makeWhiteNoise({ kFs, 2, 4.0, kFixtureSeed }, -6.0);
            DeterministicRandom random(kFixtureSeed + 31);
            const auto uni = [&](double lo, double hi) { return lo + (hi - lo) * (random.nextBipolar() * 0.5 + 0.5); };
            juce::AudioBuffer<float> block(2, 1024);
            LayerSnapshot s;
            bool finite = true;
            std::uint64_t allocations = 0;
            for (int m = 0; m < 20000; ++m)
            {
                auto& n = s.fx[static_cast<size_t>(uni(0, 4))];
                n.active = uni(0, 1) > 0.2;
                n.lowHz = static_cast<float>(uni(20, 5000));
                n.highHz = static_cast<float>(uni(200, 20000));
                n.numSlots = static_cast<int>(uni(1, 3));
                auto& sl = n.slots[static_cast<size_t>(uni(0, 2))];
                sl = echoParams(static_cast<int>(uni(0, 4.99)), static_cast<float>(uni(1, 4000)));
                sl.values[slot::kDelaySync] = static_cast<float>(std::floor(uni(0, 11.99)));
                sl.values[slot::kFeedback] = static_cast<float>(uni(0, 0.98));
                sl.values[slot::kPingPong] = uni(0, 1) > 0.5 ? 1.0f : 0.0f;
                sl.values[slot::kDampingHz] = static_cast<float>(uni(500, 20000));
                sl.values[slot::kDelayLowCutHz] = static_cast<float>(uni(10, 2000));
                sl.values[slot::kDelayWidth] = static_cast<float>(uni(0, 1.5));
                sl.values[slot::kDlyTimeRMs] = static_cast<float>(uni(1, 4000));
                sl.values[slot::kDlySyncR] = static_cast<float>(std::floor(uni(0, 11.99)));
                sl.values[slot::kDlyLink] = uni(0, 1) > 0.5 ? 1.0f : 0.0f;
                sl.values[slot::kDlyCross] = static_cast<float>(uni(0, 1));
                sl.values[slot::kDlyDriveDb] = static_cast<float>(uni(0, 24));
                sl.values[slot::kDlyModRate] = static_cast<float>(uni(0.05, 10));
                sl.values[slot::kDlyModDepth] = static_cast<float>(uni(0, 1));
                sl.values[slot::kDlyDiffusion] = static_cast<float>(uni(0, 1));
                sl.values[slot::kDlyPitch] = static_cast<float>(uni(-12, 12));
                sl.values[slot::kDlyLowRes] = static_cast<float>(uni(0, 1));
                sl.values[slot::kDlyHighRes] = static_cast<float>(uni(0, 1));
                sl.values[slot::kDlyBandHz] = static_cast<float>(uni(100, 10000));
                sl.values[slot::kDlyBandGainDb] = static_cast<float>(uni(-18, 12));
                sl.values[slot::kDlyBandQ] = static_cast<float>(uni(0.3, 6));
                sl.values[slot::kDlyDuck] = static_cast<float>(uni(0, 1));
                sl.values[slot::kDlyFreeze] = uni(0, 1) > 0.9 ? 1.0f : 0.0f;
                sl.values[slot::kDlyLofi] = static_cast<float>(uni(0, 1));
                sl.values[slot::kDlyTimeMode] = uni(0, 1) > 0.5 ? 1.0f : 0.0f;
                sl.mix = static_cast<float>(uni(0, 1));
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
                const auto stats = measureAll(view);
                finite = finite && stats.allFinite && stats.peak < 1.0e3;
            }
            expect(finite, "non-finite or runaway output under mutations");
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("UI default delay (defaultModule): Tape, 1/4 synced, feedback 0.35");
        {
            const auto m = defaultModule(FxType::Delay);
            expectEquals(static_cast<int>(m.parameters["character"]), static_cast<int>(echo::Tape));
            expectEquals(static_cast<int>(m.parameters["sync"]), 8);
            expect(std::abs(static_cast<double>(m.parameters["feedback"]) - 0.35) < 1.0e-9);
        }

        beginTest("HamloEcho page: fills the editor body; knob, switch and ON edit the project; values come back; an old delay shows Classic");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            FxNodeState n;
            n.id = 1;
            n.chain = { defaultModule(FxType::Delay) };
            FxSlotState old;
            old.type = FxType::Delay;
            old.parameters.set("timeMs", 333.0);
            old.parameters.set("sync", 0);
            FxNodeState o;
            o.id = 2;
            o.lowHz = 2000.0;
            o.highHz = 6000.0;
            o.chain = { old };
            p.layers[0].fxNodes = { n, o };
            p.nextNodeId = 3;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1400, 900);
            editor->selectNode(true, 1, 0);
            auto* panel = findChild<ui::HamloEchoPanel>(editor.get());
            expect(panel != nullptr, "no HamloEcho panel on the delay page");
            if (panel != nullptr)
            {
                expect(panel->getWidth() > 1000 && panel->getHeight() > 500, "panel does not fill the editor body: " + panel->getBounds().toString());
                expectEquals(panel->getValues().switches[ui::HamloEchoPanel::Character], static_cast<int>(echo::Tape));
                panel->onKnobChange(ui::HamloEchoPanel::Feedback, 0.66);
                panel->onKnobChange(ui::HamloEchoPanel::Pitch, 7.0);
                panel->onSwitchChange(ui::HamloEchoPanel::Character, echo::Bbd);
                panel->onSwitchChange(ui::HamloEchoPanel::Freeze, 1);
                panel->onPowerChange(false);
                const auto project = plugin->getProject();
                const auto& m = project.layers[0].fxNodes[0].chain[0];
                expectEquals(static_cast<double>(m.parameters["feedback"]), 0.66);
                expectEquals(static_cast<double>(m.parameters["pitch"]), 7.0);
                expectEquals(static_cast<int>(m.parameters["character"]), static_cast<int>(echo::Bbd));
                expectEquals(static_cast<int>(m.parameters["freeze"]), 1);
                expect(!m.enabled);
                editor->selectNode(true, 1, 0);
                auto* rebuilt = findChild<ui::HamloEchoPanel>(editor.get());
                expect(rebuilt != nullptr && rebuilt->getValues().knobs[ui::HamloEchoPanel::Feedback] == 0.66
                       && rebuilt->getValues().switches[ui::HamloEchoPanel::Character] == echo::Bbd && !rebuilt->getValues().on);
            }
            editor->selectNode(true, 2, 0);
            auto* classicPage = findChild<ui::HamloEchoPanel>(editor.get());
            expect(classicPage != nullptr && classicPage->getValues().switches[ui::HamloEchoPanel::Character] == echo::Classic
                   && std::abs(classicPage->getValues().knobs[ui::HamloEchoPanel::TimeL] - 333.0) < 1.0e-9);
        }
    }
};

static DelayPassTests delayPassTests;
} // namespace hpeq::test
