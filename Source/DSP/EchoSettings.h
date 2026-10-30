#pragma once

#include <array>
#include "DSP/DspConfig.h"

namespace hpeq
{
// Keep the original numeric type and slot indices. This header has no project/layer dependency.
enum class FxType { Delay = 3 };

namespace dsp
{
struct FxSlotParams
{
    bool active = false;
    FxType type = FxType::Delay;
    float mix = 1.0f;
    std::array<float, 48> values {};
};

namespace slot
{
constexpr int kDelayTimeMs = 0, kDelaySync = 1, kFeedback = 2, kPingPong = 3, kDampingHz = 4, kDelayLowCutHz = 5, kDelayWidth = 6,
              kDlyCharacter = 7, kDlyTimeRMs = 8, kDlySyncR = 9, kDlyLink = 10, kDlyCross = 11, kDlyDriveDb = 12,
              kDlyModRate = 13, kDlyModDepth = 14, kDlyDiffusion = 15, kDlyPitch = 16, kDlyLowRes = 17, kDlyHighRes = 18,
              kDlyBandHz = 19, kDlyBandGainDb = 20, kDlyBandQ = 21, kDlyDuck = 22, kDlyFreeze = 23, kDlyLofi = 24, kDlyTimeMode = 25;
}

struct Transport
{
    double bpm = 120.0;
    bool playing = false;
    bool hasPosition = false;
    double ppqAtBlockStart = 0.0;
};
}
}
