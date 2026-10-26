#include "Core/ModulePresets.h"

namespace hpeq::presets
{
namespace
{
// Switch values as the pages show them:
// Compressor detector 0 Peak / 1 RMS, character 0 Clean / 1 Punch / 2 Glue.
// RedWall style 0 GLASNOST / 1 STAKHANOV / 2 OCTOBER.
// Verb space 1 Room / 2 Chamber / 3 Hall / 4 Plate / 5 Cathedral / 6 Ambience; color 0 1970 / 1 1980 / 2 now.
// Echo character 1 Clean / 2 Tape / 3 BBD / 4 Lo-Fi; sync 0 free, 5 1/8, 6 1/8D, 7 1/4T, 8 1/4, 9 1/4D.
// Udarnik style 0 PRAVDA / 1 UDARNIK / 2 PERESTROIKA. Mag model 0 Classic / 1 Tape / 2 Tube / 3 Iron /
// 4 Germanium / 5 Vinyl; speed 0..2 (19 / 9.5 / 4.76 cm/s, 33 / 45 / 78 rpm). Shirota style 0 EDINSTVO /
// 1 PROSTOR / 2 KOSMOS. Zastava style 0 gate / 1 soft expander. Revolution character 0 Minimal /
// 1 Musical / 2 Experimental / 3 Destruction; grid 1 1/8, 2 1/16; motionTarget 0 Volume / 1 Low pass /
// 2 High pass / 3 Width / 4 Crush.
const std::vector<ModulePreset> kCompressor {
    { "Vocal Leveler", 1.0f, { { "thresholdDb", -24 }, { "ratio", 3 }, { "kneeDb", 8 }, { "attackMs", 5 }, { "releaseMs", 120 }, { "detector", 1 }, { "character", 0 } } },
    { "Drum Punch", 1.0f, { { "thresholdDb", -20 }, { "ratio", 4 }, { "kneeDb", 4 }, { "attackMs", 15 }, { "releaseMs", 90 }, { "detector", 0 }, { "character", 1 } } },
    { "Bus Glue", 1.0f, { { "thresholdDb", -18 }, { "ratio", 2 }, { "kneeDb", 10 }, { "attackMs", 30 }, { "releaseMs", 200 }, { "detector", 1 }, { "character", 2 } } },
    { "New York Parallel", 0.4f, { { "thresholdDb", -36 }, { "ratio", 8 }, { "kneeDb", 2 }, { "attackMs", 1 }, { "releaseMs", 70 }, { "detector", 0 }, { "character", 1 } } },
    { "Bass Control", 1.0f, { { "thresholdDb", -26 }, { "ratio", 4 }, { "kneeDb", 6 }, { "attackMs", 10 }, { "releaseMs", 140 }, { "detector", 1 }, { "character", 0 } } },
    { "Gentle Master", 1.0f, { { "thresholdDb", -14 }, { "ratio", 1.5 }, { "kneeDb", 12 }, { "attackMs", 30 }, { "releaseMs", 250 }, { "detector", 1 }, { "character", 2 } } },
    { "Squash (parallel)", 0.3f, { { "thresholdDb", -40 }, { "ratio", 20 }, { "kneeDb", 0 }, { "attackMs", 0.1 }, { "releaseMs", 50 }, { "detector", 0 }, { "character", 1 } } },
};

const std::vector<ModulePreset> kLimiter {
    { "Safe Ceiling", 1.0f, { { "thresholdDb", -1 }, { "ceilingDb", -1 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 } } },
    { "Streaming -1 dBTP", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -1 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 1 } } },
    { "Loud Master", 1.0f, { { "thresholdDb", -6 }, { "ceilingDb", -0.3 }, { "releaseMs", 40 }, { "lookaheadMs", 1.5 }, { "style", 1 }, { "truePeak", 1 }, { "autoRelease", 1 } } },
    { "Soft Clip Push", 1.0f, { { "thresholdDb", -4 }, { "ceilingDb", -0.5 }, { "style", 2 }, { "truePeak", 1 }, { "autoRelease", 1 } } },
    { "Drum Peak Catcher", 1.0f, { { "thresholdDb", -3 }, { "ceilingDb", -0.5 }, { "releaseMs", 20 }, { "lookaheadMs", 0.5 }, { "link", 0.5 }, { "style", 1 }, { "autoRelease", 0 } } },
    { "Broadcast -2 dB", 1.0f, { { "thresholdDb", -2 }, { "ceilingDb", -2 }, { "releaseMs", 120 }, { "style", 0 }, { "truePeak", 1 }, { "autoRelease", 0 } } },
};

