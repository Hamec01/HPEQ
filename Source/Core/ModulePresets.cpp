#include "Core/ModulePresets.h"

namespace hpeq::presets
{
namespace
{
const std::vector<ModulePreset> kReverb {
    { "Vocal Plate", 0.18f, { { "space", 4 }, { "color", 2 }, { "predelayMs", 40 }, { "size", 0.8 }, { "decaySeconds", 1.6 }, { "lowCutHz", 200 }, { "highCutHz", 9000 }, { "duck", 0.3 } } },
    { "Small Room", 0.15f, { { "space", 1 }, { "color", 2 }, { "predelayMs", 5 }, { "size", 0.6 }, { "decaySeconds", 0.6 }, { "lowCutHz", 120 } } },
    { "Drum Chamber", 0.15f, { { "space", 2 }, { "color", 1 }, { "predelayMs", 10 }, { "size", 0.9 }, { "decaySeconds", 1.1 }, { "lowCutHz", 150 } } },
    { "Big Hall", 0.22f, { { "space", 3 }, { "color", 2 }, { "predelayMs", 30 }, { "size", 1.2 }, { "decaySeconds", 2.8 }, { "lowCutHz", 100 } } },
    { "Cathedral", 0.25f, { { "space", 5 }, { "color", 2 }, { "predelayMs", 50 }, { "size", 1.4 }, { "decaySeconds", 6.0 }, { "lowCutHz", 120 }, { "highCutHz", 10000 } } },
    { "Ambience Glue", 0.12f, { { "space", 6 }, { "color", 2 }, { "predelayMs", 0 }, { "size", 0.5 }, { "decaySeconds", 0.4 }, { "lowCutHz", 150 } } },
    { "80s Snare Plate", 0.25f, { { "space", 4 }, { "color", 1 }, { "predelayMs", 0 }, { "size", 1.0 }, { "decaySeconds", 1.8 }, { "lowCutHz", 250 } } },
};

#define HAMLO_VERB_BANK
const std::vector<ModulePreset> kGenre {
#include "Core/GenrePresets.inc"
};
const std::vector<ModulePreset> kNone;
}
const std::vector<ModulePreset>& factoryModulePresets(FxType type)
{
    static const std::vector<ModulePreset> all = [] { auto bank = kReverb; bank.insert(bank.end(), kGenre.begin(), kGenre.end()); return bank; }();
    return type == FxType::Reverb ? all : kNone;
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
} // namespace hpeq::presets
