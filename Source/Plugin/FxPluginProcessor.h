#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/FxProjectState.h"

namespace hpeq
{
// M1 skeleton: audio insert with unity pass-through, smoothed output gain, bypass, state, meters.
// No frequency processing exists yet; nothing here pretends otherwise.
class FxPluginProcessor final : public juce::AudioProcessor
{
public:
    FxPluginProcessor();
    ~FxPluginProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioParameterBool* getBypassParameter() const override { return bypassParameter; }

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    // Project model: message thread / host state thread only, never the audio thread.
    FxProjectState getProject() const;
    void setProject(FxProjectState);

    struct MeterSnapshot
    {
        float inputPeak = 0.0f, inputRms = 0.0f, outputPeak = 0.0f, outputRms = 0.0f;
    };
    // Levels of the last processed block (linear). Written by the audio thread, lock-free.
    MeterSnapshot getMeters() const noexcept;

private:
    static void measure(const juce::AudioBuffer<float>&, int numChannels, std::atomic<float>& peak, std::atomic<float>& rms) noexcept;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* outputGainDb = nullptr;
    juce::AudioParameterBool* bypassParameter = nullptr;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> gain { 1.0f };

    mutable juce::CriticalSection projectLock; // never taken by processBlock
    FxProjectState project;

    std::atomic<float> inputPeak { 0.0f }, inputRms { 0.0f }, outputPeak { 0.0f }, outputRms { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPluginProcessor)
};
} // namespace hpeq
