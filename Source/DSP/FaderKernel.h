#pragma once
#include "DSP/FaderMeter.h"
#include <array>
#include <memory>

namespace hamlo::fader
{
enum class Mode{Trim,Match,Ride,Motion};
enum class Profile{Natural,Fast};
enum class Routing{Stereo,MonoSum,LeftBoth,RightBoth};
enum class Measure{Rms,Lufs};
enum class Shape{Custom,Sine,Triangle};
struct Settings
{
    Mode mode=Mode::Trim;bool enabled=true,bypass=false;
    double inputDb=0,gainDb=0,outputDb=0,mix=1;
    double leftDb=0,rightDb=0,leftDelayMs=0,rightDelayMs=0;
    Routing routing=Routing::Stereo;bool swap=false;int polarity=0;
    Measure measure=Measure::Rms;double targetDb=-18,matchBoost=12,matchCut=24,matchCeiling=-1,matchedDb=0;bool peakGuard=true;
    Profile profile=Profile::Natural;double strength=1,maxBoost=6,maxCut=12,floorDb=-55,holdMs=80,idleMs=100,riseMs=250,fallMs=120;
    Shape shape=Shape::Custom;int points=16;std::array<double,32> levels=[](){std::array<double,32>a{};a.fill(1);return a;}();
    bool smooth=true,sync=true;double depthDb=0,beats=1,freeMs=500,phase=0,rightPhase=0,edgeMs=1;
};
struct Clock
{double bpm=120,ppq=0;bool hasPpq=false,playing=true;std::int64_t samplePosition=0;bool hasSamples=false;};
struct MatchProposal
{bool valid=false,limited=false;double gainDb=0,measuredDb=-120,predictedPeak=-120;};
struct Reading
{
    MeterReading input,output,learn;
    bool learning=false,active=false,motionRunning=false;
    double correctionDb=0,phase=0;
    std::uint64_t badInput=0;
};
class Kernel
{
public:
    Kernel();~Kernel();
    void prepare(double rate,int channels);
    void set(const Settings&) noexcept;
    Settings settings() const noexcept;
    void reset() noexcept;
    void startLearn() noexcept;
    MatchProposal stopLearn() noexcept;
    MatchProposal proposal() const noexcept;
    void process(float* const* audio,int channels,int count,const Clock& clock={}) noexcept;
    Reading reading() const noexcept;
private:
    struct Impl;std::unique_ptr<Impl> p;
};
}
