#pragma once

#include <array>
#include <cstdint>
#include <vector>
#define HAMLO_TRANSITION_V2 1
#define HAMLO_DRUM_FILL_V3 1
#define HAMLO_DRUM_COMPOSE 1
#define HAMLO_DRUM_PHRASE 1

#include <juce_audio_basics/juce_audio_basics.h>

// RISE / FILL (docs/RISE_FILL.md): a transition rendered offline from the last N bars the plug-in heard. A pure
// function of (source, settings, seed): the same inputs give the same samples.
namespace hpeq::dsp::transition
{
enum Kind { Rise, Fill };

// R15: a hit the user marked on the wave (source samples) as kick 0, snare 1, hat 2 or perc 3
struct Mark
{
    int start = 0, end = 0, role = 0;
    bool operator==(const Mark& o) const noexcept { return start == o.start && end == o.end && role == o.role; }
};
constexpr int kKitRoles = 5; // kick, snare, hat, perc, tom (perc and tom only from marks)

struct Settings
{
    int kind = Rise;
    int bars = 2;               // the source and the result: 1, 2 or 4 bars
    double bpm = 120.0, sampleRate = 48000.0;
    // RISE: PITCH, ROLL, SWELL, SWEEP; FILL: CHOP, ROLL, REVERSE, STOP (0 .. 1)
    std::array<double, 4> knobs { 0.6, 0.8, 0.5, 0.7 };
    std::uint64_t seed = 1;
    int recipe = 0; // 0 exact Classic,1 Auto,2..7 musical source-aware recipes
    int fillBeats = 4; // New slicer: replace only the last 1, 2 or 4 beats.
    double fillFx = 0.35;
    std::array<bool,kKitRoles> kitEnabled { true,true,true,true,true };
    std::vector<Mark> marks; // R15 PHRASE FILL: the user's labels win over the automatic roles
};
struct Event
{
    int at = 0, length = 0, source = 0;
    double rate = 1, velocity = 1, pan = 0;
    bool reverse = false;
    int sourceEnd = 0, role = 1;
    double space = 0;
    bool roll = false;
};
struct Slice { int sample = 0, end = 0, role = 1; double low = 0, body = 0, air = 0, energy = 0; };
struct Plan
{
    int recipe = 0, attacks = 0;
    std::vector<Event> events;
    std::vector<Slice> slices;
    int phraseStart = 0, phraseEnd = 0, stopSample = -1;
    double inputSimilarity = 0;
    std::array<int,kKitRoles> kitCounts {};
    // R15 PHRASE FILL diagnostics (other recipes leave the defaults)
    int status = 2, genre = -1, anchors = 0; // status: 0 bypass, 1 conservative, 2 generated
    double confidence = 0;
    juce::String family, reason;
};
inline int maximumRecipe(int kind) noexcept { return kind == Fill ? 34 : 7; }
const char* recipeName(int kind,int recipe) noexcept;
int resolvedRecipe(const Settings&) noexcept;
Plan makePlan(const juce::AudioBuffer<float>&,const Settings&);
juce::AudioBuffer<float> renderEvolve(const juce::AudioBuffer<float>&,const Settings&);
Plan makeSlicedFillPlan(const juce::AudioBuffer<float>&, const Settings&);
juce::AudioBuffer<float> renderSlicedFill(const juce::AudioBuffer<float>&, const Settings&);
std::array<juce::AudioBuffer<float>,3> extractDrumBands(const juce::AudioBuffer<float>&,double sampleRate);
Plan makeComposedFillPlan(const juce::AudioBuffer<float>&,const Settings&);
juce::AudioBuffer<float> renderComposedFill(const juce::AudioBuffer<float>&,const Settings&,Plan* info=nullptr);
// R15 PHRASE FILL (recipes 30 AUTO, 31 BOOM BAP, 32 TRAP, 33 DNB, 34 TECHNO): docs/DRUM_FILL_R15.md
constexpr int kPhraseRecipe = 30;
constexpr int kWholeLoop = 99; // fillBeats for PHRASE FILL: re-make every bar of the source, not only its end
Plan makePhraseFillPlan(const juce::AudioBuffer<float>&,const Settings&);
juce::AudioBuffer<float> renderPhraseFill(const juce::AudioBuffer<float>&,const Settings&,Plan* info=nullptr);
const char* knobName(int kind, int knob);
std::array<double, 4> defaultKnobs(int kind) noexcept;

int lengthSamples(const Settings&) noexcept; // N bars of 4/4 at the tempo
// `source`: N bars (lengthSamples), stereo; the result has the same length and ends on the bar line.
juce::AudioBuffer<float> render(const juce::AudioBuffer<float>& source, const Settings&);
} // namespace hpeq::dsp::transition
