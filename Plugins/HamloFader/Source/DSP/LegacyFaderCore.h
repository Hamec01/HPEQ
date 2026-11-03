#pragma once
#include "DSP/FaderGain.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// Exact single whole-layer Gain chain: slot fade -> node delta -> layer sum.
// The host wrapper keeps its original additional bypass interpolation.
class LegacyFaderCore
{
public:
    void prepare(double rate,int block);
    void set(const FxSlotParams&) noexcept;
    void process(float* const*,int channels,int samples) noexcept;
    float getFaderInputDb(int)const noexcept{return running?gain.inputDb():-120.f;}
    float getFaderInputLongDb(int)const noexcept{return running?gain.inputLongDb():-120.f;}
    float getFaderOutputDb(int)const noexcept{return running?gain.outputDb():-120.f;}
private:
    GainProcessor gain;
    FxSlotParams params;
    LinearRamp nodeMix,slotMix;
    bool nodeRunning=false,running=false;
    int steps=1;
    std::array<std::vector<float>,2> chain,work;
};
class FaderEngine
{
public:
    void prepare(double rate,int block){core.prepare(rate,block);if(current.mix.active)core.set(current.mix.slots[0]);}
    void publish(const FaderSnapshot& s) noexcept{current=s;pending=true;}
    void update() noexcept{if(pending){if(current.mix.active)core.set(current.mix.slots[0]);pending=false;}}
    void process(float* const* audio,int channels,int samples,const Transport&) noexcept{update();core.process(audio,channels,samples);}
    int getLatencySamples()const noexcept{return 0;}
    const LegacyFaderCore& getMixNode()const noexcept{return core;}
private:
    FaderSnapshot current;
    bool pending=false;
    LegacyFaderCore core;
};
}
