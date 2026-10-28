#pragma once

#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

namespace hamlo::redwall
{
// Factory presets: the limiter presets of HPEQ (Source/Core/ModulePresets.cpp, kLimiter). A preset names only
// the values it sets, in the parameter IDs and units; everything else is the default. `mix` is the MIX knob.
struct Preset
{
    enum Depth { Gentle, Medium, Heavy };
    const char* name;
    float mix;
    std::vector<std::pair<const char*, double>> values;
    // the genre presets (docs/GENRE_PRESETS.md): genre and group on the preset screen, one line on what it is for,
    // and the material / gain-reduction depth HamloRedWall_PresetLab calibrates LOUDNESS on; empty for the others
    const char* genre = "";
    const char* group = "";
    const char* note = "";
    const char* material = "";
    Depth depth = Medium;
};

const std::vector<Preset>& factoryPresets();

// The value a preset gives `id`, or `fallback` when it leaves the parameter alone.
double presetValue(const Preset&, const juce::String& id, double fallback);
} // namespace hamlo::redwall
