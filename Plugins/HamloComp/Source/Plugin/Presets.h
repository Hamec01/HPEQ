#pragma once

#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

namespace hamlo::comp
{
// Factory presets: the compressor presets of HPEQ (Source/Core/ModulePresets.cpp, kCompressor). A preset
// names only the values it sets, in the parameter IDs and units; everything else is the preset base
// (ParameterSpec::presetBase). `mix` is the MIX knob.
struct Preset
{
    enum Depth { Gentle, Medium, Heavy };
    const char* name;
    float mix;
    std::vector<std::pair<const char*, double>> values;
    // what the preset is for (HamloComp_Lab plays it on that material, docs/COMPRESSOR_MODELS.md section 4):
    // drums, kick, snare, hats, vocal, backing, bass, keys, guitar, synth, mix
    const char* material = "";
    Depth depth = Medium;
    // the genre presets (docs/GENRE_PRESETS.md): genre and group on the preset screen and one line on what it does;
    // empty for the others
    const char* genre = "";
    const char* group = "";
    const char* note = "";
};

const std::vector<Preset>& factoryPresets();

// The value a preset gives `id`, or `fallback` when it leaves the parameter alone.
double presetValue(const Preset&, const juce::String& id, double fallback);
} // namespace hamlo::comp
