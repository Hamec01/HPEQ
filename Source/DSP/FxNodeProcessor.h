#pragma once

#include <array>
#include <vector>

#include "DSP/DspConfig.h"
#include "DSP/LayerSnapshot.h"
#include "DSP/Processors.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// One FX node: a chain of up to 6 processors on the node's band (DECISIONS D2) and the node's delta
// added to the layer output. The band M comes from the layer's CrossoverTree (H3, DECISIONS D4):
//
//   b   = M (sum of the elementary bands inside [low, high])
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

    // `band` is this node's band from the tree; out += presence * nodeMix * (chain(band) - band).
    void process(const float* const* band, const float* presence, float* const* out, int numChannels, int numSamples) noexcept;

    bool isRunning() const noexcept { return running; }
    double targetLowHz() const noexcept { return lowHz; }
    double targetHighHz() const noexcept { return highHz; }
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
    double lowHz = 250.0, highHz = 700.0; // clamped to [1 Hz, 0.45 fs], low <= high
    LinearRamp nodeMix;
    std::array<Slot, kMaxFxPerNode> slots;

    std::array<std::vector<float>, kMaxChannels> chain, work;
};
} // namespace hpeq::dsp
