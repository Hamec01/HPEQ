#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace hamlo::echo
{
enum class Action { Forward, Reverse, Stutter, TapeStop, Skip };
struct TapSettings
{
    bool enabled = false;
    int note = 60, sync = 0;
    Action action = Action::Forward;
    double timeMs = 250, gainDb = -12, pan = 0, fragmentMs = 125, repeats = 4;
    double lowCut = 10, highCut = 20000;
    double spaceSend=1;
};
struct BandSettings { double timeScale = 1, feedback = .25, gainDb = 0, pan = 0; bool enabled=true; };
struct ScoreSettings
{
    int sourceNote = 60, routing = 0, color = 0, feedbackSync = 0, feedbackPitch = 0;
    int spaceType = 0, spaceRoute = 0, glitch = 0, glitchTarget = 1;
    bool freeze = false, bands = false;
    double bpm = 120, mix = .35, outputDb = 0, width = 1, duck = 0;
    double feedback = .3, feedbackMs = 500, driveDb = 0, lowCut = 10, highCut = 20000;
    double splitLow = 250, splitHigh = 3000;
    double spaceMix = .25, spaceDecay = 2, spaceSize = 1, spaceDamping = 6000;
    double glitchMs = 125, glitchAmount = 0, swing = 0;
    std::array<TapSettings, 8> taps;
    std::array<BandSettings, 3> band;
};
inline double divisionBeats(int division) noexcept
{
    constexpr double beats[] { 0, .125, .25, .375, 1.0/3, .5, .75, 2.0/3, 1, 1.5, 2, 4 };
    return beats[division < 0 ? 0 : division > 11 ? 11 : division];
}
inline double timeMs(double freeMs, int division, double bpm) noexcept
{
    return division == 0 ? freeMs : 60000 * divisionBeats(division) / (bpm > 0 ? bpm : 120);
}
inline int64_t subdivisionAge(int64_t age,int length,int repeats) noexcept
{
    const auto part=age*repeats/length;
    return age-(part*length+repeats-1)/repeats;
}
}
