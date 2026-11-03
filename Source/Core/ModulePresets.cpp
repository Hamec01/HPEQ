#include "Core/ModulePresets.h"
namespace hpeq::presets
{
namespace
{
const std::vector<ModulePreset> kGate {
    { "Kick Gate", 1.0f, { { "style", 0 }, { "thresholdDb", -30 }, { "rangeDb", 40 }, { "attackMs", 0.1 }, { "holdMs", 30 }, { "releaseMs", 80 }, { "keyHpHz", 40 }, { "keyLpHz", 2000 } } },
    { "Snare Gate", 1.0f, { { "style", 0 }, { "thresholdDb", -28 }, { "rangeDb", 30 }, { "attackMs", 0.2 }, { "holdMs", 40 }, { "releaseMs", 120 }, { "keyHpHz", 150 }, { "keyLpHz", 8000 } } },
    { "Tom Gate", 1.0f, { { "style", 0 }, { "thresholdDb", -35 }, { "rangeDb", 30 }, { "attackMs", 0.5 }, { "holdMs", 80 }, { "releaseMs", 250 }, { "keyHpHz", 60 }, { "keyLpHz", 1000 } } },
    { "Vocal Noise Expander", 1.0f, { { "style", 1 }, { "thresholdDb", -50 }, { "rangeDb", 12 }, { "ratio", 2 }, { "attackMs", 2 }, { "holdMs", 30 }, { "releaseMs", 150 } } },
    { "Guitar Hum Cleaner", 1.0f, { { "style", 1 }, { "thresholdDb", -55 }, { "rangeDb", 18 }, { "ratio", 3 }, { "attackMs", 1 }, { "holdMs", 50 }, { "releaseMs", 200 } } },
    { "Gentle Bleed Cut", 1.0f, { { "style", 1 }, { "thresholdDb", -40 }, { "rangeDb", 8 }, { "ratio", 2 }, { "attackMs", 1 }, { "holdMs", 40 }, { "releaseMs", 180 } } },
};
}
const std::vector<ModulePreset>& factoryModulePresets(FxType type)
{
    static const std::vector<ModulePreset> all=[]
    {auto result=kGate;auto modern=genreModulePresets();result.insert(result.end(),std::make_move_iterator(modern.begin()),std::make_move_iterator(modern.end()));return result;}();
    static const std::vector<ModulePreset> empty;return type==FxType::GateExpander?all:empty;
}
double presetValue(const ModulePreset& preset, const juce::String& key, double fallback)
{
    if (key == "mix")
        return preset.mix;
    for (const auto& [k, v] : preset.values)
        if (key == k)
            return v;
    return fallback;
}
}
