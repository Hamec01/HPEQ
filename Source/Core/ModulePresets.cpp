#include "Core/ModulePresets.h"

namespace hpeq::presets
{
namespace
{
const std::vector<ModulePreset> kDelay {
    { "Slapback", 0.25f, { { "character", 2 }, { "sync", 0 }, { "timeMs", 110 }, { "feedback", 0.1 }, { "lowCutHz", 150 }, { "dampingHz", 5000 } } },
    { "Quarter Tape", 0.2f, { { "character", 2 }, { "sync", 8 }, { "feedback", 0.35 }, { "lowCutHz", 120 }, { "dampingHz", 7000 } } },
    { "Dotted Eighth", 0.2f, { { "character", 1 }, { "sync", 6 }, { "feedback", 0.4 }, { "lowCutHz", 150 }, { "dampingHz", 9000 } } },
    { "Ping-Pong 1/8", 0.22f, { { "character", 1 }, { "sync", 5 }, { "pingPong", 1 }, { "feedback", 0.45 }, { "lowCutHz", 180 }, { "dampingHz", 8000 } } },
    { "BBD Chorus Echo", 0.25f, { { "character", 3 }, { "sync", 7 }, { "feedback", 0.4 }, { "modRate", 0.8 }, { "modDepth", 0.5 }, { "dampingHz", 6000 } } },
    { "Lo-Fi Dub", 0.3f, { { "character", 4 }, { "sync", 9 }, { "feedback", 0.65 }, { "lofi", 0.6 }, { "driveDb", 6 }, { "dampingHz", 3000 }, { "lowCutHz", 200 } } },
    { "Vocal Throw (ducked)", 0.3f, { { "character", 2 }, { "sync", 8 }, { "feedback", 0.45 }, { "duck", 0.7 }, { "lowCutHz", 250 }, { "dampingHz", 6000 } } },
#include "Core/ScorePresets.inc"
};

}

const std::vector<ModulePreset>& factoryModulePresets(FxType) { return kDelay; }

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
