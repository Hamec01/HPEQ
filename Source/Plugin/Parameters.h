#pragma once

#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/MagSettings.h"

namespace hamlo::saturator
{
// HamloMag's host parameters: the IDs, ranges, defaults and the state tag of the HamloMag module VST3 HPEQ built
// (Source/Standalone/ModuleVariant.cpp, makeMag), so its automation and saved projects load here unchanged.
// Order: the knobs (the page's large row, then the small row), then the switches, then bypass.
struct ParameterSpec
{
    const char* id;
    const char* name;
    float minimum, maximum, defaultValue, skew;
    const char* unit;
    juce::StringArray choices; // empty = a float knob
    int slot;                  // dsp::slot index, kMixSlot = the MIX
};

constexpr int kMixSlot = -1;

inline const std::vector<ParameterSpec>& parameterSpecs()
{
    namespace s = dsp::slot;
    static const std::vector<ParameterSpec> specs {
        { "driveDb", "Drive", 0.0f, 48.0f, 6.0f, 1.0f, "dB", {}, s::kDriveDb },
        { "tone", "Tone", -1.0f, 1.0f, 0.0f, 1.0f, "", {}, s::kSatTone },
        { "satLowCutHz", "Low Cut", 10.0f, 1000.0f, 20.0f, 0.5f, "Hz", {}, s::kSatLowCutHz },
        { "satHighCutHz", "High Cut", 1000.0f, 20000.0f, 20000.0f, 0.5f, "Hz", {}, s::kSatHighCutHz },
        { "mix", "Mix", 0.0f, 1.0f, 1.0f, 1.0f, "", {}, kMixSlot },
        { "trimDb", "Output", -24.0f, 24.0f, 0.0f, 1.0f, "dB", {}, s::kTrimDb },
        { "harmonics", "Harmonics", 0.0f, 1.0f, 0.5f, 1.0f, "", {}, s::kSatHarmonics },
        { "evenOdd", "Even / Odd", -1.0f, 1.0f, 0.0f, 1.0f, "", {}, s::kSatEvenOdd },
        { "tapeBias", "Bias", 0.0f, 1.0f, 0.5f, 1.0f, "", {}, s::kSatTapeBias },
        { "wow", "Wow", 0.0f, 1.0f, 0.15f, 1.0f, "", {}, s::kSatWow },
        { "hiss", "Hiss", 0.0f, 1.0f, 0.15f, 1.0f, "", {}, s::kSatHiss },
        { "hum", "Hum", 0.0f, 1.0f, 0.0f, 1.0f, "", {}, s::kSatHum },
        { "crackle", "Crackle", 0.0f, 1.0f, 0.0f, 1.0f, "", {}, s::kSatCrackle },
        { "model", "Model", 0.0f, 5.0f, 1.0f, 1.0f, "", { "Classic", "Tape", "Tube", "Iron", "Germanium", "Vinyl" }, s::kSatModel },
        { "speed", "Speed", 0.0f, 2.0f, 0.0f, 1.0f, "", { "1", "2", "3" }, s::kSatSpeed },
        { "oversampling", "Oversampling", 0.0f, 2.0f, 1.0f, 1.0f, "", { "1x", "2x", "4x" }, s::kOversampling },
        { "punish", "Punish", 0.0f, 1.0f, 0.0f, 1.0f, "", { "Off", "On" }, s::kSatPunish },
        { "thump", "Thump", 0.0f, 1.0f, 0.0f, 1.0f, "", { "Off", "On" }, s::kSatThump },
        { "steep", "Steep", 0.0f, 1.0f, 0.0f, 1.0f, "", { "Off", "On" }, s::kSatSteep },
        { "autoGain", "Auto Gain", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" }, s::kAutoGain },
        { "noiseFollow", "Noise", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Always", "Follow" }, s::kSatNoiseFollow },
    };
    return specs;
}

constexpr const char* kStateTag = "HamloMag";
constexpr int kEditorWidth = 1180, kEditorHeight = 820;
} // namespace hamlo::saturator
