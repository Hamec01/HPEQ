// The plug-in around the compressor: parameters and state compatible with HPEQ's HamloComp module VST3,
// MIX / ON, block-size independence, mono, programs, the page.

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloCompEditor.h"
#include "Plugin/HamloCompProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"
#include "TestSignals.h"

namespace hamlo::test
{
using comp::HamloCompProcessor;

namespace
{
juce::AudioBuffer<float> render(HamloCompProcessor& p, const juce::AudioBuffer<float>& x, int block)
{
    p.prepareToPlay(kFs, block);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), y.getNumChannels(), start, n);
        p.processBlock(view, midi);
    }
    return y;
}
} // namespace

class PluginTests final : public juce::UnitTest
{
public:
    PluginTests() : juce::UnitTest("Plug-in", "HamloComp") {}

    void runTest() override
    {
        const auto loop = drums(3.0);

        beginTest("Parameters: the IDs and ranges of HPEQ's HamloComp, neutral defaults (0 dB, 1:1, no auto makeup), then bypass");
        {
            HamloCompProcessor p;
            const auto& params = p.getParameters();
            expectEquals(params.size(), static_cast<int>(comp::parameterSpecs().size()) + 1);
            for (const auto& spec : comp::parameterSpecs())
            {
                auto* r = p.parameter(spec.id);
                expect(r != nullptr, spec.id);
                if (r == nullptr)
                    continue;
                expectWithinAbsoluteError(p.parameterValue(spec.id), static_cast<double>(spec.defaultValue), 1.0e-4, spec.id);
                expectWithinAbsoluteError(static_cast<double>(r->getNormalisableRange().start), static_cast<double>(spec.minimum), 1.0e-6);
                expectWithinAbsoluteError(static_cast<double>(r->getNormalisableRange().end), static_cast<double>(spec.maximum), 1.0e-6);
            }
            expect(p.getBypassParameter() != nullptr && !p.getBypassParameter()->get());
            expectEquals(p.getLatencySamples(), 0);
            expectWithinAbsoluteError(p.parameterValue("thresholdDb"), 0.0, 1.0e-6);
            expectWithinAbsoluteError(p.parameterValue("ratio"), 1.0, 1.0e-6);
            expectEquals(juce::roundToInt(p.parameterValue("autoMakeup")), 0);
        }

        beginTest("A fresh instance does nothing: the output is the input, bit-exact (Punch included)");
        {
            HamloCompProcessor p;
            expect(bitExact(render(p, loop, 512), loop));
        }

        beginTest("State: a HamloComp state saved by HPEQ's module VST3 (tag HamloComp, PARAM id / value) loads; save / load round trip");
        {
            HamloCompProcessor p;
            juce::XmlElement xml("HamloComp");
            const auto add = [&](const char* id, double v)
            {
                auto* e = xml.createNewChildElement("PARAM");
                e->setAttribute("id", id);
                e->setAttribute("value", v);
            };
            add("thresholdDb", -20.0);
            add("ratio", 8.0);
            add("character", 2.0);
            add("mix", 0.5);
            add("bypass", 1.0);
            juce::MemoryBlock block;
            juce::AudioProcessor::copyXmlToBinary(xml, block);
            p.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
            expectWithinAbsoluteError(p.parameterValue("thresholdDb"), -20.0, 1.0e-4);
            expectWithinAbsoluteError(p.parameterValue("ratio"), 8.0, 1.0e-3);
            expectEquals(juce::roundToInt(p.parameterValue("character")), 2);
            expectWithinAbsoluteError(p.parameterValue("mix"), 0.5, 1.0e-4);
            expect(!p.isOn());

            juce::MemoryBlock saved;
            p.getStateInformation(saved);
            HamloCompProcessor q;
            q.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            for (const auto& spec : comp::parameterSpecs())
                expectWithinAbsoluteError(q.parameterValue(spec.id), p.parameterValue(spec.id), 1.0e-6, spec.id);
            expect(!q.isOn());
        }

        beginTest("MIX 0 and ON off (after the 20 ms ramp) give the dry signal bit-exact; MIX 1 = the compressor alone");
        {
            HamloCompProcessor p;
            p.setParameterValue("thresholdDb", -30.0);
            p.setParameterValue("ratio", 4.0);
            p.setParameterValue("mix", 0.0);
            expect(bitExact(render(p, loop, 512), loop), "MIX 0");

            HamloCompProcessor q;
            q.setParameterValue("thresholdDb", -30.0);
            q.setParameterValue("ratio", 4.0);
            q.setParameterValue("bypass", 1.0);
            expect(bitExact(render(q, loop, 512), loop), "ON off");

            HamloCompProcessor r;
            r.setParameterValue("thresholdDb", -30.0);
            r.setParameterValue("ratio", 4.0);
            r.setParameterValue("autoMakeup", 1.0);
            const auto y = render(r, loop, 512);
            dsp::CompressorProcessor c;
            c.prepare(kFs);
            auto z = loop;
            for (int start = 0; start < z.getNumSamples(); start += 512)
            {
                float* ch[2] { z.getWritePointer(0) + start, z.getWritePointer(1) + start };
                c.process(ch, 2, juce::jmin(512, z.getNumSamples() - start), r.currentSettings(), true);
            }
            expect(bitExact(y, z), "MIX 1 is not the compressor alone");
            logMessage("  -30 dB, 4:1, auto makeup on the drum loop: level change " + juce::String(rmsDb(y) - rmsDb(loop), 2) + " dB");
            expect(rmsDb(y) < rmsDb(loop) + 6.0 && !bitExact(y, loop));
        }

        beginTest("Host block size does not change the output: 1, 64, 127, 512, 4096 (prepared for less) bit-identical");
        {
            const auto set = [](HamloCompProcessor& p)
            {
                p.setParameterValue("thresholdDb", -30.0);
                p.setParameterValue("ratio", 4.0);
                p.setParameterValue("mix", 0.6);
                p.setParameterValue("character", 2.0);
            };
            HamloCompProcessor ref;
            set(ref);
            const auto expected = render(ref, loop, 256);
            for (int block : { 1, 64, 127, 512 })
            {
                HamloCompProcessor p;
                set(p);
                expect(bitExact(render(p, loop, block), expected), "block " + juce::String(block));
            }
            // a host that sends more than it announced: processed in prepared-size chunks
            HamloCompProcessor big;
            set(big);
            big.prepareToPlay(kFs, 256);
            auto y = loop;
            juce::MidiBuffer midi;
            for (int start = 0; start < y.getNumSamples(); start += 4096)
            {
                juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(4096, y.getNumSamples() - start));
                big.processBlock(view, midi);
            }
            expect(bitExact(y, expected), "block 4096 after prepare(256)");
        }

