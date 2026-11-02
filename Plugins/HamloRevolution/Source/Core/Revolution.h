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
// GLITCH 2 (docs/GLITCH_RESEARCH.md): the block engine's actions - the playing audio itself, cut, repeated in musical
// pieces with a shape per repeat (decay, gate), pitched, re-ordered. Chosen per scene by the glitchEngine parameter;
// the actions above stay for old projects (Legacy).
enum BlockAction { BlockPass, BlockRepeat, BlockRoll, BlockRise, BlockChop, BlockDrop, BlockShuffle, BlockReverse, BlockStop, BlockGap,
                   kBlockActions };
juce::String blockActionName(int action);      // "REPEAT" ...
juce::String blockActionShortName(int action); // "RP" ...
// the ADVANCED GENERATE in block mode: a legacy GLITCH action -> its block counterpart
int blockFromLegacy(int glitchAction) noexcept;
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

// LISTEN (pass 10b): the input's attack strength per step, 0..15, as measured by the engine when GENERATE
// was pressed. Saved with the pattern ("feelA" steps 0..7, "feelB" 8..15), so a project regenerates the same.
using Feel = std::array<std::uint8_t, kSteps>;
const char* feelKey(int half);
Feel readFeel(const juce::NamedValueSet&);
void writeFeel(juce::NamedValueSet&, const Feel&);
Feel feelFrom(const std::array<float, kSteps>& strength); // 0..1 -> 0..15
bool hasFeel(const Feel&) noexcept;

// Generate that follows the audio (MSAV-C): for TIME and GLITCH each free step gets a small seeded set of
// candidate actions; a beam search (width 24) picks the pattern with the best explainable score: repeats
// and retriggers on the hits, gaps between them, TIME events at phrase ends, the character's density and
// variety, a seeded preference per action; locks, allowed actions and the event limit are hard rules.
// MOTION's levels are lowered on the hits. Without a feel this is generate().
constexpr std::uint32_t kFollowingVersion = 2;

// R2: SHAPE A / B, lanes 3 and 4 after TIME, GLITCH, MOTION (levels 0 .. 15 like MOTION).
constexpr int kShapes = 2;
// R3: THROW A / B, lanes 5 and 6 after the shapes (a step's level 0 .. 15 = its send, or the effect's intensity).
constexpr int kThrows = 2;
constexpr int kFirstShape = kEngines, kFirstThrow = kEngines + kShapes;
constexpr int kLanes = kEngines + kShapes + kThrows;
enum ShapeTarget { ShapeVolume, ShapeLowPass, ShapeHighPass, ShapeBandPass, ShapePan, ShapeWidth, ShapeDrive, ShapeCrush, kShapeTargets };
const char* shapeTargetName(int target);
enum Band { Full, Low, Mid, High, Tops, kBands }; // R4: TOPS = MID + HIGH (everything above the kick)
const char* bandName(int band);
enum ThrowTarget { ThrowDelay, ThrowTonal, ThrowReverb, ThrowPhaser, ThrowFlanger, ThrowVinyl, ThrowDownsample, kThrowTargets };
const char* throwTargetName(int target);
// DELAY, TONAL and REVERB are sends (their tail rings on after the step); the others are inserts.
inline bool throwIsSend(int target) noexcept { return target <= ThrowReverb; }
// TIME: the delay (DELAY) or the LFO period (PHASER, FLANGER, VINYL wow), 1/32 .. 4 bars, sorted by length.
constexpr int kThrowTimes = 14, kDefaultThrowTime = 7; // 1/8 dotted
const char* throwTimeName(int time);
double throwTimeQ(int time) noexcept; // in quarter notes
// NOTE: TONAL's pitch, C3 .. B3.
constexpr int kNotes = 12;
const char* noteName(int note);
double noteHz(int note) noexcept;

// R1 lanes (HamloRevolution plug-in): conditions per cycle of a lane, counted from bar 1 (cycle 0).
enum LaneCondition { Always, OneOfTwo, TwoOfTwo, OneOfFour, FourOfFour, EightOfEight, NotOneOfFour, kLaneConditions };
inline bool laneConditionPasses(int condition, long long cycle) noexcept
{
    const auto m = [cycle](long long d) { const auto r = cycle % d; return r < 0 ? r + d : r; };
    switch (condition)
    {
        case OneOfTwo: return m(2) == 0;
        case TwoOfTwo: return m(2) == 1;
        case OneOfFour: return m(4) == 0;
        case FourOfFour: return m(4) == 3;
        case EightOfEight: return m(8) == 7;
        case NotOneOfFour: return m(4) != 0;
        default: return true;
    }
}
const char* laneConditionName(int condition);
// Probability codes: 0 = 100 %, 1 .. 7 = 88, 75, 63, 50, 38, 25, 13 % (steps of 1/8).
constexpr int kProbabilityCodes = 8;
inline double laneProbability(int code) noexcept { return 1.0 - 0.125 * (code < 0 ? 0 : code > 7 ? 7 : code); }
const char* laneRateName(int rate); // 0 FOLLOW, 1/4 .. 1/32T

