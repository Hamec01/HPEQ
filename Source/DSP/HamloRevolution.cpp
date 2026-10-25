#include "DSP/HamloRevolution.h"

#include <algorithm>
#include <cmath>

#include "Core/Revolution.h"

namespace hpeq::dsp
{
namespace
{
namespace rev = hpeq::revolution;
constexpr double kPiD = 3.14159265358979323846;
constexpr double kEps = 1.0e-4; // samples: a boundary closer than this to a sample belongs to the later step
constexpr double kBeats[] { 1.0, 0.5, 0.25, 0.125 };

long long floorDiv(long long a, long long b) noexcept { return a >= 0 ? a / b : -((-a + b - 1) / b); }
int wrap(long long k, int count) noexcept
{
    const auto r = k % count;
    return static_cast<int>(r < 0 ? r + count : r);
}
double raisedCos(double x) noexcept { return 0.5 - 0.5 * std::cos(kPiD * std::clamp(x, 0.0, 1.0)); }

// The humanize offsets: SplitMix64 of (seed, step), fixed so a project renders the same everywhere.
double unitHash(std::uint32_t seed, long long k) noexcept
{
    std::uint64_t z = (static_cast<std::uint64_t>(seed) << 32) ^ static_cast<std::uint64_t>(k) ^ 0x6A09E667F3BCC909ull;
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    return static_cast<double>(z >> 11) * (1.0 / 9007199254740992.0);
}
} // namespace

// ------------------------------------------------------------------------------------------ tape
void RevolutionEngine::Tape::push(const float* x, int numChannels) noexcept
{
    if (++head >= size)
        head = 0;
    data[0][head] = x[0];
    data[1][head] = numChannels > 1 ? x[1] : x[0];
    if (written < size)
        ++written;
}

float RevolutionEngine::Tape::at(int ch, long long age) const noexcept
{
    long long index = head - age;
    if (index < 0)
        index += size;
    return data[ch][index];
}

bool RevolutionEngine::Segment::same(const Segment& o) const noexcept
{
    if (valid != o.valid)
        return false;
    if (!valid)
        return true;
    if (action != o.action)
        return false;
    return action == 0 || (runK == o.runK && stamp == o.stamp);
}

// ------------------------------------------------------------------------------------- lifecycle
void RevolutionEngine::prepare(double newSampleRate, std::array<std::vector<float>, 2>& glitchStorage)
{
    sampleRate = newSampleRate;
    const auto timeSize = static_cast<size_t>(std::ceil(kTimeHistorySeconds * sampleRate)) + 32;
    for (int ch = 0; ch < 2; ++ch)
    {
        timeStorage[ch].assign(timeSize, 0.0f);
        tapes[0].data[ch] = timeStorage[ch].data();
        tapes[1].data[ch] = glitchStorage[static_cast<size_t>(ch)].data();
    }
    tapes[0].size = static_cast<int>(timeSize);
    tapes[1].size = static_cast<int>(glitchStorage[0].size());

    // 15-tap half-band low-pass (Blackman-windowed sinc, cut-off fs/4, DC gain 1) for the 2X head.
    double sum = 0.0;
    for (int k = -7; k <= 7; ++k)
    {
        double h = 0.0;
        if (k == 0)
            h = 0.5;
        else if (k % 2 != 0)
        {
            const double x = (k + 8) / 16.0;
            const double window = 0.42 - 0.5 * std::cos(2.0 * kPiD * x) + 0.08 * std::cos(4.0 * kPiD * x);
            h = std::sin(kPiD * k / 2.0) / (kPiD * k) * window;
        }
        halfband[static_cast<size_t>(k + 7)] = static_cast<float>(h);
        sum += h;
    }
    for (auto& h : halfband)
        h = static_cast<float>(h / sum);
    reset();
}

void RevolutionEngine::reset() noexcept
{
    n = n0 = 0;
    q0 = 0.0;
    anchored = false;
    lastPlaying = lastFree = false;
    fresh = true;
    shownStep.store(-1, std::memory_order_relaxed);
    newEpoch();
}

void RevolutionEngine::newEpoch() noexcept
{
    for (auto& t : tapes)
        t.written = 0;
    for (auto& s : stages)
    {
        s.cur = {};
        s.prev = {};
        s.fadePos = s.fadeLength = 0;
    }
    motionStepValid = false;
    ic1 = {};
    ic2 = {};
    held = {};
    svfG = 0.0;
    epochs.fetch_add(1, std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------------ parameters
void RevolutionEngine::readParams(const FxSlotParams& p) noexcept
{
    const auto& v = p.values;
    // What changes a head's formula (not mixes, on, order or the MOTION target): a new stamp makes the
    // next segment a different one, so the change is crossfaded instead of jumping.
    static constexpr int formula[] { slot::kRevGrid, slot::kRevTriplet, slot::kRevSwing, slot::kRevSteps, slot::kRevFree, slot::kRevFreeMs,
                                     slot::kRevHumanize, slot::kRevHumanSeed, slot::kTimeDepth, slot::kGlitchDepth, slot::kRevFadeMs };
    bool changed = false;
    for (int i : formula)
        changed = changed || v[static_cast<size_t>(i)] != lastValues[static_cast<size_t>(i)];
    for (int i = 0; i < 4; ++i)
        changed = changed || v[static_cast<size_t>(slot::kTimePattern + i)] != lastValues[static_cast<size_t>(slot::kTimePattern + i)]
                  || v[static_cast<size_t>(slot::kGlitchPattern + i)] != lastValues[static_cast<size_t>(slot::kGlitchPattern + i)];
    if (changed)
        ++stamp;
    lastValues = v;

    steps = std::clamp(static_cast<int>(std::lround(v[slot::kRevSteps])), 1, rev::kSteps);
    grid = std::clamp(static_cast<int>(std::lround(v[slot::kRevGrid])), 0, 3);
    triplet = v[slot::kRevTriplet] >= 0.5f;
    swing = 0.5 + 0.25 * std::clamp(static_cast<double>(v[slot::kRevSwing]), 0.0, 1.0);
    freeMode = v[slot::kRevFree] >= 0.5f;
    freeMs = std::clamp(static_cast<double>(v[slot::kRevFreeMs]), 20.0, 2000.0);
    humanize = std::clamp(static_cast<double>(v[slot::kRevHumanize]), 0.0, 1.0);
    humanSeed = static_cast<std::uint32_t>(std::max(0.0f, v[slot::kRevHumanSeed]));
    fadeSamples = std::max(1.0, std::clamp(static_cast<double>(v[slot::kRevFadeMs]), 0.5, 8.0) * 1.0e-3 * sampleRate);
    smooth = std::clamp(static_cast<double>(v[slot::kMotionSmooth]), 0.0, 1.0);
    motionTarget = std::clamp(static_cast<int>(std::lround(v[slot::kMotionTarget])), 0, rev::kMotionTargets - 1);
    static constexpr int base[3] { slot::kTimeOn, slot::kGlitchOn, slot::kMotionOn };
    for (int e = 0; e < 3; ++e)
    {
        const auto b = static_cast<size_t>(base[e]);
        on[static_cast<size_t>(e)] = v[b] >= 0.5f;
        mixTarget[static_cast<size_t>(e)] = std::clamp(static_cast<double>(v[b + 1]), 0.0, 1.0);
        depth[static_cast<size_t>(e)] = std::clamp(static_cast<double>(v[b + 2]), 0.0, 1.0);
        for (int q = 0; q < 4; ++q)
            patternQuads[static_cast<size_t>(4 * e + q)] = v[b + 3 + static_cast<size_t>(q)];
    }
    stepQ = freeMode ? 1.0 : kBeats[grid] * (triplet ? 2.0 / 3.0 : 1.0);
    wantedOrder = std::clamp(static_cast<int>(std::lround(v[slot::kRevOrder])), 0, rev::kOrders - 1);
    if (fresh)
    {
        activeOrder = wantedOrder;
        orderGain = 1.0;
        for (int e = 0; e < 2; ++e)
            stages[static_cast<size_t>(e)].mix = on[static_cast<size_t>(e)] ? mixTarget[static_cast<size_t>(e)] : 0.0;
        motionMix = on[2] ? mixTarget[2] : 0.0;
        fresh = false;
    }
    order = rev::orderOf(activeOrder);
}

void RevolutionEngine::syncClock(const FxSlotParams&, const Transport& transport, int blockOffset) noexcept
{
    const double bpm = std::clamp(transport.bpm, 20.0, 999.0);
    const double dqNew = freeMode ? 1.0 / (freeMs * 1.0e-3 * sampleRate) : bpm / (60.0 * sampleRate);
    const bool playing = transport.playing && transport.hasPosition;
    const double hostQ = transport.ppqAtBlockStart + blockOffset * bpm / (60.0 * sampleRate);
    const double predicted = q0 + static_cast<double>(n - n0) * dq;
    const auto anchor = [this](double q, double newDq, bool epoch)
    {
        q0 = q;
        n0 = n;
        dq = newDq;
        if (epoch)
            newEpoch();
    };

    if (!anchored || freeMode != lastFree)
        anchor(!freeMode && playing ? hostQ : 0.0, dqNew, true);
    else if (freeMode)
    {
        if (playing && !lastPlaying)
            anchor(0.0, dqNew, true); // free steps start with the transport
        else if (dqNew != dq)
            anchor(predicted, dqNew, false);
    }
    else if (playing)
    {
        if (!lastPlaying || std::abs(hostQ - predicted) > 0.02)
            anchor(hostQ, dqNew, true); // start, seek or loop: a new epoch
        else if (dqNew != dq)
            anchor(hostQ, dqNew, false); // tempo change: continue from where the host says we are
    }
    else if (dqNew != dq)
        anchor(predicted, dqNew, false); // stopped: free-running at the host tempo
    anchored = true;
    lastPlaying = playing;
    lastFree = freeMode;
}

// ------------------------------------------------------------------------------------------ grid
double RevolutionEngine::stepStartQ(long long k) const noexcept
{
    const double pair = 2.0 * stepQ;
    double start = static_cast<double>(floorDiv(k, 2)) * pair + ((k & 1) != 0 ? pair * swing : 0.0);
    if (humanize > 0.0)
        start += humanize * 0.12 * stepQ * (2.0 * unitHash(humanSeed, k) - 1.0);
    return start;
}

RevolutionEngine::Step RevolutionEngine::stepAt(double q) const noexcept
{
    const double pair = 2.0 * stepQ, qe = q + kEps * dq;
    const auto p = static_cast<long long>(std::floor(qe / pair));
    long long k = 2 * p + (qe - static_cast<double>(p) * pair >= pair * swing ? 1 : 0);
    while (qe < stepStartQ(k))
        --k;
    while (qe >= stepStartQ(k + 1))
        ++k;
    return { k, stepStartQ(k), stepStartQ(k + 1) };
}

int RevolutionEngine::value(int engine, long long k) const noexcept
{
    const int v = rev::stepValue(&patternQuads[static_cast<size_t>(4 * engine)], wrap(k, steps));
    if (engine == rev::Time)
        return v < rev::kTimeActions ? v : 0;
    if (engine == rev::Glitch)
        return v < rev::kGlitchActions ? v : 0;
    return v;
}

long long RevolutionEngine::maxAge(int engine, int action, double w, double wStep, long long loop) const noexcept
{
    double age = 0.0;
    if (engine == rev::Time)
        switch (action)
        {
            case rev::Half: age = 0.5 * depth[0] * w + 5.0; break;
            case rev::TimeReverse: age = 2.0 * w + 2.0; break;
            case rev::Freeze: age = w + 2.0 * static_cast<double>(loop) + 2.0; break;
            case rev::TapeStop: age = w + 4.0; break;
            case rev::Double: age = w + 16.0; break;
            default: break;
        }
    else
        switch (action)
        {
            case rev::Repeat: case rev::Ratchet: age = w + 1.0; break;
            case rev::Stutter: age = wStep + w + 1.0; break;
            case rev::GlitchReverse: case rev::Jump: age = 2.0 * w + 2.0; break;
            case rev::Scratch: age = 0.5 * w + 4.0; break;
            default: break;
        }
    return static_cast<long long>(std::ceil(age + fadeSamples + 4.0));
}

RevolutionEngine::Segment RevolutionEngine::segmentFor(int engine, double q) const noexcept
{
    Segment s;
    s.valid = true;
    s.stamp = stamp;
    const auto st = stepAt(q);
    s.startN = sampleOf(st.start);
    s.endN = sampleOf(st.end);
    const int a = value(engine, st.k);
    if (a == 0)
        return s;

    long long kStart = st.k, kEnd = st.k + 1;
    while (wrap(kStart, steps) != 0 && value(engine, kStart - 1) == a)
        --kStart;
    while (wrap(kEnd, steps) != 0 && value(engine, kEnd) == a)
        ++kEnd;
    const auto capacity = static_cast<long long>(tapes[static_cast<size_t>(engine)].size) - 8;
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        const double b = sampleOf(stepStartQ(kStart)), end = sampleOf(stepStartQ(kEnd));
        const double wStep = sampleOf(stepStartQ(kStart + 1)) - b;
        long long loop = 0;
        if (engine == rev::Time && a == rev::Freeze)
            loop = std::max(16LL, std::llround(wStep / static_cast<double>(1 << (1 + std::lround((1.0 - depth[0]) * 3.0)))));
        else if (engine == rev::Glitch && (a == rev::Stutter || a == rev::Ratchet))
            loop = std::max(16LL, std::llround(wStep / static_cast<double>(1 << (1 + std::lround(depth[1] * 3.0)))));
        if (maxAge(engine, a, end - b, wStep, loop) <= capacity)
        {
            s.action = a;
            s.runK = kStart;
            s.b = b;
            s.w = end - b;
            s.wStep = wStep;
            s.loop = loop;
            s.startN = b;
            s.endN = end;
            return s;
        }
        kStart = st.k; // the run does not fit the tape: this step alone
        kEnd = st.k + 1;
    }
    return s; // not even one step fits (very slow tempo): the step passes
}

// ----------------------------------------------------------------------------------------- heads
float RevolutionEngine::readFrac(const Tape& t, int ch, double age) const noexcept
{
    const auto ai = static_cast<long long>(std::floor(age));
    const double f = age - static_cast<double>(ai);
    if (f == 0.0)
        return t.at(ch, ai);
    // Catmull-Rom through the newer neighbour, the two samples around the point and the older one.
    const double pm1 = t.at(ch, ai - 1), p0 = t.at(ch, ai), p1 = t.at(ch, ai + 1), p2 = t.at(ch, ai + 2);
    return static_cast<float>(p0 + 0.5 * f * (p1 - pm1 + f * (2.0 * pm1 - 5.0 * p0 + 4.0 * p1 - p2 + f * (3.0 * (p0 - p1) + p2 - pm1))));
}

float RevolutionEngine::readHalfband(const Tape& t, int ch, long long age) const noexcept
{
    float sum = 0.0f;
    for (int k = -7; k <= 7; ++k)
        sum += halfband[static_cast<size_t>(k + 7)] * t.at(ch, age + k);
    return sum;
}

void RevolutionEngine::head(int engine, const Segment& s, const float* in, float* out, int numChannels) const noexcept
{
    const auto dry = [&] { for (int ch = 0; ch < numChannels; ++ch) out[ch] = in[ch]; };
    if (!s.valid || s.action == 0)
    {
        dry();
        return;
    }
    const auto& t = tapes[static_cast<size_t>(engine)];
    const double u = std::max(0.0, static_cast<double>(n) - s.b);
    const long long uI = std::max(0LL, n - static_cast<long long>(std::ceil(s.b - kEps)));
    const auto integer = [&](long long age, float gain)
    {
        if (age < 0 || !t.holds(static_cast<double>(age)))
            return dry();
        for (int ch = 0; ch < numChannels; ++ch)
            out[ch] = gain * t.at(ch, age);
    };
    const auto fractional = [&](double age, float gain)
    {
        if (age < 1.0 || !t.holds(age + 2.0))
            return dry();
        for (int ch = 0; ch < numChannels; ++ch)
            out[ch] = gain * readFrac(t, ch, age);
    };
    // Repeats of a short loop (STUTTER, RATCHET): a click-free envelope at each loop edge.
    const auto loopEnvelope = [&](long long phase, long long loop, bool firstPass)
    {
        const long long edge = std::clamp(std::llround(0.5 * fadeSamples), 1LL, std::max(1LL, loop / 8));
        double g = 1.0;
        if (!firstPass && phase < edge)
            g *= raisedCos(static_cast<double>(phase + 1) / static_cast<double>(edge + 1));
        if (loop - 1 - phase < edge)
            g *= raisedCos(static_cast<double>(loop - phase) / static_cast<double>(edge + 1));
        return static_cast<float>(g);
    };

    if (engine == rev::Time)
    {
        switch (s.action)
        {
            case rev::Half: // the run's first part, slower (tape: the pitch goes down with it)
                return fractional(0.5 * depth[0] * u + 2.0, 1.0f);
            case rev::TimeReverse: // the window before the run, backwards
                return integer(2 * uI + 1, 1.0f);
            case rev::Freeze: // a short loop of what came just before the run, seams crossfaded
            {
                const long long loop = s.loop, phase = uI % loop, age = uI + loop - phase;
                const long long seam = std::clamp(std::llround(fadeSamples), 1LL, loop / 4);
                if (phase < loop - seam)
                    return integer(age, 1.0f);
                if (!t.holds(static_cast<double>(age + loop)))
                    return dry();
                const float w = static_cast<float>(raisedCos(static_cast<double>(phase - (loop - seam) + 1) / static_cast<double>(seam + 1)));
                for (int ch = 0; ch < numChannels; ++ch)
                    out[ch] = t.at(ch, age) + w * (t.at(ch, age + loop) - t.at(ch, age));
                return;
            }
            case rev::TapeStop: // speed and level fall to zero over the run
            {
                const double p = 1.0 + 2.0 * depth[0], x = u / std::max(1.0, s.w);
                const double moved = x < 1.0 ? s.w / (p + 1.0) * (1.0 - std::pow(1.0 - x, p + 1.0)) : s.w / (p + 1.0);
                const auto gain = static_cast<float>(x < 1.0 ? 1.0 - x : 0.0);
                return fractional(u - moved + 2.0, gain);
            }
            case rev::Double: // the window before the run and the run itself at twice the speed
            {
                const long long age = std::max(8LL, std::llround(s.w) - uI + 8);
                if (!t.holds(static_cast<double>(age + 7)))
                    return dry();
                for (int ch = 0; ch < numChannels; ++ch)
                    out[ch] = readHalfband(t, ch, age);
                return;
            }
            default: return dry();
        }
    }

    switch (s.action)
    {
        case rev::Repeat: // the run plays the window before it again
            return integer(std::llround(s.w), 1.0f);
        case rev::Stutter: // the start of the step before the run, looped
        {
            const long long loop = s.loop, phase = uI % loop;
            const long long age = std::llround(s.wStep) + uI - phase;
            const float g = loopEnvelope(phase, loop, false);
            integer(age, g);
            return;
        }
        case rev::Ratchet: // the run's own start, retriggered
        {
            const long long loop = s.loop, phase = uI % loop;
            integer(uI - phase, loopEnvelope(phase, loop, uI < loop));
            return;
        }
        case rev::GlitchReverse:
            return integer(2 * uI + 1, 1.0f);
        case rev::Gap:
            for (int ch = 0; ch < numChannels; ++ch)
                out[ch] = 0.0f;
            return;
        case rev::Jump: // the window two runs back
            return integer(2 * std::llround(s.w), 1.0f);
        case rev::Scratch: // back and forth around the run (signed speed 1 - pi/2 .. 1 + pi/2 at full depth)
        {
            const double a = s.w * (0.06 + 0.19 * depth[1]);
            return fractional(a * (1.0 - std::cos(2.0 * kPiD * u / std::max(1.0, s.w))) + 2.0, 1.0f);
        }
        default: return dry();
    }
}

// ---------------------------------------------------------------------------------------- stages
void RevolutionEngine::tapeStage(int engine, float* x, int numChannels, double q) noexcept
{
    auto& tape = tapes[static_cast<size_t>(engine)];
    auto& st = stages[static_cast<size_t>(engine)];
    tape.push(x, numChannels);

    const double target = on[static_cast<size_t>(engine)] ? mixTarget[static_cast<size_t>(engine)] : 0.0;
    const double ramp = 1.0 / (0.01 * sampleRate);
    st.mix = st.mix < target ? std::min(target, st.mix + ramp) : std::max(target, st.mix - ramp);

    if (!st.cur.valid || static_cast<double>(n) + kEps >= st.cur.endN || static_cast<double>(n) + kEps < st.cur.startN || st.cur.stamp != stamp)
    {
        const auto next = segmentFor(engine, q);
        if (!next.same(st.cur))
        {
            st.prev = st.cur;
            st.fadeLength = static_cast<int>(std::max(1.0, next.action != 0 ? std::min(fadeSamples, 0.5 * next.wStep) : fadeSamples));
            st.fadePos = 0;
        }
        st.cur = next;
    }
    if (st.mix <= 0.0 && target <= 0.0)
    {
        if (st.fadePos < st.fadeLength)
            ++st.fadePos;
        return;
    }

    float wet[2], prev[2];
    head(engine, st.cur, x, wet, numChannels);
    if (st.fadePos < st.fadeLength)
    {
        head(engine, st.prev, x, prev, numChannels);
        const auto w = static_cast<float>(raisedCos(static_cast<double>(st.fadePos + 1) / static_cast<double>(st.fadeLength + 1)));
        for (int ch = 0; ch < numChannels; ++ch)
            wet[ch] = prev[ch] + w * (wet[ch] - prev[ch]);
        ++st.fadePos;
    }
    if (st.mix >= 1.0)
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] = wet[ch];
    else
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] += static_cast<float>(st.mix) * (wet[ch] - x[ch]);
}

