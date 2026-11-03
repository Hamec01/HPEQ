#pragma once
#include <array>
namespace hpeq
{
// Original numeric slot/type image; no graph or unrelated DSP dependencies.
enum class FxType { Compressor, Limiter, Reverb, Delay, TransientShaper, Saturator, MonstaFx, StereoProcessor, GateExpander, Gain };
namespace dsp
{
struct FxSlotParams {bool active=false;FxType type=FxType::Gain;float mix=1.f;std::array<float,48> values{};};
namespace slot
{
constexpr int kGateThresholdDb=0,kGateRatio=1,kGateRangeDb=2,kGateAttackMs=3,kGateHoldMs=4,kGateReleaseMs=5,kGateHysteresisDb=6,
    kGateStyle=7,kGateLookahead=8,kGateKeyHpHz=9,kGateKeyLpHz=10,kGateKeyListen=11,kGateFlip=12,kGateOutputDb=13;
}
struct Transport {double bpm=120.;bool playing=false,hasPosition=false;double ppqAtBlockStart=0.;};
}
}
