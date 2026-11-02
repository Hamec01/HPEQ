#include "Core/Revolution.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace hpeq::revolution
{
namespace
{
constexpr std::uint32_t kPurposeEvent = 1, kPurposeAction = 2, kPurposeLevel = 3, kPurposePriority = 4, kPurposeShape = 5;

// Chance that a free step gets an event (TIME, GLITCH) or a level (MOTION), per character.
constexpr double kDensity[kEngines][kCharacters] { { 0.08, 0.14, 0.28, 0.55 }, { 0.14, 0.26, 0.42, 0.72 }, { 0.30, 0.55, 0.75, 1.0 } };
constexpr int kMaxEvents[kEngines][kCharacters] { { 2, 3, 6, 16 }, { 3, 5, 9, 16 }, { 16, 16, 16, 16 } };
// Action weights (index = action value; 0 = pass, not drawn here).
constexpr double kTimeWeights[kCharacters][kTimeActions] { { 0, 3.0, 1.0, 0.5, 2.0, 0.3 }, { 0, 3.0, 2.0, 1.0, 1.5, 1.0 },
                                                           { 0, 1.0, 1.0, 2.0, 1.0, 1.0 }, { 0, 1.0, 2.0, 2.0, 1.0, 2.0 } };
constexpr double kGlitchWeights[kCharacters][kGlitchActions] { { 0, 3.0, 2.0, 1.0, 1.0, 1.0, 0.3, 0.0 }, { 0, 3.0, 2.0, 2.0, 1.5, 1.5, 1.0, 0.5 },
                                                               { 0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 }, { 0, 1.0, 3.0, 3.0, 2.0, 2.0, 2.0, 2.0 } };

// Where events like to fall: Minimal and Musical lean to the end of each beat and of the pattern (fills).
double positionWeight(int character, int step)
{
    switch (character)
    {
        case Minimal: return (step % 4 == 3 ? 1.6 : 0.4) + (step >= 12 ? 1.0 : 0.0);
        case Musical: return 1.0 + (step % 4 == 3 ? 0.9 : 0.0) + (step >= 12 ? 0.7 : 0.0);
        default: return 1.0;
    }
}

int drawAction(const double* weights, int count, std::uint16_t allow, double u)
{
    double total = 0.0;
    for (int a = 1; a < count; ++a)
        total += (allow >> a) & 1 ? weights[a] : 0.0;
    if (total <= 0.0)
        return 0; // nothing allowed: the step passes
    double x = u * total;
    for (int a = 1; a < count; ++a)
    {
        const double w = (allow >> a) & 1 ? weights[a] : 0.0;
        if (w > 0.0 && x < w)
            return a;
        x -= w;
    }
    for (int a = count - 1; a >= 1; --a) // rounding at the top edge
        if (((allow >> a) & 1) != 0 && weights[a] > 0.0)
            return a;
    return 0;
}

// MOTION shapes for Musical: one per seed (not per step), so locks do not move it.
double shapeAt(int shape, int step)
{
    const double x = step / 15.0;
    switch (shape)
    {
        case 0: return x;                                  // rise
        case 1: return 1.0 - x;                            // fall
        case 2: return step % 2 == 0 ? 1.0 : 0.25;          // pulse
        case 3: return step % 4 == 0 ? 0.0 : (step % 4) / 3.0; // saw per beat
        default: return 0.5 + 0.5 * std::sin(x * juce::MathConstants<double>::twoPi);
    }
}
} // namespace

const char* orderName(int order)
{
    static constexpr const char* names[kOrders] { "T-G-M", "T-M-G", "G-T-M", "G-M-T", "M-T-G", "M-G-T" };
    return names[juce::jlimit(0, kOrders - 1, order)];
}

int numValues(int engine) noexcept
{
    return engine == Time ? kTimeActions : engine == Glitch ? kGlitchActions : kMotionLevels;
}

juce::String valueName(int engine, int value)
{
    static constexpr const char* time[kTimeActions] { "PASS", "HALF", "REVERSE", "FREEZE", "STOP", "2X" };
    static constexpr const char* glitch[kGlitchActions] { "PASS", "REPEAT", "STUTTER", "RATCHET", "REVERSE", "GAP", "JUMP", "SCRATCH" };
    if (engine == Time)
        return time[juce::jlimit(0, kTimeActions - 1, value)];
    if (engine == Glitch)
        return glitch[juce::jlimit(0, kGlitchActions - 1, value)];
    return juce::String(juce::jlimit(0, kMotionLevels - 1, value));
}

juce::String valueShortName(int engine, int value)
{
    static constexpr const char* time[kTimeActions] { "", "H", "R", "F", "S", "2" };
    static constexpr const char* glitch[kGlitchActions] { "", "RP", "ST", "RT", "RV", "GP", "JP", "SC" };
    if (engine == Time)
        return time[juce::jlimit(0, kTimeActions - 1, value)];
    if (engine == Glitch)
        return glitch[juce::jlimit(0, kGlitchActions - 1, value)];
    return value == 0 ? juce::String() : juce::String(value);
}

const char* engineName(int engine)
{
    static constexpr const char* names[kEngines] { "TIME", "GLITCH", "MOTION" };
    return names[juce::jlimit(0, kEngines - 1, engine)];
}

const char* characterName(int character)
{
    static constexpr const char* names[kCharacters] { "MINIMAL", "MUSICAL", "EXPERIMENTAL", "DESTRUCTION" };
    return names[juce::jlimit(0, kCharacters - 1, character)];
}

const char* targetName(int target)
{
    static constexpr const char* names[kMotionTargets] { "VOLUME", "LOW PASS", "HIGH PASS", "WIDTH", "CRUSH" };
    return names[juce::jlimit(0, kMotionTargets - 1, target)];
}

const char* patternKey(int engine, int half)
{
    static constexpr const char* keys[kEngines][2] { { "timeA", "timeB" }, { "glitchA", "glitchB" }, { "motionA", "motionB" } };
    return keys[juce::jlimit(0, kEngines - 1, engine)][half != 0 ? 1 : 0];
}

const char* locksKey(int engine)
{
    static constexpr const char* keys[kEngines] { "timeLocks", "glitchLocks", "motionLocks" };
    return keys[juce::jlimit(0, kEngines - 1, engine)];
}

const char* allowKey(int engine)
{
    static constexpr const char* keys[kEngines] { "timeAllow", "glitchAllow", "motionAllow" };
    return keys[juce::jlimit(0, kEngines - 1, engine)];
}

Pattern readPattern(const juce::NamedValueSet& set)
{
    const auto number = [&set](const char* key, juce::int64 fallback)
    {
        const auto* v = set.getVarPointer(juce::Identifier(key));
        return v != nullptr ? static_cast<juce::int64>(*v) : fallback;
    };
    Pattern p;
    for (int e = 0; e < kEngines; ++e)
    {
        for (int half = 0; half < 2; ++half)
        {
            const auto bits = static_cast<std::uint32_t>(number(patternKey(e, half), 0) & 0xffffffff);
            for (int i = 0; i < 8; ++i)
            {
                const int v = static_cast<int>((bits >> (4 * i)) & 15u);
                p.steps[static_cast<size_t>(e)][static_cast<size_t>(8 * half + i)] = static_cast<std::uint8_t>(juce::jmin(v, numValues(e) - 1));
            }
        }
        p.locks[static_cast<size_t>(e)] = static_cast<std::uint16_t>(number(locksKey(e), 0) & 0xffff);
        p.allow[static_cast<size_t>(e)] = static_cast<std::uint16_t>(number(allowKey(e), 0xffff) & 0xffff);
    }
    return p;
}

void writePattern(juce::NamedValueSet& set, const Pattern& p)
{
    for (int e = 0; e < kEngines; ++e)
    {
        for (int half = 0; half < 2; ++half)
        {
            std::uint32_t bits = 0;
            for (int i = 0; i < 8; ++i)
                bits |= static_cast<std::uint32_t>(p.steps[static_cast<size_t>(e)][static_cast<size_t>(8 * half + i)] & 15) << (4 * i);
            set.set(juce::Identifier(patternKey(e, half)), static_cast<juce::int64>(bits));
        }
        set.set(juce::Identifier(locksKey(e)), static_cast<int>(p.locks[static_cast<size_t>(e)]));
        set.set(juce::Identifier(allowKey(e)), static_cast<int>(p.allow[static_cast<size_t>(e)]));
    }
}

std::uint64_t mix64(std::uint64_t z) noexcept
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

std::uint64_t hashOf(std::uint64_t seed, std::uint32_t engine, std::uint32_t step, std::uint32_t purpose) noexcept
{
    const std::uint64_t key = (static_cast<std::uint64_t>(kGeneratorVersion) << 48) ^ (static_cast<std::uint64_t>(engine) << 40)
                              ^ (static_cast<std::uint64_t>(step) << 16) ^ purpose;
    return mix64(seed ^ mix64(key));
}

double unitOf(std::uint64_t h) noexcept
{
    return static_cast<double>(h >> 11) * (1.0 / 9007199254740992.0);
}

int maxEvents(int engine, int character) noexcept
{
    return kMaxEvents[juce::jlimit(0, kEngines - 1, engine)][juce::jlimit(0, kCharacters - 1, character)];
}

std::uint64_t nextSeed(std::uint64_t seed) noexcept
{
    return mix64(seed + 0x5DEECE66Dull) % 100000000ull; // eight digits, as the seed counter shows them
}

Pattern generate(const Pattern& current, std::uint64_t seed, int character)
{
    character = juce::jlimit(0, kCharacters - 1, character);
    Pattern p = current;
    for (int e = 0; e < kEngines; ++e)
    {
        auto& steps = p.steps[static_cast<size_t>(e)];
        const auto locks = p.locks[static_cast<size_t>(e)];
        const auto allow = p.allow[static_cast<size_t>(e)];
        const auto shape = static_cast<int>(hashOf(seed, static_cast<std::uint32_t>(e), 0, kPurposeShape) % 5u);
        for (int s = 0; s < kSteps; ++s)
        {
            if ((locks >> s) & 1)
                continue;
            const auto es = static_cast<std::uint32_t>(e), ss = static_cast<std::uint32_t>(s);
            const double chance = juce::jmin(1.0, kDensity[e][character] * positionWeight(character, s));
            const bool event = unitOf(hashOf(seed, es, ss, kPurposeEvent)) < chance;
            const double u = unitOf(hashOf(seed, es, ss, e == Motion ? kPurposeLevel : kPurposeAction));
            int value = 0;
            if (event && e == Time)
                value = drawAction(kTimeWeights[character], kTimeActions, allow, u);
            else if (event && e == Glitch)
                value = drawAction(kGlitchWeights[character], kGlitchActions, allow, u);
            else if (event)
            {
                double level = 0.0;
                switch (character)
                {
                    case Minimal: level = 1.0 + 5.0 * u; break;
                    case Musical: level = 2.0 + 10.0 * (0.75 * shapeAt(shape, s) + 0.25 * u); break;
                    case Experimental: level = 1.0 + 14.0 * u; break;
                    default: level = 6.0 + 9.0 * u; break;
                }
                value = juce::jlimit(1, kMotionLevels - 1, static_cast<int>(level));
            }
            steps[static_cast<size_t>(s)] = static_cast<std::uint8_t>(value);
        }

        // The character's event limit: locked events stay; free ones are kept by a per-step priority.
        const int limit = kMaxEvents[e][character];
        int count = 0;
        std::vector<std::pair<std::uint64_t, int>> free;
        for (int s = 0; s < kSteps; ++s)
        {
            if (steps[static_cast<size_t>(s)] == 0)
                continue;
            if ((locks >> s) & 1)
                ++count;
            else
                free.push_back({ hashOf(seed, static_cast<std::uint32_t>(e), static_cast<std::uint32_t>(s), kPurposePriority), s });
        }
        std::sort(free.begin(), free.end());
        for (const auto& [priority, s] : free)
        {
            juce::ignoreUnused(priority);
            if (count < limit)
                ++count;
            else
                steps[static_cast<size_t>(s)] = 0;
        }
    }
    return p;
}
const char* feelKey(int half)
{
    return half != 0 ? "feelB" : "feelA";
}

Feel readFeel(const juce::NamedValueSet& set)
{
    Feel f {};
    for (int half = 0; half < 2; ++half)
    {
        const auto* v = set.getVarPointer(juce::Identifier(feelKey(half)));
        const auto bits = v != nullptr ? static_cast<std::uint32_t>(static_cast<juce::int64>(*v) & 0xffffffff) : 0u;
        for (int i = 0; i < 8; ++i)
            f[static_cast<size_t>(8 * half + i)] = static_cast<std::uint8_t>((bits >> (4 * i)) & 15u);
    }
    return f;
}

void writeFeel(juce::NamedValueSet& set, const Feel& f)
{
    for (int half = 0; half < 2; ++half)
    {
        std::uint32_t bits = 0;
        for (int i = 0; i < 8; ++i)
            bits |= static_cast<std::uint32_t>(f[static_cast<size_t>(8 * half + i)] & 15) << (4 * i);
        set.set(juce::Identifier(feelKey(half)), static_cast<juce::int64>(bits));
    }
}

Feel feelFrom(const std::array<float, kSteps>& strength)
{
    Feel f {};
    for (size_t i = 0; i < f.size(); ++i)
        f[i] = static_cast<std::uint8_t>(juce::jlimit(0, 15, juce::roundToInt(15.0f * strength[i])));
    return f;
}

bool hasFeel(const Feel& f) noexcept
{
    for (auto v : f)
        if (v != 0)
            return true;
    return false;
}

namespace
{
constexpr std::uint32_t kPurposeCandidate = 16, kPurposeTaste = 40;
constexpr int kBeamWidth = 24, kCandidates = 5;
constexpr double kTargetEvents[2][kCharacters] { { 1.0, 2.0, 4.0, 8.0 }, { 2.0, 4.0, 7.0, 11.0 } };
constexpr double kVarietyWeight[kCharacters] { 0.0, 0.15, 0.35, 0.5 };

// The score of one step: what the action does with the hit (or the quiet) under it.
double localScore(int engine, int character, int step, int action, double hit, int before)
{
    if (action == 0)
        return 0.0;
    double s = 0.0;
    const bool phraseEnd = step % 8 >= 6 || step >= 12;
    if (engine == Time)
    {
        switch (action)
        {
            case Half: case TapeStop: case Double: case TimeReverse: s += phraseEnd ? 0.8 : -0.2; break;
            case Freeze: s += 0.7 * hit; break;
            default: break;
        }
        if (action == TapeStop && step >= 12)
            s += 0.5;
        if (action == before)
            s += 0.3; // TIME runs are wanted (a half-time bar)
    }
    else
    {
        switch (action)
        {
            case Repeat: case Stutter: case Ratchet: s += hit - 0.25 * (1.0 - hit); break;
            case GlitchReverse: s += 0.4 * hit; break;
            case Gap: s += 0.5 * (1.0 - hit) - 1.5 * hit; break;
            case Jump: s += 0.2; break;
            case Scratch: s += 0.5 * hit; break;
            default: break;
        }
        if (action == before && action != Stutter && action != Ratchet)
            s -= 0.3; // the same glitch twice in a row reads as a mistake
    }
    const double* weights = engine == Time ? kTimeWeights[character] : kGlitchWeights[character];
    double top = 0.0;
    for (int a = 1; a < numValues(engine); ++a)
        top = std::max(top, weights[a]);
    s += 0.25 * std::log(1.0e-3 + weights[action] / std::max(1.0e-9, top)); // the character's taste
    return s;
}

struct BeamState
{
    std::array<std::uint8_t, kSteps> steps {};
    double score = 0.0;
    int events = 0;
    std::uint32_t used = 0;
};
} // namespace

Pattern generateFollowing(const Pattern& current, std::uint64_t seed, int character, const Feel& feel)
{
    if (!hasFeel(feel))
        return generate(current, seed, character);
    character = juce::jlimit(0, kCharacters - 1, character);
    auto p = generate(current, seed, character); // MOTION's base levels (and nothing else is kept from it)
    const auto followSeed = seed ^ (static_cast<std::uint64_t>(kFollowingVersion) << 56);

    for (int e = Time; e <= Glitch; ++e)
    {
        const auto es = static_cast<size_t>(e);
        const auto locks = current.locks[es];
        const auto allow = current.allow[es];
        const double* weights = e == Time ? kTimeWeights[character] : kGlitchWeights[character];
        const int limit = maxEvents(e, character);

        // locked events from each step on: they always count, so free events may only use what they leave
        std::array<int, kSteps + 1> lockedAhead {};
        for (int s = kSteps - 1; s >= 0; --s)
            lockedAhead[static_cast<size_t>(s)] = lockedAhead[static_cast<size_t>(s + 1)]
                                                  + (((locks >> s) & 1) != 0 && current.steps[es][static_cast<size_t>(s)] != 0 ? 1 : 0);
        std::vector<BeamState> beam(1), next;
        beam.reserve(kBeamWidth);
        next.reserve(static_cast<size_t>(kBeamWidth * (kCandidates + 1)));
        for (int s = 0; s < kSteps; ++s)
        {
            const auto ss = static_cast<std::uint32_t>(s);
            // this step's candidates: its locked value, or pass + up to 5 seeded draws of allowed actions
            std::array<int, kCandidates + 1> candidates {};
            int count = 0;
            if ((locks >> s) & 1)
                candidates[static_cast<size_t>(count++)] = current.steps[es][static_cast<size_t>(s)];
            else
            {
                candidates[static_cast<size_t>(count++)] = 0;
                for (std::uint32_t j = 0; j < 12 && count < kCandidates + 1; ++j)
                {
                    const int a = drawAction(weights, numValues(e), allow, unitOf(hashOf(followSeed, static_cast<std::uint32_t>(e), ss, kPurposeCandidate + j)));
                    if (a != 0 && std::find(candidates.begin(), candidates.begin() + count, a) == candidates.begin() + count)
                        candidates[static_cast<size_t>(count++)] = a;
                }
            }
            const double hit = feel[static_cast<size_t>(s)] / 15.0;
            next.clear();
            for (const auto& state : beam)
                for (int c = 0; c < count; ++c)
                {
                    const int a = candidates[static_cast<size_t>(c)];
                    const bool locked = ((locks >> s) & 1) != 0;
                    if (a != 0 && !locked && state.events + lockedAhead[static_cast<size_t>(s)] >= limit)
                        continue; // the event limit is a hard rule; locked events (kept anyway) take their places first
                    BeamState n = state;
                    n.steps[static_cast<size_t>(s)] = static_cast<std::uint8_t>(a);
                    const int before = s > 0 ? state.steps[static_cast<size_t>(s - 1)] : 0;
                    n.score += localScore(e, character, s, a, hit, before);
                    if (a != 0)
                    {
                        n.score += 0.6 * (unitOf(hashOf(followSeed, static_cast<std::uint32_t>(e), ss, kPurposeTaste + static_cast<std::uint32_t>(a))) - 0.5);
                        ++n.events;
                        n.used |= 1u << a;
                    }
                    next.push_back(n);
                }
            // keep the best; equal scores fall back to the step values, so the order is fully determined
            std::sort(next.begin(), next.end(), [](const BeamState& a, const BeamState& b)
            {
                if (a.score != b.score)
                    return a.score > b.score;
                return a.steps < b.steps;
            });
            if (next.size() > static_cast<size_t>(kBeamWidth))
                next.resize(static_cast<size_t>(kBeamWidth));
            beam.swap(next);
        }
        // finish: density close to the character's, variety for the adventurous ones
        const BeamState* best = nullptr;
        double bestScore = 0.0;
        for (const auto& state : beam)
        {
            int distinct = 0;
            for (std::uint32_t u = state.used; u != 0; u &= u - 1)
                ++distinct;
            const double d = state.events - kTargetEvents[e][character];
            const double total = state.score - 0.25 * d * d + kVarietyWeight[character] * distinct;
            if (best == nullptr || total > bestScore || (total == bestScore && state.steps < best->steps))
            {
                best = &state;
                bestScore = total;
            }
        }
        p.steps[es] = best->steps;
    }

    // MOTION: the generated levels, lowered on the hits (the movement breathes between them)
    for (int s = 0; s < kSteps; ++s)
        if (((current.locks[Motion] >> s) & 1) == 0)
        {
            auto& v = p.steps[Motion][static_cast<size_t>(s)];
            v = static_cast<std::uint8_t>(juce::roundToInt(v * (0.4 + 0.6 * (1.0 - feel[static_cast<size_t>(s)] / 15.0))));
        }
    p.locks = current.locks;
    p.allow = current.allow;
    return p;
}
} // namespace hpeq::revolution
