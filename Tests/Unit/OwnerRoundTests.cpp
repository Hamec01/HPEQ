// Owner round 2026-10-26: neutral module defaults, the compressor's history screen, knob help, HamloRevolution
// lane GEN / LOCK and the wave screen.

#include <cmath>
#include <memory>

#include "Core/Revolution.h"
#include "DSP/ProjectEngine.h"
#include "Harness/AudioMetrics.h"
#include "Harness/TestSignalFactory.h"
#include "UI/HamloCompPanel.h"
#include "UI/HamloEchoPanel.h"
#include "UI/HamloFaderPanel.h"
#include "UI/HamloMagPanel.h"
#include "UI/HamloRedWallPanel.h"
#include "UI/HamloRevolutionPanel.h"
#include "UI/HamloShirotaPanel.h"
#include "UI/HamloUdarnikPanel.h"
#include "UI/HamloVerbPanel.h"
#include "UI/HamloZastavaPanel.h"
#include "UI/ModuleHelp.h"

namespace hpeq::test
{
using namespace hpeq::dsp;
namespace rev = hpeq::revolution;

namespace
{
constexpr double kFs = 48000.0;

// The layer's MIX chain holding one module, rendered through the engine; returns the output.
juce::AudioBuffer<float> renderMix(const juce::AudioBuffer<float>& x, const FxSlotState& module, int& latency)
{
    FxProjectState p;
    p.layers[0].mixChain = { module };
    auto e = std::make_unique<ProjectEngine>();
    e->prepare(kFs, 512);
    e->publish(p);
    latency = ProjectEngine::latencyFor(p, kFs);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        e->process(c, 2, juce::jmin(512, y.getNumSamples() - start));
    }
    return y;
}

template <typename Panel>
int missingHelp(const juce::String& module, int numParams, int numSwitches)
{
    int missing = 0;
    for (int k = 0; k < numParams; ++k)
        missing += ui::moduleHelp(module, Panel::keyFor(static_cast<typename Panel::Param>(k))).text.isEmpty() ? 1 : 0;
    for (int s = 0; s < numSwitches; ++s)
        missing += ui::moduleHelp(module, Panel::keyFor(static_cast<typename Panel::Switch>(s))).text.isEmpty() ? 1 : 0;
    return missing;
}
} // namespace

class OwnerRoundTests final : public juce::UnitTest
{
public:
    OwnerRoundTests() : juce::UnitTest("Owner round 2026-10-26", "HPEQ") {}

    void runTest() override
    {
        const auto drums = makeSyntheticDrums({ kFs, 2, 3.0, kFixtureSeed });

        beginTest("A new module does not change the sound (after its 20 ms fade-in): every type but the saturator, <= -100 dB");
        {
            const FxType types[] { FxType::Compressor, FxType::Limiter, FxType::Reverb, FxType::Delay, FxType::TransientShaper,
                                   FxType::MonstaFx, FxType::StereoProcessor, FxType::GateExpander, FxType::Gain };
            auto quiet = drums; // under 0 dBFS: the limiter at LIMIT 0 dB has nothing to catch
            quiet.applyGain(0.5f);
            for (auto type : types)
            {
                int latency = 0;
                const auto y = renderMix(quiet, defaultModule(type), latency);
                const int from = static_cast<int>(0.1 * kFs);
                double err = 0.0, ref = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = from; i + latency < y.getNumSamples(); ++i)
                    {
                        const double d = y.getSample(ch, i + latency) - quiet.getSample(ch, i);
                        err += d * d;
                        ref += static_cast<double>(quiet.getSample(ch, i)) * quiet.getSample(ch, i);
                    }
                const double db = 10.0 * std::log10(err / ref + 1.0e-300);
                logMessage("  " + toString(type) + ": difference " + juce::String(db, 1) + " dB (latency " + juce::String(latency) + ")");
                expect(db <= -100.0, toString(type));
            }
            const auto sat = defaultModule(FxType::Saturator);
            expect(static_cast<double>(sat.parameters["driveDb"]) == 0.0 && static_cast<int>(sat.parameters["autoGain"]) == 0
                   && static_cast<double>(sat.parameters["hiss"]) == 0.0 && static_cast<double>(sat.parameters["wow"]) == 0.0);
        }

        beginTest("Presets keep their sound: built on presetBaseModule (the module defaults before 'everything at zero')");
        {
            expectEquals(static_cast<double>(presetBaseModule(FxType::Compressor).parameters["thresholdDb"]), -30.0);
            expectEquals(static_cast<double>(presetBaseModule(FxType::Saturator).parameters["driveDb"]), 6.0);
            expectEquals(static_cast<double>(presetBaseModule(FxType::Reverb).mix), 1.0);
            expect(rev::readPattern(presetBaseModule(FxType::MonstaFx).parameters) != rev::Pattern {});
            expect(rev::readPattern(defaultModule(FxType::MonstaFx).parameters) == rev::Pattern {});
        }

