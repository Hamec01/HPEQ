#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/DelayLine.h"
#include "DSP/LegacyEchoCore.h"
#include "DSP/Ramp.h"
#include "Standalone/ModuleVariant.h"
#include "Standalone/ScoreParameters.h"
#include "DSP/ScoreEngine.h"

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
    const dsp::LegacyEchoCore& moduleNode() const noexcept { return engine; }
    juce::AudioProcessorValueTreeState& valueTreeState() noexcept { return parameters; }
    bool presetModified() const noexcept;
    float scoreOutputDb() const noexcept { return scoreMeter.load(std::memory_order_relaxed); }
    void setQuickEnabled(bool);
    void expandQuickToScore();
    hamlo::echo::ScoreSettings effectiveScoreSettings(double bpm=120) const noexcept {return readScoreSettings(scoreValues,bpm);}

private:
    dsp::FxSlotParams currentSlot(bool active) const noexcept;
    dsp::Transport currentTransport() noexcept;
    uint64_t parameterHash() const noexcept;
    void choosePath(int,const hamlo::echo::ScoreSettings&,const dsp::FxSlotParams&) noexcept;
    void processScorePaths(juce::AudioBuffer<float>&,int,int,const dsp::Transport&,int,const hamlo::echo::ScoreSettings&,const dsp::FxSlotParams&) noexcept;

    ModuleKind kind;
    juce::AudioProcessorValueTreeState parameters;
    std::vector<std::atomic<float>*> rawValues;
    std::vector<std::atomic<float>*> scoreValues;
    juce::AudioParameterBool* bypassParameter = nullptr;

    dsp::LegacyEchoCore engine;
    std::array<hamlo::echo::ScoreEngine,2> scoreEngines;
    std::array<hamlo::echo::ScoreSettings,2> scoreConfigs;
    std::array<juce::AudioBuffer<float>,2> pathWork;
    dsp::LinearRamp incomingWeight,retiringWeight,scoreDry;
    int activePath=-1,retiringPath=-2,forcedRetire=0;
    int64_t retireAge=0,retireLimit=0;
    dsp::FxSlotParams retiringLegacy,lastLegacy,activeLegacy;
    hamlo::echo::ScoreSettings lastScore;
    int lastMode=0;
    std::atomic<uint64_t> bulkRevision{0},loadedHash{0};
    std::atomic<bool> restorePending{false};
    juce::CriticalSection stateWriter;
    std::atomic<float> scoreMeter{-120};
    std::atomic<bool> hasRetiringTail{false};
    juce::AudioBuffer<float> dry;
    dsp::AlignmentDelay wetAlign, dryAlign;
    dsp::LinearRamp wetAmount;
    int maximumBlockSize = 1;
    std::atomic<int> currentProgram {0};
    int fixedLatency = 0;
    int bypassRampSteps = 1;
    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<double> hostBpm { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePluginProcessor)
};
} // namespace hpeq::standalone
