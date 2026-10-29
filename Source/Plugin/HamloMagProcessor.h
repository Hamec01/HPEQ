#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/AlignmentDelay.h"
#include "DSP/Ramp.h"
#include "DSP/MagV2.h"

namespace hamlo::saturator
{
// HamloMag: the saturator alone. Signal path per block, as HPEQ's module plug-in (a one-slot LayerEngine there,
// the saturator directly here):
//   wet  = saturator(x)                      (latency L = the SAMPLER's + the oversampling's + the TAPE / VINYL wow
//                                             delay; HamloMag v2 stages: DSP/MagV2.h)
//   y    = D_L(x) + m (wet - D_L(x))         MIX, 20 ms ramp, exact at 0 and 1
//   y    = g y                               MATCH (v2): g follows the K-weighted loudness of D_L(x) over that of y
//                                             (100 ms blocks gated at -10 dB re the input's 10 s level, averaged
//                                             over 1.5 s, 0.5 s smoothing, +-24 dB), 1 when off
//   y    = y - D_L(x)                         DELTA (v2): only what HamloMag adds
//   y    = D_(F - L)(y)                       aligned to the fixed latency F = the SAMPLER + 8x + the TAPE wow delay
//   out  = D_F(x) + on (y - D_F(x))           ON crossfade, 20 ms
// F is reported to the host and never changes, so automating OVERSAMPLING or MODEL does not move the PDC.
class HamloMagProcessor final : public juce::AudioProcessor
{
public:
    HamloMagProcessor();
    ~HamloMagProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HamloMag"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    // true when a parameter differs from what the current program set (the preset screen marks it with *)
    bool programModified() const;
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

    dsp::FxSlotParams currentSettings() const noexcept;
    const dsp::MagV2Processor& saturator() const noexcept { return core; }
    int fixedLatency() const noexcept { return latency; }

private:
    juce::AudioProcessorValueTreeState parameters;
    std::vector<std::atomic<float>*> raw; // in parameterSpecs() order
    juce::AudioParameterBool* bypassParameter = nullptr;
    int mixIndex = -1;

    dsp::MagV2Processor core;
    struct KWeighting
    {
        double b[2][3] {}, a[2][3] {};
        double z[dsp::kMaxChannels][2][2] {};
        void design(double sampleRate) noexcept;
        double process(int channel, double x) noexcept;
    };
    KWeighting kIn, kOut;
    double meanIn = 0.0, meanOut = 0.0, matchDb = 0.0, matchTarget = 0.0, longIn = 0.0, blockIn = 0.0, blockOut = 0.0;
    int blockCount = 0;
    int matchIndex = -1, deltaIndex = -1;
    juce::AudioBuffer<float> dry, dryForMix;
    dsp::AlignmentDelay mixAlign, wetAlign, dryAlign;
    dsp::LinearRamp mixAmount, onAmount;
    int rampSteps = 1, maximumBlockSize = 1, latency = 0, currentProgram = 0;
    std::atomic<double> currentSampleRate { 48000.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamloMagProcessor)
};
} // namespace hamlo::saturator
