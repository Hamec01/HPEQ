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
} // namespace hpeq::revolution
