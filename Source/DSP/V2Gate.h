#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

namespace hamlo::zastava
{
enum class Mode { Clean, Drum, Duck, Rhythm };
enum class Profile { Soft, Hard };
struct Settings
{
    bool enabled=false,bypass=false,external=false,expand=false,rms=false,sync=true;
    Mode mode=Mode::Clean;
    Profile profile=Profile::Soft;
    double thresholdDb=-40,rangeDb=40,ratio=4,kneeDb=6,hysteresisDb=4;
    double attackMs=1,holdMs=20,releaseMs=100,detectorMs=10,lookaheadMs=2;
    double keyHpHz=0,keyLpHz=0,stereoLink=1,mix=1,outputDb=0;
    double bodyMs=20,tailMs=100,onsetDb=3,retriggerMs=10;
    int steps=16;
    double stepBeats=.25,stepMs=125;
    std::array<double,32> levels=[](){std::array<double,32> a{};a.fill(1);return a;}();
    bool listen=false;
};
struct Clock
{
    double bpm=120,ppq=0;
    bool playing=true,hasPpq=false;
};
struct Reading {double gain=1,keyDb=-120;bool open=true,missingKey=false;std::uint64_t hits=0;};
class Gate
{
public:
    Gate();
    ~Gate();
    void prepare(double sampleRate,int maximumBlock);
    void reset() noexcept;
    void set(const Settings&) noexcept;
    void process(float* const* audio,int channels,int frames,const float* const* key=nullptr,int keyChannels=0,const Clock& = {}) noexcept;
    int latency() const noexcept;
    Reading reading() const noexcept;
    static bool implemented() noexcept;
    static const char* sourceIdentity() noexcept;
private:
    struct State;
    std::unique_ptr<State> state;
};
}
