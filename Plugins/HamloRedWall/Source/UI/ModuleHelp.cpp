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
        { "limiter", {
            { "thresholdDb", { "LOUDNESS", "Pushes the signal into the limiter: louder without going over LIMIT. Too much flattens the transients." } },
            { "ceilingDb", { "LIMIT", "The highest the output can reach. The limiter guarantees it sample by sample (and between samples with TRUE PEAK)." } },
            { "releaseMs", { "RELEASE", "How fast the limiting recovers. Short = loud but can distort bass; long = clean but can duck." } },
            { "lookaheadMs", { "LOOKAHEAD", "How early the limiter sees a peak coming. More = cleaner catches, more latency." } },
            { "link", { "LINK", "100 %: both channels limited together (image stays put). 0 %: each side on its own." } },
            { "outputDb", { "OUTPUT", "Level after the limiter; the output peak is LIMIT + OUTPUT." } },
            { "mix", { "MIX", "Blend of the limited and the dry signal." } },
            { "style", { "STYLE", "GLASNOST transparent; STAKHANOV fast, less ducking before hits; OCTOBER soft clip first, louder and denser." } },
            { "truePeak", { "TRUE PEAK", "Also catches the peaks between samples that appear after conversion. Adds 8 samples of latency." } },
            { "autoRelease", { "RELEASE AUTO", "Release that adapts to the material: fast on short peaks, slow on sustained loud parts." } },
            { "drive", { "DRIVE", "Blends in harmonics before the limiter (mostly odd, oversampled): edge and presence. The amount follows LOUDNESS and LIMIT." } },
            { "tube", { "TUBE", "Warmth and weight: even harmonics and softly rounded peaks against the body of the music - fatter, a lower crest factor, never a higher peak." } },
            { "engine", { "ENGINE", "V2: true peak on a finer grid. CLASSIC: exactly the HPEQ limiter (older projects open in CLASSIC)." } },
            { "monitor", { "MONITOR", "NORMAL; UNITY = hear it at the input level (judge the tone, not the loudness); DELTA = hear only what is removed." } },
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
