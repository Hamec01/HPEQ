#pragma once

#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

namespace hamlo::comp
{
// Factory presets: the compressor presets of HPEQ (Source/Core/ModulePresets.cpp, kCompressor). A preset
// names only the values it sets, in the parameter IDs and units; everything else is the default. `mix` is
// the MIX knob.
struct Preset
{
    const char* name;
    float mix;
    std::vector<std::pair<const char*, double>> values;
};

const std::vector<Preset>& factoryPresets();

// The value a preset gives `id`, or `fallback` when it leaves the parameter alone.
double presetValue(const Preset&, const juce::String& id, double fallback);
} // namespace hamlo::comp
