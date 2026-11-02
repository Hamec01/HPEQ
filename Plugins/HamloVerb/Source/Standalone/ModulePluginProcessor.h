#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/LegacyVerbPath.h"
#include "DSP/VerbV2Processor.h"
#include "DSP/Ramp.h"
#include "Standalone/ModuleVariant.h"

namespace hpeq::standalone
{
class ModulePluginProcessor final : public juce::AudioProcessor
{
public:
    explicit ModulePluginProcessor(ModuleKind);
    ~ModulePluginProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return moduleSpec(kind).productName; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    // Presets pass: the module's factory presets as the host's programs (0 = Default, then the presets
    // HPEQ shows on this module's page).
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram.load(std::memory_order_relaxed); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioParameterBool* getBypassParameter() const override { return bypassParameter; }

    ModuleKind getKind() const noexcept { return kind; }
    const ModuleSpec& getSpec() const noexcept { return moduleSpec(kind); }
    double parameterValue(const juce::String& id) const noexcept;
    juce::RangedAudioParameter* parameter(const juce::String& id) const noexcept;
    void setParameterValue(const juce::String& id, double actualValue);
    bool isModuleOn() const noexcept { return bypassParameter != nullptr && !bypassParameter->get(); }
    double getHostBpm() const noexcept { return hostBpm.load(std::memory_order_relaxed); }
    float getOutputDb() const noexcept { return rawValues[22]->load() < .5f ? engine.outputLevelDb() : modern.outputLevelDb(); }
    float getDuckDb() const noexcept { return rawValues[22]->load() < .5f ? engine.duckingDb() : modern.duckingDb(); }

    bool getMixLock() const noexcept { return mixLocked.load(); }
    void setMixLock(bool value) noexcept { mixLocked.store(value); }
    bool isProgramModified() const;
    double getTankEnergy() const noexcept { return modern.energy(); }
private:
    dsp::FxSlotParams currentSlot(bool active) const noexcept;
    dsp::Transport currentTransport() noexcept;

    ModuleKind kind;
    juce::AudioProcessorValueTreeState parameters;
    std::vector<std::atomic<float>*> rawValues;
    juce::AudioParameterBool* bypassParameter = nullptr;

    dsp::LegacyVerbPath engine;
    dsp::VerbV2Processor modern;
    juce::AudioBuffer<float> wetBuffer;
    juce::ValueTree defaults;
    std::atomic<bool> mixLocked { false };
    dsp::LinearRamp insertMix, outputGain;
    int lastEngine = 0;
    juce::AudioBuffer<float> dry;
    dsp::LinearRamp wetAmount;
    int maximumBlockSize = 1;
    std::atomic<int> currentProgram { 0 };
    int fixedLatency = 0;
    int bypassRampSteps = 1;
    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<double> hostBpm { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePluginProcessor)
};
} // namespace hpeq::standalone
