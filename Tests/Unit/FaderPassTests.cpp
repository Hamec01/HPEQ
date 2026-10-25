// Module pass 9 (owner): HamloFader — the Gain module as a band level utility (gain, polarity, mute,
// solo, delay, MATCH, meters) and the owner's module names in the plugin. Thresholds fixed before the runs.

#include <cmath>
#include <memory>

#include "DSP/LayerEngine.h"
#include "DSP/Processors.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/HamloFaderPanel.h"
#include "UI/Theme.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;

FxSlotParams fader(float gainDb, int polarity = 0, float delayMs = 0.0f, bool mute = false, bool solo = false)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Gain;
    s.values[slot::kGainDb] = gainDb;
    s.values[slot::kFadPolarity] = static_cast<float>(polarity);
    s.values[slot::kFadDelayMs] = delayMs;
    s.values[slot::kFadMute] = mute ? 1.0f : 0.0f;
    s.values[slot::kFadSolo] = solo ? 1.0f : 0.0f;
    s.values[slot::kFadTargetDb] = -18.0f;
    return s;
}

template <typename Fn>
juce::AudioBuffer<float> run(const juce::AudioBuffer<float>& x, int block, Fn&& paramsAt, GainProcessor* keep = nullptr, std::uint64_t* allocations = nullptr)
{
    GainProcessor own;
    auto& g = keep != nullptr ? *keep : own;
    g.prepare(kFs);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        const auto p = paramsAt(start);
        ScopedAllocationProbe probe;
        g.process(chunk, 2, n, p);
        if (allocations != nullptr)
            *allocations += probe.counts().allocations;
    }
    return y;
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

double bandEnergyDb(const juce::AudioBuffer<float>& y, double lo, double hi, int from)
{
    // energy of channel 0 between lo and hi (FFT of a 2^15 frame)
    constexpr int order = 15, n = 1 << order;
    juce::dsp::FFT fft(order);
    std::vector<float> frame(2 * static_cast<size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i)
        frame[static_cast<size_t>(i)] = y.getSample(0, from + i) * static_cast<float>(0.5 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi * i / n));
    fft.performFrequencyOnlyForwardTransform(frame.data());
    double e = 0.0;
    for (int k = 1; k <= n / 2; ++k)
        if (const double f = k * kFs / n; f >= lo && f <= hi)
            e += static_cast<double>(frame[static_cast<size_t>(k)]) * frame[static_cast<size_t>(k)];
    return 10.0 * std::log10(e + 1e-30);
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

class FaderPassTests final : public juce::UnitTest
{
public:
    FaderPassTests() : juce::UnitTest("Fader pass", "HPEQ") {}

    void runTest() override
    {
        beginTest("Old law: gainDb only (and every new setting at rest) is y = g x bit for bit; 0 dB is an identity");
        {
            const auto x = makeWhiteNoise({ kFs, 2, 1.0, kFixtureSeed }, -12.0);
            for (float db : { 0.0f, -6.0f, 7.5f })
            {
                const auto y = run(x, 300, [db](int) { return fader(db); });
                auto expected = x;
                if (db != 0.0f)
                    expected.applyGain(std::pow(10.0f, db / 20.0f));
                expect(compareAligned(y, expected).bitExact, juce::String(db, 1) + " dB");
            }
        }

        // Disclosed test fix (report): 150 ms was inconsistent with the documented 20 ms glide (0.01 dB needs
        // ~8 time constants = 160 ms); measured at 200 ms.
        beginTest("Gain change 0 -> -12 dB glides and settles within 0.01 dB after 200 ms");
        {
            const auto x = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 100.0, -6.0);
            const auto y = run(x, 64, [](int start) { return fader(start < 24000 ? 0.0f : -12.0f); });
            const double settled = 20.0 * std::log10(y.getSample(0, 24000 + 9600 + 120) / x.getSample(0, 24000 + 9600 + 120));
            logMessage("  settled at " + juce::String(settled, 4) + " dB");
            expect(std::abs(settled + 12.0) < 0.01);
        }

        beginTest("Mute: < -120 dBFS after 100 ms; unmute comes back to unity within 0.01 dB");
        {
            const auto x = makeWhiteNoise({ kFs, 2, 1.0, kFixtureSeed }, -12.0);
            const auto y = run(x, 128, [](int start) { return fader(0.0f, 0, 0.0f, start >= 12000 && start < 30000); });
            double peak = 0.0;
            for (int i = 12000 + 4800; i < 30000; ++i)
                peak = std::max(peak, static_cast<double>(std::abs(y.getSample(0, i))));
            const double back = std::abs(y.getSample(0, 46000) - x.getSample(0, 46000));
            logMessage("  muted peak " + juce::String(20.0 * std::log10(peak + 1e-30), 1) + " dBFS, error after unmute " + juce::String(back, 8));
            expect(peak < 1.0e-6 && back < 1.0e-6);
        }

