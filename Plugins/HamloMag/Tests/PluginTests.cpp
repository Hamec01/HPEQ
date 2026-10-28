// The plug-in around the saturator: parameters and state of HPEQ's HamloMag module VST3, the fixed latency,
// MIX / ON, block-size independence, mono, programs, the page and the preset screen.

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloMagEditor.h"
#include "Plugin/HamloMagProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"
#include "TestSignals.h"

namespace hamlo::test
{
using saturator::HamloMagProcessor;

namespace
{
juce::AudioBuffer<float> render(HamloMagProcessor& p, const juce::AudioBuffer<float>& x, int block, int prepared = -1)
{
    p.prepareToPlay(kFs, prepared > 0 ? prepared : block);
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

juce::AudioBuffer<float> delayed(const juce::AudioBuffer<float>& x, int samples)
{
    juce::AudioBuffer<float> y(x.getNumChannels(), x.getNumSamples());
    y.clear();
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, samples, x, ch, 0, x.getNumSamples() - samples);
    return y;
}
} // namespace

class PluginTests final : public juce::UnitTest
{
public:
    PluginTests() : juce::UnitTest("Plug-in", "HamloMag") {}

    void runTest() override
    {
        const auto loop = drums(2.0);

        beginTest("Parameters: the IDs, ranges and defaults of HPEQ's HamloMag, then bypass");
        {
            HamloMagProcessor p;
            const auto& specs = saturator::parameterSpecs();
            expectEquals(static_cast<int>(specs.size()), 21);
            for (const auto& spec : specs)
            {
                auto* param = p.parameter(spec.id);
                expect(param != nullptr, spec.id);
                if (param != nullptr)
                    expectWithinAbsoluteError(p.parameterValue(spec.id), static_cast<double>(spec.defaultValue), 1.0e-4, spec.id);
            }
            expect(p.getBypassParameter() != nullptr && !p.getBypassParameter()->get());
        }

        beginTest("Latency: fixed at 4x + the TAPE wow delay whatever OVERSAMPLING / MODEL");
        int latency = 0;
        {
            HamloMagProcessor p;
            p.prepareToPlay(kFs, 512);
            latency = p.getLatencySamples();
            logMessage("  " + juce::String(latency) + " samples at 48 kHz");
            expect(latency > 192);
            p.setParameterValue("oversampling", 0.0);
            p.setParameterValue("model", 2.0);
            p.prepareToPlay(kFs, 512);
            expectEquals(p.getLatencySamples(), latency);
        }

        beginTest("MIX 0 and ON off: the input delayed by the latency, bit-exact (every model)");
        for (int model = 0; model < 6; ++model)
        {
            HamloMagProcessor m;
            m.setParameterValue("model", model);
            m.setParameterValue("driveDb", 18.0);
            m.setParameterValue("mix", 0.0);
            expect(bitExact(render(m, loop, 512), delayed(loop, latency)), "MIX 0, model " + juce::String(model));
            HamloMagProcessor o;
            o.setParameterValue("model", model);
            o.setParameterValue("driveDb", 18.0);
            o.setParameterValue("bypass", 1.0);
            expect(bitExact(render(o, loop, 512), delayed(loop, latency)), "ON off, model " + juce::String(model));
        }

        beginTest("State: a state saved by HPEQ's module VST3 (tag HamloMag, PARAM id / value) loads; save / load round trip");
        {
            HamloMagProcessor p;
            juce::XmlElement xml("HamloMag");
            const auto add = [&](const char* id, double v)
            {
                auto* e = xml.createNewChildElement("PARAM");
                e->setAttribute("id", id);
                e->setAttribute("value", v);
            };
            add("driveDb", 14.0);
            add("model", 3.0);
            add("crackle", 0.4);
            juce::MemoryBlock block;
            juce::AudioProcessor::copyXmlToBinary(xml, block);
            p.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
            expectWithinAbsoluteError(p.parameterValue("driveDb"), 14.0, 1.0e-4);
            expectEquals(juce::roundToInt(p.parameterValue("model")), 3);
            juce::MemoryBlock saved;
            p.getStateInformation(saved);
            HamloMagProcessor q;
            q.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            for (const auto& spec : saturator::parameterSpecs())
                expectWithinAbsoluteError(q.parameterValue(spec.id), p.parameterValue(spec.id), 1.0e-6, spec.id);
        }

        beginTest("Host blocks 1, 64, 127, 512 and 4096 after prepare(256) bit-identical (TAPE, 4x, wow, hiss, MIX 0.7)");
        {
            const auto set = [](HamloMagProcessor& p)
            {
                p.setParameterValue("model", 1.0);
                p.setParameterValue("oversampling", 2.0);
                p.setParameterValue("driveDb", 12.0);
                p.setParameterValue("wow", 0.4);
                p.setParameterValue("hiss", 0.3);
                p.setParameterValue("mix", 0.7);
            };
            HamloMagProcessor ref;
            set(ref);
            const auto expected = render(ref, loop, 256);
            for (int block : { 1, 64, 127, 512 })
            {
                HamloMagProcessor p;
                set(p);
                expect(bitExact(render(p, loop, block), expected), "block " + juce::String(block));
            }
            HamloMagProcessor big;
            set(big);
            expect(bitExact(render(big, loop, 4096, 256), expected), "block 4096 after prepare(256)");
        }

        beginTest("Mono: the mono layout is accepted and the saturator works on it");
        {
            HamloMagProcessor p;
            juce::AudioProcessor::BusesLayout mono;
            mono.inputBuses.add(juce::AudioChannelSet::mono());
            mono.outputBuses.add(juce::AudioChannelSet::mono());
            expect(p.setBusesLayout(mono));
            juce::AudioBuffer<float> m(1, loop.getNumSamples());
            m.copyFrom(0, 0, loop, 0, 0, loop.getNumSamples());
            const auto y = render(p, m, 512);
            expect(!bitExact(y, delayed(m, latency)));
        }

        beginTest("Programs: Default plus the 7 HPEQ saturator presets; Default restores");
        {
            HamloMagProcessor p;
            expectEquals(p.getNumPrograms(), 8);
            expectEquals(p.getProgramName(2), juce::String("Tube Glow"));
            p.setCurrentProgram(2);
            expectWithinAbsoluteError(p.parameterValue("driveDb"), 8.0, 1.0e-3);
            expect(!p.programModified());
            p.setCurrentProgram(0);
            for (const auto& spec : saturator::parameterSpecs())
                expectWithinAbsoluteError(p.parameterValue(spec.id), static_cast<double>(spec.defaultValue), 1.0e-3, spec.id);
        }

        beginTest("Page: 1180 x 820, resizable; the preset screen top left; a knob writes its parameter");
        {
            HamloMagProcessor p;
            std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
            auto* editor = dynamic_cast<saturator::HamloMagEditor*>(e.get());
            expect(editor != nullptr);
            if (editor == nullptr)
                return;
            expectEquals(editor->getWidth(), saturator::kEditorWidth);
            expectEquals(editor->getHeight(), saturator::kEditorHeight);
            expect(editor->getPresetScreen().getBounds() == juce::Rectangle<float>(56.0f, 20.0f, 336.0f, 116.0f).toNearestInt());
            auto& panel = editor->getPanel();
            expect(panel.onKnobChange != nullptr);
            panel.onKnobChange(ui::HamloMagPanel::Drive, 20.0);
            expectWithinAbsoluteError(p.parameterValue("driveDb"), 20.0, 1.0e-3);
        }
    }
};

static PluginTests pluginTests;
} // namespace hamlo::test
