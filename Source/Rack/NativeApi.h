#pragma once

#include <array>
#include <atomic>
#include <memory>
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
    int count = 0, latency = 0;
    std::array<float,kMaxParameters> physical {};
};
static_assert(std::is_trivially_copyable_v<Values>);

struct Parameter
{
    juce::String id;
    juce::NormalisableRange<float> range;
    float initial = 0;
    bool discrete = false;
};
struct Binding
{
    juce::RangedAudioParameter* parameter = nullptr;
    std::atomic<float>* raw = nullptr;
};

// Implemented by generated, namespace-isolated copies of the pinned native
// wrappers. No native editor or VST loader participates in audio processing.
struct Handle
{
    std::unique_ptr<juce::AudioProcessor> processor;
    std::vector<Binding> bindings;
    void bind(juce::AudioProcessorValueTreeState&);
};
Handle createComp(); Handle createVerb(); Handle createEcho(); Handle createMag();
Handle createUdarnik(); Handle createZastava(); Handle createShirota();
Handle createRedWall(); Handle createFader(); Handle createRevolution();
int latencyComp(double); int latencyVerb(double); int latencyEcho(double); int latencyMag(double);
int latencyUdarnik(double); int latencyZastava(double); int latencyShirota(double);
int latencyRedWall(double); int latencyFader(double); int latencyRevolution(double);
Handle create(FxType);
juce::String productName(FxType);

const std::vector<Parameter>& layout(FxType); // message thread, initialized once
int fixedLatency(FxType,double sampleRate);  // message thread only
bool describe(FxType,const juce::NamedValueSet&,double rate,Values&,juce::String& error);
juce::NamedValueSet defaultParameters(FxType);
struct Scene
{
    std::array<std::array<std::uint8_t,16>,7> lanes {};
    std::array<std::uint16_t,3> locks {65535,65535,65535};
    std::array<std::uint16_t,3> allow {63,1023,65535};
};
juce::MemoryBlock makeRevolutionScene(const juce::NamedValueSet&,const Scene&,juce::String& error);

class Runtime
{
public:
    Runtime(FxType,const juce::MemoryBlock& structuralState,const juce::NamedValueSet&,
            double sampleRate,int maximumBlockSize,juce::String& error);
    ~Runtime();
    Runtime(const Runtime&)=delete;
    Runtime& operator=(const Runtime&)=delete;
    bool valid() const noexcept { return ready; }
    int latency() const noexcept { return reportedLatency; }
    double tailSeconds() const noexcept;
    void retain() noexcept { users.fetch_add(1,std::memory_order_acq_rel); }
    void release() noexcept { users.fetch_sub(1,std::memory_order_acq_rel); }
    int inUse() const noexcept { return users.load(std::memory_order_acquire); }
    void process(float* const*,int channels,int samples,const Values&,double bpm,
                 bool playing,bool hasPosition,double ppq) noexcept;
    juce::MemoryBlock state(); // message thread, quiescent runtime only
    juce::AudioProcessor& referenceProcessor() noexcept { return *handle.processor; }

private:
    struct PlayHead final : juce::AudioPlayHead
    {
        double bpm=120,ppq=0; bool playing=false,position=false;
        juce::Optional<PositionInfo> getPosition() const override;
    } playHead;
    Handle handle;
    FxType type;
    int reportedLatency=0,maxBlock=1;
    bool ready=false;
    double rate=48000;
    std::atomic<int> users {0};
    juce::MidiBuffer midi;
};
} // namespace hpeq::native
