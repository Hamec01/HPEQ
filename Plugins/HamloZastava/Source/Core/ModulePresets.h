#pragma once

#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "DSP/ZastavaSettings.h"

namespace hpeq::presets
{
// Presets pass (owner 2026-10-26): every module has its own factory presets. A preset names only the
// values it sets, in the module's own parameter keys and units (the keys of FxSlotState::parameters and of
// the standalone module plug-ins); everything else stays at the module's default. `mix` is the module's mix.
struct ModulePreset
{
    const char* name;
    float mix;
    std::vector<std::pair<const char*, double>> values;
    const char* id="";
    const char* genre="";
    const char* group="";
    const char* note="";
    const char* description="";
    const char* material="";
    const char* depth="";
    double bpm=0;
};

const std::vector<ModulePreset>& factoryModulePresets(FxType);

// The value a preset gives `key`, or `fallback` when it leaves the key alone.
double presetValue(const ModulePreset&, const juce::String& key, double fallback);
std::vector<ModulePreset> genreModulePresets();
} // namespace hpeq::presets
