#include "Plugin/Presets.h"

namespace hamlo::redwall
{
// style 0 GLASNOST / 1 STAKHANOV / 2 OCTOBER. HPEQ's presets run the CLASSIC engine, so they sound as in HPEQ;
// the v2 presets leave `engine` at its default (V2).
const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list {
        { "Safe Ceiling", 1.0f, { { "thresholdDb", -1 }, { "ceilingDb", -1 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "engine", 0 } } },
        { "Streaming -1 dBTP", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -1 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "engine", 0 } } },
        { "Loud Master", 1.0f, { { "thresholdDb", -6 }, { "ceilingDb", -0.3 }, { "releaseMs", 40 }, { "lookaheadMs", 1.5 }, { "style", 1 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "engine", 0 } } },
        { "Soft Clip Push", 1.0f, { { "thresholdDb", -4 }, { "ceilingDb", -0.5 }, { "style", 2 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "engine", 0 } } },
        { "Drum Peak Catcher", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -0.5 }, { "releaseMs", 20 }, { "lookaheadMs", 0.5 }, { "link", 0.5 }, { "style", 1 }, { "autoRelease", 0 }, { "engine", 0 } } },
        { "Broadcast -2 dB", 1.0f, { { "thresholdDb", -2 }, { "ceilingDb", -2 }, { "releaseMs", 120 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 0 }, { "engine", 0 } } },
        // v2 (docs/REDWALL_V2.md): the V2 engine (the default) with the colour stage
        { "Clean V2 Master", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -1 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 } } },
        { "Warm Master", 1.0f, { { "thresholdDb", -4 }, { "ceilingDb", -1 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "tube", 0.35 } } },
        { "Fat Loud Master", 1.0f, { { "thresholdDb", -7 }, { "ceilingDb", -0.5 }, { "releaseMs", 40 }, { "style", 1 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "drive", 0.15 }, { "tube", 0.6 } } },
        { "Streaming Warm -1 dBTP", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -1 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "tube", 0.25 } } },
        { "Edge Push", 1.0f, { { "thresholdDb", -5 }, { "ceilingDb", -0.5 }, { "style", 2 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "drive", 0.4 } } },
        { "Boom Bap Weight", 1.0f, { { "thresholdDb", -5 }, { "ceilingDb", -0.8 }, { "releaseMs", 80 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "drive", 0.1 }, { "tube", 0.7 } } },
        { "Trap 808 Glue", 1.0f, { { "thresholdDb", -6 }, { "ceilingDb", -0.5 }, { "releaseMs", 120 }, { "style", 1 }, { "truePeak", 1 }, { "autoRelease", 1 }, { "tube", 0.3 } } },
        { "Drum Bus Crunch", 1.0f, { { "thresholdDb", -4 }, { "ceilingDb", -1 }, { "releaseMs", 20 }, { "lookaheadMs", 0.5 }, { "link", 0.5 }, { "style", 1 }, { "autoRelease", 0 }, { "drive", 0.6 }, { "tube", 0.3 } } },
        { "Drum Bus Thick", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -1 }, { "style", 0 }, { "autoRelease", 1 }, { "tube", 0.8 } } },
        { "Vocal Presence", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -1 }, { "releaseMs", 60 }, { "style", 0 }, { "autoRelease", 1 }, { "drive", 0.25 }, { "tube", 0.2 } } },
        { "Keys Warmth", 1.0f, { { "thresholdDb", -2 }, { "ceilingDb", -1 }, { "releaseMs", 150 }, { "style", 0 }, { "autoRelease", 1 }, { "tube", 0.5 } } },
        { "Parallel Smash", 0.4f, { { "thresholdDb", -12 }, { "ceilingDb", -1 }, { "style", 2 }, { "autoRelease", 1 }, { "drive", 0.7 }, { "tube", 0.5 } } },
        // genre presets (docs/GENRE_PRESETS.md): Boom Bap, Trap, Techno, DnB x 50, in groups, LOUDNESS calibrated
        // on the material by HamloRedWall_PresetLab
#include "Plugin/GenrePresets.inc"
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
} // namespace hamlo::redwall