const std::vector<ModulePreset> kReverb {
    { "Vocal Plate", 0.18f, { { "space", 4 }, { "color", 2 }, { "predelayMs", 40 }, { "size", 0.8 }, { "decaySeconds", 1.6 }, { "lowCutHz", 200 }, { "highCutHz", 9000 }, { "duck", 0.3 } } },
    { "Small Room", 0.15f, { { "space", 1 }, { "color", 2 }, { "predelayMs", 5 }, { "size", 0.6 }, { "decaySeconds", 0.6 }, { "lowCutHz", 120 } } },
    { "Drum Chamber", 0.15f, { { "space", 2 }, { "color", 1 }, { "predelayMs", 10 }, { "size", 0.9 }, { "decaySeconds", 1.1 }, { "lowCutHz", 150 } } },
    { "Big Hall", 0.22f, { { "space", 3 }, { "color", 2 }, { "predelayMs", 30 }, { "size", 1.2 }, { "decaySeconds", 2.8 }, { "lowCutHz", 100 } } },
    { "Cathedral", 0.25f, { { "space", 5 }, { "color", 2 }, { "predelayMs", 50 }, { "size", 1.4 }, { "decaySeconds", 6.0 }, { "lowCutHz", 120 }, { "highCutHz", 10000 } } },
    { "Ambience Glue", 0.12f, { { "space", 6 }, { "color", 2 }, { "predelayMs", 0 }, { "size", 0.5 }, { "decaySeconds", 0.4 }, { "lowCutHz", 150 } } },
    { "80s Snare Plate", 0.25f, { { "space", 4 }, { "color", 1 }, { "predelayMs", 0 }, { "size", 1.0 }, { "decaySeconds", 1.8 }, { "lowCutHz", 250 } } },
};

const std::vector<ModulePreset> kDelay {
    { "Slapback", 0.25f, { { "character", 2 }, { "sync", 0 }, { "timeMs", 110 }, { "feedback", 0.1 }, { "lowCutHz", 150 }, { "dampingHz", 5000 } } },
    { "Quarter Tape", 0.2f, { { "character", 2 }, { "sync", 8 }, { "feedback", 0.35 }, { "lowCutHz", 120 }, { "dampingHz", 7000 } } },
    { "Dotted Eighth", 0.2f, { { "character", 1 }, { "sync", 6 }, { "feedback", 0.4 }, { "lowCutHz", 150 }, { "dampingHz", 9000 } } },
    { "Ping-Pong 1/8", 0.22f, { { "character", 1 }, { "sync", 5 }, { "pingPong", 1 }, { "feedback", 0.45 }, { "lowCutHz", 180 }, { "dampingHz", 8000 } } },
    { "BBD Chorus Echo", 0.25f, { { "character", 3 }, { "sync", 7 }, { "feedback", 0.4 }, { "modRate", 0.8 }, { "modDepth", 0.5 }, { "dampingHz", 6000 } } },
    { "Lo-Fi Dub", 0.3f, { { "character", 4 }, { "sync", 9 }, { "feedback", 0.65 }, { "lofi", 0.6 }, { "driveDb", 6 }, { "dampingHz", 3000 }, { "lowCutHz", 200 } } },
    { "Vocal Throw (ducked)", 0.3f, { { "character", 2 }, { "sync", 8 }, { "feedback", 0.45 }, { "duck", 0.7 }, { "lowCutHz", 250 }, { "dampingHz", 6000 } } },
};

