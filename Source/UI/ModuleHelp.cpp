#include "UI/ModuleHelp.h"

#include <map>

namespace hamlo::ui
{
namespace
{
using Table = std::map<juce::String, HelpText>;

const std::map<juce::String, Table>& tables()
{
    static const std::map<juce::String, Table> t {
        { "saturator", {
            { "driveDb", { "DRIVE", "How hard the signal hits the tape / tube / transformer. 0 dB = clean (a new saturator starts here)." } },
            { "tone", { "TONE", "Tilts the colour: darker or brighter." } },
            { "satLowCutHz", { "LOW CUT", "Keeps lows out of the saturation (avoids mud)." } },
            { "satHighCutHz", { "HIGH CUT", "Keeps highs out (avoids fizz)." } },
            { "mix", { "MIX", "Blend of the saturated and the dry signal." } },
            { "trimDb", { "OUTPUT", "Level after the saturator." } },
            { "harmonics", { "HARMONICS", "How many overtones the model adds." } },
            { "evenOdd", { "EVEN / ODD", "Even = warm, tube-like; odd = edgy, transistor-like." } },
            { "tapeBias", { "BIAS", "Tape bias: low = gritty and dull, high = clean and bright." } },
            { "wow", { "WOW", "Slow pitch wobble of tape or vinyl." } },
            { "hiss", { "HISS", "Tape / vinyl noise floor." } },
            { "hum", { "HUM 50", "50 Hz mains hum of old gear." } },
            { "crackle", { "CRACKLE", "Vinyl crackle." } },
            { "model", { "MODEL", "CLASSIC, TAPE, TUBE, IRON (transformer), GERMANIUM, VINYL: each its own curve and colour." } },
            { "speed", { "SPEED", "Tape speed (or record rpm): slower = more low-end bump and darker top." } },
            { "oversampling", { "OVERSAMPLE", "Processes at 2x / 4x / 8x to avoid aliasing at high drive. Costs CPU." } },
            { "punish", { "OVERLOAD", "Extra hard drive." } },
            { "thump", { "BUMP", "Head-bump: a low-end lift like a tape machine." } },
            { "steep", { "24 DB", "Steeper filters on LOW / HIGH CUT (24 dB per octave)." } },
            { "sampler", { "SAMPLER", "Record through a classic sampler first: SP-1200 (26 kHz, 12-bit, aliasing grit), MPC60 (40 kHz, non-linear 12-bit), S950 (32 kHz, 12-bit, darker)." } },
            { "phoneBass", { "PHONE", "Adds the overtones of the bass and the 808 so they are heard on phones and laptops; the sub itself is untouched." } },
            { "subProtectHz", { "SUB", "Below this frequency the signal skips the saturation: kick and 808 subs stay clean and tight. OFF = everything saturates." } },
            { "dynamics", { "DYNAMICS", "+: the hits saturate, the tails stay clean (punchy grit). -: the tails saturate, the hits stay clean (dense body)." } },
            { "match", { "MATCH", "Keeps the loudness the same as the input whatever the drive or model: compare the colour, not the level." } },
            { "delta", { "DELTA", "Hear only what HamloMag adds." } },
            { "autoGain", { "AUTO", "Keeps the output at the input level as you drive (compare the colour, not the loudness). Off by default: it turns the output down." } },
            { "noiseFollow", { "NOISE", "ALWAYS: noises play all the time; FOLLOW: only while signal is present." } },
        } },
    };
    return t;
}
} // namespace

HelpText moduleHelp(const juce::String& module, const juce::String& key)
{
    const auto& all = tables();
    if (const auto m = all.find(module); m != all.end())
        if (const auto k = m->second.find(key); k != m->second.end())
            return k->second;
    return {};
}
} // namespace hamlo::ui
