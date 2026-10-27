#pragma once

#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

namespace hamlo::comp
{
// HamloComp's host parameters. The IDs, ranges, defaults and the state tag are the ones of the HamloComp
// module VST3 HPEQ built (Source/Standalone/ModuleVariant.cpp, makeComp), so automation and saved
// projects of that plug-in load here unchanged. Order: knobs as the panel's Param enum, then the switches
// as its Switch enum, then bypass.
struct ParameterSpec
{
    const char* id;
    const char* name;
    float minimum, maximum, defaultValue, skew;
    const char* unit;
    juce::StringArray choices; // empty = a float knob
};

inline const std::vector<ParameterSpec>& parameterSpecs()
{
    static const std::vector<ParameterSpec> specs {
        { "thresholdDb", "Threshold", -60.0f, 0.0f, -30.0f, 1.0f, "dB", {} },
        { "ratio", "Ratio", 1.0f, 20.0f, 4.0f, 0.5f, ":1", {} },
        { "kneeDb", "Knee", 0.0f, 40.0f, 6.0f, 1.0f, "dB", {} },
        { "attackMs", "Attack", 0.1f, 100.0f, 1.0f, 0.34f, "ms", {} },
        { "releaseMs", "Release", 10.0f, 1000.0f, 80.0f, 0.5f, "ms", {} },
        { "makeupDb", "Makeup", 0.0f, 24.0f, 0.0f, 1.0f, "dB", {} },
        { "mix", "Mix", 0.0f, 1.0f, 1.0f, 1.0f, "", {} },
        { "detector", "Detector", 0.0f, 1.0f, 0.0f, 1.0f, "", { "Peak", "RMS" } },
        { "character", "Character", 0.0f, 2.0f, 1.0f, 1.0f, "", { "Clean", "Punch", "Glue" } },
        { "autoMakeup", "Auto Makeup", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" } },
    };
    return specs;
}

constexpr const char* kStateTag = "HamloComp";
constexpr int kEditorWidth = 1180, kEditorHeight = 700;
} // namespace hamlo::comp
