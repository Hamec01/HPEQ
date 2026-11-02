#pragma once
#include <array>

namespace hpeq
{
// Retain the original reverb type value without importing the HPEQ project model.
enum class FxType { Reverb = 2 };
namespace dsp
{
// Original slot indices are a compatibility contract. Only Verb data is carried
// here; no layer snapshots, pools, graph, EQ model or other effect settings.
struct FxSlotParams
{
    bool active = false;
    FxType type = FxType::Reverb;
    float mix = 1.0f;
    std::array<float, 48> values {};
};
namespace slot
{
constexpr int kPredelayMs = 0, kDecaySeconds = 1, kHfRatio = 2, kSize = 3, kWidth = 4, kLowCutHz = 5,
              kRevSpace = 6, kRevColor = 7, kRevSync = 8, kRevAttack = 9, kRevEarlyDiff = 10, kRevLateDiff = 11,
              kRevModRate = 12, kRevModDepth = 13, kRevBassMult = 14, kRevBassXover = 15, kRevHighXover = 16,
              kRevHighCut = 17, kRevDistance = 18, kRevDuck = 19, kRevFreeze = 20;
}
struct Transport
{
    double bpm = 120.0;
    bool playing = false;
    bool hasPosition = false;
    double ppqAtBlockStart = 0.0;
};
} // namespace dsp
} // namespace hpeq
