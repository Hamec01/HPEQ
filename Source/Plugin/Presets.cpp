#include "Plugin/Presets.h"

namespace hamlo::comp
{
// detector 0 Peak / 1 RMS, character 0 Clean / 1 Punch / 2 Glue.
const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list {
        { "Vocal Leveler", 1.0f, { { "thresholdDb", -24 }, { "ratio", 3 }, { "kneeDb", 8 }, { "attackMs", 5 }, { "releaseMs", 120 }, { "detector", 1 }, { "character", 0 } } },
        { "Drum Punch", 1.0f, { { "thresholdDb", -20 }, { "ratio", 4 }, { "kneeDb", 4 }, { "attackMs", 15 }, { "releaseMs", 90 }, { "detector", 0 }, { "character", 1 } } },
        { "Bus Glue", 1.0f, { { "thresholdDb", -18 }, { "ratio", 2 }, { "kneeDb", 10 }, { "attackMs", 30 }, { "releaseMs", 200 }, { "detector", 1 }, { "character", 2 } } },
        { "New York Parallel", 0.4f, { { "thresholdDb", -36 }, { "ratio", 8 }, { "kneeDb", 2 }, { "attackMs", 1 }, { "releaseMs", 70 }, { "detector", 0 }, { "character", 1 } } },
        { "Bass Control", 1.0f, { { "thresholdDb", -26 }, { "ratio", 4 }, { "kneeDb", 6 }, { "attackMs", 10 }, { "releaseMs", 140 }, { "detector", 1 }, { "character", 0 } } },
        { "Gentle Master", 1.0f, { { "thresholdDb", -14 }, { "ratio", 1.5 }, { "kneeDb", 12 }, { "attackMs", 30 }, { "releaseMs", 250 }, { "detector", 1 }, { "character", 2 } } },
        { "Squash (parallel)", 0.3f, { { "thresholdDb", -40 }, { "ratio", 20 }, { "kneeDb", 0 }, { "attackMs", 0.1 }, { "releaseMs", 50 }, { "detector", 0 }, { "character", 1 } } },
        // models and sources (docs/COMPRESSOR_MODELS.md); model 1 VCA / 2 FET / 3 OPTO / 4 TUBE, source 0 All /
        // 1 Drums / 2 Samples / 3 Voc
        { "Drum Bus VCA Glue", 1.0f, { { "model", 1 }, { "source", 1 }, { "thresholdDb", -20 }, { "ratio", 4 }, { "kneeDb", 6 }, { "attackMs", 10 }, { "releaseMs", 100 } } },
        { "Drum Smash FET (parallel)", 0.4f, { { "model", 2 }, { "source", 1 }, { "thresholdDb", -32 }, { "ratio", 20 }, { "kneeDb", 0 }, { "attackMs", 0.1 }, { "releaseMs", 60 } } },
        { "Vocal FET Upfront", 1.0f, { { "model", 2 }, { "source", 3 }, { "thresholdDb", -24 }, { "ratio", 4 }, { "kneeDb", 4 }, { "attackMs", 0.8 }, { "releaseMs", 80 } } },
        { "Vocal Opto Leveler", 1.0f, { { "model", 3 }, { "source", 3 }, { "thresholdDb", -26 }, { "ratio", 3 }, { "attackMs", 10 }, { "releaseMs", 60 } } },
        { "Keys Opto Smooth", 1.0f, { { "model", 3 }, { "source", 2 }, { "thresholdDb", -24 }, { "ratio", 2.5 }, { "attackMs", 10 }, { "releaseMs", 80 } } },
        { "Pads Tube Warmth", 1.0f, { { "model", 4 }, { "source", 2 }, { "thresholdDb", -24 }, { "ratio", 6 }, { "attackMs", 20 }, { "releaseMs", 300 } } },
        { "Bass FET Grip", 1.0f, { { "model", 2 }, { "source", 0 }, { "thresholdDb", -26 }, { "ratio", 8 }, { "kneeDb", 4 }, { "attackMs", 3 }, { "releaseMs", 120 } } },
        { "Mix Bus VCA 2:1", 1.0f, { { "model", 1 }, { "source", 0 }, { "thresholdDb", -16 }, { "ratio", 2 }, { "kneeDb", 6 }, { "attackMs", 30 }, { "releaseMs", 100 } } },
        { "Master Tube Gentle", 1.0f, { { "model", 4 }, { "source", 0 }, { "thresholdDb", -14 }, { "ratio", 3 }, { "attackMs", 30 }, { "releaseMs", 400 } } },
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
} // namespace hamlo::comp
