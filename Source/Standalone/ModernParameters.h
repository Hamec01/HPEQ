#pragma once
#include "Standalone/ModuleVariant.h"
#include "DSP/V2Gate.h"

namespace hpeq::standalone
{
namespace modern
{
enum Index : size_t
{
    Engine, Enabled, Profile, External, Expand, Rms, Sync, Threshold, Range,
    Ratio, Knee, Hysteresis, Attack, Hold, Release, Detector, Lookahead, Hp, Lp,
    Link, Mix, Output, Body, Tail, Onset, Retrigger, Steps, Beats, Ms, Step0,
    Count=Step0+32
};
using Raw=std::array<std::atomic<float>*,Count>;
const std::vector<ParameterSpec>& specs();
void append(juce::AudioProcessorValueTreeState::ParameterLayout&);
hamlo::zastava::Settings settings(const Raw&,bool bypass,int modeOverride=-1) noexcept;
int engine(const Raw&) noexcept;
}
}
