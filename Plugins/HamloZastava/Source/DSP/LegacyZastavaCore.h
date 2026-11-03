#pragma once
#include "DSP/LegacyGateProcessor.h"
#include "DSP/DelayLine.h"
#include "DSP/Ramp.h"
namespace hpeq::dsp
{
const char* zastavaS1SourceIdentity() noexcept;
// Original single-insert slot, node delta and layer sum, in the original order.
class LegacyZastavaCore
{
public:
    void prepare(double sampleRate,int maximumBlockSize);
    void setTargets(const FxSlotParams&) noexcept;
    void process(float* const*,int channels,int samples,const Transport& = {}) noexcept;
    int getLatencySamples() const noexcept {return latency;}
    ZastavaCore::Meters getGateMeters(int) const noexcept;
private:
    LegacyGateProcessor gateFx;
    std::array<std::vector<float>,kMaxChannels> chain,work,aligned,bandDelayed;
    AlignmentDelay slotAlign,bandAlign,layerAlign;
    LinearRamp nodeMix,mix;
    FxSlotParams params;
    double fs=48000;
    int mixSteps=1,latency=0;
    bool running=false,hasTargets=false;
};
}
