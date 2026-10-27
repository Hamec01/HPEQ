#pragma once

#include <array>

namespace hamlo::dsp
{
constexpr int kMaxChannels = 2;

// The transient shaper's settings, in the slot layout of HPEQ (Source/DSP/LayerSnapshot.h, slot::kAttackGainDb ..
// kTrOutputDb, indices 0 .. 10), so the code reads `p.values[slot::k...]` exactly as in HPEQ.
namespace slot
{
constexpr int kAttackGainDb = 0, kSustainGainDb = 1, kTransientSpeed = 2,
              kTrStyle = 3, kTrLookahead = 4, kTrRiseLog2 = 5, kTrDecayLog2 = 6, kTrStrictness = 7, kTrPreserve = 8, kTrListen = 9,
              kTrOutputDb = 10, kCount = 11;
}

struct TransientSettings
{
    std::array<float, slot::kCount> values {};
};
} // namespace hamlo::dsp
