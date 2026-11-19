#pragma once

#include "DSP/TransitionRender.h"

#include <array>
#include <vector>

// R15 PHRASE FILL (docs/DRUM_FILL_R15.md): analysis of a drum loop into classified, quality-rated source
// slices, a phrase planner (pulse -> build -> cadence) with hard constraints, and a slice renderer.
// Everything here runs offline on the message thread; nothing is called from processBlock.
namespace hpeq::dsp::transition::phrase
{
enum Role { Kick, Snare, Hat, kRoles };
enum Genre { BoomBap, Trap, DnB, Techno, kGenres };
enum Status { Bypass, Conservative, Generated };
enum Clean { Full, LowPass, HighPassLow, HighPassHigh }; // which contaminating band a slice is cleaned of

struct Onset
{
    int attack = 0, start = 0, free = 0; // samples: attack, slice start (pre-roll), time to the next onset
    double tick = 0;                     // 24ths of a beat from the (phase-corrected) source start
    std::array<double, kRoles> p {};     // role posterior
    int role = Kick, clean = Full;
    bool secondary = false;              // a second drum inside another hit (clap on a kick), cleaned of the first
    double conf = 0, isolation = 0, peak = 0, decayMs = 0;
    std::array<double, 4> novelty {};    // band novelty energy: sub, low-mid, mid, high
};

struct Analysis
{
    std::vector<Onset> onsets;
    double beat = 0;                 // samples per beat
    double phase = 0;                // samples: the grid's origin in the source (capture latency)
    double swing = 0;                // ticks late on odd 16ths (6 ticks = one 16th)
    std::array<double, kRoles> offset {}; // per-role median microtiming, ticks
    double drumness = 0, timing = 0; // [0, 1]: transient vs sustained energy; onsets on the swung grid
    std::array<int, kRoles> usable {};
    std::array<double, kRoles> typicalPeak {};
    int genre = BoomBap;
    double genreConfidence = 0;
};

constexpr double kUsable = 0.30; // minimum conf * isolation for a source slice
inline double quality(const Onset& o) noexcept { return o.conf * o.isolation; }

Analysis analyse(const juce::AudioBuffer<float>& stereo, double sampleRate, double bpm);
const char* genreName(int genre) noexcept;
const char* roleName(int role) noexcept;
} // namespace hpeq::dsp::transition::phrase
