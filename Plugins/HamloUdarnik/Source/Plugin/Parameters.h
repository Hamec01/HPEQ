#pragma once

#include <cmath>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/TransientSettings.h"

namespace hamlo::udarnik
{
// HamloUdarnik's host parameters: the IDs, ranges, defaults and the state tag of the HamloUdarnik module VST3
// HPEQ built (Source/Standalone/ModuleVariant.cpp, makeUdarnik), so its automation and saved projects load here
// unchanged. Order: the panel's knobs (Param enum, MODE last), then its switches (Switch enum), then bypass.
enum class Transform
{
    Direct,
    Log2,    // RISE / DECAY: the page's x 0.5 .. 2, the DSP's log2
    OneMinus // SENSITIVITY: the DSP's strictness = 1 - sensitivity
};

struct ParameterSpec
{
    const char* id;
    const char* name;
    float minimum, maximum, defaultValue, skew;
    const char* unit;
    juce::StringArray choices; // empty = a float knob
    int slot;                  // dsp::slot index, kMixSlot = the MIX, kPluginOnly = the plug-in's output stage
    Transform transform = Transform::Direct;
};

constexpr int kMixSlot = -1, kPluginOnly = -2;

inline const std::vector<ParameterSpec>& parameterSpecs()
{
    namespace s = dsp::slot;
    static const std::vector<ParameterSpec> specs {
        { "attackDb", "Snap", -24.0f, 24.0f, 0.0f, 1.0f, "dB", {}, s::kAttackGainDb },
        { "sustainDb", "Body", -24.0f, 24.0f, 0.0f, 1.0f, "dB", {}, s::kSustainGainDb },
        { "speed", "Speed", 0.0f, 1.0f, 0.5f, 1.0f, "", {}, s::kTransientSpeed },
        { "rise", "Rise", 0.5f, 2.0f, 1.0f, 0.5f, "x", {}, s::kTrRiseLog2, Transform::Log2 },
        { "decay", "Decay", 0.5f, 2.0f, 1.0f, 0.5f, "x", {}, s::kTrDecayLog2, Transform::Log2 },
        { "sensitivity", "Sensitivity", 0.0f, 1.0f, 1.0f, 1.0f, "", {}, s::kTrStrictness, Transform::OneMinus },
        { "preserve", "Preserve", 0.0f, 1.0f, 1.0f, 1.0f, "", {}, s::kTrPreserve },
        { "mix", "Mix", 0.0f, 1.0f, 1.0f, 1.0f, "", {}, kMixSlot },
        { "outputDb", "Output", -24.0f, 12.0f, 0.0f, 1.0f, "dB", {}, s::kTrOutputDb },
        { "style", "Mode", 0.0f, 2.0f, 0.0f, 1.0f, "", { "PRAVDA", "UDARNIK", "PERESTROIKA" }, s::kTrStyle },
        { "lookahead", "Look-ahead", 0.0f, 2.0f, 1.0f, 1.0f, "", { "Sputnik", "Vostok", "Soyuz" }, s::kTrLookahead },
        { "listen", "Listen", 0.0f, 2.0f, 0.0f, 1.0f, "", { "Full", "Attack", "Body" }, s::kTrListen },
        // docs/UDARNIK_V2.md: a new instance runs the v2 engine with the output protection; a state saved without
        // `engine` (HPEQ's HamloUdarnik, HamloUdarnik 0.1.0) loads as CLASSIC without protection - it sounds as before
        { "engine", "Engine", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Classic", "V2" }, s::kTrEngine },
        { "protect", "Protect", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" }, kPluginOnly },
    };
    return specs;
}

inline float transformed(float value, Transform t) noexcept
{
    switch (t)
    {
        case Transform::Direct: return value;
        case Transform::Log2: return std::log2(juce::jlimit(0.5f, 2.0f, value));
        case Transform::OneMinus: return 1.0f - juce::jlimit(0.0f, 1.0f, value);
    }
    return value;
}

constexpr const char* kStateTag = "HamloUdarnik";
constexpr int kEditorWidth = 1180, kEditorHeight = 820;
} // namespace hamlo::udarnik
