#include "Core/ModulePresets.h"
namespace hpeq::presets
{
namespace
{
const std::vector<ModulePreset> kGain {
    { "Unity", 1.0f, { { "gainDb", 0 } } },
    { "Pad -6 dB", 1.0f, { { "gainDb", -6 } } },
    { "Pad -12 dB", 1.0f, { { "gainDb", -12 } } },
    { "Boost +6 dB", 1.0f, { { "gainDb", 6 } } },
    { "Polarity Flip", 1.0f, { { "gainDb", 0 }, { "polarity", 3 } } },
    { "Match to -18 dBFS", 1.0f, { { "gainDb", 0 }, { "targetDb", -18 } } },
    { "Match to -14 dBFS", 1.0f, { { "gainDb", 0 }, { "targetDb", -14 } } },
};

}
const std::vector<ModulePreset>& factoryModulePresets(FxType)
{return kGain;}

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
