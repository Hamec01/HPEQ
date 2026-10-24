#include <array>
#include <cmath>
#include <set>

#include <juce_core/juce_core.h>

#include "Standalone/ModulePluginProcessor.h"

namespace hpeq::standalone
{
namespace
{
constexpr std::array<ModuleKind, 8> kinds { ModuleKind::Comp, ModuleKind::Verb, ModuleKind::Echo, ModuleKind::Mag,
                                            ModuleKind::RedWall, ModuleKind::Udarnik, ModuleKind::Zastava, ModuleKind::Shirota };

bool finite(const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (!std::isfinite(buffer.getSample(channel, sample)))
                return false;
    return true;
}

class ModulePluginTests final : public juce::UnitTest
{
public:
    ModulePluginTests() : juce::UnitTest("Standalone module processors", "HPEQ Modules") {}

    void runTest() override
    {
        beginTest("Eight unique processors expose their complete panels as host parameters");
        std::set<std::string> names;
        for (const auto kind : kinds)
        {
            auto processor = std::make_unique<ModulePluginProcessor>(kind);
            names.insert(processor->getName().toStdString());
            expectEquals(processor->getParameters().size(), static_cast<int>(processor->getSpec().parameters.size() + 1));
            expect(processor->getBypassParameter() != nullptr);
            expect(processor->hasEditor());
        }
        expectEquals(static_cast<int>(names.size()), static_cast<int>(kinds.size()));

        beginTest("Mono/stereo layouts, state and editor round-trip");
        for (const auto kind : kinds)
        {
            auto a = std::make_unique<ModulePluginProcessor>(kind);
            auto b = std::make_unique<ModulePluginProcessor>(kind);
            auto stereo = a->getBusesLayout();
            expect(a->checkBusesLayoutSupported(stereo), a->getName() + " stereo");
            stereo.inputBuses.getReference(0) = juce::AudioChannelSet::mono();
            stereo.outputBuses.getReference(0) = juce::AudioChannelSet::mono();
            expect(a->checkBusesLayoutSupported(stereo), a->getName() + " mono");

            const auto& first = a->getSpec().parameters.front();
            const double changed = first.defaultValue == first.maximum ? first.minimum : first.maximum;
            a->setParameterValue(first.id, changed);
            juce::MemoryBlock state;
            a->getStateInformation(state);
            b->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            expectWithinAbsoluteError(b->parameterValue(first.id), changed, 1.0e-4, a->getName() + " state");

            std::unique_ptr<juce::AudioProcessorEditor> editor(a->createEditor());
            expect(editor != nullptr && editor->getWidth() > 0 && editor->getHeight() > 0, a->getName() + " editor");
        }

        beginTest("Default processing is finite and bypass is fixed-latency dry");
        for (const auto kind : kinds)
        {
            auto processor = std::make_unique<ModulePluginProcessor>(kind);
            constexpr int block = 512;
            processor->prepareToPlay(48000.0, block);
            juce::AudioBuffer<float> audio(2, block);
            for (int channel = 0; channel < 2; ++channel)
                for (int i = 0; i < block; ++i)
                    audio.setSample(channel, i, 0.15f * std::sin(0.017f * static_cast<float>(i + channel * 11)));
            juce::MidiBuffer midi;
            processor->processBlock(audio, midi);
            expect(finite(audio), processor->getName() + " finite");

            auto bypassed = std::make_unique<ModulePluginProcessor>(kind);
            bypassed->setParameterValue("bypass", 1.0);
            bypassed->prepareToPlay(48000.0, block);
            juce::AudioBuffer<float> dry(2, block), original(2, block);
            for (int channel = 0; channel < 2; ++channel)
                for (int i = 0; i < block; ++i)
                    dry.setSample(channel, i, static_cast<float>(i + 1) / static_cast<float>(block));
            original.makeCopyOf(dry);
            bypassed->processBlock(dry, midi);
            const int latency = bypassed->getLatencySamples();
            bool aligned = true;
            for (int channel = 0; channel < 2; ++channel)
                for (int i = latency; i < block; ++i)
                    aligned = aligned && dry.getSample(channel, i) == original.getSample(channel, i - latency);
            expect(aligned, processor->getName() + " bypass/PDC");
        }
    }
};

ModulePluginTests modulePluginTests;
} // namespace
} // namespace hpeq::standalone