        beginTest("Mono: the mono layout is accepted and compresses like the left channel of a dual-mono signal");
        {
            HamloCompProcessor p;
            p.setParameterValue("thresholdDb", -30.0);
            p.setParameterValue("ratio", 4.0);
            juce::AudioProcessor::BusesLayout mono;
            mono.inputBuses.add(juce::AudioChannelSet::mono());
            mono.outputBuses.add(juce::AudioChannelSet::mono());
            expect(p.setBusesLayout(mono));
            const auto x = segment(loop, 0, loop.getNumSamples());
            juce::AudioBuffer<float> m(1, x.getNumSamples());
            m.copyFrom(0, 0, x, 0, 0, x.getNumSamples());
            const auto y = render(p, m, 512);
            expect(!bitExact(y, m) && rmsDb(y) < rmsDb(m));
        }

        beginTest("Programs: Default plus the 7 HPEQ compressor presets; a preset starts from the preset base (-30 / 4:1 / auto makeup on); Default restores the neutral defaults");
        {
            HamloCompProcessor p;
            expectEquals(p.getNumPrograms(), 8);
            expectEquals(p.getProgramName(0), juce::String("Default"));
            expectEquals(p.getProgramName(4), juce::String("New York Parallel"));
            p.setCurrentProgram(4);
            expectWithinAbsoluteError(p.parameterValue("thresholdDb"), -36.0, 1.0e-3);
            expectWithinAbsoluteError(p.parameterValue("mix"), 0.4, 1.0e-4);
            expectWithinAbsoluteError(p.parameterValue("makeupDb"), 0.0, 1.0e-4); // not in the preset: the base
            expectEquals(juce::roundToInt(p.parameterValue("autoMakeup")), 1);      // the base, not the neutral default
            p.setCurrentProgram(0);
            for (const auto& spec : comp::parameterSpecs())
                expectWithinAbsoluteError(p.parameterValue(spec.id), static_cast<double>(spec.defaultValue), 1.0e-3, spec.id);
        }

        beginTest("Page: 1180 x 700, resizable; knob, switch and ON write the parameters; host changes come back to the page");
        {
            HamloCompProcessor p;
            std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
            auto* editor = dynamic_cast<comp::HamloCompEditor*>(e.get());
            expect(editor != nullptr);
            if (editor == nullptr)
                return;
            expectEquals(editor->getWidth(), comp::kEditorWidth);
            expectEquals(editor->getHeight(), comp::kEditorHeight);
            auto& panel = editor->getPanel();
            expect(panel.getBounds() == editor->getLocalBounds());
            panel.onKnobGestureStart(ui::HamloCompPanel::Threshold);
            panel.onKnobChange(ui::HamloCompPanel::Threshold, -41.5);
            panel.onKnobGestureEnd();
            panel.onSwitchChange(ui::HamloCompPanel::Character, 2);
            panel.onPowerChange(false);
            expectWithinAbsoluteError(p.parameterValue("thresholdDb"), -41.5, 1.0e-3);
            expectEquals(juce::roundToInt(p.parameterValue("character")), 2);
            expect(!p.isOn());
            p.setParameterValue("ratio", 10.0);
            p.setParameterValue("bypass", 0.0);
            editor->setSize(590, 350);
            expect(panel.getWidth() == 590 && panel.getHeight() == 350);
            // the 30 Hz refresh brings the host's values back
            juce::MessageManager::getInstance()->runDispatchLoopUntil(120);
            expectWithinAbsoluteError(panel.getValues().knobs[ui::HamloCompPanel::Ratio], 10.0, 1.0e-3);
            expect(panel.getValues().on);
        }
    }
};

static PluginTests pluginTests;
} // namespace hamlo::test
