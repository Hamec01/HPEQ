#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "Core/Revolution.h"

namespace hamlo::link
{
struct Snapshot;
}

// R4 (docs/REVOLUTION_V2.md section 9): the SIMPLE page's generator. A pure function of (settings, hits, seed): it
// writes what ADVANCED shows - the TIME / GLITCH / MOTION steps, the seven lanes (steps, length, rate, condition,
// probability, band), the groove table and the lane parameters. Candidate events come from the genre's rules for
// the INTENT; AMOUNT keeps the first round(AMOUNT x N) of them by a seeded priority, so a higher AMOUNT keeps every
// event of a lower one.
namespace hpeq::revolution::simple
{
enum Genre { AutoGenre, BoomBap, Trap, Techno, DnB, kGenres };
enum Intent { Subtle, Groove, Fills, Breakdown, Chaos, kIntents };
enum Fill { FillOff, FillEvery1, FillEvery2, FillEvery4, FillEvery8, kFills };
enum Target { Hats, Snare, TopsTarget, FullTarget, kTargets };
const char* genreName(int);
const char* intentName(int);
const char* fillName(int);
const char* targetName(int);
int intentOf(double knob) noexcept; // fifths of 0 .. 1
int fillCondition(int fill) noexcept; // the fill lanes' LaneCondition

struct Settings
{
    int genre = AutoGenre, fill = FillEvery4, target = TopsTarget;
    double intent = 0.3, amount = 0.5, variation = 0.25;
    std::uint64_t seed = 1;
};

// The source's hits per 1/16 of the bar: the share of bars with a hit per role (0 .. 1) and the groove (the mean
// offset of the hits at that 1/16, in quarter notes).
struct Hits
{
    std::array<float, kSteps> kick {}, snare {}, hats {}, groove {};
    bool linked = false, listened = false;
    int hpdgGenre = -1, keyRoot = 0;
    double bpm = 0.0;
};
constexpr float kHit = 0.5f; // a role "hits" a 1/16 when it does in at least half the bars
Hits hitsFromSnapshot(const hamlo::link::Snapshot&);
Hits hitsFromFeel(const Feel&, int genre);  // LISTEN: every role = the input's attack strength; the genre's swing
Hits templateHits(int genre);               // the genre's kick / snare / hats and swing
int resolveGenre(int genre, const Hits&, double hostBpm) noexcept;

struct Result
{
    Pattern pattern;
    Lanes lanes {};
    std::array<float, kSteps> groove {};
    int genre = BoomBap, intent = Subtle;
    std::vector<std::pair<const char*, double>> parameters; // ids of the Revolution spec (ModuleVariant.cpp)
    std::array<std::uint32_t, kLanes> onHit {}, fills {};   // bit s: the lane's step s was placed on a hit / is a fill
    int events = 0;
    bool linked = false, listened = false; // where the hits came from (HPDG, LISTEN, else the genre's template)
};
Result generate(const Settings&, const Hits&, double hostBpm);
} // namespace hpeq::revolution::simple
