#pragma once
#include "DSP/FaderKernel.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace hpeq::standalone
{
// Appended host ABI. Do not reorder; original eight live in ModuleVariant.
enum FaderControl : size_t
{
    Engine, Mode, Enabled, Input, Output, Left, Right, LeftDelay, RightDelay,
    Routing, Swap, Measure, MatchBoost, MatchCut, Ceiling, Matched, Guard,
    Profile, Strength, Boost, Cut, Floor, Hold, Idle, Rise, Fall, Shape, Points,
    Smooth, Sync, Depth, Beats, Free, Phase, RightPhase, Edge, Level0, ControlCount=Level0+32
};
struct FaderControlSpec
{
    juce::String id,name,unit,choices;
    float minimum=0,maximum=1,initial=0,interval=0;
};
const std::array<FaderControlSpec,ControlCount>& faderControls();
void appendFaderControls(juce::AudioProcessorValueTreeState::ParameterLayout&);
class FaderHostControls
{
public:
    void bind(juce::AudioProcessorValueTreeState&);
    float value(FaderControl c) const noexcept {return raw[c]->load(std::memory_order_relaxed);}
    hamlo::fader::Settings read(double gain,double mix,double target,int polarity,bool mute,bool bypass) const noexcept;
private:
    std::array<std::atomic<float>*,ControlCount> raw{};
};
}
