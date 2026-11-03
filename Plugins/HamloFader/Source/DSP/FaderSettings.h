#pragma once
#include <array>

namespace hpeq
{
// Original Gain tag and Stereo ordinal, retained without importing a project model.
enum class FxType{Gain=9};
enum class ChannelMode{Stereo=0};
namespace dsp
{
struct FxSlotParams
{bool active=false;FxType type=FxType::Gain;float mix=1;std::array<float,48> values{};};
namespace slot
{constexpr int kGainDb=0,kFadPolarity=1,kFadDelayMs=2,kFadMute=3,kFadSolo=4,kFadTargetDb=5;}
struct Transport{double bpm=120;bool playing=false,hasPosition=false;double ppqAtBlockStart=0;};
struct FaderNodeParams
{bool active=false;float mix=1;ChannelMode channel=ChannelMode::Stereo;int numSlots=0;std::array<FxSlotParams,1> slots{};};
struct FaderSnapshot{FaderNodeParams mix;};
}
}
