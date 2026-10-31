#include "Core/ModulePresets.h"

namespace hpeq::presets
{
namespace
{
const std::vector<ModulePreset> kStereo {
    { "Wider Mix", 1.0f, { { "style", 0 }, { "width", 1.3 } } },
    { "Mono Bass 120 Hz", 1.0f, { { "style", 0 }, { "width", 1.0 }, { "monoBassHz", 120 } } },
    { "Master Width", 1.0f, { { "style", 0 }, { "width", 1.15 }, { "monoBassHz", 100 } } },
    { "Narrow", 1.0f, { { "style", 0 }, { "width", 0.6 } } },
    { "Stereoize Mono", 1.0f, { { "style", 1 }, { "stereoize", 0.6 }, { "space", 0.4 } } },
    { "Cosmos Pad", 1.0f, { { "style", 2 }, { "space", 0.7 }, { "width", 1.2 } } },
};

}
const std::vector<ModulePreset>& factoryModulePresets(FxType) {return kStereo;}

double presetValue(const ModulePreset& preset, const juce::String& key, double fallback)
{
    if (key == "mix")
        return preset.mix;
    for (const auto& [k, v] : preset.values)
        if (key == k)
            return v;
    return fallback;
}
} // namespace hpeq::presets
