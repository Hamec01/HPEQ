#pragma once
#include "DSP/HamloShirota.h"

namespace hpeq::dsp
{
class LegacyStereoProcessor
{
public:
    void prepare(double sampleRate) { core.prepare(sampleRate); } // allocates (SAFE rings)
    void reset() noexcept { core.reset(); }
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    // HamloShirota (HamloShirota.h): runs instead of the M6 law when any of its options is set.
    static bool usesShirota(const FxSlotParams&) noexcept;
    // Latency: the SAFE look-ahead (2.5 ms) when SAFE is on, else 0.
    static int latencySamples(const FxSlotParams& p, double sampleRate) noexcept
    {
        return usesShirota(p) && p.values[slot::kStSafe] != 0.0f ? ShirotaCore::safeLookaheadSamples(sampleRate) : 0;
    }
    const ShirotaCore& shirota() const noexcept { return core; }
    bool shirotaActive() const noexcept { return coreActive; }

private:
    ShirotaCore core;
    bool coreActive = false;
};
}
