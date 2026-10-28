#pragma once

#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/LimiterSettings.h"

namespace hamlo::redwall
{
// HamloRedWall's host parameters: the IDs, ranges, defaults and the state tag of the HamloRedWall module VST3 HPEQ
// built (Source/Standalone/ModuleVariant.cpp, makeRedWall), so its automation and saved projects load here unchanged.
// Order: the panel's knobs (Param enum, STYLE last), then its switches (Switch enum), then bypass.
struct ParameterSpec
{
    const char* id;
    const char* name;
    float minimum, maximum, defaultValue, skew;
    const char* unit;
    juce::StringArray choices; // empty = a float knob
    int slot;                  // dsp::slot index, kMixSlot = the MIX, kPluginOnly = the plug-in's own stage
};

constexpr int kMixSlot = -1, kPluginOnly = -2;

inline const std::vector<ParameterSpec>& parameterSpecs()
{
    namespace s = dsp::slot;
    static const std::vector<ParameterSpec> specs {
        { "thresholdDb", "Loudness", -30.0f, 0.0f, 0.0f, 1.0f, "dB", {}, s::kLimThresholdDb },
        { "ceilingDb", "Limit", -20.0f, 0.0f, -1.0f, 1.0f, "dB", {}, s::kCeilingDb },
        { "releaseMs", "Release", 1.0f, 1000.0f, 60.0f, 0.33f, "ms", {}, s::kLimiterReleaseMs },
        { "lookaheadMs", "Lookahead", 0.1f, 10.0f, 2.0f, 0.5f, "ms", {}, s::kLookaheadMs },
        { "link", "Link", 0.0f, 1.0f, 1.0f, 1.0f, "", {}, s::kLimLink },
        { "outputDb", "Output", -24.0f, 0.0f, 0.0f, 1.0f, "dB", {}, s::kLimOutputDb },
        { "mix", "Mix", 0.0f, 1.0f, 1.0f, 1.0f, "", {}, kMixSlot },
        { "style", "Style", 0.0f, 2.0f, 0.0f, 1.0f, "", { "GLASNOST", "STAKHANOV", "OCTOBER" }, s::kLimStyle },
        { "truePeak", "True Peak", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" }, s::kLimTruePeak },
        { "autoRelease", "Release", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Manual", "Auto" }, s::kLimAutoRelease },
        { "monitor", "Monitor", 0.0f, 2.0f, 0.0f, 1.0f, "", { "Normal", "Unity", "Delta" }, s::kLimMonitor },
    };
    return specs;
}

constexpr const char* kStateTag = "HamloRedWall";
constexpr int kEditorWidth = 1180, kEditorHeight = 700;
} // namespace hamlo::redwall
