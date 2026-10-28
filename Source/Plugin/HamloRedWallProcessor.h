#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/AlignmentDelay.h"
#include "DSP/Ramp.h"
#include "DSP/Limiter.h"
#include "DSP/RedWallColour.h"

namespace hamlo::redwall
{
// HamloRedWall: the limiter alone. Signal path per block, as HPEQ's module plug-in (a one-slot
// LayerEngine there, the limiter directly here):
//   wet  = limiter(colour(x))                (colour = DRIVE / TUBE, latency C constant, a pure delay at 0;
//                                             limiter latency L = the look-ahead of the current settings, + 8 with TRUE PEAK)
//   y    = D_(C+L)(x) + m (wet - D_(C+L)(x)) MIX, 20 ms ramp, exact at 0 and 1
//   y    = D_(F - C - L)(y)                   aligned to the fixed latency F = C + the 10 ms look-ahead + 8 (true peak)
//   out  = D_F(x) + on (y - D_F(x))           ON crossfade, 20 ms
// F is reported to the host and never changes, so automating LOOKAHEAD or TRUE PEAK does not move the PDC.
class HamloRedWallProcessor final : public juce::AudioProcessor
{
public:
    HamloRedWallProcessor();
    ~HamloRedWallProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HamloRedWall"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

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

    dsp::LimiterSettings currentSettings() const noexcept;
    const dsp::LimiterProcessor& limiter() const noexcept { return core; }
    int fixedLatency() const noexcept { return latency; }

private:
    juce::AudioProcessorValueTreeState parameters;
    std::vector<std::atomic<float>*> raw; // in parameterSpecs() order
    juce::AudioParameterBool* bypassParameter = nullptr;
    int mixIndex = -1;

    dsp::RedWallColour colour;
    dsp::LimiterProcessor core;
    juce::AudioBuffer<float> dry, dryForMix;
    dsp::AlignmentDelay mixAlign, wetAlign, dryAlign;
    dsp::LinearRamp mixAmount, onAmount;
    int rampSteps = 1, maximumBlockSize = 1, latency = 0, currentProgram = 0;
    std::atomic<double> currentSampleRate { 48000.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamloRedWallProcessor)
};
} // namespace hamlo::redwall
