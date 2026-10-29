#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/LayerSnapshot.h"

namespace hpeq::standalone
{
enum class ModuleKind
{
    Comp,
    Verb,
    Echo,
    Mag,
    RedWall,
    Udarnik,
    Zastava,
    Shirota,
    Fader // HamloFader (the Gain slot): HPEQ has no module VST3 for it; added here (kind 8, plug-in code HmFd)
};

enum class ValueTransform
{
    Direct,
    Log2,
    OneMinus,
    HighPassOff,
    LowPassOff,
    MonoBassOff
};

struct ParameterSpec
{
    juce::String id;
    juce::String name;
    float minimum = 0.0f;
    float maximum = 1.0f;
    float defaultValue = 0.0f;
    float interval = 0.0f;
    float skew = 1.0f;
    juce::String unit;
    juce::StringArray choices;
    int slotValueIndex = -1; // -1 is the slot's wet/dry mix.
    ValueTransform transform = ValueTransform::Direct;
};

struct ModuleSpec
{
    ModuleKind kind;
    FxType type;
    const char* productName;
    int editorWidth;
    int editorHeight;
    int knobCount;
    std::vector<ParameterSpec> parameters; // knobs first, then switches, matching the panel enums.
};

const ModuleSpec& moduleSpec(ModuleKind);
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(ModuleKind);
dsp::FxSlotParams makeSlot(ModuleKind, const std::vector<std::atomic<float>*>& values, bool active) noexcept;
int maximumLatencySamples(ModuleKind, double sampleRate) noexcept;

} // namespace hpeq::standalone
