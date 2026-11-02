#pragma once

#include <array>
#include <cstdint>

#include <juce_audio_basics/juce_audio_basics.h>

// RISE / FILL (docs/RISE_FILL.md): a transition rendered offline from the last N bars the plug-in heard. A pure
// function of (source, settings, seed): the same inputs give the same samples.
namespace hpeq::dsp::transition
{
enum Kind { Rise, Fill };

struct Settings
{
    int kind = Rise;
    int bars = 2;               // the source and the result: 1, 2 or 4 bars
    double bpm = 120.0, sampleRate = 48000.0;
    // RISE: PITCH, ROLL, SWELL, SWEEP; FILL: CHOP, ROLL, REVERSE, STOP (0 .. 1)
    std::array<double, 4> knobs { 0.6, 0.8, 0.5, 0.7 };
    std::uint64_t seed = 1;
};
const char* knobName(int kind, int knob);
std::array<double, 4> defaultKnobs(int kind) noexcept;

int lengthSamples(const Settings&) noexcept; // N bars of 4/4 at the tempo
// `source`: N bars (lengthSamples), stereo; the result has the same length and ends on the bar line.
juce::AudioBuffer<float> render(const juce::AudioBuffer<float>& source, const Settings&);
} // namespace hpeq::dsp::transition
