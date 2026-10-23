// Module pass 8: HamloShirota stereo (STEREO_DESIGN.md §4).
// Thresholds fixed in the design note before the runs.

#include <cmath>
#include <complex>
#include <memory>

#include "DSP/FxNodeProcessor.h"
#include "DSP/ProcessorsM6.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/HamloShirotaPanel.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;
constexpr double kTwoPi = 2.0 * juce::MathConstants<double>::pi;

struct St
{
    float width = 1.0f, balance = 0.0f;
    int style = 0;
    float stereoize = 0.0f, space = 0.0f, monoBass = 0.0f, rotation = 0.0f;
    int safe = 0, mono = 0;
    float outputDb = 0.0f;
};

FxSlotParams st(const St& t)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::StereoProcessor;
    s.values[slot::kStereoWidth] = t.width;
    s.values[slot::kStereoBalance] = t.balance;
    s.values[slot::kStStyle] = static_cast<float>(t.style);
    s.values[slot::kStStereoize] = t.stereoize;
    s.values[slot::kStSpace] = t.space;
    s.values[slot::kStMonoBassHz] = t.monoBass;
    s.values[slot::kStRotation] = t.rotation;
    s.values[slot::kStSafe] = static_cast<float>(t.safe);
    s.values[slot::kStMonoListen] = static_cast<float>(t.mono);
    s.values[slot::kStOutputDb] = t.outputDb;
    return s;
}

juce::AudioBuffer<float> run(const juce::AudioBuffer<float>& x, const FxSlotParams& p, int block = 256, StereoProcessor* keep = nullptr)
{
    StereoProcessor local;
    auto& proc = keep != nullptr ? *keep : local;
    proc.prepare(kFs);
    proc.reset();
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        proc.process(c, 2, juce::jmin(block, y.getNumSamples() - start), p);
    }
    return y;
}

juce::AudioBuffer<float> monoOf(const juce::AudioBuffer<float>& x)
{
    juce::AudioBuffer<float> y(2, x.getNumSamples());
    for (int i = 0; i < x.getNumSamples(); ++i)
    {
        const float m = 0.5f * (x.getSample(0, i) + x.getSample(1, i));
        y.setSample(0, i, m);
        y.setSample(1, i, m);
    }
    return y;
}

double correlation(const juce::AudioBuffer<float>& y, int from, int length)
{
    double lr = 0, ll = 0, rr = 0;
    for (int i = from; i < from + length; ++i)
    {
        const double l = y.getSample(0, i), r = y.getSample(1, i);
        lr += l * r;
        ll += l * l;
        rr += r * r;
    }
    return ll * rr > 1.0e-30 ? lr / std::sqrt(ll * rr) : 1.0;
}

// RMS (dB) of the mid or the side of y over [from, from + length).
double partDb(const juce::AudioBuffer<float>& y, bool side, int from, int length)
{
    double e = 0;
    for (int i = from; i < from + length; ++i)
    {
        const double v = 0.5 * (y.getSample(0, i) + (side ? -1.0 : 1.0) * y.getSample(1, i));
        e += v * v;
    }
    return 10.0 * std::log10(e / length + 1.0e-30);
}

// |H| of an RBJ peaking filter, computed here independently.
double peakingDb(double f, double f0, double q, double gainDb)
{
    const double A = std::pow(10.0, gainDb / 40.0), w0 = kTwoPi * f0 / kFs, alpha = std::sin(w0) / (2.0 * q), cw = std::cos(w0);
    const double b0 = 1 + alpha * A, b1 = -2 * cw, b2 = 1 - alpha * A, a0 = 1 + alpha / A, a1 = -2 * cw, a2 = 1 - alpha / A;
    const std::complex<double> z = std::polar(1.0, -kTwoPi * f / kFs);
    const auto h = (b0 + b1 * z + b2 * z * z) / (a0 + a1 * z + a2 * z * z);
    return 20.0 * std::log10(std::abs(h));
}

juce::AudioBuffer<float> tone(double hz, double amp, bool side, double seconds = 1.0)
{
    juce::AudioBuffer<float> b(2, static_cast<int>(seconds * kFs));
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const auto v = static_cast<float>(amp * std::sin(kTwoPi * hz * i / kFs));
        b.setSample(0, i, v);
        b.setSample(1, i, side ? -v : v);
    }
    return b;
}
} // namespace

