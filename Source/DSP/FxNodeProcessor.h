#pragma once

#include <array>
#include <vector>

#include "DSP/LayerSnapshot.h"
#include "DSP/Processors.h"
#include "DSP/Ramp.h"
#include "DSP/RegionSelector.h"

namespace hpeq::dsp
{
// One FX node: band selection of xE, a chain of up to 6 processors on the band (DECISIONS D2),
// and the node's delta added to the layer output:
//
//   b   = Select(xE)
//   s_0 = b;   s_k = s_{k-1} + mix_k * (P_k(s_{k-1}) - s_{k-1})      (mix_k == 1  ->  s_k = P_k(s_{k-1}))
//   out += nodeMix * (s_K - b)
//
// All processors in M2 have zero latency, so every term is already time-aligned. A disabled node
// fades its mix to 0 and then leaves the path; while enabled at mix 0 it keeps running (warm state,
// no click on re-entry) but adds nothing, so MIX-0 is exact.
class FxNodeProcessor
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset() noexcept;
    void setTargets(const FxNodeParams&) noexcept;

    void process(const float* const* xE, float* const* out, int numChannels, int numSamples, int controlPhase) noexcept;

    bool isRunning() const noexcept { return running; }
    float getGainReductionDb(int slotIndex) const noexcept;

private:
    struct Slot
    {
        bool wanted = false, running = false;
        FxType type = FxType::Gain;
        FxSlotParams params;
        LinearRamp mix;
        GainProcessor gain;
        CompressorProcessor compressor;
    };

    void runProcessor(Slot&, float* const* channels, int numChannels, int numSamples) noexcept;

    double sampleRate = 48000.0;
    int mixSteps = 1;
    bool wanted = false, running = false;
    LinearRamp nodeMix;
    RegionSelector selector;
    std::array<Slot, kMaxFxPerNode> slots;

    std::array<std::vector<float>, kMaxChannels> band, chain, work;
};
} // namespace hpeq::dsp
