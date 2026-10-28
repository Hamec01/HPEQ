#include "Plugin/Presets.h"

namespace hamlo::redwall
{
// style 0 GLASNOST / 1 STAKHANOV / 2 OCTOBER.
const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list {
        { "Safe Ceiling", 1.0f, { { "thresholdDb", -1 }, { "ceilingDb", -1 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 } } },
        { "Streaming -1 dBTP", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -1 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 } } },
        { "Loud Master", 1.0f, { { "thresholdDb", -6 }, { "ceilingDb", -0.3 }, { "releaseMs", 40 }, { "lookaheadMs", 1.5 }, { "style", 1 }, { "truePeak", 1 }, { "autoRelease", 1 } } },
        { "Soft Clip Push", 1.0f, { { "thresholdDb", -4 }, { "ceilingDb", -0.5 }, { "style", 2 }, { "truePeak", 1 }, { "autoRelease", 1 } } },
        { "Drum Peak Catcher", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -0.5 }, { "releaseMs", 20 }, { "lookaheadMs", 0.5 }, { "link", 0.5 }, { "style", 1 }, { "autoRelease", 0 } } },
        { "Broadcast -2 dB", 1.0f, { { "thresholdDb", -2 }, { "ceilingDb", -2 }, { "releaseMs", 120 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 0 } } },
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
