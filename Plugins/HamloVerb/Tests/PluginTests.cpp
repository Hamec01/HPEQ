// HamloVerb M0: HPEQ's module plug-in in its own repository - parameters, fixed latency, bypass, blocks, state,
// programs, the page with the preset screen top left.

#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"

namespace hamlo::test
{
using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;

namespace
{
constexpr auto kKind = static_cast<ModuleKind>(HPEQ_MODULE_KIND);

juce::AudioBuffer<float> render(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x, int block, int prepared = -1)
{
    p.prepareToPlay(kFs, prepared > 0 ? prepared : block);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(block, y.getNumSamples() - start));
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
    PluginTests() : juce::UnitTest("Plug-in", "HamloVerb") {}

    void runTest() override
    {
        const auto loop = drums(2.0);

        beginTest("Parameters: every spec of HPEQ's HamloVerb at its default, then bypass");
        {
            auto pOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& p = *pOwned;
            for (const auto& spec : p.getSpec().parameters)
            {
                expect(p.parameter(spec.id) != nullptr, spec.id);
                expectWithinAbsoluteError(p.parameterValue(spec.id), static_cast<double>(spec.defaultValue), 1.0e-3, spec.id);
            }
            expect(p.getBypassParameter() != nullptr && !p.getBypassParameter()->get());
        }

        beginTest("Latency: fixed, the same at 44.1 and 48 kHz settings and for every program");
        {
            auto pOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& p = *pOwned;
            p.prepareToPlay(kFs, 512);
            const int latency = p.getLatencySamples();
            logMessage("  " + juce::String(latency) + " samples at 48 kHz");
            for (int i = 1; i < p.getNumPrograms(); ++i)
            {
                p.setCurrentProgram(i);
                p.prepareToPlay(kFs, 512);
                expectEquals(p.getLatencySamples(), latency, p.getProgramName(i));
            }
        }

        beginTest("ON off: the input delayed by the latency, bit-exact");
        {
            auto pOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& p = *pOwned;
            p.setParameterValue("bypass", 1.0);
            const auto y = render(p, loop, 512);
            expect(bitExact(y, delayed(loop, p.getLatencySamples())));
        }

        beginTest("Every program: finite and bounded on drums");
        {
            auto probeOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& probe = *probeOwned;
            for (int i = 0; i < probe.getNumPrograms(); ++i)
            {
                auto pOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& p = *pOwned;
                p.setCurrentProgram(i);
                const auto y = render(p, loop, 512);
                bool finite = true;
                for (int ch = 0; ch < 2; ++ch)
                    for (int k = 0; k < y.getNumSamples(); ++k)
                        finite = finite && std::isfinite(y.getSample(ch, k));
                expect(finite && y.getMagnitude(0, y.getNumSamples()) < 16.0f, p.getProgramName(i));
            }
        }

        beginTest("Host blocks 1, 64, 127, 512 and 4096 after prepare(256) bit-identical (the last program)");
        {
            auto refOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& ref = *refOwned;
            const int program = ref.getNumPrograms() - 1;
            ref.setCurrentProgram(program);
            const auto expected = render(ref, loop, 256);
            for (int block : { 1, 64, 127, 512 })
            {
                auto pOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& p = *pOwned;
                p.setCurrentProgram(program);
                expect(bitExact(render(p, loop, block, 256), expected), "block " + juce::String(block));
            }
            auto bigOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& big = *bigOwned;
            big.setCurrentProgram(program);
            expect(bitExact(render(big, loop, 4096, 256), expected), "block 4096 after prepare(256)");
        }

        beginTest("State: save / load round trip");
        {
            auto pOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& p = *pOwned;
            p.setCurrentProgram(p.getNumPrograms() - 1);
            juce::MemoryBlock saved;
            p.getStateInformation(saved);
            auto qOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& q = *qOwned;
            q.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            for (const auto& spec : p.getSpec().parameters)
                expectWithinAbsoluteError(q.parameterValue(spec.id), p.parameterValue(spec.id), 1.0e-5, spec.id);
        }

        beginTest("Page: the spec's size; the preset screen top left with every program; a program from it loads");
        {
            auto pOwned = std::make_unique<ModulePluginProcessor>(kKind); // the module engine is large: heap, not stack
            auto& p = *pOwned;
            std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(e.get());
            expect(editor != nullptr);
            if (editor == nullptr)
                return;
            expectEquals(editor->getWidth(), p.getSpec().editorWidth);
            expectEquals(editor->getHeight(), p.getSpec().editorHeight);
            auto& screen = editor->getPresetScreen();
            expect(screen.getBounds() == juce::Rectangle<float>(56.0f, 20.0f, 336.0f, 116.0f).toNearestInt(), screen.getBounds().toString());
            expectEquals(static_cast<int>(screen.filtered().size()), p.getNumPrograms() - 1);
            if (p.getNumPrograms() > 1)
            {
                screen.setCurrent(0, false);
                screen.step(1);
                expectEquals(p.getCurrentProgram(), 1);
            }
        }
    }
};

static PluginTests pluginTests;
} // namespace hamlo::test