// The value lane `lane` plays at its absolute step k (R1: the condition per cycle of the lane, then the step's
// probability drawn from a hash of (seed, lane, k)). The engine and the generator's checks share this.
inline double laneHash(std::uint32_t seed, long long k) noexcept
{
    std::uint64_t z = (static_cast<std::uint64_t>(seed) << 32) ^ static_cast<std::uint64_t>(k) ^ 0x6A09E667F3BCC909ull;
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    return static_cast<double>(z >> 11) * (1.0 / 9007199254740992.0);
}
// firstQuads: steps 1 .. 16, extraQuads: 17 .. 32, probabilityQuads: 8 quads of codes; length = the lane's steps.
inline int laneEventValue(const float* firstQuads, const float* extraQuads, const float* probabilityQuads, bool chance, int length,
                          int condition, std::uint32_t seed, int lane, long long k) noexcept
{
    const auto r = k % length;
    const int index = static_cast<int>(r < 0 ? r + length : r);
    const long long cycle = k >= 0 ? k / length : -((-k + length - 1) / length);
    if (condition != 0 && !laneConditionPasses(condition, cycle))
        return 0;
    if (chance)
    {
        const int code = stepValue(probabilityQuads, index);
        if (code != 0 && laneHash(seed ^ (0x9E3779B9u * static_cast<std::uint32_t>(lane + 1)), k) >= laneProbability(code))
            return 0;
    }
    return index < kSteps ? stepValue(firstQuads, index) : stepValue(extraQuads, index - kSteps);
}

// A lane's settings (all 0 = as before R1): length 0 = follow STEPS / 1 .. 32, rate 0 = follow GRID / 1 .. 8,
// condition, the values of steps 17 .. 32, a probability code per step (0 = 100 %).
// R2: the lane's band, and for SHAPE A / B their steps 1 .. 16 (TIME, GLITCH, MOTION keep theirs in Pattern).
struct Lane
{
    int length = 0, rate = 0, condition = 0;
    std::array<std::uint8_t, kSteps> extra {};
    std::array<std::uint8_t, 2 * kSteps> probability {};
    int band = 0;
    std::array<std::uint8_t, kSteps> first {};
    bool operator==(const Lane& o) const noexcept
    {
        return length == o.length && rate == o.rate && condition == o.condition && extra == o.extra && probability == o.probability
               && band == o.band && first == o.first;
    }
    bool operator!=(const Lane& o) const noexcept { return !(*this == o); }
};
using Lanes = std::array<Lane, kLanes>;
constexpr int kMaxLaneLength = 2 * kSteps;
constexpr int kLaneFloats = 15; // slot::kLaneStride
constexpr int kScenes = 8;

// The lane as the engine reads it (slot::kLane*): length, rate, condition, 4 quads of steps 17 .. 32, 8 quads of codes.
inline void packLane(const Lane& lane, float* dst) noexcept
{
    const auto quad = [](const std::uint8_t* a, int q)
    {
        int v = 0;
        for (int i = 0; i < 4; ++i)
            v |= (a[4 * q + i] & 15) << (4 * i);
        return static_cast<float>(v);
    };
    dst[0] = static_cast<float>(lane.length);
    dst[1] = static_cast<float>(lane.rate);
    dst[2] = static_cast<float>(lane.condition);
    for (int q = 0; q < 4; ++q)
        dst[3 + q] = quad(lane.extra.data(), q);
    for (int q = 0; q < 8; ++q)
        dst[7 + q] = quad(lane.probability.data(), q);
}
// Saved form: "length,rate,condition,<16 hex digits: steps 17 .. 32>,<32 hex digits: codes>,band,<16 hex: steps 1 .. 16>"
// (R1 wrote the first five parts; the rest default).
juce::String laneToText(const Lane&);
Lane laneFromText(const juce::String&);
Pattern generateFollowing(const Pattern& current, std::uint64_t seed, int character, const Feel& feel);
} // namespace hpeq::revolution