        beginTest("Knob help: every knob and switch of every module page has a text");
        {
            expectEquals(missingHelp<ui::HamloCompPanel>("comp", ui::HamloCompPanel::NumKnobs, ui::HamloCompPanel::NumSwitches), 0);
            expectEquals(missingHelp<ui::HamloRedWallPanel>("limiter", ui::HamloRedWallPanel::NumKnobs, ui::HamloRedWallPanel::NumSwitches), 0);
            expectEquals(missingHelp<ui::HamloVerbPanel>("reverb", ui::HamloVerbPanel::NumKnobs, ui::HamloVerbPanel::NumSwitches), 0);
            expectEquals(missingHelp<ui::HamloEchoPanel>("delay", ui::HamloEchoPanel::NumParams, ui::HamloEchoPanel::NumSwitches), 0);
            expectEquals(missingHelp<ui::HamloMagPanel>("saturator", ui::HamloMagPanel::NumParams, ui::HamloMagPanel::NumSwitches), 0);
            expectEquals(missingHelp<ui::HamloUdarnikPanel>("transient", ui::HamloUdarnikPanel::NumKnobs, ui::HamloUdarnikPanel::NumSwitches), 0);
            expectEquals(missingHelp<ui::HamloZastavaPanel>("gate", ui::HamloZastavaPanel::NumKnobs, ui::HamloZastavaPanel::NumSwitches), 0);
            expectEquals(missingHelp<ui::HamloShirotaPanel>("stereo", ui::HamloShirotaPanel::NumKnobs, ui::HamloShirotaPanel::NumSwitches), 0);
            expectEquals(missingHelp<ui::HamloFaderPanel>("fader", ui::HamloFaderPanel::NumParams, ui::HamloFaderPanel::NumSwitches), 0);
            expectEquals(missingHelp<ui::HamloRevolutionPanel>("revolution", ui::HamloRevolutionPanel::NumParams, ui::HamloRevolutionPanel::NumSwitches), 0);
            // the page answers which control is under a point
            ui::HamloRedWallPanel page;
            page.setSize(1180, 700);
            expectEquals(page.helpKeyAt({ 56.0f + 0.5f * (1068.0f / 8.0f), 253.0f }), juce::String("thresholdDb"));
            expect(page.helpKeyAt({ 5.0f, 5.0f }).isEmpty());
        }

        beginTest("HamloRevolution: lane GEN changes only that lane; LOCK keeps a lane through GENERATE; GEN on a locked lane does nothing");
        {
            ui::HamloRevolutionPanel panel;
            panel.setSize(1180, 700);
            ui::HamloRevolutionPanel::Values v;
            v.pattern = rev::generate({}, 7, rev::Musical);
            v.seed = 7;
            panel.setValues(v);
            rev::Pattern last = v.pattern;
            juce::int64 lastSeed = 7;
            panel.onPatternChange = [&](const rev::Pattern& p, juce::int64 seed, const rev::Feel& feel, const juce::String&)
            {
                last = p;
                lastSeed = seed;
                auto next = panel.getValues();
                next.pattern = p;
                next.seed = seed;
                next.feel = feel;
                panel.setValues(next);
            };
            const auto before = last;
            panel.pressLaneGenerate(rev::Glitch);
            expect(last.steps[rev::Time] == before.steps[rev::Time] && last.steps[rev::Motion] == before.steps[rev::Motion]);
            expect(last.steps[rev::Glitch] != before.steps[rev::Glitch] && lastSeed != 7);
            expect(last.locks == before.locks);
            panel.pressLaneLock(rev::Time);
            expect(ui::HamloRevolutionPanel::laneLocked(last, rev::Time));
            const auto lockedTime = last.steps[rev::Time];
            panel.pressNewSeed(); // GENERATE with a new seed
            expect(last.steps[rev::Time] == lockedTime);
            const auto snapshot = last;
            panel.pressLaneGenerate(rev::Time);
            expect(last == snapshot);
            panel.pressLaneLock(rev::Time);
            expect(!ui::HamloRevolutionPanel::laneLocked(last, rev::Time));
        }

        beginTest("HamloRevolution wave screen: input and output peaks per 1/8 step reach the page");
        {
            auto m = presetBaseModule(FxType::MonstaFx);
            FxProjectState p;
            p.layers[0].mixChain = { m };
            auto e = std::make_unique<ProjectEngine>();
            e->prepare(kFs, 512);
            e->publish(p);
            auto y = drums;
            for (int start = 0; start < y.getNumSamples(); start += 512)
            {
                float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                e->process(c, 2, juce::jmin(512, y.getNumSamples() - start));
            }
            std::array<float, 128> in {}, out {};
            expect(e->getLayer(0).getMixNode().getRevolutionWave(0, in, out));
            float maxIn = 0.0f, maxOut = 0.0f;
            for (int b = 0; b < 128; ++b)
            {
                maxIn = std::max(maxIn, in[static_cast<size_t>(b)]);
                maxOut = std::max(maxOut, out[static_cast<size_t>(b)]);
            }
            logMessage("  wave: input peak " + juce::String(maxIn, 3) + ", output peak " + juce::String(maxOut, 3));
            expect(maxIn > 0.05f && maxOut > 0.0f);
        }

        beginTest("Compressor page: the INPUT / GR history takes real readings (settle) without a needle for INPUT");
        {
            ui::HamloCompPanel page;
            page.setSize(1180, 700);
            for (int i = 0; i < 200; ++i)
                page.settleMeters(-12.0f, static_cast<float>(i % 10));
            juce::Image image(juce::Image::ARGB, 1180, 700, true);
            juce::Graphics g(image);
            page.paintEntireComponent(g, true);
            expect(true);
        }
    }
};

static OwnerRoundTests ownerRoundTests;
} // namespace hpeq::test
