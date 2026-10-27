#pragma once

#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

namespace hamlo::comp
{
// HamloComp's host parameters. The IDs, ranges and the state tag are the ones of the HamloComp module VST3
// HPEQ built (Source/Standalone/ModuleVariant.cpp, makeComp), so automation and saved projects of that
// plug-in load here unchanged (a state stores every parameter, so the defaults do not change old projects).
// Two sets of starting values, as HPEQ's module page (owner 2026-10-27, as HPEQ 2026-10-26):
//  - defaultValue: a fresh instance and the Default program are neutral - 0 dB, 1:1, no auto makeup (the
//    compressor does nothing until it is set);
//  - presetBase: what a factory preset starts from (HPEQ's presetBaseModule: -30 dB, 4:1, auto makeup on),
//    so the presets sound as they did.
// Order: knobs as the panel's Param enum, then the switches as its Switch enum, then bypass.
struct ParameterSpec
{
    const char* id;
    const char* name;
    float minimum, maximum, defaultValue, presetBase, skew;
    const char* unit;
    juce::StringArray choices; // empty = a float knob
};

inline const std::vector<ParameterSpec>& parameterSpecs()
{
    static const std::vector<ParameterSpec> specs {
        //  id             name           min      max      default  preset base  skew
        { "thresholdDb", "Threshold", -60.0f, 0.0f, 0.0f, -30.0f, 1.0f, "dB", {} },
        { "ratio", "Ratio", 1.0f, 20.0f, 1.0f, 4.0f, 0.5f, ":1", {} },
        { "kneeDb", "Knee", 0.0f, 40.0f, 6.0f, 6.0f, 1.0f, "dB", {} },
        { "attackMs", "Attack", 0.1f, 100.0f, 1.0f, 1.0f, 0.34f, "ms", {} },
        { "releaseMs", "Release", 10.0f, 1000.0f, 80.0f, 80.0f, 0.5f, "ms", {} },
        { "makeupDb", "Makeup", 0.0f, 24.0f, 0.0f, 0.0f, 1.0f, "dB", {} },
        { "mix", "Mix", 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, "", {} },
        { "detector", "Detector", 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, "", { "Peak", "RMS" } },
        { "character", "Character", 0.0f, 2.0f, 1.0f, 1.0f, 1.0f, "", { "Clean", "Punch", "Glue" } },
        { "autoMakeup", "Auto Makeup", 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, "", { "Off", "On" } },
    };
    return specs;
}

constexpr const char* kStateTag = "HamloComp";
constexpr int kEditorWidth = 1180, kEditorHeight = 700;
} // namespace hamlo::comp