class StereoPassTests final : public juce::UnitTest
{
public:
    StereoPassTests() : juce::UnitTest("Stereo pass", "HPEQ") {}

    void runTest() override
    {
        const auto drums = makeSyntheticDrums({ kFs, 2, 3.0, kFixtureSeed });
        const auto monoDrums = monoOf(drums); // the synthetic drums are mono already
        const int n = drums.getNumSamples();
        // stereo test source: two independent pink noises; mono pink noise for the stereoize correlation
        auto stereoNoise = makePinkNoise({ kFs, 2, 3.0, kFixtureSeed }, -18.0);
        {
            const auto other = makePinkNoise({ kFs, 1, 3.0, kFixtureSeed + 1 }, -18.0);
            stereoNoise.copyFrom(1, 0, other, 0, 0, stereoNoise.getNumSamples());
        }
        const auto monoPink = monoOf(makePinkNoise({ kFs, 2, 3.0, kFixtureSeed }, -18.0));

        beginTest("S1. Keys absent = M6 (unchanged at width 100 %); WIDTH 0 -> L = R bit-exact (M6 and HamloShirota)");
        {
            expect(!StereoProcessor::usesShirota(st({})));
            expect(compareAligned(run(drums, st({})), drums).bitExact);
            for (int safe : { 0, 1 })
            {
                St t;
                t.width = 0.0f;
                t.safe = safe;
                const auto y = run(drums, st(t));
                bool equal = true;
                for (int i = 0; i < n; ++i)
                    equal = equal && y.getSample(0, i) == y.getSample(1, i);
                expect(equal, "safe " + juce::String(safe));
            }
        }

        beginTest("S2. WIDTH 200 %: side +6.02 +-0.01 dB, mid unchanged (<= 1e-6)");
        {
            for (int core : { 0, 1 })
            {
                St t;
                t.width = 2.0f;
                t.style = core; // PROSTOR with STEREOIZE 0 runs the HamloShirota core
                const auto y = run(stereoNoise, st(t)); // the drums are mono: the side needs a stereo source
                const double sideGain = partDb(y, true, 0, n) - partDb(stereoNoise, true, 0, n);
                double midError = 0.0;
                for (int i = 0; i < n; ++i)
                    midError = std::max(midError, std::abs(0.5 * (y.getSample(0, i) + y.getSample(1, i)) - 0.5 * (stereoNoise.getSample(0, i) + stereoNoise.getSample(1, i))));
                logMessage("  " + juce::String(core ? "HamloShirota" : "M6") + ": side " + juce::String(sideGain, 3) + " dB, worst mid error " + juce::String(midError));
                expect(std::abs(sideGain - 6.0206) <= 0.01);
                expect(midError <= 1.0e-6);
            }
        }

        beginTest("S3. STEREOIZE 100 % on mono drums: correlation <= 0.5; mono sum = input (PROSTOR) / input through the space bands (KOSMOS), <= -120 dB");
        {
            for (int style : { 1, 2 })
            {
                St t;
                t.style = style;
                t.stereoize = 1.0f;
                t.space = 0.6f;
                const auto y = run(monoDrums, st(t));
                const double corrDrums = correlation(y, 0, n);
                const double corr = correlation(run(monoPink, st(t)), 0, n); // the criterion (report §5)
                // the expected mono sum: the input's mid, through the space bands in KOSMOS (an independent cascade)
                juce::AudioBuffer<float> expected(1, n);
                for (int i = 0; i < n; ++i)
                    expected.setSample(0, i, monoDrums.getSample(0, i));
                if (style == 2)
                {
                    juce::dsp::IIR::Filter<double> f[3];
                    const double hz[] { 1000.0, 11000.0, 4500.0 }, q[] { 1.2, 1.2, 0.9 }, g[] { 3.6, 3.6, -3.6 };
                    for (int k = 0; k < 3; ++k)
                        f[k].coefficients = juce::dsp::IIR::Coefficients<double>::makePeakFilter(kFs, hz[k], q[k], juce::Decibels::decibelsToGain(g[k]));
                    for (int i = 0; i < n; ++i)
                    {
                        double v = expected.getSample(0, i);
                        for (auto& filter : f)
                            v = filter.processSample(v);
                        expected.setSample(0, i, static_cast<float>(v));
                    }
                }
                double err = 0.0, ref = 0.0;
                for (int i = 0; i < n; ++i)
                {
                    const double m = 0.5 * (y.getSample(0, i) + y.getSample(1, i)), e = expected.getSample(0, i);
                    err += (m - e) * (m - e);
                    ref += e * e;
                }
                const double errDb = 10.0 * std::log10(err / ref + 1.0e-300);
                logMessage("  style " + juce::String(style) + ": correlation on mono pink noise " + juce::String(corr, 3) + " (on the mono drums " + juce::String(corrDrums, 3)
                           + "), mono sum error " + juce::String(errDb, 1) + " dB");
                expect(corr <= 0.5);
                expect(errDb <= -120.0);
            }
        }

        beginTest("S4. KOSMOS space 100 %: mid response at 1 k / 4.5 k / 11 kHz = the formula +-0.1 dB; correlation < 0 with SAFE off");
        {
            St t;
            t.style = 2;
            t.space = 1.0f;
            for (double hz : { 1000.0, 4500.0, 11000.0 })
            {
                const auto x = tone(hz, 0.25, false);
                const auto y = run(x, st(t));
                const int from = static_cast<int>(0.5 * kFs), length = static_cast<int>(0.4 * kFs);
                const double measured = partDb(y, false, from, length) - partDb(x, false, from, length);
                const double formula = peakingDb(hz, 1000.0, 1.2, 6.0) + peakingDb(hz, 11000.0, 1.2, 6.0) + peakingDb(hz, 4500.0, 0.9, -6.0);
                logMessage("  " + juce::String(hz, 0) + " Hz: measured " + juce::String(measured, 3) + " dB, formula " + juce::String(formula, 3) + " dB");
                expect(std::abs(measured - formula) <= 0.1);
            }
            St wide;
            wide.style = 2;
            wide.width = 2.0f;
            wide.stereoize = 1.0f;
            wide.space = 1.0f;
            const double corr = correlation(run(monoPink, st(wide)), 0, n);
            logMessage("  KOSMOS width 200 %, stereoize 100 %, space 100 %, SAFE off, mono pink noise: correlation " + juce::String(corr, 3)
                       + " (mono drums " + juce::String(correlation(run(monoDrums, st(wide)), 0, n), 3) + ")");
            expect(corr < 0.0);
        }

        beginTest("S5. MONO BASS 120 Hz: side at 60 Hz <= -40 dB, at 480 Hz within +-0.2 dB; mid unchanged");
        {
            St t;
            t.style = 0;
            t.monoBass = 120.0f;
            const int from = static_cast<int>(0.5 * kFs), length = static_cast<int>(0.4 * kFs);
            const auto lo = tone(60.0, 0.25, true), hi = tone(480.0, 0.25, true), mid = tone(60.0, 0.25, false);
            const double atLo = partDb(run(lo, st(t)), true, from, length) - partDb(lo, true, from, length);
            const double atHi = partDb(run(hi, st(t)), true, from, length) - partDb(hi, true, from, length);
            const auto ym = run(mid, st(t));
            double midError = 0.0;
            for (int i = 0; i < mid.getNumSamples(); ++i)
                midError = std::max(midError, static_cast<double>(std::abs(ym.getSample(0, i) - mid.getSample(0, i))));
            logMessage("  side at 60 Hz " + juce::String(atLo, 2) + " dB, at 480 Hz " + juce::String(atHi, 3) + " dB, mid error " + juce::String(midError));
            expect(atLo <= -40.0);
            expect(std::abs(atHi) <= 0.2);
            expect(midError <= 1.0e-6);
        }

        beginTest("S6. SAFE: width 200 % + stereoize + KOSMOS, on drums and on independent noise: every 50 ms window's correlation >= -0.02");
        {
            auto noise = makeWhiteNoise({ kFs, 2, 3.0, kFixtureSeed }, -18.0);
            {
                juce::Random r(57);
                for (int i = 0; i < noise.getNumSamples(); ++i)
                    noise.setSample(1, i, (r.nextFloat() * 2.0f - 1.0f) * 0.2f); // R independent of L
            }
            for (const auto* source : std::array<const juce::AudioBuffer<float>*, 3> { &drums, &monoDrums, &noise })
            {
                St t;
                t.style = 2;
                t.width = 2.0f;
                t.stereoize = 1.0f;
                t.space = 1.0f;
                t.safe = 1;
                const auto y = run(*source, st(t));
                const int w = static_cast<int>(0.05 * kFs);
                // windows are compared with the input window the output came from (SAFE look-ahead: latency L)
                const int L = StereoProcessor::latencySamples(st(t), kFs);
                double worst = 1.0;
                int silent = 0;
                double worstRing = 1.0;
                for (int s = 0; s + w + L <= y.getNumSamples(); s += w)
                {
                    if (partDb(*source, false, s, w) < -80.0 && partDb(*source, true, s, w) < -80.0)
                    {
                        if (partDb(y, false, s + L, w) > -80.0)
                        {
                            ++silent;
                            worstRing = std::min(worstRing, correlation(y, s + L, w));
                        }
                        continue;
                    }
                    if (partDb(y, false, s + L, w) > -80.0)
                        worst = std::min(worst, correlation(y, s + L, w));
                }
                logMessage("  windows with a silent input that still sound (filter ring only): " + juce::String(silent) + ", their worst correlation "
                           + juce::String(worstRing, 3));
                logMessage("  worst 50 ms correlation " + juce::String(worst, 3));
                expect(worst >= -0.02);
            }
        }

        beginTest("S7. ROTATION 30 deg = the rotation matrix on the unrotated output; MONO lever: L = R = (L + R)/2 of the normal output (<= 1e-6)");
        {
            St t;
            t.style = 1;
            t.stereoize = 0.5f;
            const auto plain = run(drums, st(t));
            t.rotation = 30.0f;
            const auto turned = run(drums, st(t));
            const double c = std::cos(juce::degreesToRadians(30.0)), s = std::sin(juce::degreesToRadians(30.0));
            double worst = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const double l = plain.getSample(0, i), r = plain.getSample(1, i);
                worst = std::max({ worst, std::abs(turned.getSample(0, i) - (c * l - s * r)), std::abs(turned.getSample(1, i) - (s * l + c * r)) });
            }
            t.rotation = 0.0f;
            t.mono = 1;
            const auto mono = run(drums, st(t));
            double monoWorst = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const double m = 0.5 * (plain.getSample(0, i) + plain.getSample(1, i));
                monoWorst = std::max({ monoWorst, std::abs(mono.getSample(0, i) - m), std::abs(mono.getSample(1, i) - m) });
            }
            logMessage("  rotation worst " + juce::String(worst) + ", mono worst " + juce::String(monoWorst));
            expect(worst <= 1.0e-6);
            expect(monoWorst <= 1.0e-6);
        }

        beginTest("S8. Correlation meter = independent value +-0.02 (L and R 60 deg apart: 0.5); one channel: untouched, flag set");
        {
            juce::AudioBuffer<float> x(2, static_cast<int>(2.0 * kFs));
            for (int i = 0; i < x.getNumSamples(); ++i)
            {
                x.setSample(0, i, static_cast<float>(0.3 * std::sin(kTwoPi * 440.0 * i / kFs)));
                x.setSample(1, i, static_cast<float>(0.3 * std::sin(kTwoPi * 440.0 * i / kFs + kTwoPi / 6.0)));
            }
            StereoProcessor proc;
            St t;
            t.safe = 1; // the core, nothing else (side < mid here, so the guard does nothing)
            const auto y = run(x, st(t), 256, &proc);
            const double independent = correlation(y, x.getNumSamples() - static_cast<int>(0.5 * kFs), static_cast<int>(0.5 * kFs));
            const double meter = proc.shirota().meters().correlation;
            logMessage("  meter " + juce::String(meter, 3) + ", independent " + juce::String(independent, 3));
            expect(std::abs(meter - independent) <= 0.02 && std::abs(independent - 0.5) <= 0.02);

            auto one = drums;
            StereoProcessor single;
            single.prepare(kFs);
            float* c[1] { one.getWritePointer(0) };
            St wide;
            wide.style = 2;
            wide.width = 2.0f;
            single.process(c, 1, one.getNumSamples(), st(wide));
            bool same = true;
            for (int i = 0; i < n; ++i)
                same = same && one.getSample(0, i) == drums.getSample(0, i);
            expect(same && single.shirota().meters().oneChannel);
        }

        beginTest("S9. Latency 0 (SAFE: 2.5 ms look-ahead, reported = measured); blocks 1 / 64 / 512 identical; 0 allocations; OUTPUT +-6 dB exact");
        {
            St t;
            t.style = 2;
            t.width = 1.6f;
            t.stereoize = 0.7f;
            t.space = 0.6f;
            t.monoBass = 120.0f;
            t.rotation = 10.0f;
            t.safe = 1;
            const auto p = st(t);
            expectEquals(FxNodeProcessor::slotLatency(p, kFs), 120); // the SAFE look-ahead, 2.5 ms
            auto noSafe = t;
            noSafe.safe = 0;
            expectEquals(FxNodeProcessor::slotLatency(st(noSafe), kFs), 0);
            const auto noise = makeWhiteNoise({ kFs, 2, 0.5, kFixtureSeed }, -30.0);
            St mild; // SAFE on, nothing to guard: the output is the input L samples later
            mild.safe = 1;
            expectEquals(estimateDelay(run(noise, st(mild)), noise, 600), 120);
            const auto ref = run(drums, p, 1);
            for (int block : { 64, 512 })
                expect(compareAligned(run(drums, p, block), ref).bitExact, "block " + juce::String(block));
            StereoProcessor proc;
            proc.prepare(kFs);
            auto y = drums;
            std::uint64_t allocations = 0;
            for (int start = 0; start < y.getNumSamples(); start += 512)
            {
                float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                ScopedAllocationProbe probe;
                proc.process(c, 2, juce::jmin(512, y.getNumSamples() - start), p);
                allocations += probe.counts().allocations;
            }
            expectEquals(static_cast<int>(allocations), 0);
            for (float o : { -6.0f, 6.0f })
            {
                auto to = t;
                to.outputDb = o;
                const auto out = run(drums, st(to));
                const double k = std::pow(10.0, o / 20.0);
                double worst = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < n; ++i)
                        worst = std::max(worst, std::abs(out.getSample(ch, i) - ref.getSample(ch, i) * k));
                expect(worst <= 1.0e-6 * std::max(1.0, k));
            }
        }

        beginTest("S10. Page in the real editor: older stereo shows the M6 values; knobs edit; a Mid node shows OSHIBKA");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            FxNodeState a, b;
            a.id = 1;
            FxSlotState old;
            old.type = FxType::StereoProcessor; // saved before this pass: no HamloShirota keys
            old.parameters.set("width", 1.4);
            a.chain = { old };
            b.id = 2;
            b.lowHz = 2000.0;
            b.highHz = 6000.0;
            b.channel = ChannelMode::Mid;
            b.chain = { defaultModule(FxType::StereoProcessor) };
            p.layers[0].fxNodes = { a, b };
            p.nextNodeId = 3;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1400, 900);
            const auto find = [&editor]
            {
                ui::HamloShirotaPanel* panel = nullptr;
                juce::Array<juce::Component*> stack { editor.get() };
                while (!stack.isEmpty())
                {
                    auto* c = stack.removeAndReturn(stack.size() - 1);
                    for (auto* child : c->getChildren())
                        stack.add(child);
                    if (auto* w = dynamic_cast<ui::HamloShirotaPanel*>(c))
                        panel = w;
                }
                return panel;
            };
            editor->selectNode(true, 1, 0);
            auto* panel = find();
            expect(panel != nullptr, "no HamloShirota panel on the stereo page");
            if (panel != nullptr)
            {
                expect(panel->getWidth() > 1000 && panel->getHeight() > 500);
                const auto& v = panel->getValues();
                expect(v.knobs[ui::HamloShirotaPanel::Width] == 1.4 && v.switches[ui::HamloShirotaPanel::Safe] == 0 && !panel->showsError());
                panel->onKnobChange(ui::HamloShirotaPanel::Mode, 2.0);
                panel->onKnobChange(ui::HamloShirotaPanel::Space, 0.8);
                panel->onSwitchChange(ui::HamloShirotaPanel::Safe, 1);
                const auto project = plugin->getProject(); // a copy: keep it alive while reading from it
                const auto& m = project.layers[0].fxNodes[0].chain[0];
                expectEquals(static_cast<double>(m.parameters["style"]), 2.0);
                expectEquals(static_cast<double>(m.parameters["space"]), 0.8);
                expectEquals(static_cast<int>(m.parameters["safe"]), 1);
            }
            editor->selectNode(true, 2, 0);
            panel = find();
            expect(panel != nullptr && panel->showsError(), "a Mid node must show OSHIBKA");
        }
    }
};

static StereoPassTests stereoPassTests;
} // namespace hpeq::test
