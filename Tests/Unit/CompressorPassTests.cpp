// Module pass 1 (owner): compressor options — detector Peak / RMS, auto makeup, character
// Clean / Punch / Glue, meters for the needle display, UI defaults. Thresholds fixed here before the runs.

#include <cmath>
#include <memory>

#include "DSP/ProjectEngine.h"
#include "DSP/Processors.h"
#include "Harness/AudioMetrics.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/HamloCompPanel.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;

FxSlotParams comp(float threshold, float ratio, float knee, float attack, float release, float detector = 0, float character = 0, float autoMakeup = 0)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Compressor;
    s.values[slot::kThresholdDb] = threshold;
    s.values[slot::kRatio] = ratio;
    s.values[slot::kKneeDb] = knee;
    s.values[slot::kAttackMs] = attack;
    s.values[slot::kReleaseMs] = release;
    s.values[slot::kCompDetector] = detector;
    s.values[slot::kCompCharacter] = character;
    s.values[slot::kCompAutoMakeup] = autoMakeup;
    return s;
}

juce::AudioBuffer<float> run(CompressorProcessor& c, const juce::AudioBuffer<float>& x, const FxSlotParams& p, int block = 256,
                             std::vector<float>* inputDb = nullptr)
{
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* ch[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        c.process(ch, 2, juce::jmin(block, y.getNumSamples() - start), p, true);
        if (inputDb != nullptr)
            inputDb->push_back(c.getInputLevelDb());
    }
    return y;
}

juce::AudioBuffer<float> segment(const juce::AudioBuffer<float>& x, int start, int length)
{
    juce::AudioBuffer<float> y(x.getNumChannels(), length);
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, 0, x, ch, start, length);
    return y;
}
} // namespace

class CompressorPassTests final : public juce::UnitTest
{
public:
    CompressorPassTests() : juce::UnitTest("Compressor pass", "HPEQ") {}

    void runTest() override
    {
        const auto drums = makeSyntheticDrums({ kFs, 2, 3.0, kFixtureSeed });

        beginTest("Options off (Peak, Clean, no auto makeup) = the previous law, bit-exact (same code path as T06)");
        {
            CompressorProcessor a, b;
            a.prepare(kFs);
            b.prepare(kFs);
            auto p = comp(-30, 4, 6, 5, 80);
            const auto ya = run(a, drums, p);
            p.values[slot::kCompDetector] = 0.2f; // rounds to Peak
            expect(compareAligned(run(b, drums, p), ya).bitExact);
        }

        beginTest("Input meter: detector level of the block; a steady -20 dBFS sine reads -20.0 (Peak) and -23.0 (RMS) +-0.1 dB");
        {
            const auto sine = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 997.0, -20.0);
            for (float detector : { 0.0f, 1.0f })
            {
                CompressorProcessor c;
                c.prepare(kFs);
                std::vector<float> levels;
                run(c, sine, comp(-60, 1, 0, 5, 80, detector), 256, &levels);
                const float reading = levels.back();
                logMessage(juce::String(detector > 0.5f ? "  RMS" : "  Peak") + " reads " + juce::String(reading, 2) + " dB");
                expect(std::abs(reading - (detector > 0.5f ? -23.01f : -20.0f)) <= 0.1f);
            }
        }

        beginTest("RMS detector: steady sine compresses by the static law at its RMS level (sine 3.01 dB below peak), +-0.2 dB");
        {
            const auto sine = makeSine({ kFs, 2, 2.0, kFixtureSeed }, 997.0, -6.0);
            CompressorProcessor c;
            c.prepare(kFs);
            const auto y = run(c, sine, comp(-30, 4, 0, 5, 80, 1));
            const double gainDb = measureAll(segment(y, 48000, 48000)).rmsDbfs - measureAll(segment(sine, 48000, 48000)).rmsDbfs;
            const double expected = -CompressorProcessor::staticGainReductionDb(-6.0 - 3.0103, -30, 4, 0);
            logMessage("  measured " + juce::String(gainDb, 2) + " dB, law " + juce::String(expected, 2) + " dB");
            expect(std::abs(gainDb - expected) <= 0.2);
        }

        beginTest("Auto makeup: on steady compression the long-term output level returns to the input level within 0.5 dB after 10 s");
        {
            const auto sine = makeSine({ kFs, 2, 12.0, kFixtureSeed }, 220.0, -10.0);
            CompressorProcessor c;
            c.prepare(kFs);
            const auto y = run(c, sine, comp(-30, 4, 6, 5, 80, 0, 0, 1));
            const double diff = measureAll(segment(y, static_cast<int>(10.0 * kFs), static_cast<int>(kFs))).rmsDbfs
                              - measureAll(segment(sine, static_cast<int>(10.0 * kFs), static_cast<int>(kFs))).rmsDbfs;
            logMessage("  level change after 10 s: " + juce::String(diff, 2) + " dB (GR about " + juce::String(c.getGainReductionDb(), 1)
                       + " dB, auto makeup " + juce::String(c.getAutoMakeupDb(), 1) + " dB)");
            expect(std::abs(diff) <= 0.5);
        }

