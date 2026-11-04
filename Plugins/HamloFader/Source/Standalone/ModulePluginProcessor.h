#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/DelayLine.h"
#include "DSP/LegacyFaderCore.h"
#include "DSP/Ramp.h"
#include "Standalone/ModuleVariant.h"
#include "Standalone/FaderHostControls.h"
#include "Standalone/FaderAudioBridge.h"

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
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    const dsp::LegacyFaderCore& moduleNode() const noexcept { return engine.getMixNode(); }

// Control-thread actions return false if busy/full/out of order. Frames count processed
    // samples, not host transport positions. UINT64_MAX requests the next opportunity.
    bool requestAction(FaderAction, std::uint64_t frame=std::numeric_limits<std::uint64_t>::max()) noexcept;
    bool readStatus(FaderStatus& out) const noexcept { return status.read(out); }
    bool applyMatch();
    bool canApplyMatch() const;
    bool isProgramModified() const;
    int editorPage() const noexcept { return page.load(std::memory_order_relaxed); }
    void setEditorPage(int p) noexcept { page.store(juce::jlimit(0,2,p),std::memory_order_relaxed); }
    std::uint64_t nextActionFrame() const noexcept { return publishedFrames.load(std::memory_order_acquire); }

private:
    bool matchProposal(FaderStatus&) const;
    void processAudio(juce::AudioBuffer<float>&, bool forceBypass);
    hamlo::fader::Settings modernSettings(bool forceBypass=false) const noexcept;
    void publishStatus(bool bypassed) noexcept;
    void resetNewParameters(bool legacy);
    dsp::FxSlotParams currentSlot(bool active) const noexcept;
    dsp::Transport currentTransport() noexcept;

    ModuleKind kind;
    juce::AudioProcessorValueTreeState parameters;
    std::vector<std::atomic<float>*> rawValues;
    juce::AudioParameterBool* bypassParameter = nullptr;

    FaderHostControls controls;
    hamlo::fader::Kernel kernel;
    FaderCommands commands;
    FaderSnapshot<FaderStatus> status;
    std::atomic<std::uint64_t> epoch {1}, publishedFrames {0};
    std::uint64_t audioEpoch=0, processedFrames=0;
    hamlo::fader::Clock callbackClock;
    dsp::LinearRamp selection;
    juce::AudioBuffer<float> modern;
    dsp::FaderEngine engine;
    juce::AudioBuffer<float> dry;
    dsp::AlignmentDelay wetAlign, dryAlign;
    dsp::LinearRamp wetAmount;
    int maximumBlockSize = 1;
    std::atomic<int> currentProgram {0};
    std::atomic<int> page {0};
    int fixedLatency = 0;
    int bypassRampSteps = 1;
    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<double> hostBpm { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePluginProcessor)
};
} // namespace hpeq::standalone
