#include "Plugin/Presets.h"

namespace hamlo::comp
{
// detector 0 Peak / 1 RMS, character 0 Clean / 1 Punch / 2 Glue.
const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list {
        { "Vocal Leveler", 1.0f, { { "thresholdDb", -24 }, { "ratio", 3 }, { "kneeDb", 8 }, { "attackMs", 5 }, { "releaseMs", 120 }, { "detector", 1 }, { "character", 0 } }, "vocal", Preset::Medium },
        { "Drum Punch", 1.0f, { { "thresholdDb", -20 }, { "ratio", 4 }, { "kneeDb", 4 }, { "attackMs", 15 }, { "releaseMs", 90 }, { "detector", 0 }, { "character", 1 } }, "drums", Preset::Medium },
        { "Bus Glue", 1.0f, { { "thresholdDb", -18 }, { "ratio", 2 }, { "kneeDb", 10 }, { "attackMs", 30 }, { "releaseMs", 200 }, { "detector", 1 }, { "character", 2 } }, "mix", Preset::Gentle },
        { "New York Parallel", 0.4f, { { "thresholdDb", -36 }, { "ratio", 8 }, { "kneeDb", 2 }, { "attackMs", 1 }, { "releaseMs", 70 }, { "detector", 0 }, { "character", 1 } }, "drums", Preset::Heavy },
        { "Bass Control", 1.0f, { { "thresholdDb", -26 }, { "ratio", 4 }, { "kneeDb", 6 }, { "attackMs", 10 }, { "releaseMs", 140 }, { "detector", 1 }, { "character", 0 } }, "bass", Preset::Medium },
        { "Gentle Master", 1.0f, { { "thresholdDb", -14 }, { "ratio", 1.5 }, { "kneeDb", 12 }, { "attackMs", 30 }, { "releaseMs", 250 }, { "detector", 1 }, { "character", 2 } }, "mix", Preset::Gentle },
        { "Squash (parallel)", 0.3f, { { "thresholdDb", -40 }, { "ratio", 20 }, { "kneeDb", 0 }, { "attackMs", 0.1 }, { "releaseMs", 50 }, { "detector", 0 }, { "character", 1 } }, "drums", Preset::Heavy },
        // models and sources (docs/COMPRESSOR_MODELS.md); model 1 VCA / 2 FET / 3 OPTO / 4 TUBE, source 0 All /
        // 1 Drums / 2 Samples / 3 Voc
        { "Drum Bus VCA Glue", 1.0f, { { "model", 1 }, { "source", 1 }, { "thresholdDb", -20 }, { "ratio", 4 }, { "kneeDb", 6 }, { "attackMs", 10 }, { "releaseMs", 100 } }, "drums", Preset::Medium },
        { "Drum Smash FET (parallel)", 0.4f, { { "model", 2 }, { "source", 1 }, { "thresholdDb", -32 }, { "ratio", 20 }, { "kneeDb", 0 }, { "attackMs", 0.1 }, { "releaseMs", 60 } }, "drums", Preset::Heavy },
        { "Vocal FET Upfront", 1.0f, { { "model", 2 }, { "source", 3 }, { "thresholdDb", -24 }, { "ratio", 4 }, { "kneeDb", 4 }, { "attackMs", 0.8 }, { "releaseMs", 80 } }, "vocal", Preset::Medium },
        { "Vocal Opto Leveler", 1.0f, { { "model", 3 }, { "source", 3 }, { "thresholdDb", -26 }, { "ratio", 3 }, { "attackMs", 10 }, { "releaseMs", 60 } }, "vocal", Preset::Medium },
        { "Keys Opto Smooth", 1.0f, { { "model", 3 }, { "source", 2 }, { "thresholdDb", -24 }, { "ratio", 2.5 }, { "attackMs", 10 }, { "releaseMs", 80 } }, "keys", Preset::Gentle },
        { "Pads Tube Warmth", 1.0f, { { "model", 4 }, { "source", 2 }, { "thresholdDb", -24 }, { "ratio", 6 }, { "attackMs", 20 }, { "releaseMs", 300 } }, "synth", Preset::Medium },
        { "Bass FET Grip", 1.0f, { { "model", 2 }, { "source", 0 }, { "thresholdDb", -26 }, { "ratio", 8 }, { "kneeDb", 4 }, { "attackMs", 3 }, { "releaseMs", 120 } }, "bass", Preset::Medium },
        { "Mix Bus VCA 2:1", 1.0f, { { "model", 1 }, { "source", 0 }, { "thresholdDb", -16 }, { "ratio", 2 }, { "kneeDb", 6 }, { "attackMs", 30 }, { "releaseMs", 100 } }, "mix", Preset::Gentle },
        { "Master Tube Gentle", 1.0f, { { "model", 4 }, { "source", 0 }, { "thresholdDb", -14 }, { "ratio", 3 }, { "attackMs", 30 }, { "releaseMs", 400 } }, "mix", Preset::Gentle },
        // Presets pass (owner 2026-10-27: "35 more"). Material and depth: docs/COMPRESSOR_MODELS.md section 4; the
        // thresholds come from HamloComp_Lab (the depth's average gain reduction on the material at -18 dBFS RMS,
        // mixes -14), every other value is designed.
        // drums
        { "Kick Punch FET", 1.0f, { { "model", 2 }, { "thresholdDb", -19.5 }, { "ratio", 4 }, { "kneeDb", 4 }, { "attackMs", 8 }, { "releaseMs", 80 } }, "kick", Preset::Medium },
        { "Kick Tight VCA", 1.0f, { { "model", 1 }, { "thresholdDb", -20 }, { "ratio", 6 }, { "kneeDb", 2 }, { "attackMs", 1 }, { "releaseMs", 50 } }, "kick", Preset::Medium },
        { "Snare Crack FET", 1.0f, { { "model", 2 }, { "thresholdDb", -21.5 }, { "ratio", 6 }, { "kneeDb", 2 }, { "attackMs", 3 }, { "releaseMs", 70 } }, "snare", Preset::Medium },
        { "Snare Fat Opto", 1.0f, { { "model", 3 }, { "thresholdDb", -23.5 }, { "ratio", 4 }, { "attackMs", 10 }, { "releaseMs", 120 } }, "snare", Preset::Medium },
        { "Hi-Hat Tamer Opto", 1.0f, { { "model", 3 }, { "thresholdDb", -16.5 }, { "ratio", 3 }, { "attackMs", 10 }, { "releaseMs", 80 } }, "hats", Preset::Gentle },
        { "Drum Room Smash FET (parallel)", 0.35f, { { "model", 2 }, { "source", 1 }, { "thresholdDb", -21.5 }, { "ratio", 20 }, { "kneeDb", 0 }, { "attackMs", 0.1 }, { "releaseMs", 50 } }, "drums", Preset::Heavy },
        { "Boom Bap Drum Bus VCA", 1.0f, { { "model", 1 }, { "source", 1 }, { "thresholdDb", -34.5 }, { "ratio", 4 }, { "kneeDb", 6 }, { "attackMs", 30 }, { "releaseMs", 100 } }, "drums", Preset::Medium },
        { "Trap Drums Tight VCA", 1.0f, { { "model", 1 }, { "source", 1 }, { "thresholdDb", -30.5 }, { "ratio", 6 }, { "kneeDb", 2 }, { "attackMs", 3 }, { "releaseMs", 60 } }, "drums", Preset::Medium },
        { "Lo-Fi Drums Tube Squeeze", 1.0f, { { "model", 4 }, { "source", 1 }, { "thresholdDb", -37 }, { "ratio", 8 }, { "attackMs", 15 }, { "releaseMs", 250 } }, "drums", Preset::Heavy },
        // vocals
        { "Rap Vocal FET Aggressive", 1.0f, { { "model", 2 }, { "source", 3 }, { "thresholdDb", -15.5 }, { "ratio", 8 }, { "kneeDb", 2 }, { "attackMs", 0.5 }, { "releaseMs", 60 } }, "vocal", Preset::Heavy },
        { "Rap Vocal VCA Control", 1.0f, { { "model", 1 }, { "source", 3 }, { "detector", 1 }, { "thresholdDb", -23 }, { "ratio", 4 }, { "kneeDb", 4 }, { "attackMs", 3 }, { "releaseMs", 80 } }, "vocal", Preset::Medium },
        { "Pop Vocal Opto Smooth", 1.0f, { { "model", 3 }, { "source", 3 }, { "thresholdDb", -24.5 }, { "ratio", 3 }, { "attackMs", 10 }, { "releaseMs", 60 } }, "vocal", Preset::Medium },
        { "Ballad Vocal Opto Gentle", 1.0f, { { "model", 3 }, { "source", 3 }, { "thresholdDb", -20.5 }, { "ratio", 2 }, { "attackMs", 15 }, { "releaseMs", 100 } }, "vocal", Preset::Gentle },
        { "Vocal Tube Warmth", 1.0f, { { "model", 4 }, { "source", 3 }, { "thresholdDb", -25 }, { "ratio", 4 }, { "attackMs", 10 }, { "releaseMs", 200 } }, "vocal", Preset::Medium },
        { "Vocal Parallel FET (NY)", 0.35f, { { "model", 2 }, { "source", 3 }, { "thresholdDb", -14 }, { "ratio", 20 }, { "kneeDb", 0 }, { "attackMs", 0.2 }, { "releaseMs", 50 } }, "vocal", Preset::Heavy },
        { "Ad-Lib Squash FET", 1.0f, { { "model", 2 }, { "source", 3 }, { "thresholdDb", -14 }, { "ratio", 12 }, { "kneeDb", 0 }, { "attackMs", 0.3 }, { "releaseMs", 40 } }, "vocal", Preset::Heavy },
        { "Backing Vocals VCA Glue", 1.0f, { { "model", 1 }, { "source", 3 }, { "detector", 1 }, { "thresholdDb", -24 }, { "ratio", 3 }, { "kneeDb", 6 }, { "attackMs", 10 }, { "releaseMs", 120 } }, "backing", Preset::Medium },
        { "Podcast Voice Leveler", 1.0f, { { "model", 3 }, { "source", 3 }, { "thresholdDb", -23.5 }, { "ratio", 3 }, { "attackMs", 5 }, { "releaseMs", 120 } }, "vocal", Preset::Medium },
        // bass
        { "808 Control VCA", 1.0f, { { "model", 1 }, { "thresholdDb", -22 }, { "ratio", 4 }, { "kneeDb", 4 }, { "attackMs", 10 }, { "releaseMs", 150 } }, "bass", Preset::Medium },
        { "Bass Even Opto", 1.0f, { { "model", 3 }, { "thresholdDb", -24 }, { "ratio", 3 }, { "attackMs", 10 }, { "releaseMs", 80 } }, "bass", Preset::Medium },
        { "Bass Round Tube", 1.0f, { { "model", 4 }, { "thresholdDb", -24.5 }, { "ratio", 6 }, { "attackMs", 20 }, { "releaseMs", 200 } }, "bass", Preset::Medium },
        { "Slap Bass FET Grip", 1.0f, { { "model", 2 }, { "thresholdDb", -20.5 }, { "ratio", 6 }, { "kneeDb", 2 }, { "attackMs", 1 }, { "releaseMs", 60 } }, "bass", Preset::Heavy },
        // samples: keys, guitars, synths, pads
        { "Piano Natural Opto", 1.0f, { { "model", 3 }, { "source", 2 }, { "thresholdDb", -19 }, { "ratio", 2 }, { "attackMs", 15 }, { "releaseMs", 100 } }, "keys", Preset::Gentle },
        { "Rhodes Tube Glow", 1.0f, { { "model", 4 }, { "source", 2 }, { "thresholdDb", -23.5 }, { "ratio", 4 }, { "attackMs", 15 }, { "releaseMs", 200 } }, "keys", Preset::Medium },
        { "Soul Chop VCA Glue", 1.0f, { { "model", 1 }, { "source", 2 }, { "thresholdDb", -23 }, { "ratio", 4 }, { "kneeDb", 6 }, { "attackMs", 10 }, { "releaseMs", 100 } }, "keys", Preset::Medium },
        { "Sample Flip Pump VCA", 1.0f, { { "model", 1 }, { "source", 2 }, { "thresholdDb", -26 }, { "ratio", 10 }, { "kneeDb", 0 }, { "attackMs", 1 }, { "releaseMs", 150 } }, "keys", Preset::Heavy },
        { "Pad Tube Breathe", 1.0f, { { "model", 4 }, { "source", 2 }, { "thresholdDb", -23 }, { "ratio", 8 }, { "attackMs", 30 }, { "releaseMs", 400 } }, "synth", Preset::Medium },
        { "Synth Lead FET Bite", 1.0f, { { "model", 2 }, { "source", 2 }, { "thresholdDb", -15 }, { "ratio", 6 }, { "kneeDb", 2 }, { "attackMs", 1 }, { "releaseMs", 60 } }, "synth", Preset::Medium },
        { "Acoustic Guitar Opto", 1.0f, { { "model", 3 }, { "source", 2 }, { "thresholdDb", -22 }, { "ratio", 3 }, { "attackMs", 10 }, { "releaseMs", 80 } }, "guitar", Preset::Medium },
        { "Electric Guitar FET Bite", 1.0f, { { "model", 2 }, { "source", 2 }, { "thresholdDb", -15 }, { "ratio", 6 }, { "kneeDb", 2 }, { "attackMs", 2 }, { "releaseMs", 70 } }, "guitar", Preset::Medium },
        // mix bus / master
        { "Mix Bus VCA Punch 4:1", 1.0f, { { "model", 1 }, { "detector", 1 }, { "thresholdDb", -14 }, { "ratio", 4 }, { "kneeDb", 6 }, { "attackMs", 10 }, { "releaseMs", 100 } }, "mix", Preset::Gentle },
        { "Mix Bus Tube Glue", 1.0f, { { "model", 4 }, { "thresholdDb", -15 }, { "ratio", 3 }, { "attackMs", 30 }, { "releaseMs", 300 } }, "mix", Preset::Gentle },
        { "Master Opto Leveling", 1.0f, { { "model", 3 }, { "thresholdDb", -15.5 }, { "ratio", 2 }, { "attackMs", 20 }, { "releaseMs", 120 } }, "mix", Preset::Gentle },
        { "Boom Bap Mix Tube Dust", 1.0f, { { "model", 4 }, { "thresholdDb", -17 }, { "ratio", 6 }, { "attackMs", 20 }, { "releaseMs", 250 } }, "mix", Preset::Medium },
        { "Trap Mix VCA Slam", 1.0f, { { "model", 1 }, { "thresholdDb", -16 }, { "ratio", 10 }, { "kneeDb", 2 }, { "attackMs", 3 }, { "releaseMs", 60 } }, "mix", Preset::Medium },
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
