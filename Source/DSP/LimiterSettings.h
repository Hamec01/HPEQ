#pragma once

#include <array>

namespace hamlo::dsp
{
constexpr int kMaxChannels = 2;

// The limiter's settings, in the slot layout of HPEQ (Source/DSP/LayerSnapshot.h, slot::kCeilingDb .. kLimOutputDb,
// indices 0 .. 9), so the code reads `p.values[slot::k...]` exactly as in HPEQ.
namespace slot
{
constexpr int kCeilingDb = 0, kLimiterReleaseMs = 1, kLookaheadMs = 2,
              kLimThresholdDb = 3, kLimTruePeak = 4, kLimAutoRelease = 5, kLimStyle = 6, kLimLink = 7, kLimMonitor = 8, kLimOutputDb = 9,
              kCount = 10;
}

struct LimiterSettings
{
    std::array<float, slot::kCount> values {};
};
} // namespace hamlo::dsp
