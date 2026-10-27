#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/Compressor.h"
#include "DSP/Ramp.h"

namespace hamlo::comp
{
// HamloComp: the compressor alone. Signal path per block: dry copy -> CompressorProcessor (linked
// stereo) -> MIX (dry + m (wet - dry), 20 ms ramp) -> ON / bypass crossfade (20 ms) against the dry.
// The compressor keeps running while bypassed, so switching ON back does not start from a cold state.
// Latency 0, no tail. Allocation-free on the audio thread.
class HamloCompProcessor final : public juce::AudioProcessor
{
public:
    HamloCompProcessor();
    ~HamloCompProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HamloComp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // The factory presets as the host's programs (0 = Default).
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioParameterBool* getBypassParameter() const override { return bypassParameter; }

    double parameterValue(const juce::String& id) const noexcept;
    juce::RangedAudioParameter* parameter(const juce::String& id) const noexcept;
    void setParameterValue(const juce::String& id, double actualValue);
    bool isOn() const noexcept { return bypassParameter != nullptr && !bypassParameter->get(); }

    // Meters for the page: detector level of the last block (dB) and its peak gain reduction (dB, >= 0).
    float inputLevelDb() const noexcept { return compressor.getInputLevelDb(); }
    float gainReductionDb() const noexcept { return compressor.getGainReductionDb(); }

    dsp::CompressorSettings currentSettings() const noexcept;

private:
    juce::AudioProcessorValueTreeState parameters;
    std::vector<std::atomic<float>*> raw; // in parameterSpecs() order
    juce::AudioParameterBool* bypassParameter = nullptr;

    dsp::CompressorProcessor compressor;
    juce::AudioBuffer<float> dry;
    dsp::LinearRamp mixAmount, onAmount;
    int rampSteps = 1;
    int maximumBlockSize = 1;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamloCompProcessor)
};
} // namespace hamlo::comp
