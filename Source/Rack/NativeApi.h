#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <functional>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>

namespace hpeq { enum class FxType; }
namespace hpeq::native
{
constexpr int kRevision = 1;
constexpr int kMaxParameters = 256;
class Runtime;

// Fixed-capacity, immutable audio-thread image. Runtime lifetime belongs to the
// message-thread registry; running slots retain it with an atomic user count.
struct Values
{
    Runtime* runtime = nullptr;
    Runtime* rightRuntime = nullptr; // independent mono instance in Dual Mono
    int count = 0, latency = 0;
    int faderRouting=0;bool faderSwap=false;
    bool additiveSend=false;
    std::array<float,kMaxParameters> physical {};
};
static_assert(std::is_trivially_copyable_v<Values>);

struct Parameter
{
    juce::String id;
    juce::NormalisableRange<float> range;
    float initial = 0;
    bool discrete = false;
    juce::String name, unit;
    juce::StringArray choices;
};
struct Binding
{
    juce::RangedAudioParameter* parameter = nullptr;
    std::atomic<float>* raw = nullptr;
};

// Implemented by generated, namespace-isolated copies of the pinned native
// wrappers and their original editors. VST files are not loaded at runtime.
struct Handle
{
    std::unique_ptr<juce::AudioProcessor> processor;
    std::vector<Binding> bindings;
    void bind(juce::AudioProcessorValueTreeState&);
};
Handle createComp(); Handle createVerb(); Handle createEcho(); Handle createMag();
Handle createUdarnik(); Handle createZastava(); Handle createShirota();
Handle createRedWall(); Handle createFader(); Handle createRevolution(bool realtime=false);
int latencyComp(double); int latencyVerb(double); int latencyEcho(double); int latencyMag(double);
int latencyUdarnik(double); int latencyZastava(double); int latencyShirota(double);
int latencyRedWall(double); int latencyFader(double); int latencyRevolution(double);
Handle create(FxType,bool realtime=false);
juce::String productName(FxType);

const std::vector<Parameter>& layout(FxType); // message thread, initialized once
int fixedLatency(FxType,double sampleRate);  // message thread only
bool describe(FxType,const juce::NamedValueSet&,double rate,Values&,juce::String& error);
juce::NamedValueSet defaultParameters(FxType);
juce::NamedValueSet legacyDefaults(FxType); // actual former HPEQ defaults for imported state
struct Scene
{
    std::array<std::array<std::uint8_t,16>,7> lanes {};
    std::array<std::uint16_t,3> locks {65535,65535,65535};
    std::array<std::uint16_t,3> allow {63,1023,65535};
};
juce::MemoryBlock makeRevolutionScene(const juce::NamedValueSet&,const Scene&,juce::String& error);
juce::MemoryBlock editRevolutionScene(const juce::MemoryBlock&,const juce::NamedValueSet&,const Scene&,juce::String& error);
bool readRevolutionScene(const juce::MemoryBlock&,Scene&,juce::String& error);
bool readRevolutionSceneAt(const juce::MemoryBlock&,int index,Scene&,juce::String& error);
struct SceneUpdate {virtual ~SceneUpdate()=default;virtual void apply(juce::AudioProcessor&) noexcept=0;};
std::unique_ptr<SceneUpdate> prepareRevolutionUpdate(const juce::MemoryBlock&,juce::String& error);
bool validStructuralState(FxType,const juce::MemoryBlock&,juce::String& error);
struct Program {int index=0;juce::String name;};
const std::vector<Program>& programs(FxType);
bool loadProgram(FxType,int,juce::NamedValueSet&,juce::MemoryBlock&,juce::String& error,float* rackMix=nullptr,bool completePlugin=false);
struct FaderView
{
    bool available=false,learning=false,canApply=false;
    double input=-120,output=-120,correction=0,proposal=0;
    double measured=-120,predictedPeak=-120;
};
FaderView readFader(juce::AudioProcessor&);
bool actionFader(juce::AudioProcessor&,int action,double& applied);
void installFaderActionBridge(juce::AudioProcessor&,std::function<bool(int)>);
float reductionComp(juce::AudioProcessor&);
float reductionRedWall(juce::AudioProcessor&);
struct Meters {float input=-120,output=-120,reduction=0;int latency=0;};

class Runtime
{
public:
    Runtime(FxType,const juce::MemoryBlock& structuralState,const juce::NamedValueSet&,
            double sampleRate,int maximumBlockSize,juce::String& error,int channels=2,bool rightChannel=false);
    ~Runtime();
    Runtime(const Runtime&)=delete;
    Runtime& operator=(const Runtime&)=delete;
    bool valid() const noexcept { return ready; }
    void reprepare(double sampleRate,int maximumBlockSize,int channels);
    FxType kind() const noexcept {return type;}
    int latency() const noexcept { return reportedLatency; }
    double tailSeconds() const noexcept;
    bool returnTarget() const noexcept;
    Meters meters() const noexcept {return {inputMeter.load(),outputMeter.load(),reductionMeter.load(),reportedLatency};}
    void retain() noexcept { users.fetch_add(1,std::memory_order_acq_rel); }
    void release() noexcept { users.fetch_sub(1,std::memory_order_acq_rel); }
    int inUse() const noexcept { return users.load(std::memory_order_acquire); }
    bool process(float* const*,int channels,int samples,const Values&,double bpm,
                 bool playing,bool hasPosition,double ppq) noexcept;
    juce::MemoryBlock state(); // message thread, quiescent runtime only
    juce::AudioProcessor& referenceProcessor() noexcept { return *handle.processor; }
    void requestEditorValue(int parameterIndex,float physical) noexcept;
    void applyPendingEditorValues(juce::NamedValueSet&) const;
    juce::MemoryBlock captureEditorState(); // native public serializer; control thread
    void synchronizeEditor(const Values&,bool preservePending=false);
    bool isSynchronizingEditor() const noexcept {return editorSynchronizing.load(std::memory_order_acquire);}

private:
    struct PlayHead final : juce::AudioPlayHead
    {
        double bpm=120,ppq=0; bool playing=false,position=false;
        juce::Optional<PositionInfo> getPosition() const override;
    } playHead;
    Handle handle;
    FxType type;
    int reportedLatency=0,maxBlock=1;
    bool ready=false,prepared=false;
    double rate=48000;
    std::atomic<int> users {0};
    std::atomic<float> inputMeter{-120},outputMeter{-120},reductionMeter{0};
    std::atomic<double> reportedTail{0};
    std::array<int,kMaxParameters> sourceIndex{};
    int phaseIndex=-1,rightPhaseIndex=-1,polarityIndex=-1;
    int returnEngineIndex=-1,returnRoutingIndex=-1,returnBypassIndex=-1;
    bool rightChannel=false;
    juce::MidiBuffer midi;
    std::array<std::atomic<float>,kMaxParameters> editorValues {};
    std::array<std::atomic<unsigned>,kMaxParameters> editorSerial {};
    std::atomic<unsigned> nextEditorSerial {1};
    std::atomic<bool> editorSynchronizing {false};
};
} // namespace hpeq::native
