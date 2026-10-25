#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <optional>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Analysis/AudioCapture.h"
#include "Analysis/LoudnessMeter.h"
#include "Core/FxProjectState.h"
#include "DSP/DelayLine.h"
#include "DSP/ProjectEngine.h"
#include "DSP/TubePreamp.h"

namespace hpeq
{
// Audio insert. Layer 1 runs PRE-EQ -> FX nodes through LayerEngine; layers 2-10 are stored but not
// processed until M7. Output gain and bypass come after the layer. Project edits from the UI go
// through editProject() (undoable); every change is broadcast to the editor on the message thread.
class FxPluginProcessor final : public juce::AudioProcessor, public juce::ChangeBroadcaster
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
    // Longest reverb tail of the processed layer (pre-delay + T60), so hosts keep processing after stop.
    double getTailLengthSeconds() const override { return tailSeconds.load(std::memory_order_relaxed); }

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

    // Undoable edit of the project (message thread). No-op if `change` leaves the project unchanged.
    void editProject(const juce::String& actionName, const std::function<void(FxProjectState&)>& change);
    juce::UndoManager& getUndoManager() noexcept { return undoManager; }

    // The layer the editor shows (saved with the state, not an undoable edit).
    void selectLayer(int index);
    // Session clipboard for one layer (Copy / Paste); paste is undoable.
    void copyLayer(int index);
    bool canPasteLayer() const noexcept { return layerClipboard.has_value(); }
    void pasteLayer(int index);

    // A/B comparison of two whole projects (not saved with the state; switching clears undo history).
    // A slot used for the first time starts as a copy of the current project.
    int getActiveAB() const noexcept { return activeAB; }
    void selectAB(int slot);
    void copyToOtherAB();

    // Real input/output PCM for the analysers (UI thread reads).
    AudioCapture& getCapture() noexcept { return capture; }
    double getCurrentSampleRate() const noexcept { return currentSampleRate.load(std::memory_order_relaxed); }
    const dsp::ProjectEngine& getEngine() const noexcept { return engine; }
    // Last host tempo seen from the play head (120 until a host reports one).
    double getHostBpm() const noexcept { return hostBpm.load(std::memory_order_relaxed); }

    struct MeterSnapshot
    {
        float inputPeak = 0.0f, inputRms = 0.0f, outputPeak = 0.0f, outputRms = 0.0f;
    };
    // Levels of the last processed block (linear). Written by the audio thread, lock-free.
    MeterSnapshot getMeters() const noexcept;
    // BS.1770-4 loudness (input and output) and output true peak, computed on the audio thread.
    const LoudnessMeter& getInputLoudness() const noexcept { return inputLoudness; }
    const LoudnessMeter& getOutputLoudness() const noexcept { return outputLoudness; }
    void resetLoudness() noexcept { inputLoudness.requestReset(); outputLoudness.requestReset(); }

private:
    static void measure(const juce::AudioBuffer<float>&, int numChannels, std::atomic<float>& peak, std::atomic<float>& rms) noexcept;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* outputGainDb = nullptr;
    juce::AudioParameterBool* bypassParameter = nullptr;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> gain { 1.0f };

    mutable juce::CriticalSection projectLock; // never taken by processBlock; also serialises publish()
    FxProjectState project;
    dsp::ProjectEngine engine;
    dsp::TubeColour tubes; // BRING LENIN / STALIN BACK: the tube colour of the whole output
    std::atomic<bool> tubeLenin { false }, tubeStalin { false };
    std::atomic<double> tubeLeninMix { 1.0 }, tubeStalinMix { 1.0 };
    juce::AudioBuffer<float> dry;   // sized in prepareToPlay; longer host blocks are chunked
    dsp::AlignmentDelay dryAlign;   // delays the dry copy by the layer latency (bypass, capture)
    dsp::LinearRamp wetAmount;      // 1 = layer output, 0 = bypassed (dry)
    int bypassRampSteps = 1;

    std::atomic<double> tailSeconds { 0.0 };
    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<double> hostBpm { 120.0 }; // last tempo seen from the play head (Delay tail estimate)
    void updateLatencyAndTail(const FxProjectState&);
    AudioCapture capture;
    LoudnessMeter inputLoudness, outputLoudness;
    juce::UndoManager undoManager;
    std::array<FxProjectState, 2> abStore;
    std::array<bool, 2> abUsed { true, false };
    int activeAB = 0;
    std::optional<LayerState> layerClipboard;
    std::atomic<float> inputPeak { 0.0f }, inputRms { 0.0f }, outputPeak { 0.0f }, outputRms { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPluginProcessor)
};
} // namespace hpeq