double RevolutionEngine::motionLevel(double q) noexcept
{
    if (!motionStepValid || motionStamp != stamp || q + kEps * dq >= motionStep.end || q + kEps * dq < motionStep.start)
    {
        motionStep = stepAt(q);
        motionStepValid = true;
        motionStamp = stamp;
    }
    const auto level = [this](long long k) { return depth[2] * value(rev::Motion, k) / 15.0; };
    const double here = level(motionStep.k);
    const double x = (q - motionStep.start) / std::max(1.0e-12, motionStep.end - motionStep.start);
    if (smooth > 0.0 && x > 1.0 - smooth)
        return here + (level(motionStep.k + 1) - here) * (x - (1.0 - smooth)) / smooth;
    const double u = (q - motionStep.start) / dq;
    if (smooth <= 0.0 && u < fadeSamples)
    {
        const double before = level(motionStep.k - 1);
        return before + (here - before) * raisedCos((u + 1.0) / (fadeSamples + 1.0));
    }
    return here;
}

void RevolutionEngine::motionStage(float* x, int numChannels, double q) noexcept
{
    const double target = on[2] ? mixTarget[2] : 0.0;
    const double ramp = 1.0 / (0.01 * sampleRate);
    motionMix = motionMix < target ? std::min(target, motionMix + ramp) : std::max(target, motionMix - ramp);
    const double m = motionLevel(q);
    float wet[2] { x[0], numChannels > 1 ? x[1] : 0.0f };

    switch (motionTarget)
    {
        case rev::Volume:
            for (int ch = 0; ch < numChannels; ++ch)
                wet[ch] = x[ch] * static_cast<float>(1.0 - m);
            break;
        case rev::LowPass:
        case rev::HighPass:
        {
            // TPT state-variable filter (stable under modulation); its frequency follows the level on a
            // 16-sample grid of the engine clock; the level fades the filter in from exactly dry.
            if (svfG <= 0.0 || n % 16 == 0)
            {
                const double f = motionTarget == rev::LowPass ? 20000.0 * std::pow(150.0 / 20000.0, m) : 20.0 * std::pow(8000.0 / 20.0, m);
                svfG = std::tan(kPiD * std::min(f, 0.45 * sampleRate) / sampleRate);
            }
            constexpr double k = 1.4142135623730951;
            const double a1 = 1.0 / (1.0 + svfG * (svfG + k)), a2 = svfG * a1, a3 = svfG * a2;
            const auto blend = static_cast<float>(std::min(1.0, m / 0.1));
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& s1 = ic1[static_cast<size_t>(ch)];
                auto& s2 = ic2[static_cast<size_t>(ch)];
                const double v3 = x[ch] - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
                s1 = 2.0 * v1 - s1;
                s2 = 2.0 * v2 - s2;
                const auto y = static_cast<float>(motionTarget == rev::LowPass ? v2 : x[ch] - k * v1 - v2);
                wet[ch] = x[ch] + blend * (y - x[ch]);
            }
            break;
        }
        case rev::Width:
            if (numChannels > 1 && m > 0.0)
            {
                const float mid = 0.5f * (x[0] + x[1]), side = 0.5f * (x[0] - x[1]) * static_cast<float>(1.0 - m);
                wet[0] = mid + side;
                wet[1] = mid - side;
            }
            break;
        case rev::Crush:
            if (m > 0.0)
            {
                const long long hold = 1 + static_cast<long long>(15.0 * m * m);
                const auto scale = static_cast<float>(std::pow(2.0, 15.0 - 12.0 * m));
                const auto blend = static_cast<float>(std::min(1.0, m / 0.1));
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    if (n % hold == 0)
                        held[static_cast<size_t>(ch)] = x[ch];
                    const float y = std::round(held[static_cast<size_t>(ch)] * scale) / scale;
                    wet[ch] = x[ch] + blend * (y - x[ch]);
                }
            }
            else
                for (int ch = 0; ch < numChannels; ++ch)
                    held[static_cast<size_t>(ch)] = x[ch];
            break;
        default: break;
    }
    if (motionMix >= 1.0)
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] = wet[ch];
    else if (motionMix > 0.0)
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] += static_cast<float>(motionMix) * (wet[ch] - x[ch]);
}

