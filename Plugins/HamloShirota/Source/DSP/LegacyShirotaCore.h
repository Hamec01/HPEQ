#pragma once
#include "DSP/LegacyStereoProcessor.h"
#include "DSP/DelayLine.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// The original single Stereo insert, node delta and layer sum, in the same order.
// Legacy intentionally retains its initial ramps, conditional delay clocks,
// SAFE behaviour and one-channel behaviour.
class LegacyShirotaCore
{
public:
    void prepare(double sampleRate,int maximumBlockSize);
    void setTargets(const FxSlotParams&) noexcept;
    void process(float* const*,int channels,int samples,const Transport& = {}) noexcept;
    int getLatencySamples() const noexcept {return latency;}
    ShirotaCore::Meters getStereoMeters(int) const noexcept;
    bool getStereoScope(int,std::array<float,ShirotaCore::kScopePoints>&,std::array<float,ShirotaCore::kScopePoints>&) const noexcept;
private:
    LegacyStereoProcessor stereo;
    std::array<std::vector<float>,kMaxChannels> chain,work,aligned,bandDelayed;
    AlignmentDelay slotAlign,bandAlign,layerAlign;
    LinearRamp nodeMix,mix;
    FxSlotParams params;
    double fs=48000;
    int mixSteps=1,latency=0;
    bool running=false,hasTargets=false;
};
}