        beginTest("Polarity: left only inverts the left channel (exact after the 10 ms ramp); both inverts both");
        {
            const auto x = makeWhiteNoise({ kFs, 2, 0.5, kFixtureSeed }, -12.0);
            for (int pol : { 1, 3 })
            {
                const auto y = run(x, 256, [pol](int start) { return fader(0.0f, start < 4800 ? 0 : pol); });
                double errL = 0.0, errR = 0.0;
                for (int i = 4800 + 960; i < x.getNumSamples(); ++i)
                {
                    errL = std::max(errL, std::abs(static_cast<double>(y.getSample(0, i)) + x.getSample(0, i)));
                    errR = std::max(errR, std::abs(static_cast<double>(y.getSample(1, i)) - (pol == 3 ? -1.0 : 1.0) * x.getSample(1, i)));
                }
                expect(errL < 1.0e-6 && errR < 1.0e-6, "polarity " + juce::String(pol));
            }
        }

        beginTest("Delay 5 ms: the band arrives 240 samples later (+-1)");
        {
            const auto x = makeWhiteNoise({ kFs, 2, 1.0, kFixtureSeed }, -12.0);
            const auto y = run(x, 512, [](int) { return fader(0.0f, 0, 5.0f); });
            const int lag = estimateDelay(y, x, 600);
            logMessage("  lag " + juce::String(lag));
            expect(std::abs(lag - 240) <= 1);
        }

        beginTest("Meters: -20 dBFS RMS noise reads -20 +-0.5 (3 s input); +6 dB of gain reads +6 more at the output +-0.5");
        {
            const auto x = makeWhiteNoise({ kFs, 2, 12.0, kFixtureSeed }, -20.0);
            GainProcessor g;
            run(x, 512, [](int) { return fader(6.0f); }, &g);
            logMessage("  input (3 s) " + juce::String(g.inputLongDb(), 2) + ", input (300 ms) " + juce::String(g.inputDb(), 2) + ", output " + juce::String(g.outputDb(), 2));
            expect(std::abs(g.inputLongDb() + 20.0f) < 0.5f);
            expect(std::abs(g.outputDb() - g.inputDb() - 6.0f) < 0.5f);
        }

        beginTest("MATCH: the gain becomes target - band level (-18 target, -24 band -> +6 dB); nothing measured -> unchanged");
        {
            expectWithinAbsoluteError(ui::HamloFaderPanel::matchGain(-18.0, -24.0), 6.0, 1.0e-12);
            expectWithinAbsoluteError(ui::HamloFaderPanel::matchGain(-18.0, -130.0), 0.0, 1.0e-12);
            ui::HamloFaderPanel panel;
            double set = 999.0;
            panel.readMeters = [] { return ui::HamloFaderPanel::Meters { -24.0f, -24.0f, -30.0f }; };
            panel.onKnobChange = [&set](ui::HamloFaderPanel::Param p, double v) { if (p == ui::HamloFaderPanel::Gain) set = v; };
            panel.pressMatch();
            expectWithinAbsoluteError(set, 12.0, 1.0e-9);
        }

