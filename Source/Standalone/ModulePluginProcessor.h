#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/Revolution.h"
#include "Core/RevolutionSimple.h"
#include "DSP/DelayLine.h"
#include "DSP/LayerEngine.h"
#include "DSP/Ramp.h"
#include "Link/HamloLinkClient.h"
#include "Standalone/ModuleVariant.h"

namespace hpeq::standalone
{
class ModulePluginProcessor final : public juce::AudioProcessor
{
public:
    explicit ModulePluginProcessor(ModuleKind);
    ~ModulePluginProcessor() override;

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
    int getCurrentProgram() override { return currentProgram; }
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
    const dsp::FxNodeProcessor& moduleNode() const noexcept { return engine.getMixNode(); }
    dsp::FxSlotParams currentSlot(bool active) const noexcept;

    // HamloRevolution: the pattern, the LISTEN feel it was generated from and the seed - the module's state in HPEQ
    // (FxSlotState keys timeA .. revSeed), here the plug-in's (saved in its state, set by the page, made by programs).
    // R1: a scene = the pattern, feel, seed and the lanes' settings. 8 scenes; the SCENE parameter picks the one that
    // plays (and that the page shows and edits). A scene never edited is a copy of scene 1.
    struct RevolutionState
    {
        revolution::Pattern pattern;
        revolution::Feel feel {};
        juce::int64 seed = 1;
        revolution::Lanes lanes {};
        std::array<float, revolution::kSteps> groove {}; // R4: the groove table (quarter notes per 1/16), all 0 = none
        std::array<float, revolution::kSteps> heard {};  // R5: the input's timing per 1/16 captured by SIMPLE's GENERATE
        std::uint16_t heardMask = 0;                     // ... bit s: 1/16 s had an onset (presence >= 0.5)
    };
    RevolutionState getRevolutionState() const;          // the current scene
    void setRevolutionState(const RevolutionState&);     // the current scene
    int currentScene() const noexcept;
    RevolutionState getScene(int) const;
    void setScene(int, const RevolutionState&);
    bool isSceneUsed(int) const;
    void clearScene(int);                                // back to following scene 1 (scene 1: no change)
    void resetScenes(const RevolutionState&);            // scene 1 = this, the others follow it (a program)

    // CONNECT TO HPDG (docs/HPDG_LINK.md): the link to a running HPDG; on / off and the HPDG followed are saved.
    hamlo::link::Client& getHpdgLink() noexcept { return hpdgLink; }

    // R4: the SIMPLE page. Its settings are the simple* parameters; GENERATE writes the shown scene and the lane
    // parameters from the generator (Core/RevolutionSimple). Linked to HPDG, a new HPDG pattern regenerates (not when
    // LOCK is on); a 4 Hz message-thread timer looks for one.
    enum class Page { Simple, Advanced };
    Page getPage() const noexcept { return page.load() == 0 ? Page::Simple : Page::Advanced; }
    void setPage(Page p) noexcept { page.store(p == Page::Simple ? 0 : 1); }
    void generateSimple(bool newSeed);
    void simpleEdited();   // a SIMPLE setting changed: regenerate unless LOCK
    bool followHpdg();     // a new HPDG pattern taken (and regenerated from, in SIMPLE without LOCK)
    revolution::simple::Settings simpleSettings() const;
    revolution::simple::Result lastSimple() const;

private:
    dsp::Transport currentTransport() noexcept;

    ModuleKind kind;
    juce::AudioProcessorValueTreeState parameters;
    std::vector<std::atomic<float>*> rawValues;
    juce::AudioParameterBool* bypassParameter = nullptr;

    dsp::LayerEngine engine;
    juce::AudioBuffer<float> dry;
    dsp::AlignmentDelay wetAlign, dryAlign;
    dsp::LinearRamp wetAmount;
    int maximumBlockSize = 1;
    int currentProgram = 0;
    int fixedLatency = 0;
    int bypassRampSteps = 1;
    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<double> hostBpm { 120.0 };

    juce::CriticalSection revolutionLock; // message thread and the host's state calls; never the audio thread
    std::array<RevolutionState, revolution::kScenes> scenes;
    std::array<bool, revolution::kScenes> sceneUsed { true };
    void publishScene(int) noexcept; // lock held
    // per scene, for the audio thread: 12 pattern quads, the humanize seed, 7 x 15 lane floats, 4 x 4 quads of the
    // SHAPE / THROW lanes' steps 1 .. 16, 7 bands
    static constexpr int kSceneLanes = 13, kSceneShapes = kSceneLanes + revolution::kLanes * revolution::kLaneFloats,
                         kSceneBands = kSceneShapes + 4 * (revolution::kLanes - revolution::kEngines), kSceneGroove = kSceneBands + revolution::kLanes,
                         kSceneFloats = kSceneGroove + revolution::kSteps;
    std::array<std::array<std::atomic<float>, kSceneFloats>, revolution::kScenes> sceneSlots {};
    int sceneParameterIndex = -1;
    hamlo::link::Client hpdgLink;
    std::atomic<int> page { 0 };
    revolution::simple::Result simpleResult; // revolutionLock
    struct Follower;
    std::unique_ptr<Follower> follower; // after hpdgLink: stopped first

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePluginProcessor)
};
} // namespace hpeq::standalone
