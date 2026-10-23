// Module pass 4 (owner): HamloMag — the Saturator after Soundtoys Decapitator, with our own HamloLaw
// harmonics, tape / vinyl emulation and Soviet analog noises. Thresholds fixed here before the runs
// (docs/fx-addon/SATURATOR_DESIGN.md §5).

#include <cmath>
#include <memory>

#include <juce_dsp/juce_dsp.h>

#include "DSP/FxNodeProcessor.h"
#include "DSP/LayerEngine.h"
#include "DSP/ProcessorsM6.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/HamloMagPanel.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;
constexpr int kOrder = 15, kN = 1 << kOrder;

// A HamloMag slot with every colour off: no noises, no wow, filters open, harmonics at the signature.
FxSlotParams magParams(int model, float driveDb = 0.0f, int oversampling = 2)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Saturator;
    s.values[slot::kDriveDb] = driveDb;
    s.values[slot::kCurve] = 0.0f;
    s.values[slot::kBias] = 0.0f;
    s.values[slot::kTrimDb] = 0.0f;
    s.values[slot::kOversampling] = static_cast<float>(oversampling);
    s.values[slot::kAutoGain] = 1.0f;
    s.values[slot::kSatModel] = static_cast<float>(model);
    s.values[slot::kSatPunish] = 0.0f;
    s.values[slot::kSatTone] = 0.0f;
    s.values[slot::kSatLowCutHz] = 10.0f;
    s.values[slot::kSatHighCutHz] = 20000.0f;
    s.values[slot::kSatThump] = 0.0f;
    s.values[slot::kSatSteep] = 0.0f;
    s.values[slot::kSatHarmonics] = 0.5f;
    s.values[slot::kSatEvenOdd] = 0.0f;
    s.values[slot::kSatTapeBias] = 0.5f;
    s.values[slot::kSatSpeed] = 0.0f;
    s.values[slot::kSatWow] = 0.0f;
    s.values[slot::kSatHiss] = 0.0f;
    s.values[slot::kSatHum] = 0.0f;
    s.values[slot::kSatCrackle] = 0.0f;
    s.values[slot::kSatNoiseFollow] = 0.0f;
    return s;
}

template <typename Fn>
juce::AudioBuffer<float> render(const juce::AudioBuffer<float>& x, int block, Fn&& paramsAt, std::uint64_t* allocations = nullptr)
{
    auto sat = std::make_unique<SaturatorProcessor>();
    sat->prepare(kFs, 512);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(y.getNumChannels() - 1) + start };
        const auto p = paramsAt(start);
        ScopedAllocationProbe probe;
        sat->process(chunk, y.getNumChannels(), n, p);
        if (allocations != nullptr)
            *allocations += probe.counts().allocations;
    }
    return y;
}

juce::AudioBuffer<float> renderWith(const juce::AudioBuffer<float>& x, const FxSlotParams& p)
{
    return render(x, 512, [&p](int) { return p; });
}

// A sine on an exact bin of a kN frame, `seconds` long.
juce::AudioBuffer<float> binSine(int bin, double dbfs, double seconds)
{
    juce::AudioBuffer<float> x(2, static_cast<int>(seconds * kFs));
    const double a = std::pow(10.0, dbfs / 20.0);
    for (int i = 0; i < x.getNumSamples(); ++i)
        for (int ch = 0; ch < 2; ++ch)
            x.setSample(ch, i, static_cast<float>(a * std::sin(2.0 * juce::MathConstants<double>::pi * bin * i / kN)));
    return x;
}

// Rectangular-window magnitudes (dBFS of a sine) of the frame y[ch][start .. start + kN) at bins.
std::vector<double> binLevels(const juce::AudioBuffer<float>& y, int ch, int start, std::initializer_list<int> bins)
{
    std::vector<double> out;
    for (int b : bins)
    {
        double re = 0.0, im = 0.0;
        for (int i = 0; i < kN; ++i)
        {
            const double w = 2.0 * juce::MathConstants<double>::pi * b * i / kN;
            re += y.getSample(ch, start + i) * std::cos(w);
            im += y.getSample(ch, start + i) * std::sin(w);
        }
        out.push_back(20.0 * std::log10(2.0 * std::sqrt(re * re + im * im) / kN + 1e-15));
    }
    return out;
}

