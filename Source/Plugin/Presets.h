#pragma once

#include <optional>
#include <vector>

#include <juce_core/juce_core.h>

#include "Core/FxProjectState.h"
#include "Core/ModulePresets.h"

namespace hpeq::presets
{
// Presets pass (owner 2026-10-26). Whole-plugin presets: the factory library (Assets/Presets/Factory,
// 185 saved states, embedded in the plug-in) and the user's own (.hpeq, the same saved state). Module
// presets: the factory table (ModulePresets) and the user's own (.hpeqmod, one module as saved in a project).

struct FactoryPreset
{
    juce::String pack, category, group, name; // group: a sub-folder of the category (Mastering / Pop), or empty
    const char* data = nullptr;
    int size = 0;
};
// Ordered as the library numbers them (pack, category, preset).
const std::vector<FactoryPreset>& factoryPresets();

// %APPDATA%/HamloProd/HPEQ/Presets (User/*.hpeq, Modules/<Type>/*.hpeqmod); tests pass their own root.
juce::File userRoot();
juce::File userPresetFolder(const juce::File& root = userRoot());
juce::File userModuleFolder(FxType, const juce::File& root = userRoot());
juce::Array<juce::File> listFiles(const juce::File& folder, const juce::String& pattern);
// A file name from what the user typed (no path characters, never empty).
juce::String safeFileName(const juce::String& name);

// The module as the preset sets it: the module's defaults, then the preset's values and mix; a
// Revolution pattern is made from the preset's seed and character. `enabled` is kept by the caller.
FxSlotState makeModule(FxType, const ModulePreset&);
bool saveModulePreset(const FxSlotState&, const juce::File&);
// Only a module of the `expected` type loads.
std::optional<FxSlotState> loadModulePreset(const juce::File&, FxType expected);
} // namespace hpeq::presets
