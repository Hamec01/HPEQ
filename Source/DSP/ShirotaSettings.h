#pragma once
#include <array>

// Shirota-owned parameter image. Numeric values are the original M0 contract;
// no layer graph, other processors or project-state types are required.
namespace hpeq
{
enum class FxType { Compressor, Limiter, Reverb, Delay, TransientShaper, Saturator, MonstaFx, StereoProcessor, GateExpander, Gain };
namespace dsp
{
struct FxSlotParams
{
    bool active=false;
    FxType type=FxType::Gain;
    float mix=1.f;
    std::array<float,48> values{};
};
namespace slot
{
constexpr int kStereoWidth=0,kStereoBalance=1,kStStyle=2,kStStereoize=3,kStSpace=4,
    kStMonoBassHz=5,kStRotation=6,kStSafe=7,kStMonoListen=8,kStOutputDb=9;
}
struct Transport
{
    double bpm=120.;bool playing=false,hasPosition=false;double ppqAtBlockStart=0.;
};
}
}
