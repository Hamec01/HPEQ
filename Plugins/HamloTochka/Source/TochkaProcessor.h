#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Analysis/AudioCapture.h"
#include "Analysis/LoudnessMeter.h"
#include "KeyedEq.h"

namespace hamlo::tochka
{
struct Preset
{
    juce::String name;
    const char* group;
    const char* description;
    std::array<hpeq::dsp::EqBandParams, 2> bands {};
    const char* genre = "Factory";
    bool firstBandExternal = false;
};

class Processor final : public juce::AudioProcessor
{
public:
    using Band = hpeq::dsp::EqBandParams;
    using Bands = std::array<Band, hpeq::kMaxEqNodesPerLayer>;

    Processor();

    const juce::String getName() const override { return "HamloTochka"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override;
    using juce::AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override { return program.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static const std::vector<Preset>& presets();
    Band band(int index) const noexcept;
    bool hasBand(int index) const noexcept;
    void editBand(int index, const Band&, bool inGesture = false);
    void deleteBand(int index);
    void beginBandGesture(int index, int field);
    void endBandGesture(int index, int field);
    float dynamicGain(int slot) const noexcept { return eq.dynamicOffsetDb(slot); }
    bool presetEdited() const noexcept { return modified.load(); }
    double sampleRateHz() const noexcept { return sampleRate.load(); }
    hpeq::AudioCapture& audioCapture() noexcept { return capture; }
    hpeq::AudioCapture& keyAudioCapture() noexcept { return keyCapture; }
    bool keyBusActive() const noexcept { return keyActive.load(std::memory_order_relaxed); }
    void enableCapture(bool enabled) noexcept { captureEnabled.store(enabled); }
    bool usesExternalKey(int index) const noexcept;
    void setExternalKey(int index, bool);
    bool showsDifference() const noexcept { return differenceView.load(std::memory_order_relaxed); }
    void showDifference(bool enabled) noexcept { differenceView.store(enabled, std::memory_order_relaxed); }

    void setDry(bool);
    bool isDry() const noexcept { return bypassValue->load() >= 0.5f; }
    void startLearn() noexcept;
    bool stopLearn();
    bool isLearning() const noexcept { return learnRequest.load() != 0; }
    float matchDb() const noexcept { return matchValue->load(); }
    void clearMatch();

    enum Field { On, Shape, Slope, Channel, Solo, Dynamic, Detect, Frequency, Gain, Q,
                 Threshold, Ratio, Range, Attack, Release, ExternalKey, FieldCount };

private:
    static juce::String parameterId(int band, int field);
    static juce::AudioProcessorValueTreeState::ParameterLayout parameterLayout();
    void process(juce::AudioBuffer<float>&, bool forceDry);
    static void writeParameter(juce::RangedAudioParameter&, float value, bool inGesture);
    void writeBand(int, const Band&, bool inGesture);
    void setMatch(float value);

    juce::AudioProcessorValueTreeState state;
    std::array<std::array<juce::RangedAudioParameter*, FieldCount>, hpeq::kMaxEqNodesPerLayer> bandParameters {};
    std::array<std::array<std::atomic<float>*, FieldCount>, hpeq::kMaxEqNodesPerLayer> bandValues {};
    std::atomic<float>* bypassValue = nullptr;
    std::atomic<float>* matchValue = nullptr;
    juce::RangedAudioParameter* bypassParameter = nullptr;
    juce::RangedAudioParameter* matchParameter = nullptr;
    std::atomic<int> program { 0 };
    std::atomic<bool> modified { false };
    std::atomic<bool> differenceView { false };
    std::atomic<std::uint32_t> occupied { 0 };

    hpeq::dsp::KeyedEq eq;
    int controlPhase = 0;
    std::atomic<double> sampleRate { 48000.0 };
    std::array<std::array<float, 512>, 2> dryStorage {};
    juce::SmoothedValue<float> wetMix, matchGain;
    hpeq::AudioCapture capture, keyCapture;
    std::atomic<bool> captureEnabled { false };
    std::atomic<bool> keyActive { false };

    hpeq::LoudnessMeter dryMeter, wetMeter;
    unsigned nextLearnId = 0;
    unsigned activeLearnId = 0;
    std::atomic<unsigned> learnRequest { 0 }, learnReady { 0 };
    std::atomic<float> learnedCorrection { 0.0f };
};
} // namespace hamlo::tochka
