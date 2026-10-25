#pragma once

#include <array>
#include <cstdint>

#include <juce_core/juce_core.h>

// HamloRevolution (module pass 10): the Monsta FX slot grown into three engines, TIME, GLITCH and MOTION,
// each with its own 16-step pattern. The pattern is explicit, editable state (saved as it is, not as a
// seed); Generate fills the unlocked steps from (seed, character) on the message thread. The audio thread
// only reads the packed pattern from the slot snapshot (LayerSnapshot.h, slot::kRev*).
namespace hpeq::revolution
{
constexpr int kSteps = 16;
constexpr int kEngines = 3;
enum Engine { Time, Glitch, Motion };

// The six serial orders, TGM .. MGT.
constexpr int kOrders = 6;
inline std::array<int, kEngines> orderOf(int order) noexcept
{
    static constexpr int orders[kOrders][kEngines] { { Time, Glitch, Motion }, { Time, Motion, Glitch }, { Glitch, Time, Motion },
                                                     { Glitch, Motion, Time }, { Motion, Time, Glitch }, { Motion, Glitch, Time } };
    const auto& o = orders[order < 0 || order >= kOrders ? 0 : order];
    return { o[0], o[1], o[2] };
}
const char* orderName(int order);

// TIME actions on a step. Consecutive equal steps form one run (within a pattern cycle).
enum TimeAction { TimePass, Half, TimeReverse, Freeze, TapeStop, Double, kTimeActions };
// GLITCH actions on a step.
enum GlitchAction { GlitchPass, Repeat, Stutter, Ratchet, GlitchReverse, Gap, Jump, Scratch, kGlitchActions };
// MOTION: a level 0..15 per step (0 = untouched) applied to the target.
constexpr int kMotionLevels = 16;
enum MotionTarget { Volume, LowPass, HighPass, Width, Crush, kMotionTargets };
// Characters steer Generate only (density, which actions, where); they are not DSP engines.
enum Character { Minimal, Musical, Experimental, Destruction, kCharacters };

constexpr std::uint32_t kGeneratorVersion = 1;

int numValues(int engine) noexcept; // 6, 8, 16
juce::String valueName(int engine, int value);      // "HALF", "STUT", "7" ...
juce::String valueShortName(int engine, int value); // one or two letters for the step cells
const char* engineName(int engine);
const char* characterName(int character);
const char* targetName(int target);

struct Pattern
{
    std::array<std::array<std::uint8_t, kSteps>, kEngines> steps {};
    std::array<std::uint16_t, kEngines> locks {};                    // bit s: step s is locked
    std::array<std::uint16_t, kEngines> allow { 0xffff, 0xffff, 0xffff }; // bit a: action a may be generated
    bool operator==(const Pattern& o) const noexcept { return steps == o.steps && locks == o.locks && allow == o.allow; }
    bool operator!=(const Pattern& o) const noexcept { return !(*this == o); }
};

// State keys: "timeA" (steps 0..7, 4 bits each), "timeB" (8..15), "timeLocks", "timeAllow"; glitch*, motion*.
const char* patternKey(int engine, int half);
const char* locksKey(int engine);
const char* allowKey(int engine);
Pattern readPattern(const juce::NamedValueSet&);
void writePattern(juce::NamedValueSet&, const Pattern&);

// Snapshot packing: float `quad` holds steps 4 quad .. 4 quad + 3, 4 bits each (16 bits, exact in a float).
inline float packQuad(const std::array<std::uint8_t, kSteps>& steps, int quad) noexcept
{
    int v = 0;
    for (int i = 0; i < 4; ++i)
        v |= (steps[static_cast<size_t>(4 * quad + i)] & 15) << (4 * i);
    return static_cast<float>(v);
}
inline int stepValue(const float* quads, int step) noexcept
{
    return (static_cast<int>(quads[step >> 2]) >> (4 * (step & 3))) & 15;
}

// Fixed, documented integer hash (SplitMix64 finaliser). Generation is a function of these values only.
std::uint64_t mix64(std::uint64_t) noexcept;
std::uint64_t hashOf(std::uint64_t seed, std::uint32_t engine, std::uint32_t step, std::uint32_t purpose) noexcept;
double unitOf(std::uint64_t h) noexcept; // [0, 1), the top 53 bits

// Generate: locked steps are kept exactly; the others come from (seed, character, generator version),
// each step from its own hash so a lock never shifts the rest; actions outside `allow` are never chosen;
// the character's maximum number of events per engine is a hard limit (locked events count, are kept).
Pattern generate(const Pattern& current, std::uint64_t seed, int character);
int maxEvents(int engine, int character) noexcept;
std::uint64_t nextSeed(std::uint64_t seed) noexcept; // NEW SEED
} // namespace hpeq::revolution
