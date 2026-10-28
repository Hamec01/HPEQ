#include "Plugin/Presets.h"

namespace hamlo::saturator
{
// model 0 CLASSIC / 1 TAPE / 2 TUBE / 3 IRON / 4 GERMANIUM / 5 VINYL; speed 0 / 1 / 2. HPEQ's presets.
const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list {
        { "Tape Warmth", 1.0f, { { "model", 1 }, { "speed", 0 }, { "driveDb", 6 }, { "wow", 0.05 } } },
        { "Tube Glow", 1.0f, { { "model", 2 }, { "driveDb", 8 }, { "evenOdd", 0.4 } } },
        { "Iron Weight", 1.0f, { { "model", 3 }, { "driveDb", 10 } } },
        { "Germanium Fuzz", 0.5f, { { "model", 4 }, { "driveDb", 24 } } },
        { "Vinyl Dust", 1.0f, { { "model", 5 }, { "speed", 0 }, { "driveDb", 4 }, { "crackle", 0.3 }, { "hiss", 0.1 } } },
        { "Cassette Lo-Fi", 1.0f, { { "model", 1 }, { "speed", 2 }, { "driveDb", 8 }, { "wow", 0.4 }, { "hiss", 0.2 }, { "satHighCutHz", 9000 } } },
        { "Parallel Grit", 0.3f, { { "model", 0 }, { "driveDb", 18 } } },
    };
    return list;
}

double presetValue(const Preset& preset, const juce::String& id, double fallback)
{
    if (id == "mix")
        return preset.mix;
    for (const auto& [k, v] : preset.values)
        if (id == k)
            return v;
    return fallback;
}
} // namespace hamlo::saturator