const std::vector<ModulePreset> kTransient {
    { "Kick Punch", 1.0f, { { "attackDb", 6 }, { "sustainDb", -2 }, { "speed", 0.6 }, { "style", 1 } } },
    { "Snare Crack", 1.0f, { { "attackDb", 8 }, { "sustainDb", -1 }, { "speed", 0.7 }, { "style", 1 } } },
    { "Tighten Room", 1.0f, { { "attackDb", 0 }, { "sustainDb", -8 }, { "speed", 0.5 }, { "style", 0 } } },
    { "Soften Attack", 1.0f, { { "attackDb", -6 }, { "sustainDb", 2 }, { "speed", 0.5 }, { "style", 0 } } },
    { "Drum Bus Snap", 0.6f, { { "attackDb", 3 }, { "sustainDb", 0 }, { "speed", 0.6 }, { "style", 1 } } },
    { "Sustain Bloom", 1.0f, { { "attackDb", 0 }, { "sustainDb", 6 }, { "speed", 0.4 }, { "style", 0 } } },
};

const std::vector<ModulePreset> kSaturator {
    { "Tape Warmth", 1.0f, { { "model", 1 }, { "speed", 0 }, { "driveDb", 6 }, { "wow", 0.05 } } },
    { "Tube Glow", 1.0f, { { "model", 2 }, { "driveDb", 8 }, { "evenOdd", 0.4 } } },
    { "Iron Weight", 1.0f, { { "model", 3 }, { "driveDb", 10 } } },
    { "Germanium Fuzz", 0.5f, { { "model", 4 }, { "driveDb", 24 } } },
    { "Vinyl Dust", 1.0f, { { "model", 5 }, { "speed", 0 }, { "driveDb", 4 }, { "crackle", 0.3 }, { "hiss", 0.1 } } },
    { "Cassette Lo-Fi", 1.0f, { { "model", 1 }, { "speed", 2 }, { "driveDb", 8 }, { "wow", 0.4 }, { "hiss", 0.2 }, { "satHighCutHz", 9000 } } },
    { "Parallel Grit", 0.3f, { { "model", 0 }, { "driveDb", 18 } } },
};

const std::vector<ModulePreset> kStereo {
    { "Wider Mix", 1.0f, { { "style", 0 }, { "width", 1.3 } } },
    { "Mono Bass 120 Hz", 1.0f, { { "style", 0 }, { "width", 1.0 }, { "monoBassHz", 120 } } },
    { "Master Width", 1.0f, { { "style", 0 }, { "width", 1.15 }, { "monoBassHz", 100 } } },
    { "Narrow", 1.0f, { { "style", 0 }, { "width", 0.6 } } },
    { "Stereoize Mono", 1.0f, { { "style", 1 }, { "stereoize", 0.6 }, { "space", 0.4 } } },
    { "Cosmos Pad", 1.0f, { { "style", 2 }, { "space", 0.7 }, { "width", 1.2 } } },
};