        beginTest("SOLO: with a fader soloed on 2-8 kHz the layer plays that band only (below 1 kHz >= 30 dB down); solo off = the plain render, bit-exact");
        {
            const auto x = makePinkNoise({ kFs, 2, 2.0, kFixtureSeed }, -18.0);
            const auto render = [&x](bool solo, bool soloKeyPresent)
            {
                auto e = std::make_unique<LayerEngine>();
                e->prepare(kFs, 512);
                LayerSnapshot s;
                s.fx[0] = nodeWith(100.0f, 400.0f, { fader(-3.0f) });
                auto high = fader(0.0f, 0, 0.0f, false, solo);
                if (!soloKeyPresent)
                    high.values[slot::kFadSolo] = 0.0f;
                s.fx[1] = nodeWith(2000.0f, 8000.0f, { high });
                e->publish(s);
                auto y = x;
                for (int start = 0; start < y.getNumSamples(); start += 512)
                {
                    float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    e->process(chunk, 2, juce::jmin(512, y.getNumSamples() - start));
                }
                return y;
            };
            const auto normal = render(false, true), soloed = render(true, true), plain = render(false, false);
            const double lowNormal = bandEnergyDb(normal, 20.0, 1000.0, 48000), lowSolo = bandEnergyDb(soloed, 20.0, 1000.0, 48000);
            // Solo plays the band exactly as the crossovers cut it (LR skirts: -0.26 dB even at 4 kHz, more towards
            // 2 / 8 kHz). Disclosed: windows 2.5-7 and 3.2-5 kHz read the skirts (-1.5 / -1.0 dB); compared at the centre.
            const double bandNormal = bandEnergyDb(normal, 3600.0, 4400.0, 48000), bandSolo = bandEnergyDb(soloed, 3600.0, 4400.0, 48000);
            logMessage("  below 1 kHz: normal " + juce::String(lowNormal, 1) + " dB, solo " + juce::String(lowSolo, 1) + " dB; 3.6-4.4 kHz: " + juce::String(bandNormal, 1)
                       + " / " + juce::String(bandSolo, 1) + " dB");
            expect(lowNormal - lowSolo >= 30.0);
            expect(std::abs(bandNormal - bandSolo) <= 1.0);
            expect(compareAligned(normal, plain).bitExact);
        }

        beginTest("Block sizes 1, 64, 127, 512 identical (gain +4 dB, left inverted, delay 3.3 ms); 0 allocations");
        {
            const auto x = makePinkNoise({ kFs, 2, 1.0, kFixtureSeed }, -14.0);
            const auto params = [](int) { return fader(4.0f, 1, 3.3f); };
            std::uint64_t allocations = 0;
            GainProcessor a;
            const auto reference = run(x, 512, params, &a, &allocations);
            for (int block : { 1, 64, 127 })
            {
                GainProcessor b;
                const auto y = run(x, block, params, &b, &allocations);
                expect(compareAligned(y, reference).bitExact, "block " + juce::String(block));
            }
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("Names: the plugin shows the owner's names with the function next to them");
        {
            expect(ui::fxFullName(FxType::Gain) == juce::String::fromUTF8("HamloFader \xc2\xb7 Band level"));
            expect(ui::fxName(FxType::StereoProcessor) == "HamloShirota" && ui::fxName(FxType::Limiter) == "HamloRedWall");
        }

        beginTest("HamloFader page: fills the body; fader, levers, MIX and ON edit the project; values come back");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            FxNodeState n;
            n.id = 1;
            FxSlotState g;
            g.type = FxType::Gain;
            n.chain = { g };
            p.layers[0].fxNodes = { n };
            p.nextNodeId = 2;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1400, 900);
            editor->selectNode(true, 1, 0);
            auto* panel = findChild<ui::HamloFaderPanel>(editor.get());
            expect(panel != nullptr, "no HamloFader page");
            if (panel != nullptr)
            {
                expect(panel->getWidth() > 1000 && panel->getHeight() > 500);
                panel->onKnobChange(ui::HamloFaderPanel::Gain, -4.5);
                panel->onKnobChange(ui::HamloFaderPanel::Delay, 2.0);
                panel->onKnobChange(ui::HamloFaderPanel::Mix, 0.5);
                panel->onSwitchChange(ui::HamloFaderPanel::Polarity, 2);
                panel->onSwitchChange(ui::HamloFaderPanel::Solo, 1);
                panel->onPowerChange(false);
                const auto project = plugin->getProject();
                const auto& m = project.layers[0].fxNodes[0].chain[0];
                expectEquals(static_cast<double>(m.parameters["gainDb"]), -4.5);
                expectEquals(static_cast<double>(m.parameters["delayMs"]), 2.0);
                expectEquals(static_cast<int>(m.parameters["polarity"]), 2);
                expectEquals(static_cast<int>(m.parameters["solo"]), 1);
                expectWithinAbsoluteError(static_cast<double>(m.mix), 0.5, 1.0e-6);
                expect(!m.enabled);
                editor->selectNode(true, 1, 0);
                auto* rebuilt = findChild<ui::HamloFaderPanel>(editor.get());
                expect(rebuilt != nullptr && rebuilt->getValues().knobs[ui::HamloFaderPanel::Gain] == -4.5
                       && rebuilt->getValues().switches[ui::HamloFaderPanel::Polarity] == 2 && !rebuilt->getValues().on);
            }
        }
    }
};

static FaderPassTests faderPassTests;
} // namespace hpeq::test
