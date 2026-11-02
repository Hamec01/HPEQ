#include "Core/RevolutionSimple.h"

#include <algorithm>
#include <cmath>

#include "Link/HamloLinkProtocol.h"

namespace hpeq::revolution::simple
{
namespace
{
constexpr int kGlitchLane = Glitch, kTimeLane = Time, kMotionLane = Motion, kShapeA = kFirstShape, kShapeB = kFirstShape + 1,
              kThrowA = kFirstThrow, kThrowB = kFirstThrow + 1;
constexpr double kMaxGroove = 0.1125; // quarter notes: 0.45 of a 1/16 (the engine's limit)

// The genre's swing (the odd 1/16s' delay, in quarter notes) when there is no HPDG to take the groove from.
double genreSwing(int genre) noexcept
{
    switch (genre)
    {
        case BoomBap: return 0.25 * (2.0 * 0.58 - 1.0);
        case Trap: return 0.25 * (2.0 * 0.52 - 1.0);
        case DnB: return 0.25 * (2.0 * 0.53 - 1.0);
        default: return 0.0;
    }
}

struct Candidate
{
    int lane = 0, step = 0, value = 0;
    double priority = 0.0;
    bool onHit = false, fill = false, core = false;
};
} // namespace

const char* genreName(int g)
{
    static constexpr const char* names[kGenres] { "AUTO", "BOOM BAP", "TRAP", "TECHNO", "DNB" };
    return names[juce::jlimit(0, kGenres - 1, g)];
}

const char* intentName(int i)
{
    static constexpr const char* names[kIntents] { "SUBTLE", "GROOVE", "FILLS", "BREAKDOWN", "CHAOS" };
    return names[juce::jlimit(0, kIntents - 1, i)];
}

const char* fillName(int f)
{
    static constexpr const char* names[kFills] { "OFF", "1", "2", "4", "8" };
    return names[juce::jlimit(0, kFills - 1, f)];
}

const char* targetName(int t)
{
    static constexpr const char* names[kTargets] { "HATS", "SNARE", "TOPS", "FULL" };
    return names[juce::jlimit(0, kTargets - 1, t)];
}

int intentOf(double knob) noexcept { return juce::jlimit(0, kIntents - 1, static_cast<int>(std::floor(juce::jlimit(0.0, 1.0, knob) * kIntents))); }

int fillCondition(int fill) noexcept
{
    switch (fill)
    {
        case FillEvery2: return TwoOfTwo;
        case FillEvery4: return FourOfFour;
        case FillEvery8: return EightOfEight;
        default: return Always;
    }
}

Hits hitsFromSnapshot(const hamlo::link::Snapshot& s)
{
    namespace link = hamlo::link;
    Hits h;
    h.linked = true;
    h.hpdgGenre = s.genre;
    h.keyRoot = juce::jlimit(0, 11, static_cast<int>(s.keyRoot));
    h.bpm = s.bpm;
    const int bars = std::max(1, static_cast<int>(s.bars));
    constexpr int sixteenth = link::kPpq / 4;
    std::array<std::array<std::uint32_t, kSteps>, 3> seen {}; // bit b: bar b had a hit (kick, snare, hats)
    std::array<double, kSteps> offsetSum {};
    std::array<int, kSteps> offsetCount {};
    const int count = juce::jlimit(0, link::kMaxEvents, static_cast<int>(s.eventCount));
    for (int i = 0; i < count; ++i)
    {
        const auto& e = s.events[i];
        int role = -1;
        switch (e.lane)
        {
            case link::Kick: role = 0; break;
            case link::Snare:
            case link::Clap: role = 1; break;
            case link::HiHat:
            case link::OpenHat:
            case link::Ride:
            case link::HatFx: role = 2; break;
            default: break;
        }
        if (role < 0 || (e.flags & link::Ghost) != 0)
            continue;
        const int grid = static_cast<int>(std::lround(static_cast<double>(e.tick) / sixteenth));
        const int step = ((grid % kSteps) + kSteps) % kSteps;
        const int barIndex = juce::jlimit(0, 31, static_cast<int>(std::floor(static_cast<double>(grid) / kSteps)) % std::max(1, bars));
        seen[static_cast<size_t>(role)][static_cast<size_t>(step)] |= 1u << barIndex;
        offsetSum[static_cast<size_t>(step)] += static_cast<double>(e.tick - grid * sixteenth) / link::kPpq;
        ++offsetCount[static_cast<size_t>(step)];
    }
    const auto share = [bars](std::uint32_t mask)
    {
        int n = 0;
        for (int b = 0; b < std::min(bars, 32); ++b)
            n += static_cast<int>((mask >> b) & 1u);
        return static_cast<float>(n) / static_cast<float>(std::min(bars, 32));
    };
    // a 1/16 without hits: HPDG's swing on the odd ones (the second 1/16 of a pair at swing % of the pair), 0 on the even
    const double swing = (2.0 * juce::jlimit(50.0, 75.0, static_cast<double>(s.swingPercent)) / 100.0 - 1.0) * 0.25;
    for (int st = 0; st < kSteps; ++st)
    {
        const auto i = static_cast<size_t>(st);
        if (offsetCount[i] == 0 && st % 2 == 1)
            h.groove[i] = static_cast<float>(juce::jlimit(-kMaxGroove, kMaxGroove, swing));
        h.kick[i] = share(seen[0][i]);
        h.snare[i] = share(seen[1][i]);
        h.hats[i] = share(seen[2][i]);
        if (offsetCount[i] > 0)
            h.groove[i] = static_cast<float>(juce::jlimit(-kMaxGroove, kMaxGroove, offsetSum[i] / offsetCount[i]));
    }
    return h;
}

Hits templateHits(int genre)
{
    Hits h;
    const auto set = [](std::array<float, kSteps>& a, std::initializer_list<int> steps)
    {
        for (int s : steps)
            a[static_cast<size_t>(s)] = 1.0f;
    };
    switch (genre)
    {
        case Trap:
            set(h.kick, { 0, 7, 10 });
            set(h.snare, { 8 });
            set(h.hats, { 0, 2, 4, 6, 8, 10, 12, 13, 14, 15 });
            break;
        case Techno:
            set(h.kick, { 0, 4, 8, 12 });
            set(h.snare, { 4, 12 });
            set(h.hats, { 2, 6, 10, 14 });
            break;
        case DnB:
            set(h.kick, { 0, 10 });
            set(h.snare, { 4, 12 });
            set(h.hats, { 0, 2, 4, 6, 8, 10, 12, 14 });
            break;
        default:
            set(h.kick, { 0, 7, 10 });
            set(h.snare, { 4, 12 });
            set(h.hats, { 0, 2, 4, 6, 8, 10, 12, 14 });
            break;
    }
    for (int s = 1; s < kSteps; s += 2)
        h.groove[static_cast<size_t>(s)] = static_cast<float>(genreSwing(genre));
    return h;
}

Hits hitsFromFeel(const Feel& feel, int genre)
{
    Hits h = templateHits(genre);
    h.listened = true;
    for (int s = 0; s < kSteps; ++s)
    {
        const float v = static_cast<float>(feel[static_cast<size_t>(s)]) / 15.0f;
        h.kick[static_cast<size_t>(s)] = h.snare[static_cast<size_t>(s)] = h.hats[static_cast<size_t>(s)] = v;
    }
    return h;
}

int resolveGenre(int genre, const Hits& hits, double hostBpm) noexcept
{
    if (genre > AutoGenre && genre < kGenres)
        return genre;
    if (hits.linked && hits.hpdgGenre >= 0)
    {
        switch (hits.hpdgGenre)
        {
            case hamlo::link::Trap:
            case hamlo::link::Drill: return Trap;
            case hamlo::link::DnB: return DnB;
            case hamlo::link::Techno:
            case hamlo::link::Atmospheric: return Techno;
            default: return BoomBap;
        }
    }
    const double bpm = hits.bpm > 0.0 ? hits.bpm : hostBpm;
    if (bpm >= 155.0)
        return DnB;
    if (bpm >= 120.0 && bpm <= 135.0)
        return Techno;
    if (bpm > 135.0)
        return Trap;
    return BoomBap;
}

Result generate(const Settings& settings, const Hits& hits, double hostBpm)
{
    Result r;
    r.genre = resolveGenre(settings.genre, hits, hostBpm);
    r.intent = intentOf(settings.intent);
    r.linked = hits.linked;
    r.listened = hits.listened;
    const int g = r.genre, zone = r.intent;
    const double amount = juce::jlimit(0.0, 1.0, settings.amount), variation = juce::jlimit(0.0, 1.0, settings.variation);
    const bool fills = settings.fill != FillOff;
    // TRAP keeps the 808 clean: FULL becomes TOPS
    const int target = g == Trap && settings.target == FullTarget ? static_cast<int>(TopsTarget) : settings.target;
    const int band = target == Hats ? High : target == Snare ? Mid : target == TopsTarget ? static_cast<int>(Tops) : static_cast<int>(Full);
    const auto& role = target == Hats ? hits.hats : hits.snare;
    const auto hitAt = [&](int s)
    {
        const auto i = static_cast<size_t>(s & 15);
        if (target == TopsTarget)
            return std::max(hits.hats[i], hits.snare[i]) >= kHit;
        if (target == FullTarget)
            return std::max({ hits.hats[i], hits.snare[i], hits.kick[i] }) >= kHit;
        return role[i] >= kHit;
    };
    const auto snareAt = [&](int s) { return hits.snare[static_cast<size_t>(s & 15)] >= kHit; };
    const auto kickAt = [&](int s) { return hits.kick[static_cast<size_t>(s & 15)] >= kHit; };

    // weight of a rule family in this INTENT zone (SUBTLE, GROOVE, FILLS, BREAKDOWN, CHAOS)
    const auto in = [zone](double s, double gr, double f, double b, double c)
    {
        const double w[kIntents] { s, gr, f, b, c };
        return w[zone];
    };
    std::vector<Candidate> c;
    const bool twoBars = variation >= 0.34;
    const auto add = [&](int lane, int step, int value, double weight, bool onHit = false)
    {
        if (weight <= 0.0 || value <= 0)
            return;
        for (int bar = 0; bar < (twoBars && lane == kGlitchLane ? 2 : 1); ++bar)
        {
            const auto h = hashOf(settings.seed, static_cast<std::uint32_t>(lane), static_cast<std::uint32_t>(step + 16 * bar), 41);
            c.push_back({ lane, step + 16 * bar, value, weight * (0.35 + 0.65 * unitOf(h)), onHit, false, weight >= 0.9 });
        }
    };
    const auto addFill = [&](int lane, int step, int value, double weight)
    {
        if (!fills || weight <= 0.0 || value <= 0)
            return;
        const auto h = hashOf(settings.seed, static_cast<std::uint32_t>(lane), static_cast<std::uint32_t>(step), 43);
        c.push_back({ lane, step, value, weight * (0.35 + 0.65 * unitOf(h)), false, true, true });
    };
    const auto stepHash = [&](int lane, int step, std::uint32_t purpose)
    {
        return unitOf(hashOf(settings.seed, static_cast<std::uint32_t>(lane), static_cast<std::uint32_t>(step), purpose));
    };

    // ---- the genre rules (section 3) -------------------------------------------------------------------
    int motionTarget = LowPass, shapeATarget = ShapeLowPass, shapeBTarget = ShapeVolume, throwATarget = ThrowDelay, throwBTarget = ThrowReverb;
    int throwATime = kDefaultThrowTime, throwBTime = kDefaultThrowTime;
    double throwATail = 0.4, throwBTail = 0.5, throwBTone = 0.5;
    bool shapeACurve = false, shapeBCurve = false;
    int shapeBRate = 0;
    switch (g)
    {
        case BoomBap:
        {
            for (int s = 0; s < kSteps; ++s)
            {
                if (!hitAt(s))
                    add(kMotionLane, s, 7 + (s % 4), in(0.4, 0.5, 0.3, 0.9, 0.5));
                if (snareAt(s) && s >= 8)
                    add(kThrowA, s, 12, in(0.3, 0.7, 0.8, 0.4, 0.6)); // a delay throw on the last snare
            }
            for (int s = 12; s < kSteps; ++s)
            {
                addFill(kTimeLane, s, zone == Breakdown ? Half : TapeStop, in(0.3, 0.5, 1.0, 1.0, 0.8)); // the last beat
                addFill(kThrowB, s, s == 15 ? 15 : 0, in(0.4, 0.6, 1.0, 0.8, 0.8)); // the vinyl brake at the phrase end
            }
            shapeACurve = true;
            shapeBTarget = ShapeLowPass;
            shapeBCurve = true;
            throwATime = 7;                // 1/8 dotted
            throwBTarget = ThrowVinyl;
            throwBTail = 1.0;              // the brake
            throwBTone = 0.3;
            break;
        }
        case Trap:
        {
            for (int s = 0; s < kSteps; ++s)
            {
                if (hitAt(s))
                {
                }
                add(kMotionLane, s, 1 + (s * 11) / 15, in(0.0, 0.0, 0.0, 0.8, 0.3)); // a high-pass build
                if (s % 4 == 3)
                    add(kThrowA, s, 10, in(0.2, 0.6, 0.6, 0.3, 0.6));
            }
            for (int s = 0; s < kSteps; ++s)
                addFill(kTimeLane, s, zone == Breakdown ? (s >= 8 ? Half : 0) : s >= 12 ? TapeStop : 0, in(0.3, 0.4, 1.0, 1.0, 0.8));
            addFill(kThrowB, 12, 15, in(0.3, 0.5, 1.0, 0.8, 0.8));
            motionTarget = HighPass;
            shapeATarget = ShapePan;
            shapeBTarget = ShapeLowPass;
            shapeBCurve = true;
            throwATime = 2;                // 1/16
            throwBTarget = ThrowReverb;
            break;
        }
        case Techno:
        {
            for (int s = 0; s < kSteps; ++s)
            {
                if (s % 4 == 2 && (snareAt(s) || hits.hats[static_cast<size_t>(s)] >= kHit))
                    add(kThrowA, s, 13, in(0.3, 0.9, 0.9, 0.5, 0.7), hitAt(s)); // delay on the off-beat
                if (s == 0 || s == 8)
                    add(kMotionLane, s, 9, in(0.4, 0.4, 0.3, 0.6, 0.3)); // slow width moves
            }
            for (int s = 12; s < kSteps; ++s)
                addFill(kTimeLane, s, zone == Breakdown ? Freeze : s >= 14 ? TapeStop : 0, in(0.2, 0.3, 0.8, 1.0, 0.7));
            addFill(kThrowB, 12, 15, in(0.4, 0.6, 1.0, 0.9, 0.8));
            addFill(kThrowB, 14, 12, in(0.3, 0.4, 0.8, 0.8, 0.8));
            motionTarget = Width;
            shapeATarget = ShapeVolume;
            shapeBTarget = ShapeLowPass;
            shapeBCurve = true;
            shapeBRate = variation >= 0.25 ? 1 : 0; // from 25 %: the sweep at 1/4 = over 4 bars
            throwATime = 7;
            throwBTarget = ThrowReverb;
            throwBTail = 0.6;
            break;
        }
        case DnB:
        default:
        {
            for (int s = 0; s < kSteps; ++s)
            {
                if (snareAt(s))
                {
                    add(kThrowA, s, 9, in(0.2, 0.5, 0.6, 0.3, 0.6), hitAt(s));
                }
                add(kMotionLane, s, 10, in(0.0, 0.0, 0.0, 0.9, 0.0));
            }
            for (int s = 0; s < kSteps; ++s)
                addFill(kTimeLane, s, zone == Breakdown ? (s >= 8 ? Half : 0) : s >= 12 ? TapeStop : 0, in(0.3, 0.5, 1.0, 1.0, 0.8));
            addFill(kThrowB, 12, 15, in(0.3, 0.5, 0.9, 0.8, 0.8));
            shapeATarget = ShapeDrive;
            shapeBTarget = ShapeHighPass;
            shapeBCurve = true;
            throwATime = 2; // 1/16
            throwBTarget = ThrowReverb;
            break;
        }
    }

    // ---- AMOUNT: the first round(AMOUNT x N) by priority (ties: lane, step) ------------------------------
    std::stable_sort(c.begin(), c.end(), [](const Candidate& a, const Candidate& b)
    {
        if (a.priority != b.priority)
            return a.priority > b.priority;
        return a.lane != b.lane ? a.lane < b.lane : a.step < b.step;
    });
    const auto keep = amount <= 0.0 || c.empty() ? size_t { 0 } : std::max<size_t>(1, static_cast<size_t>(std::lround(amount * static_cast<double>(c.size()))));
    const int code = static_cast<int>(std::lround(4.0 * variation)); // up to 50 % on the weaker events
    for (size_t i = 0; i < keep && i < c.size(); ++i)
    {
        const auto& e = c[i];
        auto& lane = r.lanes[static_cast<size_t>(e.lane)];
        const auto s = static_cast<size_t>(e.step);
        std::uint8_t* slot = e.step >= kSteps                 ? &lane.extra[s - kSteps]
                             : e.lane < kEngines ? &r.pattern.steps[static_cast<size_t>(e.lane)][s]
                                                 : &lane.first[s];
        if (*slot != 0)
            continue; // a stronger event has this step
        *slot = static_cast<std::uint8_t>(e.value);
        ++r.events;
        if (e.onHit)
            r.onHit[static_cast<size_t>(e.lane)] |= 1u << e.step;
        if (e.fill)
            r.fills[static_cast<size_t>(e.lane)] |= 1u << e.step;
        if (!e.fill && !e.core && code > 0)
            lane.probability[s] = static_cast<std::uint8_t>(code);
    }

    // ---- SHAPE: movements, not blips (ShaperBox: a drawn curve looping over a beat, a bar or bars) -----------
    // Each genre has a movement per lane; INTENT scales its depth (SUBTLE half), AMOUNT how deep it goes, so a higher
    // AMOUNT never writes fewer levels. SHAPE B sweeps over 4 bars (rate 1/4) from VARIATION 25 %, else over a bar.
    {
        static constexpr double zoneScale[kIntents] { 0.5, 0.75, 0.85, 1.0, 1.0 };
        const double scale = (0.35 + 0.65 * amount) * zoneScale[zone];
        const auto write = [&](int laneIndex, int rate, int length, std::initializer_list<int> levels)
        {
            auto& lane = r.lanes[static_cast<size_t>(laneIndex)];
            lane.rate = rate;
            lane.length = length == kSteps ? 0 : length;
            int i = 0;
            for (int v : levels)
            {
                const auto level = static_cast<std::uint8_t>(juce::jlimit(0, 15, static_cast<int>(std::lround(v * scale))));
                if (i < kSteps)
                    lane.first[static_cast<size_t>(i)] = level;
                else
                    lane.extra[static_cast<size_t>(i - kSteps)] = level;
                r.events += level != 0;
                ++i;
            }
        };
        const bool longSweep = variation >= 0.25;
        switch (g)
        {
            case Techno: // the pump on every beat (down on the kick, back up), CHAOS: a 1/16 gate; a low-pass sweep
                // written over the whole bar (the page shows all 16 cells; a 4-step lane would look empty after beat 1)
                if (zone == Chaos)
                    write(kShapeA, 0, kSteps, { 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0 });
                else
                    write(kShapeA, 0, kSteps, { 15, 10, 5, 1, 15, 10, 5, 1, 15, 10, 5, 1, 15, 10, 5, 1 });
                shapeACurve = zone != Chaos;
                break;
            case Trap: // the pan moves a beat at a time, left / right
                write(kShapeA, 1, 4, { 9, 9, 9, 9 });
                break;
            case DnB: // drive breathing on the snares (2 and 4), over the eighth after each
                write(kShapeA, 0, kSteps, { 0, 0, 1, 3, 9, 6, 3, 1, 0, 0, 1, 3, 9, 6, 3, 1 });
                shapeACurve = true;
                break;
            default: // BOOM BAP: a dusty low-pass that opens into the bar end, at 1/8
                write(kShapeA, 2, 8, { 6, 3, 1, 0, 1, 3, 6, 10 });
                shapeACurve = true;
                break;
        }
        // SHAPE B: the sweep (TECHNO / BOOM BAP / TRAP low pass, DNB high pass) - 4 bars at 1/4, or one bar at 1/16
        if (longSweep)
            write(kShapeB, 1, kSteps, { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 8, 9, 11, 13, 6 });
        else
            write(kShapeB, 0, kSteps, { 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 10, 12, 4 });
        shapeBCurve = true;
        shapeBRate = longSweep ? 1 : 0;
    }
    // ---- THROW: a send of an eighth (A) or the rest of the beat (B, the fill throw), not a 1/16 blip ---------------
    for (const int laneIndex : { kThrowA, kThrowB })
    {
        auto& lane = r.lanes[static_cast<size_t>(laneIndex)];
        const int extend = laneIndex == kThrowA ? 1 : 3;
        for (int st = kSteps - 1; st >= 0; --st) // backwards: a written step's extension is not extended again
            if (lane.first[static_cast<size_t>(st)] != 0 && (st == 0 || lane.first[static_cast<size_t>(st - 1)] == 0))
                for (int k = 1; k <= extend && st + k < kSteps && (laneIndex == kThrowA || (st + k) / 4 == st / 4); ++k) // B: within its beat
                    if (lane.first[static_cast<size_t>(st + k)] == 0)
                    {
                        lane.first[static_cast<size_t>(st + k)] = static_cast<std::uint8_t>(std::max(1, lane.first[static_cast<size_t>(st)] * 3 / 4));
                        ++r.events;
                        if (((r.fills[static_cast<size_t>(laneIndex)] >> st) & 1u) != 0)
                            r.fills[static_cast<size_t>(laneIndex)] |= 1u << (st + k);
                    }
    }

    // ---- GLITCH 2 (docs/GLITCH_RESEARCH.md): the playing audio cut and repeated, as events, not a texture -------
    // A budget of events per bar by INTENT (x AMOUNT; sustained material: fewer); rolls and risers only in the phrase
    // tail (the bar's last beat), repeats on the strongest hits (drums) or on the beats (sustained); never two events
    // side by side. The events are ranked once: a higher AMOUNT keeps every event of a lower one.
    {
        struct Weighted { int action; double weight; };
        std::vector<Weighted> onHits, tails;
        switch (g)
        {
            case Trap: onHits = { { BlockRoll, 1.0 }, { BlockRepeat, 0.7 }, { BlockDrop, 0.6 }, { BlockChop, 0.4 } };
                       tails = { { BlockRise, 0.9 }, { BlockRoll, 0.8 }, { BlockStop, 0.6 } }; break;
            case Techno: onHits = { { BlockChop, 1.0 }, { BlockRepeat, 0.7 }, { BlockShuffle, 0.4 }, { BlockDrop, 0.3 } };
                         tails = { { BlockRise, 1.0 }, { BlockRoll, 0.6 }, { BlockReverse, 0.4 } }; break;
            case DnB: onHits = { { BlockShuffle, 1.0 }, { BlockRoll, 0.8 }, { BlockReverse, 0.6 }, { BlockDrop, 0.4 } };
                      tails = { { BlockReverse, 0.9 }, { BlockRoll, 0.8 }, { BlockStop, 0.6 }, { BlockRise, 0.5 } }; break;
            default: onHits = { { BlockShuffle, 1.0 }, { BlockRepeat, 0.8 }, { BlockDrop, 0.5 }, { BlockChop, 0.3 } };
                     tails = { { BlockStop, 0.8 }, { BlockReverse, 0.6 }, { BlockRoll, 0.4 }, { BlockRise, 0.2 } }; break;
        }
        static constexpr double budget[kIntents] { 1.0, 2.0, 2.0, 3.0, 6.0 }, hitZone[kIntents] { 1.0, 1.0, 0.6, 0.7, 1.0 },
                                tailZone[kIntents] { 0.3, 0.6, 1.0, 0.8, 1.0 };
        const auto boost = [zone](int action)
        {
            if (zone == Subtle)
                return action == BlockRepeat || action == BlockShuffle ? 1.0 : 0.2;
            if (zone == Breakdown)
                return action == BlockDrop || action == BlockChop || action == BlockStop ? 1.5 : 1.0;
            return 1.0;
        };
        int count = static_cast<int>(std::lround(budget[zone] * amount * (settings.sustained ? 0.6 : 1.0)));
        if (count == 0 && amount >= 0.3)
            count = 1;
        struct Event { int step, length, action; double priority; bool onHit, tail; };
        auto& glitchSteps = r.pattern.steps[static_cast<size_t>(kGlitchLane)];
        auto& glitchLane = r.lanes[static_cast<size_t>(kGlitchLane)];
        for (int bar = 0; bar < (twoBars ? 2 : 1); ++bar)
        {
            std::vector<Event> events;
            const auto rank = [&](int step, int action, std::uint32_t purpose)
            {
                return 0.35 + 0.65 * unitOf(hashOf(settings.seed, static_cast<std::uint32_t>(50 + bar), static_cast<std::uint32_t>(16 * step + action), purpose));
            };
            // where: the strongest hits of the TARGET (drums) or the beats (sustained, or a source without hits)
            std::vector<std::pair<int, double>> places;
            if (!settings.sustained)
                for (int st = 0; st < 12; ++st)
                    if (hitAt(st))
                        places.push_back({ st, snareAt(st) ? 1.2 : 1.0 });
            const bool onTheHits = !places.empty();
            if (places.empty())
                places = { { 4, 1.0 }, { 8, 1.0 }, { 0, 0.6 }, { 6, 0.6 }, { 10, 0.6 } };
            for (const auto& [st, weight] : places)
                for (const auto& [action, w] : onHits)
                {
                    // R8: an eighth for REPEAT / ROLL / SHUFFLE, a beat for DROP / CHOP (ShaperBox-length moves, not blips)
                    const int length = action == BlockDrop || action == BlockChop ? 4 : 2;
                    if (st + length > 12)
                        continue;
                    events.push_back({ st, length, action, weight * w * hitZone[zone] * boost(action) * rank(st, action, 61), onTheHits, false });
                }
            for (const auto& [action, w] : tails)
            {
                const int start = action == BlockRise && (zone == Fills || zone == Chaos) ? 8 : 12; // R8: the last beat, RISE half a bar
                events.push_back({ start, 16 - start, action, w * tailZone[zone] * (bar == 1 ? 1.3 : 1.0) * boost(action) * rank(start, action, 67), false, true });
            }
            std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b)
            {
                if (a.priority != b.priority)
                    return a.priority > b.priority;
                return a.step != b.step ? a.step < b.step : a.action < b.action;
            });
            std::uint32_t used = 0;
            int picked = 0, stepsUsed = 0;
            static constexpr int caps[kIntents] { 4, 6, 8, 8, kSteps }; // R8: GLITCH's share of the bar per INTENT
            const int stepCap = caps[zone];
            bool tailTaken = false;
            for (const auto& e : events)
            {
                if (picked >= count)
                    break;
                if (e.tail && tailTaken)
                    continue;
                // the event's steps and their neighbours must be free: never two events side by side
                const std::uint32_t mask = ((1u << e.length) - 1u) << e.step;
                const std::uint32_t around = (mask << 1) | (mask >> 1) | mask;
                if ((used & around) != 0 || stepsUsed + e.length > stepCap)
                    continue;
                stepsUsed += e.length;
                used |= mask;
                tailTaken = tailTaken || e.tail;
                for (int k = 0; k < e.length; ++k)
                {
                    const int st = e.step + k + 16 * bar;
                    if (st < kSteps)
                        glitchSteps[static_cast<size_t>(st)] = static_cast<std::uint8_t>(e.action);
                    else
                        glitchLane.extra[static_cast<size_t>(st - kSteps)] = static_cast<std::uint8_t>(e.action);
                    if (!e.tail && picked > 0 && code > 0)
                        glitchLane.probability[static_cast<size_t>(st)] = static_cast<std::uint8_t>(code); // VARIATION: the weaker ones by chance
                    ++r.events;
                }
                if (e.onHit && hitAt(e.step))
                    r.onHit[static_cast<size_t>(kGlitchLane)] |= 1u << (e.step + 16 * bar);
                ++picked;
            }
        }
    }

    // ---- the lanes' settings ---------------------------------------------------------------------------
    for (int l = 0; l < kLanes; ++l)
    {
        auto& lane = r.lanes[static_cast<size_t>(l)];
        lane.band = band;
        if (l == kTimeLane || l == kThrowB)
            lane.condition = fills ? fillCondition(settings.fill) : Always;
    }
    if (twoBars)
        r.lanes[static_cast<size_t>(kGlitchLane)].length = 2 * kSteps;
    r.lanes[static_cast<size_t>(kShapeB)].rate = shapeBRate;
    for (int s = 0; s < kSteps; ++s)
        r.groove[static_cast<size_t>(s)] = static_cast<float>(juce::jlimit(-kMaxGroove, kMaxGroove, static_cast<double>(hits.groove[static_cast<size_t>(s)])));

    // ---- the lane parameters ---------------------------------------------------------------------------
    const double depth = 0.4 + 0.6 * amount;
    r.parameters = { { "grid", 2 },
                     { "triplet", 0 },
                     { "free", 0 },
                     { "steps", 16 },
                     { "swing", 0 },
                     { "humanize", 0 },
                     { "order", 0 },
                     { "timeStretch", 1 },
                     { "timeOn", 1 },
                     { "timeMix", 1 },
                     { "timeDepth", 1 },
                     { "glitchOn", 1 },
                     { "glitchMix", 1 },
                     { "glitchDepth", 0.5 }, // GLITCH 2: DEPTH = how far the repeats decay, not their rate
                     { "glitchEngine", 1 },
                     { "motionOn", 1 },
                     { "motionMix", 1 },
                     { "motionDepth", 0.3 + 0.5 * amount },
                     { "motionSmooth", 0.3 },
                     { "motionTarget", motionTarget },
                     { "shapeAOn", 1 },
                     { "shapeATarget", shapeATarget },
                     { "shapeACurve", shapeACurve ? 1 : 0 },
                     { "shapeAMix", 1 },
                     { "shapeADepth", depth },
                     { "shapeASmooth", 0 },
                     { "shapeARes", 0.2 },
                     { "shapeBOn", 1 },
                     { "shapeBTarget", shapeBTarget },
                     { "shapeBCurve", shapeBCurve ? 1 : 0 },
                     { "shapeBMix", 1 },
                     { "shapeBDepth", depth },
                     { "shapeBSmooth", 0 },
                     { "shapeBRes", 0.3 },
                     { "throwAOn", 1 },
                     { "throwATarget", throwATarget },
                     { "throwATime", throwATime },
                     { "throwANote", hits.keyRoot },
                     { "throwATail", throwATail },
                     { "throwATone", 0.5 },
                     { "throwAMix", 0.6 },
                     { "throwADepth", depth },
                     { "throwBOn", 1 },
                     { "throwBTarget", throwBTarget },
                     { "throwBTime", throwBTime },
                     { "throwBNote", hits.keyRoot },
                     { "throwBTail", throwBTail },
                     { "throwBTone", throwBTone },
                     { "throwBMix", 0.7 },
                     { "throwBDepth", 1 } };
    return r;
}
} // namespace hpeq::revolution::simple