const std::vector<ModulePreset> kGate {
    { "Kick Gate", 1.0f, { { "style", 0 }, { "thresholdDb", -30 }, { "rangeDb", 40 }, { "attackMs", 0.1 }, { "holdMs", 30 }, { "releaseMs", 80 }, { "keyHpHz", 40 }, { "keyLpHz", 2000 } } },
    { "Snare Gate", 1.0f, { { "style", 0 }, { "thresholdDb", -28 }, { "rangeDb", 30 }, { "attackMs", 0.2 }, { "holdMs", 40 }, { "releaseMs", 120 }, { "keyHpHz", 150 }, { "keyLpHz", 8000 } } },
    { "Tom Gate", 1.0f, { { "style", 0 }, { "thresholdDb", -35 }, { "rangeDb", 30 }, { "attackMs", 0.5 }, { "holdMs", 80 }, { "releaseMs", 250 }, { "keyHpHz", 60 }, { "keyLpHz", 1000 } } },
    { "Vocal Noise Expander", 1.0f, { { "style", 1 }, { "thresholdDb", -50 }, { "rangeDb", 12 }, { "ratio", 2 }, { "attackMs", 2 }, { "holdMs", 30 }, { "releaseMs", 150 } } },
    { "Guitar Hum Cleaner", 1.0f, { { "style", 1 }, { "thresholdDb", -55 }, { "rangeDb", 18 }, { "ratio", 3 }, { "attackMs", 1 }, { "holdMs", 50 }, { "releaseMs", 200 } } },
    { "Gentle Bleed Cut", 1.0f, { { "style", 1 }, { "thresholdDb", -40 }, { "rangeDb", 8 }, { "ratio", 2 }, { "attackMs", 1 }, { "holdMs", 40 }, { "releaseMs", 180 } } },
};

const std::vector<ModulePreset> kGain {
    { "Unity", 1.0f, { { "gainDb", 0 } } },
    { "Pad -6 dB", 1.0f, { { "gainDb", -6 } } },
    { "Pad -12 dB", 1.0f, { { "gainDb", -12 } } },
    { "Boost +6 dB", 1.0f, { { "gainDb", 6 } } },
    { "Polarity Flip", 1.0f, { { "gainDb", 0 }, { "polarity", 3 } } },
    { "Match to -18 dBFS", 1.0f, { { "gainDb", 0 }, { "targetDb", -18 } } },
    { "Match to -14 dBFS", 1.0f, { { "gainDb", 0 }, { "targetDb", -14 } } },
};

// Revolution: the pattern is made from the preset's seed and character when the preset is applied.
const std::vector<ModulePreset> kRevolution {
    { "Subtle Movement", 1.0f, { { "character", 0 }, { "revSeed", 5 }, { "timeOn", 0 }, { "glitchOn", 0 }, { "motionOn", 1 }, { "motionTarget", 1 }, { "motionDepth", 0.4 } } },
    { "Musical Stutter", 1.0f, { { "character", 1 }, { "revSeed", 7 }, { "grid", 2 } } },
    { "Half-Time Tape", 1.0f, { { "character", 1 }, { "revSeed", 3 }, { "glitchOn", 0 }, { "motionOn", 0 }, { "timeStretch", 0 } } },
    { "Filter Motion", 1.0f, { { "character", 1 }, { "revSeed", 11 }, { "timeOn", 0 }, { "glitchOn", 0 }, { "motionOn", 1 }, { "motionTarget", 1 }, { "motionDepth", 0.8 } } },
    { "Width Pulse", 1.0f, { { "character", 1 }, { "revSeed", 13 }, { "timeOn", 0 }, { "glitchOn", 0 }, { "motionOn", 1 }, { "motionTarget", 3 }, { "motionDepth", 0.7 } } },
    { "Experimental", 1.0f, { { "character", 2 }, { "revSeed", 21 } } },
    { "Destruction", 0.7f, { { "character", 3 }, { "revSeed", 66 }, { "motionTarget", 4 } } },
};

const std::vector<ModulePreset> kNone;
} // namespace

const std::vector<ModulePreset>& factoryModulePresets(FxType type)
{
    switch (type)
    {
        case FxType::Compressor: return kCompressor;
        case FxType::Limiter: return kLimiter;
        case FxType::Reverb: return kReverb;
        case FxType::Delay: return kDelay;
        case FxType::TransientShaper: return kTransient;
        case FxType::Saturator: return kSaturator;
        case FxType::MonstaFx: return kRevolution;
        case FxType::StereoProcessor: return kStereo;
        case FxType::GateExpander: return kGate;
        case FxType::Gain: return kGain;
    }
    return kNone;
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
