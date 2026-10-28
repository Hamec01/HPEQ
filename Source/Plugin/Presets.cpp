#include "Plugin/Presets.h"

namespace hamlo::udarnik
{
// style 0 PRAVDA / 1 UDARNIK / 2 PERESTROIKA. HPEQ's presets run the CLASSIC engine (engine 0, no protection), so they
// sound as in HPEQ.
const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list {
        { "Kick Punch", 1.0f, { { "attackDb", 6 }, { "sustainDb", -2 }, { "speed", 0.6 }, { "style", 1 }, { "engine", 0 }, { "protect", 0 } } },
        { "Snare Crack", 1.0f, { { "attackDb", 8 }, { "sustainDb", -1 }, { "speed", 0.7 }, { "style", 1 }, { "engine", 0 }, { "protect", 0 } } },
        { "Tighten Room", 1.0f, { { "attackDb", 0 }, { "sustainDb", -8 }, { "speed", 0.5 }, { "style", 0 }, { "engine", 0 }, { "protect", 0 } } },
        { "Soften Attack", 1.0f, { { "attackDb", -6 }, { "sustainDb", 2 }, { "speed", 0.5 }, { "style", 0 }, { "engine", 0 }, { "protect", 0 } } },
        { "Drum Bus Snap", 0.6f, { { "attackDb", 3 }, { "sustainDb", 0 }, { "speed", 0.6 }, { "style", 1 }, { "engine", 0 }, { "protect", 0 } } },
        { "Sustain Bloom", 1.0f, { { "attackDb", 0 }, { "sustainDb", 6 }, { "speed", 0.4 }, { "style", 0 }, { "engine", 0 }, { "protect", 0 } } },
        // V2 engine (docs/UDARNIK_V2.md), PROTECT on (the defaults): the knobs deliver about their value in dB of
        // attack / body contrast, so these read as what they do
        { "Kick Punch V2", 1.0f, { { "attackDb", 6 }, { "sustainDb", -2 }, { "style", 1 } } },
        { "Kick Long Boom", 1.0f, { { "attackDb", 2 }, { "sustainDb", 6 }, { "style", 0 } } },
        { "Snare Crack V2", 1.0f, { { "attackDb", 8 }, { "sustainDb", -2 }, { "speed", 0.6 }, { "style", 1 } } },
        { "Snare Fat Body", 1.0f, { { "attackDb", 2 }, { "sustainDb", 5 }, { "style", 0 } } },
        { "Tight Drums", 1.0f, { { "attackDb", 3 }, { "sustainDb", -12 }, { "speed", 0.4 }, { "style", 0 } } },
        { "Room Killer", 1.0f, { { "sustainDb", -18 }, { "speed", 0.3 }, { "style", 0 } } },
        { "Drum Bus Punch V2", 0.7f, { { "attackDb", 5 }, { "style", 1 } } },
        { "Soft Hits", 1.0f, { { "attackDb", -8 }, { "sustainDb", 2 }, { "style", 0 } } },
        { "Hats Shorter", 1.0f, { { "attackDb", 2 }, { "sustainDb", -6 }, { "decay", 0.5 }, { "style", 2 } } },
        { "Long Decay Bloom", 1.0f, { { "sustainDb", 3 }, { "decay", 2.0 }, { "style", 2 } } },
        { "Snappy Perc", 1.0f, { { "attackDb", 8 }, { "sustainDb", -4 }, { "rise", 0.5 }, { "style", 2 } } },
        { "Lo-Fi Dull Attack", 1.0f, { { "attackDb", -12 }, { "sustainDb", 4 }, { "speed", 0.7 }, { "style", 0 } } },
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
} // namespace hamlo::udarnik
