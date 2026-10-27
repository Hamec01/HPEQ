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
        { "transient", {
            { "attackDb", { "SNAP", "Makes the attack of each hit stronger (+) or softer (-)." } },
            { "sustainDb", { "BODY", "Makes the tail of each hit longer and fuller (+) or drier and shorter (-)." } },
            { "speed", { "SPEED", "How long the detector counts as 'attack'. Faster = only the click." } },
            { "rise", { "RISE", "PERESTROIKA: makes the attack itself faster (x0.5) or slower (x2)." } },
            { "decay", { "DECAY", "PERESTROIKA: makes the decay time shorter (x0.5) or longer (x2)." } },
            { "sensitivity", { "SENS", "What counts as a hit. Lower = only strong, sharp hits are shaped (ignores bleed and noise)." } },
            { "preserve", { "PRESERVE", "Keeps the groove: quiet ghost notes are treated like the accents, not flattened." } },
            { "mix", { "MIX", "Blend with the dry signal." } },
            { "outputDb", { "OUTPUT", "Level after the shaper." } },
            { "style", { "MODE", "PRAVDA classic shaper; UDARNIK punch without louder peaks; PERESTROIKA changes the shape of the hit." } },
            { "lookahead", { "LOOK-AHEAD", "Sees the hit coming so the first sample is already shaped. Adds 2 or 5 ms latency." } },
            { "listen", { "LISTEN", "Hear only the attack or only the body that the shaper separates." } },
            { "engine", { "ENGINE", "V2: the knobs deliver what they say on every hit, the effect does not depend on the level, steady tones are left alone. CLASSIC: the original HamloUdarnik laws (older projects open on it)." } },
            { "protect", { "PROTECT", "V2: a soft clip from -3 dBFS that never lets the output pass 0 dBFS - push SNAP without clipping the DAW." } },
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