double rmsDb(const juce::AudioBuffer<float>& y, int ch, int start, int length)
{
    double s = 0.0;
    for (int i = start; i < start + length; ++i)
        s += static_cast<double>(y.getSample(ch, i)) * y.getSample(ch, i);
    return 10.0 * std::log10(s / length + 1e-30);
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

class SaturatorPassTests final : public juce::UnitTest
{
public:
    SaturatorPassTests() : juce::UnitTest("Saturator pass", "HPEQ") {}

    void runTest() override
    {
        beginTest("Classic (no model key): the M6 law, untouched by every new setting");
        {
            const auto noise = makeWhiteNoise({ kFs, 2, 1.0, kFixtureSeed }, -12.0);
            auto plain = magParams(mag::Classic, 18.0f, 1);
            plain.values[slot::kCurve] = 1.0f;
            plain.values[slot::kBias] = 0.3f;
            auto busy = plain;
            for (int i = slot::kSatPunish; i <= slot::kSatNoiseFollow; ++i)
                busy.values[static_cast<size_t>(i)] = 0.7f; // every HamloMag setting moved
            const auto a = renderWith(noise, plain), b = renderWith(noise, busy);
            expect(compareAligned(a, b).bitExact);
            FxSlotState old;
            old.type = FxType::Saturator;
            old.parameters.set("driveDb", 12.0);
            LayerState layer;
            FxNodeState node;
            node.chain = { old };
            layer.fxNodes = { node };
            expectEquals(static_cast<int>(makeSnapshot(layer).fx[0].slots[0].values[slot::kSatModel]), 0, "a saved saturator must stay Classic");
        }

        // Disclosed design change (report §5): the first law, h = E sum H k_n l^(n-1) T_n, grew with E and was
        // unbounded at extreme drive (the fuzz test found it); the envelope now saturates too.
        beginTest("HamloLaw: harmonics 2..7 of a steady tone equal E_m H k_n tanh(A / E_m)^n within 0.5 dB (every model, A = 0.3 and 0.8)");
        {
            constexpr int bin = 683; // ~1000 Hz on a 2^15 frame
            double worst = 0.0;
            for (int model = mag::Tape; model < mag::NumModels; ++model)
                for (double amplitude : { 0.3, 0.8 })
                {
                    const auto s = mag::lawSettings(model, 1.0, 0.0);
                    mag::Core core;
                    core.prepare(kFs);
                    juce::AudioBuffer<float> y(1, 2 * kN);
                    for (int i = 0; i < y.getNumSamples(); ++i)
                        y.setSample(0, i, static_cast<float>(core.harmonics(amplitude * std::cos(2.0 * juce::MathConstants<double>::pi * bin * i / kN), s)));
                    const auto got = binLevels(y, 0, kN, { 2 * bin, 3 * bin, 4 * bin, 5 * bin, 6 * bin, 7 * bin });
                    for (size_t k = 0; k < got.size(); ++k)
                    {
                        const double expected = 20.0 * std::log10(s.knee * s.weights[k] * std::pow(std::tanh(amplitude / s.knee), static_cast<double>(k + 2)));
                        worst = std::max(worst, std::abs(got[k] - expected));
                    }
                }
            logMessage("  worst deviation from the formula: " + juce::String(worst, 4) + " dB");
            expect(worst <= 0.5);
        }

        beginTest("Power law: below the knee H2 / H1 rises 10 dB and H3 / H1 20 dB for +10 dB of input (+-10 %), TUBE");
        {
            constexpr int bin = 683;
            const auto rel = [&](double dbfs)
            {
                const auto y = renderWith(binSine(bin, dbfs, 2.0), magParams(mag::Tube, 0.0f));
                const auto l = binLevels(y, 0, kN, { bin, 2 * bin, 3 * bin });
                return std::pair { l[1] - l[0], l[2] - l[0] };
            };
            const auto lo = rel(-40.0), hi = rel(-30.0);
            logMessage("  H2/H1 " + juce::String(lo.first, 2) + " -> " + juce::String(hi.first, 2) + " dB, H3/H1 " + juce::String(lo.second, 2) + " -> "
                       + juce::String(hi.second, 2) + " dB");
            expect(std::abs((hi.first - lo.first) - 10.0) <= 1.0);
            expect(std::abs((hi.second - lo.second) - 20.0) <= 2.0);
        }

        beginTest("Models: TUBE even > odd (H2 > H3), GERMANIUM odd > even (H3 > H2) at -12 dBFS, drive 6 dB");
        {
            constexpr int bin = 683;
            const auto levels = [&](int model)
            {
                const auto y = renderWith(binSine(bin, -12.0, 2.0), magParams(model, 6.0f));
                return binLevels(y, 0, kN, { bin, 2 * bin, 3 * bin });
            };
            const auto tube = levels(mag::Tube), germanium = levels(mag::Germanium);
            logMessage("  TUBE H2 " + juce::String(tube[1] - tube[0], 1) + " H3 " + juce::String(tube[2] - tube[0], 1) + " dB; GERMANIUM H2 "
                       + juce::String(germanium[1] - germanium[0], 1) + " H3 " + juce::String(germanium[2] - germanium[0], 1) + " dB");
            expect(tube[1] > tube[2] && germanium[2] > germanium[1]);
        }

        beginTest("TAPE: head bump +2.5 dB at 60 Hz (19 cm/s) +-0.7; at 12 kHz 4.76 cm/s is >= 10 dB below 19 cm/s");
        {
            const auto gainAt = [](double hz, int speed)
            {
                auto p = magParams(mag::Tape, 0.0f);
                p.values[slot::kSatSpeed] = static_cast<float>(speed);
                p.values[slot::kSatHarmonics] = 0.0f;
                const auto x = makeSine({ kFs, 2, 1.5, kFixtureSeed }, hz, -50.0);
                const auto y = renderWith(x, p);
                return rmsDb(y, 0, 24000, 48000) - rmsDb(x, 0, 24000, 48000);
            };
            const double bump = gainAt(60.0, 0) - gainAt(1000.0, 0);
            const double loss = gainAt(12000.0, 0) - gainAt(12000.0, 2);
            logMessage("  bump at 60 Hz " + juce::String(bump, 2) + " dB; 12 kHz: 19 vs 4.76 cm/s " + juce::String(loss, 2) + " dB");
            expect(std::abs(bump - 2.5) <= 0.7 && loss >= 10.0);
        }

        beginTest("IRON: at -12 dBFS, drive 6 dB, H3 / H1 at 50 Hz is >= 10 dB above that at 1 kHz");
        {
            const auto h3 = [](int bin)
            {
                const auto y = renderWith(binSine(bin, -12.0, 3.0), magParams(mag::Iron, 6.0f));
                const auto l = binLevels(y, 0, kN, { bin, 3 * bin });
                return l[1] - l[0];
            };
            const double low = h3(34), mid = h3(683); // 49.8 Hz, 1000.4 Hz
            logMessage("  H3/H1: 50 Hz " + juce::String(low, 1) + " dB, 1 kHz " + juce::String(mid, 1) + " dB");
            expect(low - mid >= 10.0);
        }

        beginTest("VINYL 78 rpm: 50 Hz and 8 kHz are >= 12 dB below 1 kHz (shellac band)");
        {
            const auto gainAt = [](double hz)
            {
                auto p = magParams(mag::Vinyl, 0.0f);
                p.values[slot::kSatSpeed] = 2.0f;
                p.values[slot::kSatHarmonics] = 0.0f;
                const auto x = makeSine({ kFs, 2, 1.5, kFixtureSeed }, hz, -50.0);
                const auto y = renderWith(x, p);
                return rmsDb(y, 0, 24000, 48000) - rmsDb(x, 0, 24000, 48000);
            };
            const double g50 = gainAt(50.0), g1k = gainAt(1000.0), g8k = gainAt(8000.0);
            logMessage("  50 Hz " + juce::String(g50, 1) + ", 1 kHz " + juce::String(g1k, 1) + ", 8 kHz " + juce::String(g8k, 1) + " dB");
            expect(g1k - g50 >= 12.0 && g1k - g8k >= 12.0);
        }

        beginTest("Latency: measured = reported for TAPE / VINYL (oversampling + 4 ms wow centre) and TUBE (oversampling only); wow spreads TAPE only");
        {
            for (int model : { mag::Tape, mag::Vinyl, mag::Tube })
            {
                auto p = magParams(model, 0.0f, 1);
                p.values[slot::kSatHarmonics] = 0.0f;
                auto x = makeWhiteNoise({ kFs, 2, 1.0, kFixtureSeed }, -40.0);
                const auto y = renderWith(x, p);
                const int measured = estimateDelay(y, x, 600);
                const int reported = SaturatorProcessor::latencySamples(p, kFs);
                logMessage("  " + juce::String(mag::modelName(model)) + ": reported " + juce::String(reported) + ", measured " + juce::String(measured));
                expectEquals(measured, reported);
            }
            const auto spread = [](int model)
            {
                auto p = magParams(model, 0.0f);
                p.values[slot::kSatWow] = 1.0f;
                p.values[slot::kSatHarmonics] = 0.0f;
                const auto y = renderWith(binSine(683, -20.0, 3.0), p);
                double inside = 0.0, total = 0.0;
                for (int b = 600; b < 760; ++b)
                {
                    const double l = std::pow(10.0, binLevels(y, 0, 2 * kN / 2, { b })[0] / 10.0);
                    total += l;
                    if (std::abs(b - 683) <= 3)
                        inside += l;
                }
                return 10.0 * std::log10((total - inside) / total + 1e-30);
            };
            const double tape = spread(mag::Tape), tube = spread(mag::Tube);
            logMessage("  energy outside +-4 Hz of a 1 kHz tone with wow 1: TAPE " + juce::String(tape, 1) + " dB, TUBE " + juce::String(tube, 1) + " dB");
            expect(tape >= -40.0 && tube < -60.0);
        }

        beginTest("Hum: no input, ALWAYS: 50 Hz is the strongest line below 1 kHz, harmonics present, level -50..-35 dBFS");
        {
            auto p = magParams(mag::Tube, 0.0f);
            p.values[slot::kSatHum] = 1.0f;
            juce::AudioBuffer<float> silence(2, static_cast<int>(2.0 * kFs));
            silence.clear();
            const auto y = renderWith(silence, p);
            // bins on the 2^15 frame: 50 Hz = 34.13 -> use 34, 100 Hz = 68, 150 Hz = 102
            const auto l = binLevels(y, 0, kN, { 34, 68, 102, 300, 500 });
            const double rms = rmsDb(y, 0, kN, kN);
            logMessage("  50 / 100 / 150 Hz: " + juce::String(l[0], 1) + " / " + juce::String(l[1], 1) + " / " + juce::String(l[2], 1) + " dB; RMS " + juce::String(rms, 1) + " dBFS");
            expect(l[0] > l[1] && l[1] > l[3] && l[2] > l[4] && rms > -50.0 && rms < -35.0);
        }

        beginTest("Crackle: the clicks per second follow the knob (0.5 -> 7.5 / s) within +-35 %");
        {
            auto p = magParams(mag::Vinyl, 0.0f);
            p.values[slot::kSatCrackle] = 0.5f;
            juce::AudioBuffer<float> silence(2, static_cast<int>(20.0 * kFs));
            silence.clear();
            const auto y = renderWith(silence, p);
            int clicks = 0, last = -100000;
            for (int i = 0; i < y.getNumSamples(); ++i)
                if (std::abs(y.getSample(0, i)) > 0.01f && i - last > static_cast<int>(0.005 * kFs))
                {
                    ++clicks;
                    last = i;
                }
            const double rate = clicks / 20.0;
            logMessage("  " + juce::String(rate, 2) + " clicks / s (expected 7.5)");
            expect(std::abs(rate / 7.5 - 1.0) <= 0.35);
        }

        beginTest("Noise FOLLOW: silent without input (< -100 dBFS after 1 s); ALWAYS: the hiss stays (> -80 dBFS)");
        {
            juce::AudioBuffer<float> silence(2, static_cast<int>(2.0 * kFs));
            silence.clear();
            auto p = magParams(mag::Tape, 0.0f);
            p.values[slot::kSatHiss] = 1.0f;
            p.values[slot::kSatNoiseFollow] = 1.0f;
            const double followed = rmsDb(renderWith(silence, p), 0, 48000, 48000);
            p.values[slot::kSatNoiseFollow] = 0.0f;
            const double always = rmsDb(renderWith(silence, p), 0, 48000, 48000);
            logMessage("  hiss without input: FOLLOW " + juce::String(followed, 1) + " dBFS, ALWAYS " + juce::String(always, 1) + " dBFS");
            expect(followed < -100.0 && always > -80.0);
        }

        beginTest("Auto gain: a -40 dBFS 1 kHz tone keeps unity +-0.5 dB at drive 12 dB, every model");
        {
            for (int model = mag::Tape; model < mag::NumModels; ++model)
            {
                const auto x = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 1000.0, -40.0);
                const auto y = renderWith(x, magParams(model, 12.0f));
                const double g = rmsDb(y, 0, 24000, 19200) - rmsDb(x, 0, 24000, 19200);
                logMessage("  " + juce::String(mag::modelName(model)) + ": " + juce::String(g, 3) + " dB");
                expect(std::abs(g) <= 0.5);
            }
        }

        beginTest("No DC: TUBE (asymmetric) at drive 24 dB, -6 dBFS: |mean| < 1e-3 after 0.5 s");
        {
            const auto y = renderWith(makeSine({ kFs, 2, 2.0, kFixtureSeed }, 220.0, -6.0), magParams(mag::Tube, 24.0f));
            double mean = 0.0;
            for (int i = 24000; i < y.getNumSamples(); ++i)
                mean += y.getSample(0, i);
            mean /= y.getNumSamples() - 24000;
            logMessage("  mean " + juce::String(mean, 6));
            expect(std::abs(mean) < 1.0e-3);
        }

        beginTest("Aliasing: GERMANIUM drive 24 dB on a 7 kHz tone: off-harmonic energy at 4x >= 10 dB below 1x");
        {
            const auto alias = [](int os)
            {
                constexpr int bin = 4779; // 7000.3 Hz
                const auto y = renderWith(binSine(bin, -6.0, 2.0), magParams(mag::Germanium, 24.0f, os));
                juce::dsp::FFT fft(kOrder);
                std::vector<float> frame(2 * static_cast<size_t>(kN), 0.0f);
                for (int i = 0; i < kN; ++i)
                    frame[static_cast<size_t>(i)] = y.getSample(0, kN / 2 + i);
                fft.performFrequencyOnlyForwardTransform(frame.data());
                double off = 0.0, total = 0.0;
                for (int k = 1; k <= kN / 2; ++k)
                {
                    const double e = static_cast<double>(frame[static_cast<size_t>(k)]) * frame[static_cast<size_t>(k)];
                    total += e;
                    bool harmonic = false;
                    for (int h = 1; h * bin <= kN / 2; ++h)
                        harmonic = harmonic || std::abs(k - h * bin) <= 2;
                    off += harmonic ? 0.0 : e;
                }
                return 10.0 * std::log10(off / total + 1e-30);
            };
            const double one = alias(0), four = alias(2);
            logMessage("  off-harmonic energy: 1x " + juce::String(one, 1) + " dB, 4x " + juce::String(four, 1) + " dB");
            expect(one - four >= 10.0);
        }

        beginTest("Display: harmonicProfile() equals the processor's harmonics within 1 dB (TUBE, -12 dBFS, drive 6, 4x)");
        {
            auto p = magParams(mag::Tube, 6.0f, 2);
            std::array<double, 9> profile {};
            mag::harmonicProfile(p, -12.0, profile);
            constexpr int bin = 683;
            const auto y = renderWith(binSine(bin, -12.0, 2.0), p);
            const auto l = binLevels(y, 0, kN, { bin, 2 * bin, 3 * bin });
            const double h2 = l[1] + 12.0, h3 = l[2] + 12.0; // dB re the input tone
            logMessage("  H2 display " + juce::String(profile[1], 2) + " / measured " + juce::String(h2, 2) + "; H3 " + juce::String(profile[2], 2) + " / "
                       + juce::String(h3, 2));
            expect(std::abs(profile[1] - h2) <= 1.0 && std::abs(profile[2] - h3) <= 1.0);
        }

        beginTest("Block sizes 1, 64, 127, 512 render identically (everything on: TAPE 9.5, wow, hiss, hum, crackle, steep, thump, tone); 0 allocations");
        {
            auto x = makePinkNoise({ kFs, 2, 1.0, kFixtureSeed }, -14.0);
            const auto params = [](int)
            {
                auto p = magParams(mag::Tape, 12.0f, 1);
                p.values[slot::kSatSpeed] = 1.0f;
                p.values[slot::kSatWow] = 0.8f;
                p.values[slot::kSatHiss] = 0.6f;
                p.values[slot::kSatHum] = 0.4f;
                p.values[slot::kSatCrackle] = 0.7f;
                p.values[slot::kSatSteep] = 1.0f;
                p.values[slot::kSatThump] = 1.0f;
                p.values[slot::kSatTone] = 0.4f;
                p.values[slot::kSatLowCutHz] = 60.0f;
                p.values[slot::kSatHighCutHz] = 9000.0f;
                p.values[slot::kSatPunish] = 1.0f;
                p.values[slot::kSatEvenOdd] = 0.5f;
                p.values[slot::kSatNoiseFollow] = 1.0f;
                return p;
            };
            std::uint64_t allocations = 0;
            const auto reference = render(x, 512, params, &allocations);
            for (int block : { 1, 64, 127 })
            {
                const auto y = render(x, block, params, &allocations);
                const auto null = compareAligned(y, reference);
                expect(null.bitExact, "block " + juce::String(block) + ": " + juce::String(null.errorDb, 1) + " dB");
            }
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("Stability: 20k random mutations of every saturator setting in the engine: finite, bounded, 0 allocations");
        {
            auto e = std::make_unique<LayerEngine>();
            e->prepare(kFs, 512);
            const auto noise = makeWhiteNoise({ kFs, 2, 4.0, kFixtureSeed }, -6.0);
            DeterministicRandom random(kFixtureSeed + 41);
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
                sl = magParams(static_cast<int>(uni(0, 5.99)), static_cast<float>(uni(0, 48)), static_cast<int>(uni(0, 2.99)));
                sl.values[slot::kCurve] = static_cast<float>(std::floor(uni(0, 2.99)));
                sl.values[slot::kBias] = static_cast<float>(uni(-0.9, 0.9));
                sl.values[slot::kTrimDb] = static_cast<float>(uni(-24, 24));
                sl.values[slot::kAutoGain] = uni(0, 1) > 0.5 ? 1.0f : 0.0f;
                sl.values[slot::kSatPunish] = uni(0, 1) > 0.7 ? 1.0f : 0.0f;
                sl.values[slot::kSatTone] = static_cast<float>(uni(-1, 1));
                sl.values[slot::kSatLowCutHz] = static_cast<float>(uni(10, 1000));
                sl.values[slot::kSatHighCutHz] = static_cast<float>(uni(1000, 20000));
                sl.values[slot::kSatThump] = uni(0, 1) > 0.5 ? 1.0f : 0.0f;
                sl.values[slot::kSatSteep] = uni(0, 1) > 0.5 ? 1.0f : 0.0f;
                sl.values[slot::kSatHarmonics] = static_cast<float>(uni(0, 1));
                sl.values[slot::kSatEvenOdd] = static_cast<float>(uni(-1, 1));
                sl.values[slot::kSatTapeBias] = static_cast<float>(uni(0, 1));
                sl.values[slot::kSatSpeed] = static_cast<float>(std::floor(uni(0, 2.99)));
                sl.values[slot::kSatWow] = static_cast<float>(uni(0, 1));
                sl.values[slot::kSatHiss] = static_cast<float>(uni(0, 1));
                sl.values[slot::kSatHum] = static_cast<float>(uni(0, 1));
                sl.values[slot::kSatCrackle] = static_cast<float>(uni(0, 1));
                sl.values[slot::kSatNoiseFollow] = uni(0, 1) > 0.5 ? 1.0f : 0.0f;
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

        beginTest("UI default saturator (defaultModule): TAPE, drive 6 dB, 2x");
        {
            const auto m = defaultModule(FxType::Saturator);
            expectEquals(static_cast<int>(m.parameters["model"]), static_cast<int>(mag::Tape));
            expectEquals(static_cast<int>(m.parameters["oversampling"]), 1);
            expect(std::abs(static_cast<double>(m.parameters["driveDb"]) - 6.0) < 1.0e-9);
        }

        beginTest("HamloMag page: fills the editor body; knob, switch, mix and ON edit the project; values come back; an old saturator shows Classic");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            FxNodeState n;
            n.id = 1;
            n.chain = { defaultModule(FxType::Saturator) };
            FxSlotState old;
            old.type = FxType::Saturator;
            old.parameters.set("driveDb", 17.0);
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
            auto* panel = findChild<ui::HamloMagPanel>(editor.get());
            expect(panel != nullptr, "no HamloMag panel on the saturator page");
            if (panel != nullptr)
            {
                expect(panel->getWidth() > 1000 && panel->getHeight() > 500, "panel does not fill the editor body: " + panel->getBounds().toString());
                expectEquals(panel->getValues().switches[ui::HamloMagPanel::Model], static_cast<int>(mag::Tape));
                panel->onKnobChange(ui::HamloMagPanel::Drive, 15.0);
                panel->onKnobChange(ui::HamloMagPanel::Hum, 0.4);
                panel->onKnobChange(ui::HamloMagPanel::Mix, 0.6);
                panel->onSwitchChange(ui::HamloMagPanel::Model, mag::Vinyl);
                panel->onSwitchChange(ui::HamloMagPanel::Speed, 2);
                panel->onPowerChange(false);
                const auto project = plugin->getProject();
                const auto& m = project.layers[0].fxNodes[0].chain[0];
                expectEquals(static_cast<double>(m.parameters["driveDb"]), 15.0);
                expectEquals(static_cast<double>(m.parameters["hum"]), 0.4);
                expectWithinAbsoluteError(static_cast<double>(m.mix), 0.6, 1.0e-6);
                expectEquals(static_cast<int>(m.parameters["model"]), static_cast<int>(mag::Vinyl));
                expectEquals(static_cast<int>(m.parameters["speed"]), 2);
                expect(!m.enabled);
                editor->selectNode(true, 1, 0);
                auto* rebuilt = findChild<ui::HamloMagPanel>(editor.get());
                expect(rebuilt != nullptr && rebuilt->getValues().knobs[ui::HamloMagPanel::Drive] == 15.0
                       && rebuilt->getValues().switches[ui::HamloMagPanel::Model] == mag::Vinyl && !rebuilt->getValues().on
                       && std::abs(rebuilt->getValues().knobs[ui::HamloMagPanel::Mix] - 0.6) < 1.0e-6);
            }
            editor->selectNode(true, 2, 0);
            auto* classicPage = findChild<ui::HamloMagPanel>(editor.get());
            expect(classicPage != nullptr && classicPage->getValues().switches[ui::HamloMagPanel::Model] == mag::Classic
                   && std::abs(classicPage->getValues().knobs[ui::HamloMagPanel::Drive] - 17.0) < 1.0e-9);
        }
    }
};

static SaturatorPassTests saturatorPassTests;
} // namespace hpeq::test
