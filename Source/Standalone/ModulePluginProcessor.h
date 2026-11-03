#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/DelayLine.h"
#include "DSP/LegacyZastavaCore.h"
#include "DSP/Ramp.h"
#include "Standalone/ModuleVariant.h"
#include "Standalone/ModernParameters.h"

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
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
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
    bool isHostPlaying() const noexcept { return hostPlaying.load(std::memory_order_relaxed); }
    const dsp::LegacyZastavaCore& moduleNode() const noexcept { return engine; }
    hamlo::zastava::Reading modernReading() const noexcept { return modernEngine.reading(); }
    int editorView() const noexcept { return view.load(std::memory_order_relaxed); }
    int editorTab() const noexcept { return tab.load(std::memory_order_relaxed); }
    void setEditorView(int value) noexcept { view.store(juce::jlimit(0,2,value),std::memory_order_relaxed); }
    void setEditorTab(int value) noexcept { tab.store(juce::jlimit(0,2,value),std::memory_order_relaxed); }
    bool isProgramModified() const;

private:
    dsp::FxSlotParams currentSlot(bool active) const noexcept;
    dsp::Transport currentTransport() noexcept;
    void process(juce::AudioBuffer<float>&,bool forcedBypass);
    bool compactLegacyState() const noexcept;

    ModuleKind kind;
    juce::AudioProcessorValueTreeState parameters;
    std::vector<std::atomic<float>*> rawValues;
    juce::AudioParameterBool* bypassParameter = nullptr;
    modern::Raw modernValues {};

    dsp::LegacyZastavaCore engine;
    hamlo::zastava::Gate modernEngine;
    juce::AudioBuffer<float> modernAudio;
    juce::AudioBuffer<float> keyAudio;
    dsp::LinearRamp modernAmount;
    std::atomic<bool> resetModern { false };
    int lastModernMode=0;
    std::atomic<int> view { 0 }, tab { 0 };
    juce::AudioBuffer<float> dry;
    dsp::AlignmentDelay wetAlign, dryAlign;
    dsp::LinearRamp wetAmount;
    int maximumBlockSize = 1;
    std::atomic<int> currentProgram { 0 };
    int fixedLatency = 0;
    int bypassRampSteps = 1;
    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<double> hostBpm { 120.0 };
    std::atomic<bool> hostPlaying { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePluginProcessor)
};
} // namespace hpeq::standalone