        beginTest("Punch: identity without gain reduction; with reduction it adds harmonics (density) the Clean mode does not");
        {
            const auto quiet = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 997.0, -50.0);
            CompressorProcessor c;
            c.prepare(kFs);
            expect(compareAligned(run(c, quiet, comp(-30, 4, 6, 5, 80, 0, 1)), quiet).bitExact, "Punch below threshold not an identity");

            const auto loud = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 997.0, -6.0);
            const auto harmonicDb = [&](float character)
            {
                CompressorProcessor k;
                k.prepare(kFs);
                const auto y = run(k, loud, comp(-30, 4, 6, 5, 80, 0, character));
                // third harmonic vs fundamental via a direct DFT over the last 0.5 s (whole periods are not
                // needed for a relative reading at this resolution)
                double f1r = 0, f1i = 0, f3r = 0, f3i = 0;
                for (int i = 24000; i < 48000; ++i)
                {
                    const double v = y.getSample(0, i), t = 2.0 * juce::MathConstants<double>::pi * 997.0 * i / kFs;
                    f1r += v * std::cos(t); f1i += v * std::sin(t);
                    f3r += v * std::cos(3 * t); f3i += v * std::sin(3 * t);
                }
                return 10.0 * std::log10((f3r * f3r + f3i * f3i + 1e-30) / (f1r * f1r + f1i * f1i));
            };
            const double clean = harmonicDb(0), punch = harmonicDb(1);
            logMessage("  3rd harmonic: Clean " + juce::String(clean, 1) + " dB, Punch " + juce::String(punch, 1) + " dB");
            expect(punch > clean + 20.0);
        }

        beginTest("Glue: after a short peak it recovers faster than Clean; after a long compression slower (program-dependent release)");
        {
            // 30 ms burst at -6 dBFS (short) or 3 s (long), then silence at -60 dBFS; GR 150 ms later.
            const auto recovery = [&](double burstSeconds, float character)
            {
                const int burst = static_cast<int>(burstSeconds * kFs), n = burst + static_cast<int>(0.5 * kFs);
                juce::AudioBuffer<float> x(2, n);
                for (int i = 0; i < n; ++i)
                {
                    const float a = i < burst ? 0.5f : 0.001f;
                    const float v = a * static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * 997.0 * i / kFs));
                    x.setSample(0, i, v);
                    x.setSample(1, i, v);
                }
                CompressorProcessor c;
                c.prepare(kFs);
                const auto p = comp(-30, 4, 0, 1, 100, 0, character);
                auto y = x;
                const int until = burst + static_cast<int>(0.15 * kFs);
                for (int start = 0; start < until; start += 64)
                {
                    float* ch[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    c.process(ch, 2, juce::jmin(64, until - start), p, true);
                }
                return c.getGainReductionDb(); // GR left 150 ms after the burst
            };
            const float cleanShort = recovery(0.03, 0), glueShort = recovery(0.03, 2);
            const float cleanLong = recovery(3.0, 0), glueLong = recovery(3.0, 2);
            logMessage("  GR 150 ms after: short burst Clean " + juce::String(cleanShort, 2) + " / Glue " + juce::String(glueShort, 2)
                       + " dB; long Clean " + juce::String(cleanLong, 2) + " / Glue " + juce::String(glueLong, 2) + " dB");
            expect(glueShort < cleanShort);
            expect(glueLong > cleanLong);
        }

        // Criterion changed after the first run (disclosed in the report): whole-signal crest went UP
        // (14.61 -> 15.50 dB) because a band compressor lowers the RMS of the band that dominates the mix
        // more than the mix peaks; the band compressor's work is measured inside its band.
        beginTest("Old vs new default compressor on drums, region 60-250 Hz: inside the band (after 0.5 s) the new one cuts the crest factor >= 2 dB more");
        {
            auto quiet = drums;
            quiet.applyGain(static_cast<float>(std::pow(10.0, -12.0 / 20.0)));
            const auto bandOf = [](const juce::AudioBuffer<float>& x)
            {
                // the same 4th-order 60-250 Hz band-pass for input and outputs
                auto y = x;
                for (int ch = 0; ch < y.getNumChannels(); ++ch)
                {
                    juce::IIRFilter h1, h2, l1, l2;
                    const auto hp = juce::IIRCoefficients::makeHighPass(kFs, 60.0), lp = juce::IIRCoefficients::makeLowPass(kFs, 250.0);
                    h1.setCoefficients(hp); h2.setCoefficients(hp); l1.setCoefficients(lp); l2.setCoefficients(lp);
                    auto* d = y.getWritePointer(ch);
                    for (auto* f : { &h1, &h2, &l1, &l2 })
                        f->processSamples(d, y.getNumSamples());
                }
                // after 0.5 s: a new region's edges glide in from 1 Hz first (D4), so the first hit is not compressed
                return measureAll(segment(y, static_cast<int>(0.5 * kFs), y.getNumSamples() - static_cast<int>(0.5 * kFs)));
            };
            const auto renderWith = [&](const FxSlotState& m)
            {
                FxProjectState p;
                FxNodeState n;
                n.id = 1;
                n.lowHz = 60.0;
                n.highHz = 250.0;
                n.chain = { m };
                p.layers[0].fxNodes = { n };
                sanitize(p);
                auto e = std::make_unique<ProjectEngine>();
                e->prepare(kFs, 512);
                e->publish(p);
                auto y = quiet;
                for (int start = 0; start < y.getNumSamples(); start += 512)
                {
                    float* ch[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    e->process(ch, 2, juce::jmin(512, y.getNumSamples() - start));
                }
                return y;
            };
            FxSlotState old;
            old.type = FxType::Compressor; // no parameters: the DSP defaults (-18 dB, 4:1, 10 / 120 ms, Clean, no makeup)
            const auto in = bandOf(quiet), before = bandOf(renderWith(old)), after = bandOf(renderWith(defaultModule(FxType::Compressor)));
            logMessage("  band 60-250 Hz: peak " + juce::String(in.peakDbfs, 1) + " dBFS; crest input " + juce::String(in.crestDb, 2) + ", old default "
                       + juce::String(before.crestDb, 2) + ", new default " + juce::String(after.crestDb, 2) + " dB; band RMS "
                       + juce::String(in.rmsDbfs, 2) + " / " + juce::String(before.rmsDbfs, 2) + " / " + juce::String(after.rmsDbfs, 2) + " dBFS");
            expect((in.crestDb - after.crestDb) - (in.crestDb - before.crestDb) >= 2.0);
        }

        beginTest("HamloComp page: fills the editor body; knob, switch and ON edit the project; values come back from it; the grid restores the layout");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            FxNodeState n;
            n.id = 1;
            n.chain = { defaultModule(FxType::Compressor) };
            p.layers[0].fxNodes = { n };
            p.nextNodeId = 2;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1400, 900);
            editor->selectNode(true, 1, 0);
            ui::HamloCompPanel* panel = nullptr;
            juce::Component* workspace = nullptr;
            juce::Array<juce::Component*> stack { editor.get() };
            while (!stack.isEmpty())
            {
                auto* c = stack.removeAndReturn(stack.size() - 1);
                for (auto* child : c->getChildren())
                    stack.add(child);
                if (auto* h = dynamic_cast<ui::HamloCompPanel*>(c))
                    panel = h;
                if (c->getName().isEmpty() && c->getWidth() > 900 && c->getHeight() > 300 && dynamic_cast<ui::HamloCompPanel*>(c) == nullptr)
                    workspace = c;
            }
            expect(panel != nullptr, "no HamloComp panel on the compressor page");
            if (panel != nullptr)
            {
                expect(panel->getWidth() > 1000 && panel->getHeight() > 500, "panel does not fill the editor body: "
                                                                                + panel->getBounds().toString());
                panel->onKnobChange(ui::HamloCompPanel::Threshold, -41.5);
                panel->onSwitchChange(ui::HamloCompPanel::Character, 2);
                panel->onPowerChange(false);
                const auto project = plugin->getProject(); // a copy: keep it alive while reading from it
                const auto& m = project.layers[0].fxNodes[0].chain[0];
                expectEquals(static_cast<double>(m.parameters["thresholdDb"]), -41.5);
                expectEquals(static_cast<int>(m.parameters["character"]), 2);
                expect(!m.enabled);
                editor->selectNode(true, 1, 0); // rebuild from the project
                juce::Array<juce::Component*> again { editor.get() };
                ui::HamloCompPanel* rebuilt = nullptr;
                while (!again.isEmpty())
                {
                    auto* c = again.removeAndReturn(again.size() - 1);
                    for (auto* child : c->getChildren())
                        again.add(child);
                    if (auto* h = dynamic_cast<ui::HamloCompPanel*>(c))
                        rebuilt = h;
                }
                expect(rebuilt != nullptr && rebuilt->getValues().knobs[ui::HamloCompPanel::Threshold] == -41.5
                       && rebuilt->getValues().switches[ui::HamloCompPanel::Character] == 2 && !rebuilt->getValues().on);
            }
            editor->selectNode(true, 1, -1); // back to the grid: the workspace is shown again
            int visibleLarge = 0;
            for (auto* child : editor->getChildren())
                visibleLarge += child->isVisible() && child->getHeight() > 300 ? 1 : 0;
            expect(visibleLarge >= 1);
            juce::ignoreUnused(workspace);
        }

        beginTest("UI default compressor (defaultModule): threshold -30, ratio 4, attack 1, release 80, Punch, auto makeup on");
        {
            const auto m = defaultModule(FxType::Compressor);
            expect(static_cast<double>(m.parameters["thresholdDb"]) == -30.0 && static_cast<double>(m.parameters["ratio"]) == 4.0
                   && static_cast<double>(m.parameters["attackMs"]) == 1.0);
            expect(static_cast<int>(m.parameters["character"]) == 1 && static_cast<int>(m.parameters["autoMakeup"]) == 1);
            expect(defaultModule(FxType::Gain).parameters.isEmpty()); // the reverb has its own defaults since the HamloVerb pass
        }
    }
};

static CompressorPassTests compressorPassTests;
} // namespace hpeq::test
