#pragma once

#include <array>
#include "DSP/VerbSettings.h"

namespace hpeq::dsp
{
struct VerbEqBand
{
    bool enabled = false;
    int type = 0; // Bell, LowShelf, HighShelf; Post EQ also LowCut, HighCut.
    float hz = 1000.0f, value = 0.0f, q = 0.707f;
};

struct VerbV2Settings
{
    FxSlotParams base;
    int engine = 0; // 0 Legacy (handled by the plug-in), 1 Clean .. 6 Bloom.
    int timeShape = 0, shapeSync = 0; // Off, Gated, Reverse Swell, Reverse Audio.
    float earlyDb = 0.0f, lateDb = 0.0f;
    float gateThresholdDb = -30.0f, shapeTimeMs = 250.0f, gateReleaseMs = 30.0f;
    float shimmerAmount = 0.0f, shimmerPitch = 12.0f, shimmerFeedback = 0.35f;
    float duckAttackMs = 5.0f, duckReleaseMs = 250.0f;
    std::array<VerbEqBand, 6> decayEq {}, postEq {};
};
} // namespace hpeq::dsp
