#pragma once

#include <array>
#include <vector>
#include "DSP/LegacyDelay.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// The single Echo slot of the M0 mix node, with its original arithmetic and lifecycle.
// No LayerEngine, processor pool, other effects or band/channel routing is needed.
class LegacyEchoCore
{
public:
    void prepare(double sampleRate, int maximumBlockSize);
    void reset() noexcept;
    void setTargets(const FxSlotParams&) noexcept;
    void process(float* const* channels, int numChannels, int samples, const Transport&) noexcept;
    int getLatencySamples() const noexcept { return 0; }
    float getDelayOutputDb(int) const noexcept { return running ? delay.outputLevelDb() : -120.0f; }
    float getDelayDuckDb(int) const noexcept { return running ? delay.duckingDb() : 0.0f; }

private:
    DelayProcessor delay;
    FxSlotParams params;
    LinearRamp nodeMix, mix, inputGate;
    std::array<std::vector<float>, kMaxChannels> work, wet, chain;
    int mixSteps = 1, silenceSamples = 1, silentSamples = 0;
    bool running = false;
};
}