// --------------------------------------------------------------------------------------- process
void RevolutionEngine::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p, const Transport& transport,
                               int blockOffset) noexcept
{
    if (numSamples <= 0 || numChannels <= 0 || tapes[0].size == 0)
        return;
    numChannels = std::min(numChannels, 2);
    readParams(p);
    syncClock(p, transport, blockOffset);
    const double ramp = 1.0 / (0.01 * sampleRate);
    double q = q0;
    for (int i = 0; i < numSamples; ++i, ++n)
    {
        q = q0 + static_cast<double>(n - n0) * dq;
        // A new order: fade to dry, switch with fresh histories, fade back in (10 ms each way).
        if (wantedOrder != activeOrder)
        {
            orderGain = std::max(0.0, orderGain - ramp);
            if (orderGain <= 0.0)
            {
                activeOrder = wantedOrder;
                order = rev::orderOf(activeOrder);
                newEpoch();
            }
        }
        else if (orderGain < 1.0)
            orderGain = std::min(1.0, orderGain + ramp);

        const float in[2] { channels[0][i], numChannels > 1 ? channels[1][i] : 0.0f };
        float x[2] { in[0], in[1] };
        for (int e : order)
        {
            if (e == rev::Motion)
                motionStage(x, numChannels, q);
            else
                tapeStage(e, x, numChannels, q);
        }
        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][i] = orderGain >= 1.0 ? x[ch] : in[ch] + static_cast<float>(orderGain) * (x[ch] - in[ch]);
    }
    shownStep.store(wrap(stepAt(q).k, steps), std::memory_order_relaxed);
    const double stepSamples = stepQ / dq;
    warming.store(static_cast<double>(std::min(tapes[0].written, tapes[1].written)) < std::min(2.0 * stepSamples, static_cast<double>(tapes[1].size - 1)),
                  std::memory_order_relaxed);
}
} // namespace hpeq::dsp
