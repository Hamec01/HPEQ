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
        { "comp", {
            { "thresholdDb", { "THRESHOLD", "The level where compression starts. Lower = more of the signal is turned down. At 0 dB with RATIO 1:1 the compressor does nothing." } },
            { "ratio", { "RATIO", "How hard the level above the threshold is reduced. 2:1 gentle, 4:1 firm, 10:1 and up close to limiting. 1:1 = off." } },
            { "kneeDb", { "KNEE", "How softly compression begins around the threshold. Wide = smooth and natural, narrow = sudden and audible." } },
            { "attackMs", { "ATTACK", "How fast it clamps down once the level goes over. Short tames hits; longer lets the transient through for punch." } },
            { "releaseMs", { "RELEASE", "How fast it lets go after the level drops. Short = lively, may pump; long = smooth, steady." } },
            { "makeupDb", { "MAKEUP", "Gain after compression, to get back the level the compressor took away." } },
            { "mix", { "MIX", "Parallel compression: how much of the compressed signal is blended with the dry one." } },
            { "detector", { "DETECTOR", "PEAK reacts to every spike (drums); RMS follows the average loudness (smoother, vocals and buses)." } },
            { "character", { "CHARACTER", "CLEAN: transparent. PUNCH: adds bite on the hits as it compresses. GLUE: program-dependent release that holds a mix together." } },
            { "autoMakeup", { "AUTO MAKEUP", "Turns the output back up by what the compressor takes off on average, so you compare at the same loudness." } },
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
