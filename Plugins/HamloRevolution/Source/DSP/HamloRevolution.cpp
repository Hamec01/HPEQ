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
// R3: VINYL and DOWNSAMPLE reach their full blend at a quarter of the level, smoothly (a linear ramp to 1/10 had a
// kink a 2 ms step fade turned into a splice)
double smoothBlend(double level) noexcept
{
    const double t = std::clamp(level / 0.25, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}
double princarg(double phase) noexcept { return phase - 2.0 * kPiD * std::floor((phase + kPiD) / (2.0 * kPiD)); }

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

void RevolutionEngine::Tape::copy(int ch, long long age, int count, float* dst) const noexcept
{
    long long index = head - age;
    if (index < 0)
        index += size;
    const auto first = static_cast<int>(std::min<long long>(count, size - index));
    std::copy(data[ch] + index, data[ch] + index + first, dst);
    if (first < count)
        std::copy(data[ch], data[ch] + (count - first), dst + first);
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
RevolutionEngine::RevolutionEngine() = default;
RevolutionEngine::~RevolutionEngine() = default;

void RevolutionEngine::getWave(std::array<float, kWaveBins>& in, std::array<float, kWaveBins>& out) const noexcept
{
    for (int b = 0; b < kWaveBins; ++b)
    {
        in[static_cast<size_t>(b)] = waveIn[static_cast<size_t>(b)].load(std::memory_order_relaxed);
        out[static_cast<size_t>(b)] = waveOut[static_cast<size_t>(b)].load(std::memory_order_relaxed);
    }
}

void RevolutionEngine::getFeel(std::array<float, kFeelSteps>& out) const noexcept
{
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = feel[i].load(std::memory_order_relaxed);
}

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

    // Stretch frames: RHYTHM about 21 ms (WSOLA, hop 1/2), CLEAN twice that (phase vocoder, hop 1/4).
    frameW = 1 << static_cast<int>(std::lround(std::log2(0.0213 * sampleRate)));
    frameP = 2 * frameW;
    fft.prepare(frameP);
    const auto hann = [](std::vector<float>& w, int size)
    {
        w.resize(static_cast<size_t>(size));
        for (int i = 0; i < size; ++i)
            w[static_cast<size_t>(i)] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * kPiD * i / size)); // periodic
    };
    hann(windowW, frameW);
    hann(windowP, frameP);
    const auto bins = static_cast<size_t>(frameP / 2 + 1);
    specIn.assign(static_cast<size_t>(frameP), {});
    specOut.assign(static_cast<size_t>(frameP), {});
    for (auto* v : { &specL, &specR, &specRef, &peakRot })
        v->assign(bins, {});
    peakMag.assign(bins, 0.0f);
    for (auto* v : { &spanL, &spanR })
        v->assign(static_cast<size_t>(frameP + frameW + 16), 0.0f);
    for (auto* v : { &naturalL, &naturalR })
        v->assign(static_cast<size_t>(frameW), 0.0f);
    nearestPeak.assign(bins, -1);
    for (auto& v : voices)
    {
        for (auto& o : v.ola)
            o.assign(static_cast<size_t>(frameP), 0.0f);
        v.prevRef.assign(bins, {});
        v.rot.assign(bins, { 1.0f, 0.0f });
    }
    // R3: the throw lanes' tapes (2.5 s: the longest DELAY clamps there) and reverb networks
    static constexpr double fdnMs[8] { 31.1, 37.3, 41.9, 47.3, 53.9, 61.1, 67.7, 73.1 }, diffuserMs[4] { 4.7, 3.6, 2.9, 1.9 };
    for (auto& th : throws)
    {
        for (auto& l : th.line)
            l.assign(static_cast<size_t>(std::ceil(2.5 * sampleRate)) + 8, 0.0f);
        for (int i = 0; i < 8; ++i)
            th.fdn[static_cast<size_t>(i)].assign(static_cast<size_t>(std::max(8L, std::lround(fdnMs[i] * 1.0e-3 * sampleRate))), 0.0f);
        for (int i = 0; i < 4; ++i)
            th.diffuser[static_cast<size_t>(i)].assign(static_cast<size_t>(std::max(4L, std::lround(diffuserMs[i] * 1.0e-3 * sampleRate))), 0.0f);
        th.asleep = true;
    }
    feelFrame = std::max(16, static_cast<int>(std::lround(64.0 * sampleRate / 48000.0)));
    for (auto& f : feel)
        f.store(0.0f, std::memory_order_relaxed);
    onsetCoef = 1.0 - std::exp(-1.0 / (0.02 * sampleRate));
    onsetDeadSamples = static_cast<int>(std::lround(0.03 * sampleRate));
    dynFrame = std::max(16, static_cast<int>(std::lround(0.01 * sampleRate)));
    dynFill = dynCount = dynPos = 0;
    dynEnergy = 0.0;
    dynamics.store(0.0f, std::memory_order_relaxed);
    for (int i = 0; i < 16; ++i)
    {
        timing[static_cast<size_t>(i)].store(0.0f, std::memory_order_relaxed);
        presence[static_cast<size_t>(i)].store(0.0f, std::memory_order_relaxed);
    }
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
    for (auto& th : throws)
    {
        th.asleep = true;
        th.mix = 0.0;
        th.stepValid = false;
    }
    waveBin = -1;
    waveInPeak = waveOutPeak = 0.0f;
    for (int b = 0; b < kWaveBins; ++b)
    {
        waveIn[static_cast<size_t>(b)].store(0.0f, std::memory_order_relaxed);
        waveOut[static_cast<size_t>(b)].store(0.0f, std::memory_order_relaxed);
    }
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
    for (auto& v : voices)
        v.active = false;
    voiceCur = voicePrev = voiceNext = -1;
    upcomingFor = -1.0;
    feelFill = 0;
    feelEnergy = 0.0;
    timingWindow = kNoWindow; // a jump: the window under way is dropped, not counted as empty
    onsetDead = 0;
    feelHistoryCount = 0;
    feelLastStep = -1;
    stepPeak = {};
    epochs.fetch_add(1, std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------------ parameters
void RevolutionEngine::readParams(const FxSlotParams& p) noexcept
{
    const auto& v = p.values;
    // What changes a head's formula (not mixes, on, order or the MOTION target): a new stamp makes the
    // next segment a different one, so the change is crossfaded instead of jumping.
    static constexpr int formula[] { slot::kRevGrid, slot::kRevTriplet, slot::kRevSwing, slot::kRevSteps, slot::kRevFree, slot::kRevFreeMs,
                                     slot::kRevHumanize, slot::kRevHumanSeed, slot::kTimeDepth, slot::kGlitchDepth, slot::kRevFadeMs,
                                     slot::kTimeStretch };
    bool changed = false;
    for (int i : formula)
        changed = changed || v[static_cast<size_t>(i)] != lastValues[static_cast<size_t>(i)];
    for (int i = slot::kLaneBase; i < slot::kLaneBase + 3 * slot::kLaneStride; ++i)
        changed = changed || v[static_cast<size_t>(i)] != lastValues[static_cast<size_t>(i)];
    for (int i = slot::kBandBase; i < slot::kBandBase + 2; ++i)
        changed = changed || v[static_cast<size_t>(i)] != lastValues[static_cast<size_t>(i)];
    changed = changed || v[slot::kGlitchEngine] != lastValues[slot::kGlitchEngine]; // GLITCH 2: another head formula
    for (int i = slot::kGrooveBase; i <= slot::kGrooveAmount; ++i) // R4: the groove moves step boundaries
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
    stretchMode = std::clamp(static_cast<int>(std::lround(v[slot::kTimeStretch])), 0, 2);
    smartJoin = v[slot::kRevSmartJoin] >= 0.5f;
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
    blockGlitch = v[slot::kGlitchEngine] >= 0.5f;
    {
        const double amount = std::clamp(static_cast<double>(v[slot::kGrooveAmount]), 0.0, 1.0);
        grooved = false;
        for (int i = 0; i < slot::kGrooveSteps; ++i)
        {
            const double offset = amount * std::clamp(static_cast<double>(v[static_cast<size_t>(slot::kGrooveBase + i)]), -0.1125, 0.1125);
            groove[static_cast<size_t>(i)] = offset;
            grooved = grooved || offset != 0.0;
        }
        grooved = grooved && !freeMode;
    }
    for (int e = 0; e < rev::kLanes; ++e)
    {
        const auto b = static_cast<size_t>(slot::laneSettings(e));
        const auto le = static_cast<size_t>(e);
        laneLength[le] = std::clamp(static_cast<int>(std::lround(v[b + slot::kLaneLength])), 0, 2 * rev::kSteps);
        const int rate = std::clamp(static_cast<int>(std::lround(v[b + slot::kLaneRate])), 0, 8);
        laneQ[le] = rate == 0 || freeMode ? stepQ : kBeats[(rate - 1) % 4] * (rate > 4 ? 2.0 / 3.0 : 1.0);
        laneCondition[le] = std::clamp(static_cast<int>(std::lround(v[b + slot::kLaneCondition])), 0, rev::kLaneConditions - 1);
        bool chance = false;
        for (int q = 0; q < 4; ++q)
            extraQuads[4 * le + static_cast<size_t>(q)] = v[b + slot::kLaneSteps + static_cast<size_t>(q)];
        for (int q = 0; q < 8; ++q)
        {
            probabilityQuads[8 * le + static_cast<size_t>(q)] = v[b + slot::kLaneProbability + static_cast<size_t>(q)];
            chance = chance || v[b + slot::kLaneProbability + static_cast<size_t>(q)] != 0.0f;
        }
        laneChance[le] = chance;
        // R2: the lane's band (a change starts its split from silence: the filters' state is cleared)
        const int newBand = std::clamp(static_cast<int>(std::lround(v[static_cast<size_t>(slot::laneBand(e))])), 0, rev::kBands - 1);
        if (newBand != band[le])
            splitters[le].clear();
        band[le] = newBand;
    }
    crossoverLow = std::clamp(static_cast<double>(v[slot::kCrossoverLow]), 40.0, 1000.0);
    crossoverHigh = std::clamp(static_cast<double>(v[slot::kCrossoverHigh]), 1000.0, 12000.0);
    if (v[slot::kCrossoverLow] == 0.0f)
        crossoverLow = 200.0; // a slot without R2 values
    if (v[slot::kCrossoverHigh] == 0.0f)
        crossoverHigh = 2500.0;
    crossoverHigh = std::max(crossoverHigh, 2.0 * crossoverLow);
    for (int i = 0; i < rev::kShapes; ++i)
    {
        auto& sh = shapes[static_cast<size_t>(i)];
        const auto b = static_cast<size_t>(slot::kShapeBase + i * slot::kShapeStride);
        sh.on = v[b + slot::kShapeOn] >= 0.5f;
        sh.mixTarget = std::clamp(static_cast<double>(v[b + slot::kShapeMix]), 0.0, 1.0);
        sh.depth = std::clamp(static_cast<double>(v[b + slot::kShapeDepth]), 0.0, 1.0);
        sh.target = std::clamp(static_cast<int>(std::lround(v[b + slot::kShapeTarget])), 0, rev::kShapeTargets - 1);
        sh.smooth = std::clamp(static_cast<double>(v[b + slot::kShapeSmooth]), 0.0, 1.0);
        sh.res = std::clamp(static_cast<double>(v[b + slot::kShapeRes]), 0.0, 1.0);
        sh.curve = v[b + slot::kShapeCurve] >= 0.5f;
        bool empty = true;
        for (int q = 0; q < 4; ++q)
        {
            sh.quads[static_cast<size_t>(q)] = v[b + slot::kShapePattern + static_cast<size_t>(q)];
            empty = empty && sh.quads[static_cast<size_t>(q)] == 0.0f;
        }
        for (int q = 0; q < 4; ++q)
            empty = empty && extraQuads[static_cast<size_t>(4 * (3 + i) + q)] == 0.0f;
        sh.empty = empty;
    }
    for (int i = 0; i < rev::kThrows; ++i)
    {
        auto& th = throws[static_cast<size_t>(i)];
        const auto b = static_cast<size_t>(slot::kThrowBase + i * slot::kThrowStride);
        th.on = v[b + slot::kThrowOn] >= 0.5f;
        th.mixTarget = std::clamp(static_cast<double>(v[b + slot::kThrowMix]), 0.0, 1.0);
        th.depth = std::clamp(static_cast<double>(v[b + slot::kThrowDepth]), 0.0, 1.0);
        th.target = std::clamp(static_cast<int>(std::lround(v[b + slot::kThrowTarget])), 0, rev::kThrowTargets - 1);
        th.tail = std::clamp(static_cast<double>(v[b + slot::kThrowTail]), 0.0, 1.0);
        th.tone = std::clamp(static_cast<double>(v[b + slot::kThrowTone]), 0.0, 1.0);
        th.time = std::clamp(static_cast<int>(std::lround(v[b + slot::kThrowTime])), 0, rev::kThrowTimes - 1);
        th.note = std::clamp(static_cast<int>(std::lround(v[b + slot::kThrowNote])), 0, rev::kNotes - 1);
        bool empty = true;
        for (int q = 0; q < 4; ++q)
        {
            th.quads[static_cast<size_t>(q)] = v[b + slot::kThrowPattern + static_cast<size_t>(q)];
            empty = empty && th.quads[static_cast<size_t>(q)] == 0.0f;
        }
        for (int q = 0; q < 4; ++q)
            empty = empty && extraQuads[static_cast<size_t>(4 * (rev::kFirstThrow + i) + q)] == 0.0f;
        th.empty = empty;
    }
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
double RevolutionEngine::stepStartQ(long long k, double sq) const noexcept
{
    const double pair = 2.0 * sq;
    double start = static_cast<double>(floorDiv(k, 2)) * pair + ((k & 1) != 0 ? pair * swing : 0.0);
    if (humanize > 0.0)
        start += humanize * 0.12 * sq * (2.0 * unitHash(humanSeed, k) - 1.0);
    if (grooved)
    {
        // R4: a boundary on a 1/16 of the bar (its straight place) moves by the groove of that 1/16
        const double straight = static_cast<double>(floorDiv(k, 2)) * pair + ((k & 1) != 0 ? sq : 0.0);
        const double sixteenth = straight / 0.25, nearest = std::round(sixteenth);
        if (std::abs(sixteenth - nearest) < 1.0e-9)
            start += groove[static_cast<size_t>(wrap(static_cast<long long>(nearest), 16))];
    }
    return start;
}

RevolutionEngine::Step RevolutionEngine::stepAt(double q, double sq) const noexcept
{
    const double pair = 2.0 * sq, qe = q + kEps * dq;
    const auto p = static_cast<long long>(std::floor(qe / pair));
    long long k = 2 * p + (qe - static_cast<double>(p) * pair >= pair * swing ? 1 : 0);
    while (qe < stepStartQ(k, sq))
        --k;
    while (qe >= stepStartQ(k + 1, sq))
        ++k;
    return { k, stepStartQ(k, sq), stepStartQ(k + 1, sq) };
}

int RevolutionEngine::value(int engine, long long k) const noexcept
{
    const auto le = static_cast<size_t>(engine);
    const float* first = engine < rev::kFirstShape   ? &patternQuads[4 * le]
                         : engine < rev::kFirstThrow ? shapes[le - rev::kFirstShape].quads.data()
                                                     : throws[le - rev::kFirstThrow].quads.data();
    // R1: the lane's condition (per cycle of the lane, counted from bar 1) and the step's probability (Core, R4)
    const int v = rev::laneEventValue(first, &extraQuads[4 * le], &probabilityQuads[8 * le], laneChance[le], laneSteps(engine), laneCondition[le],
                                      humanSeed, engine, k);
    if (engine == rev::Time)
        return v < rev::kTimeActions ? v : 0;
    if (engine == rev::Glitch)
        return v < (blockGlitch ? static_cast<int>(rev::kBlockActions) : static_cast<int>(rev::kGlitchActions)) ? v : 0;
    return v;
}

long long RevolutionEngine::maxAge(int engine, int action, double w, double wStep, long long loop) const noexcept
{
    double age = 0.0;
    if (engine == rev::Time && stretchMode > 0 && (action == rev::Half || action == rev::Double))
    {
        // a stretch voice reads whole grains 1.5 frames (+ the WSOLA search) behind its source position
        const double frame = stretchMode == 1 ? frameW : frameP;
        age = (action == rev::Half ? 0.5 * depth[0] * w : w) + 2.75 * frame + 16.0;
    }
    else if (engine == rev::Time)
        switch (action)
        {
            case rev::Half: age = 0.5 * depth[0] * w + 5.0; break;
            case rev::TimeReverse: age = 2.0 * w + 2.0; break;
            case rev::Freeze: age = w + 2.0 * static_cast<double>(loop) + 2.0; break;
            case rev::TapeStop: age = w + 4.0; break;
            case rev::Double: age = w + 16.0; break;
            default: break;
        }
    else if (blockGlitch)
    {
        const double quarter = 1.0 / dq;
        switch (action)
        {
            case rev::BlockRepeat: age = w + wStep + 2.0; break;
            case rev::BlockRoll: case rev::BlockChop: case rev::BlockRise: age = w + 2.0; break;
            case rev::BlockDrop: age = w + 0.5 * quarter + 4.0; break;
            case rev::BlockShuffle: age = 2.0 * quarter + 4.0; break;
            case rev::BlockReverse: age = 2.0 * w + 2.0; break;
            case rev::BlockStop: age = w + 4.0; break;
            default: break;
        }
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
    const double sq = laneQ[static_cast<size_t>(engine)];
    const int length = laneSteps(engine);
    const auto st = stepAt(q, sq);
    s.startN = sampleOf(st.start);
    s.endN = sampleOf(st.end);
    const int a = value(engine, st.k);
    if (a == 0)
        return s;

    long long kStart = st.k, kEnd = st.k + 1;
    while (wrap(kStart, length) != 0 && value(engine, kStart - 1) == a)
        --kStart;
    while (wrap(kEnd, length) != 0 && value(engine, kEnd) == a)
        ++kEnd;
    const auto capacity = static_cast<long long>(tapes[static_cast<size_t>(engine)].size) - 8;
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        const double b = sampleOf(stepStartQ(kStart, sq)), end = sampleOf(stepStartQ(kEnd, sq));
        const double wStep = sampleOf(stepStartQ(kStart + 1, sq)) - b;
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

bool RevolutionEngine::headAt(int engine, const Segment& s, long long nn, long long ahead, const float* in, float* out, int numChannels) const noexcept
{
    // `ahead` > 0 (C-ATD looking at the next samples): the tape still ends at n, so every age is read
    // `ahead` samples younger, and the stage input itself is not known yet.
    const auto dry = [&]
    {
        if (ahead > 0)
            return false;
        for (int ch = 0; ch < numChannels; ++ch)
            out[ch] = in[ch];
        return true;
    };
    if (!s.valid || s.action == 0)
        return dry();
    if (engine == rev::Glitch && blockGlitch)
        return blockHead(s, nn, ahead, in, out, numChannels);
    const auto& t = tapes[static_cast<size_t>(engine)];
    const double u = std::max(0.0, static_cast<double>(nn) - s.b);
    const long long uI = std::max(0LL, nn - static_cast<long long>(std::ceil(s.b - kEps)));
    const auto integer = [&](long long age, float gain)
    {
        age -= ahead;
        if (age < 0 || !t.holds(static_cast<double>(age)))
            return dry();
        for (int ch = 0; ch < numChannels; ++ch)
            out[ch] = gain * t.at(ch, age);
        return true;
    };
    const auto fractional = [&](double age, float gain)
    {
        age -= static_cast<double>(ahead);
        if (age < 1.0 || !t.holds(age + 2.0))
            return dry();
        for (int ch = 0; ch < numChannels; ++ch)
            out[ch] = gain * readFrac(t, ch, age);
        return true;
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
                const long long loop = s.loop, phase = uI % loop, age = uI + loop - phase - ahead;
                const long long seam = std::clamp(std::llround(fadeSamples), 1LL, loop / 4);
                if (phase < loop - seam)
                    return integer(age + ahead, 1.0f);
                if (age < 0 || !t.holds(static_cast<double>(age + loop)))
                    return dry();
                const float w = static_cast<float>(raisedCos(static_cast<double>(phase - (loop - seam) + 1) / static_cast<double>(seam + 1)));
                for (int ch = 0; ch < numChannels; ++ch)
                    out[ch] = t.at(ch, age) + w * (t.at(ch, age + loop) - t.at(ch, age));
                return true;
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
                const long long age = std::max(8LL, std::llround(s.w) - uI + 8) - ahead;
                if (age < 7 || !t.holds(static_cast<double>(age + 7)))
                    return dry();
                for (int ch = 0; ch < numChannels; ++ch)
                    out[ch] = readHalfband(t, ch, age);
                return true;
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
            return integer(std::llround(s.wStep) + uI - phase, loopEnvelope(phase, loop, false));
        }
        case rev::Ratchet: // the run's own start, retriggered
        {
            const long long loop = s.loop, phase = uI % loop;
            return integer(uI - phase, loopEnvelope(phase, loop, uI < loop));
        }
        case rev::GlitchReverse:
            return integer(2 * uI + 1, 1.0f);
        case rev::Gap:
            for (int ch = 0; ch < numChannels; ++ch)
                out[ch] = 0.0f;
            return true;
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

// ------------------------------------------------------------------------------------ GLITCH 2: the block engine
// docs/GLITCH_RESEARCH.md: an action is a grab (where the audio comes from) + repeats (note-valued) + a shape per
// repeat (decay, gate, pitch). What plays is always the stage's own input, cut and re-ordered (insert); no repeat is
// shorter than 30 ms except inside RISE's accelerating roll, so nothing turns into a steady buzz.
double RevolutionEngine::rollPeriod() const noexcept
{
    const double quarter = 1.0 / dq, floor30 = 0.03 * sampleRate;
    const double preferred = quarter / (triplet ? 12.0 : 8.0); // 1/48 on a triplet grid, else 1/32
    if (preferred >= floor30)
        return preferred;
    return quarter / 8.0 >= floor30 ? quarter / 8.0 : quarter / 4.0;
}

bool RevolutionEngine::blockHead(const Segment& s, long long nn, long long ahead, const float* in, float* out, int numChannels) const noexcept
{
    const auto dry = [&]
    {
        if (ahead > 0)
            return false;
        for (int ch = 0; ch < numChannels; ++ch)
            out[ch] = in[ch];
        return true;
    };
    const auto& t = tapes[1];
    const double u = std::max(0.0, static_cast<double>(nn) - s.b);
    const double quarter = 1.0 / dq;
    // the source sample `age` before nn, times gain; age 0 = the live input (that is the stage input itself)
    const auto read = [&](double age, double gain)
    {
        age -= static_cast<double>(ahead);
        if (age < 0.5)
        {
            if (ahead > 0)
                return false;
            for (int ch = 0; ch < numChannels; ++ch)
                out[ch] = static_cast<float>(gain) * in[ch];
            return true;
        }
        if (!t.holds(age + 2.0))
            return dry();
        const double whole = std::floor(age);
        for (int ch = 0; ch < numChannels; ++ch)
            out[ch] = static_cast<float>(gain * (age - whole < 1.0e-9 ? t.at(ch, static_cast<long long>(whole)) : readFrac(t, ch, age)));
        return true;
    };
    // a grain's envelope: a 1.5 ms raised-cosine attack (not for the live first grain), open for `gate` of the grain,
    // a release of up to 4 ms that reaches 0 at the gate's end
    const auto grain = [&](double v, double length, double gate, bool attack)
    {
        const double a = 0.0015 * sampleRate, end = std::max(1.0, gate * length);
        const double release = std::min(0.004 * sampleRate, 0.25 * end);
        double g = 1.0;
        if (attack && v < a)
            g *= raisedCos((v + 1.0) / (a + 1.0));
        if (v >= end)
            return 0.0;
        if (v > end - release)
            g *= raisedCos((end - v) / (release + 1.0));
        return g;
    };
    const double strength = depth[1]; // DEPTH: how far each repeat decays (and how deep DROP / RISE go)
    // the audio before the run that the action reaches back to; when the tape did not hold that much at the run's
    // start (just after play or a jump), the whole run plays dry - a switch inside it would click
    {
        double need = 0.0;
        switch (s.action)
        {
            case rev::BlockRepeat: need = std::max(1.0, s.wStep) + 2.0; break;
            case rev::BlockDrop: need = 0.5 * quarter + 2.0; break;
            case rev::BlockShuffle: need = 2.0 * quarter + 2.0; break;
            case rev::BlockReverse: need = s.w + 2.0; break;
            default: break;
        }
        if (need > 0.0 && static_cast<double>(std::min<long long>(t.written, t.size)) - u < need)
            return dry();
    }
    switch (s.action)
    {
        case rev::BlockRepeat: // the step before the run, played again, each repeat a little quieter
        {
            const double g = std::max(1.0, s.wStep), v = std::fmod(u, g), r = std::floor(u / g);
            const double gain = std::pow(1.0 - 0.2 * strength, r) * grain(v, g, 1.0, true);
            return read(u + g - v, gain);
        }
        case rev::BlockRoll: // the run's own start retriggered at 1/32, -1 .. -3 dB per repeat, gated to 85 %
        case rev::BlockChop: // ... at 1/16, gated to 45 %: the dry pieces with silence between
        {
            const bool roll = s.action == rev::BlockRoll;
            const double period = roll ? rollPeriod() : quarter / 4.0;
            const double v = std::fmod(u, period), r = std::floor(u / period);
            const double decay = roll ? 1.0 - 0.3 * strength : 1.0 - 0.1 * strength;
            const double gain = std::pow(decay, r) * grain(v, period, roll ? 0.85 : 0.45, r > 0.0);
            return read(u - v, gain); // r = 0: the live input
        }
        case rev::BlockRise: // an accelerating roll from 1/16 down to 1/64 (>= 5 ms), the pitch rising to +7 st
        {
            const double first = quarter / 4.0, shortest = std::max(0.005 * sampleRate, quarter / 16.0);
            double start = 0.0, length = first;
            int r = 0;
            while (u >= start + length && r < 400)
            {
                start += length;
                length = std::max(shortest, length * 0.82);
                ++r;
            }
            const double v = u - start;
            const double x = std::clamp(u / std::max(1.0, s.w), 0.0, 1.0);
            const double speed = r == 0 ? 1.0 : std::pow(2.0, strength * 7.0 * x / 12.0);
            const double from = std::min(v * speed, std::max(0.0, u - 1.0)); // never ahead of what was heard
            return read(u - from, grain(v, length, 0.85, r > 0));
        }
        case rev::BlockDrop: // the 1/8 before the run at half speed (an octave down), repeats decaying
        {
            const double g = 0.5 * quarter, period = 2.0 * g, v = std::fmod(u, period), r = std::floor(u / period);
            const double gain = std::pow(1.0 - 0.25 * strength, r) * grain(v, period, 1.0, true);
            return read(u + g - 0.5 * v, gain);
        }
        case rev::BlockShuffle: // another piece of the bar plays here: 3/16, 1/4, 3/8 or 1/2 earlier (BBCut offsets)
        {
            static constexpr double offsets[] { 0.75, 1.0, 1.5, 2.0 };
            const auto pick = static_cast<size_t>(std::min(3.0, std::floor(4.0 * unitHash(0x5A0FF1E5u ^ humanSeed, s.runK))));
            return read(offsets[pick] * quarter, 1.0);
        }
        case rev::BlockReverse: // the window before the run, backwards
            return read(2.0 * u + 1.0, 1.0);
        case rev::BlockStop: // a tape stop over the run: speed and level fall to zero
        {
            const double p = 2.0, x = u / std::max(1.0, s.w);
            const double moved = x < 1.0 ? s.w / (p + 1.0) * (1.0 - std::pow(1.0 - x, p + 1.0)) : s.w / (p + 1.0);
            return read(u - moved + 2.0, x < 1.0 ? 1.0 - x : 0.0);
        }
        case rev::BlockGap:
            for (int ch = 0; ch < numChannels; ++ch)
                out[ch] = 0.0f;
            return true;
        default: return dry();
    }
}

bool RevolutionEngine::isStretch(int engine, const Segment& s) const noexcept
{
    return engine == rev::Time && s.valid && stretchMode > 0 && (s.action == rev::Half || s.action == rev::Double);
}

void RevolutionEngine::startVoice(int index, const Segment& s) noexcept
{
    // Frames sit on a grid of hops ending at the run start: started early (the next run, one frame ahead)
    // they are computed one per hop as their time comes; started late, the missing ones are done at once.
    auto& v = voices[static_cast<size_t>(index)];
    v.active = true;
    v.fallback = false;
    v.havePrev = false;
    v.segment = s;
    v.mode = stretchMode;
    v.size = stretchMode == 1 ? frameW : frameP;
    v.hop = stretchMode == 1 ? v.size / 2 : v.size / 4;
    v.base = static_cast<long long>(std::ceil(s.b - kEps));
    v.rate = s.action == rev::Half ? 1.0 - 0.5 * depth[0] : 2.0;
    v.start = s.action == rev::Half ? static_cast<double>(v.base) : static_cast<double>(v.base) - (v.rate - 1.0) * s.w;
    v.delay = 1.5 * v.size + (stretchMode == 1 ? v.hop / 2 : 0) + 8.0;
    const long long t = n - v.base;
    v.outPos = std::max(0LL, t);
    v.nextFrame = std::max(t, 0LL) - v.size + v.hop;
    for (auto& o : v.ola)
        std::fill(o.begin(), o.begin() + v.size, 0.0f);
}

void RevolutionEngine::advanceVoice(StretchVoice& v) noexcept
{
    const long long t = n - v.base;
    while (v.active && !v.fallback && v.nextFrame <= t)
    {
        voiceFrame(v, v.nextFrame);
        v.nextFrame += v.hop;
    }
}

void RevolutionEngine::voiceFrame(StretchVoice& v, long long t) noexcept
{
    const auto& tape = tapes[0];
    const int size = v.size, hop = v.hop;
    const double centre = v.start + v.rate * (static_cast<double>(t) + 0.5 * size) - v.delay;
    const auto readable = [&](long long first, long long last) { return last <= n && tape.holds(static_cast<double>(n - first)); };

    if (v.mode == 1)
    {
        // RHYTHM: WSOLA. The grain moves by up to half a hop so that its start continues the previous
        // grain's natural continuation (normalised cross-correlation of both channels over the overlap);
        // coarse lags every 4 samples on every 2nd sample, the 3 best refined +-3 at full resolution.
        const long long nominal = std::llround(centre - 0.5 * size);
        const int search = hop / 2, overlap = size - hop, margin = search + 3, span = size + 2 * margin;
        long long p = nominal;
        if (!readable(nominal - margin, nominal - margin + span - 1))
        {
            v.fallback = true;
            return;
        }
        // every candidate grain lies in one contiguous copy: candidate d starts at span index margin + d
        tape.copy(0, n - (nominal - margin), span, spanL.data());
        tape.copy(1, n - (nominal - margin), span, spanR.data());
        if (v.havePrev && readable(v.prevGrain + hop, v.prevGrain + hop + overlap - 1))
        {
            tape.copy(0, n - (v.prevGrain + hop), overlap, naturalL.data());
            tape.copy(1, n - (v.prevGrain + hop), overlap, naturalR.data());
            float naturalEnergy = 0.0f;
            for (int i = 0; i < overlap; ++i)
                naturalEnergy += naturalL[static_cast<size_t>(i)] * naturalL[static_cast<size_t>(i)] + naturalR[static_cast<size_t>(i)] * naturalR[static_cast<size_t>(i)];
            if (naturalEnergy > 1.0e-9f * overlap)
            {
                const float* aL = spanL.data();
                const float* aR = spanR.data();
                const float* bL = naturalL.data();
                const float* bR = naturalR.data();
                const auto score = [&](long long candidate, int stride)
                {
                    const auto offset = static_cast<int>(candidate - nominal) + margin;
                    float num = 0.0f, ea = 0.0f, eb = 0.0f;
                    for (int i = 0; i < overlap; i += stride)
                    {
                        const float l = aL[offset + i], r = aR[offset + i], ml = bL[i], mr = bR[i];
                        num += l * ml + r * mr;
                        ea += l * l + r * r;
                        eb += ml * ml + mr * mr;
                    }
                    return static_cast<double>(num) / std::sqrt((static_cast<double>(ea) + 1.0e-12) * (static_cast<double>(eb) + 1.0e-12));
                };
                // shortlist: the 3 best coarse lags more than 4 apart (ties: the smaller shift)
                std::array<std::pair<double, int>, 3> best { { { -2.0, 0 }, { -2.0, 0 }, { -2.0, 0 } } };
                for (int d = -search; d <= search; d += 4)
                {
                    const double sc = score(nominal + d, 2);
                    bool merged = false;
                    for (auto& b : best)
                        if (b.first > -2.0 && std::abs(b.second - d) <= 4)
                        {
                            if (sc > b.first)
                                b = { sc, d };
                            merged = true;
                            break;
                        }
                    if (merged)
                        continue;
                    for (size_t i = 0; i < best.size(); ++i)
                        if (sc > best[i].first || (sc == best[i].first && std::abs(d) < std::abs(best[i].second)))
                        {
                            for (size_t j = best.size() - 1; j > i; --j)
                                best[j] = best[j - 1];
                            best[i] = { sc, d };
                            break;
                        }
                }
                double top = score(nominal, 1); // the expected position is always a candidate
                int shift = 0;
                for (const auto& b : best)
                    if (b.first > -2.0)
                        for (int r = -3; r <= 3; ++r)
                        {
                            const int d = std::clamp(b.second + r, -search, search);
                            const double sc = score(nominal + d, 1);
                            if (sc > top + 1.0e-12 || (sc >= top - 1.0e-12 && std::abs(d) < std::abs(shift)))
                            {
                                top = sc;
                                shift = d;
                            }
                        }
                p = nominal + shift;
            }
        }
        v.prevGrain = p;
        v.havePrev = true;
        const auto offset = static_cast<size_t>(p - nominal + margin);
        for (int i = 0; i < size; ++i)
        {
            const long long pos = t + i;
            if (pos < v.outPos)
                continue;
            const float w = windowW[static_cast<size_t>(i)];
            const auto k = static_cast<size_t>(pos % size);
            v.ola[0][k] += w * spanL[offset + static_cast<size_t>(i)];
            v.ola[1][k] += w * spanR[offset + static_cast<size_t>(i)];
        }
        return;
    }

    // CLEAN: phase vocoder, both channels in one complex FFT (L + iR). Each peak's phase advances by its
    // measured frequency over the real analysis hop; the bins around a peak turn with it (identity phase
    // locking), so a region needs one rotation; both channels get the same rotation, so their phase
    // difference stays. Trigonometry only at the peaks; one complex FFT each way.
    const long long c = std::llround(centre);
    const long long first = c - size / 2;
    if (!readable(first, first + size - 1))
    {
        v.fallback = true;
        return;
    }
    using C = std::complex<float>;
    const int bins = size / 2 + 1;
    tape.copy(0, n - first, size, spanL.data());
    tape.copy(1, n - first, size, spanR.data());
    for (int i = 0; i < size; ++i)
    {
        const auto j = static_cast<size_t>(i);
        const float w = windowP[j];
        specIn[j] = C(w * spanL[j], w * spanR[j]);
    }
    fft.perform(specIn.data(), specOut.data(), false);
    for (int k = 0; k < bins; ++k)
    {
        const auto i = static_cast<size_t>(k);
        const C zk = specOut[i], zn = std::conj(specOut[static_cast<size_t>((size - k) % size)]);
        const C l = 0.5f * (zk + zn), d = zk - zn, r(0.5f * d.imag(), -0.5f * d.real());
        specL[i] = l;
        specR[i] = r;
        const C sum = l + r;
        const float el = std::norm(l), er = std::norm(r);
        specRef[i] = std::norm(sum) >= 0.125f * (el + er) ? sum : (el >= er ? l : r);
        peakMag[i] = el + er;
    }
    // peaks, and for every bin the nearest one
    int lastPeak = -1, peaks = 0;
    std::fill(nearestPeak.begin(), nearestPeak.begin() + bins, -1);
    for (int k = 2; k < bins - 2; ++k)
    {
        const auto m = peakMag[static_cast<size_t>(k)];
        if (m > 1.0e-18f && m > peakMag[static_cast<size_t>(k - 1)] && m >= peakMag[static_cast<size_t>(k + 1)] && m > peakMag[static_cast<size_t>(k - 2)]
            && m >= peakMag[static_cast<size_t>(k + 2)])
        {
            const int from = lastPeak < 0 ? 0 : (lastPeak + k) / 2 + 1;
            if (lastPeak >= 0)
                for (int j = lastPeak + 1; j < from; ++j)
                    nearestPeak[static_cast<size_t>(j)] = lastPeak;
            for (int j = from; j <= k; ++j)
                nearestPeak[static_cast<size_t>(j)] = k;
            lastPeak = k;
            ++peaks;
        }
    }
    if (lastPeak >= 0)
        for (int j = lastPeak + 1; j < bins; ++j)
            nearestPeak[static_cast<size_t>(j)] = lastPeak;

    // rotation per bin: e^{i (synthesis phase - analysis phase)}
    const double ha = static_cast<double>(c - v.prevCentre);
    if (!v.havePrev)
        std::fill(v.rot.begin(), v.rot.begin() + bins, C(1.0f, 0.0f));
    else if (peaks > 0)
    {
        for (int k = 2; k < bins - 2; ++k)
        {
            const auto i = static_cast<size_t>(k);
            if (nearestPeak[i] != k)
                continue;
            const double omega = 2.0 * kPiD * k / size;
            const C turn = specRef[i] * std::conj(v.prevRef[i]);
            const double moved = std::norm(turn) > 1.0e-30f ? std::atan2(turn.imag(), turn.real()) : omega * ha; // analysis phase advance
            const double inst = ha != 0.0 ? omega + princarg(moved - omega * ha) / ha : omega;
            const double a = hop * inst - moved;
            C rot = v.rot[i] * C(static_cast<float>(std::cos(a)), static_cast<float>(std::sin(a)));
            rot /= std::max(1.0e-12f, std::abs(rot));
            peakRot[i] = rot;
        }
        for (int k = 0; k < bins; ++k)
        {
            const auto i = static_cast<size_t>(k);
            v.rot[i] = peakRot[static_cast<size_t>(nearestPeak[i])];
        }
    }
    v.rot[0] = v.rot[static_cast<size_t>(bins - 1)] = C(1.0f, 0.0f); // DC and Nyquist stay real
    std::copy(specRef.begin(), specRef.begin() + bins, v.prevRef.begin());
    v.prevCentre = c;
    v.havePrev = true;

    // back: W = Y_L + i Y_R over the whole spectrum (each Y Hermitian), one inverse FFT
    for (int k = 0; k < size; ++k)
    {
        const int b = k <= size / 2 ? k : size - k;
        const auto i = static_cast<size_t>(b);
        C yl = specL[i] * v.rot[i], yr = specR[i] * v.rot[i];
        if (k > size / 2)
        {
            yl = std::conj(yl);
            yr = std::conj(yr);
        }
        specIn[static_cast<size_t>(k)] = C(yl.real() - yr.imag(), yl.imag() + yr.real());
    }
    fft.perform(specIn.data(), specOut.data(), true);
    constexpr float norm = 1.0f / 1.5f; // periodic Hann analysis x synthesis at a quarter-frame hop sums to 1.5
    for (int i = 0; i < size; ++i)
    {
        const long long pos = t + i;
        if (pos < v.outPos)
            continue;
        const float w = norm * windowP[static_cast<size_t>(i)];
        const auto k = static_cast<size_t>(pos % size);
        v.ola[0][k] += w * specOut[static_cast<size_t>(i)].real();
        v.ola[1][k] += w * specOut[static_cast<size_t>(i)].imag();
    }
}

void RevolutionEngine::voiceOut(StretchVoice& v, const float* in, float* out, int numChannels) noexcept
{
    const long long t = n - v.base;
    advanceVoice(v);
    if (!v.active || v.fallback)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            out[ch] = in[ch];
        return;
    }
    for (; v.outPos < t; ++v.outPos) // samples this voice was not asked for (engine mix at 0)
    {
        const auto k = static_cast<size_t>(v.outPos % v.size);
        v.ola[0][k] = v.ola[1][k] = 0.0f;
    }
    const auto k = static_cast<size_t>(t % v.size);
    for (int ch = 0; ch < numChannels; ++ch)
        out[ch] = v.ola[static_cast<size_t>(ch)][k];
    v.ola[0][k] = v.ola[1][k] = 0.0f;
    v.outPos = t + 1;
}

void RevolutionEngine::estimateJoin(int engine, Stage& st, bool prevStretch, bool curStretch, int numChannels) noexcept
{
    // C-ATD: the energies of the outgoing and incoming heads over the fade and their correlation. A head
    // that cannot be evaluated ahead (the live input, a stretch voice) is estimated by the stage's input
    // over the last fade length (already written: nothing from the future is used).
    const auto& tape = tapes[static_cast<size_t>(engine)];
    double ea = 0.0, eb = 0.0, c = 0.0;
    for (int i = 0; i < st.fadeLength; ++i)
    {
        const long long age = st.fadeLength - 1 - i;
        float proxy[2] {}, a[2] {}, b[2] {};
        if (tape.holds(static_cast<double>(age)))
            for (int ch = 0; ch < numChannels; ++ch)
                proxy[ch] = tape.at(ch, age);
        if (prevStretch || !headAt(engine, st.prev, n + i, i, proxy, a, numChannels))
            std::copy(proxy, proxy + numChannels, a);
        if (curStretch || !headAt(engine, st.cur, n + i, i, proxy, b, numChannels))
            std::copy(proxy, proxy + numChannels, b);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            ea += static_cast<double>(a[ch]) * a[ch];
            eb += static_cast<double>(b[ch]) * b[ch];
            c += static_cast<double>(a[ch]) * b[ch];
        }
    }
    st.joinA = ea;
    st.joinB = eb;
    st.joinC = c;
    st.join = ea + eb > 1.0e-9 * std::max(1, st.fadeLength);
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
    const bool timeStage = engine == rev::Time;

    if (!st.cur.valid || static_cast<double>(n) + kEps >= st.cur.endN || static_cast<double>(n) + kEps < st.cur.startN || st.cur.stamp != stamp)
    {
        const auto next = segmentFor(engine, q);
        if (!next.same(st.cur))
        {
            st.prev = st.cur;
            st.fadeLength = static_cast<int>(std::max(1.0, next.action != 0 ? std::min(fadeSamples, 0.5 * next.wStep) : fadeSamples));
            st.fadePos = 0;
            const bool prevStretch = timeStage && voiceCur >= 0;
            if (timeStage)
            {
                if (voicePrev >= 0)
                    voices[static_cast<size_t>(voicePrev)].active = false; // a fade cut short: its voice stops
                voicePrev = voiceCur;
                voiceCur = -1;
                if (voiceNext >= 0 && voices[static_cast<size_t>(voiceNext)].active && voices[static_cast<size_t>(voiceNext)].segment.same(next)
                    && voices[static_cast<size_t>(voiceNext)].segment.b == next.b)
                    voiceCur = voiceNext; // started a frame early: no burst at the run start
                else
                {
                    if (voiceNext >= 0)
                        voices[static_cast<size_t>(voiceNext)].active = false;
                    if (isStretch(engine, next))
                    {
                        voiceCur = 0;
                        while (voiceCur == voicePrev)
                            ++voiceCur;
                        startVoice(voiceCur, next);
                    }
                }
                voiceNext = -1;
            }
            st.cur = next;
            // GLITCH 2's grains carry their own envelopes; C-ATD's level correction (capped at +3 dB) put a kink in a
            // fade whenever one head was nearly silent (a gate, a grain's attack)
            if (smartJoin && !(engine == rev::Glitch && blockGlitch))
                estimateJoin(engine, st, prevStretch, timeStage && voiceCur >= 0, numChannels);
            else
                st.join = false;
        }
        else
            st.cur = next;
    }

    if (timeStage && stretchMode > 0)
        lookAhead(st);

    const auto endFade = [&]
    {
        if (timeStage && voicePrev >= 0)
        {
            voices[static_cast<size_t>(voicePrev)].active = false;
            voicePrev = -1;
        }
    };
    if (st.mix <= 0.0 && target <= 0.0)
    {
        if (st.fadePos < st.fadeLength && ++st.fadePos == st.fadeLength)
            endFade();
        return;
    }

    float wet[2], prev[2];
    if (timeStage && voiceCur >= 0)
        voiceOut(voices[static_cast<size_t>(voiceCur)], x, wet, numChannels);
    else
        headAt(engine, st.cur, n, 0, x, wet, numChannels);
    if (st.fadePos < st.fadeLength)
    {
        if (timeStage && voicePrev >= 0)
            voiceOut(voices[static_cast<size_t>(voicePrev)], x, prev, numChannels);
        else
            headAt(engine, st.prev, n, 0, x, prev, numChannels);
        const double w = raisedCos(static_cast<double>(st.fadePos + 1) / static_cast<double>(st.fadeLength + 1));
        float g = 1.0f;
        if (st.join)
        {
            // keep the mix's expected energy on the line between the two heads' energies (C-ATD, +-3 dB cap)
            const double mixE = (1.0 - w) * (1.0 - w) * st.joinA + w * w * st.joinB + 2.0 * w * (1.0 - w) * st.joinC;
            const double wanted = (1.0 - w) * st.joinA + w * st.joinB;
            g = static_cast<float>(mixE > 1.0e-12 ? std::clamp(std::sqrt(wanted / mixE), 0.70794578, 1.41253754) : 1.0);
        }
        for (int ch = 0; ch < numChannels; ++ch)
            wet[ch] = g * (prev[ch] + static_cast<float>(w) * (wet[ch] - prev[ch]));
        if (++st.fadePos == st.fadeLength)
            endFade();
    }
    if (st.mix >= 1.0)
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] = wet[ch];
    else
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] += static_cast<float>(st.mix) * (wet[ch] - x[ch]);
}

void RevolutionEngine::lookAhead(const Stage& st) noexcept
{
    // The run after this segment: if it is a stretch run, its voice starts one frame before it.
    if (st.cur.valid && upcomingFor != st.cur.endN)
    {
        upcomingFor = st.cur.endN;
        const double start = std::ceil(st.cur.endN - kEps);
        upcoming = segmentFor(rev::Time, q0 + (start - static_cast<double>(n0)) * dq);
        if (voiceNext >= 0)
        {
            voices[static_cast<size_t>(voiceNext)].active = false;
            voiceNext = -1;
        }
    }
    if (!isStretch(rev::Time, upcoming) || upcoming.same(st.cur))
        return;
    const int lead = stretchMode == 1 ? frameW : frameP;
    if (voiceNext < 0 && static_cast<double>(n) >= upcoming.b - lead && static_cast<double>(n) < upcoming.b)
    {
        voiceNext = 0;
        while (voiceNext == voiceCur || voiceNext == voicePrev)
            ++voiceNext;
        startVoice(voiceNext, upcoming);
    }
    if (voiceNext >= 0)
        advanceVoice(voices[static_cast<size_t>(voiceNext)]);
}

void RevolutionEngine::analyse(const float* in, int numChannels, int step) noexcept
{
    // Energy novelty per ~1.3 ms frame (the frame's level over the mean of the four before, in dB); each
    // pattern step keeps its strongest; every finished pattern cycle is averaged into `feel` (18 dB = 1).
    double e = 0.0;
    for (int ch = 0; ch < numChannels; ++ch)
        e += static_cast<double>(in[ch]) * in[ch];
    feelEnergy += e / numChannels;
    if (++feelFill < feelFrame)
        return;
    const double db = 10.0 * std::log10(feelEnergy / feelFrame + 1.0e-12);
    feelEnergy = 0.0;
    feelFill = 0;
    double novelty = 0.0;
    if (feelHistoryCount >= static_cast<int>(feelHistory.size()))
    {
        double mean = 0.0;
        for (double h : feelHistory)
            mean += h;
        novelty = std::max(0.0, db - mean / static_cast<double>(feelHistory.size()));
    }
    feelHistory[static_cast<size_t>(feelHistoryCount % static_cast<int>(feelHistory.size()))] = db;
    ++feelHistoryCount;
    if (step < feelLastStep)
        for (size_t s = 0; s < stepPeak.size(); ++s)
        {
            const auto v = static_cast<float>(std::clamp(stepPeak[s] / 18.0, 0.0, 1.0));
            feel[s].store(0.5f * feel[s].load(std::memory_order_relaxed) + 0.5f * v, std::memory_order_relaxed);
            stepPeak[s] = 0.0;
        }
    feelLastStep = step;
    auto& peak = stepPeak[static_cast<size_t>(std::clamp(step, 0, kFeelSteps - 1))];
    peak = std::max(peak, novelty);
}

void RevolutionEngine::getTiming(std::array<float, 16>& offsets, std::array<float, 16>& shares) const noexcept
{
    for (size_t i = 0; i < 16; ++i)
    {
        offsets[i] = timing[i].load(std::memory_order_relaxed);
        shares[i] = presence[i].load(std::memory_order_relaxed);
    }
}

void RevolutionEngine::listenTiming(const float* in, int numChannels, double q) noexcept
{
    {
        // the dynamics measure (any mode)
        double e = 0.0;
        for (int ch = 0; ch < numChannels; ++ch)
            e += static_cast<double>(in[ch]) * in[ch];
        dynEnergy += e / numChannels;
        if (++dynFill >= dynFrame)
        {
            dynFrames[static_cast<size_t>(dynPos)] = static_cast<float>(10.0 * std::log10(dynEnergy / dynFrame + 1.0e-12));
            dynPos = (dynPos + 1) % static_cast<int>(dynFrames.size());
            dynCount = std::min(dynCount + 1, static_cast<int>(dynFrames.size()));
            dynFill = 0;
            dynEnergy = 0.0;
            if (dynCount == static_cast<int>(dynFrames.size()))
            {
                auto sorted = dynFrames;
                std::nth_element(sorted.begin(), sorted.begin() + 50, sorted.end());
                const float median = sorted[50], loudest = *std::max_element(dynFrames.begin(), dynFrames.end());
                dynamics.store(median < -90.0f ? 0.0f : loudest - median, std::memory_order_relaxed);
            }
        }
    }
    if (freeMode)
        return;
    double level = 0.0;
    for (int ch = 0; ch < numChannels; ++ch)
        level = std::max(level, static_cast<double>(std::abs(in[ch])));
    const bool onset = onsetDead <= 0 && level > 0.0031622776601683794 && level > 3.0 * onsetSlow;
    onsetSlow += (level - onsetSlow) * onsetCoef;
    if (onsetDead > 0)
        --onsetDead;
    const auto window = static_cast<long long>(std::llround(q / 0.25));
    if (window != timingWindow)
    {
        if (timingWindow != kNoWindow)
        {
            const auto i = static_cast<size_t>(wrap(timingWindow, 16));
            const float was = presence[i].load(std::memory_order_relaxed);
            if (windowHit)
            {
                const float t = timing[i].load(std::memory_order_relaxed);
                timing[i].store(was < 0.01f ? static_cast<float>(windowOffset) : 0.5f * t + 0.5f * static_cast<float>(windowOffset),
                                std::memory_order_relaxed);
            }
            presence[i].store(0.5f * was + (windowHit ? 0.5f : 0.0f), std::memory_order_relaxed);
        }
        timingWindow = window;
        windowHit = false;
    }
    if (onset)
    {
        onsetDead = onsetDeadSamples;
        if (!windowHit)
        {
            windowHit = true;
            windowOffset = q - static_cast<double>(window) * 0.25;
        }
    }
}

double RevolutionEngine::motionLevel(double q) noexcept
{
    if (!motionStepValid || motionStamp != stamp || q + kEps * dq >= motionStep.end || q + kEps * dq < motionStep.start)
    {
        motionStep = stepAt(q, laneQ[rev::Motion]);
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

// ------------------------------------------------------------------------------------ R2: bands, SHAPE
void RevolutionEngine::updateCrossovers() noexcept
{
    if (appliedLow == crossoverLow && appliedHigh == crossoverHigh)
        return;
    appliedLow = crossoverLow;
    appliedHigh = crossoverHigh;
    // Butterworth sections (bilinear, prewarped); two in series = Linkwitz-Riley 4th order
    const auto design = [this](Section& s, double f, bool high)
    {
        const double w = std::tan(kPiD * std::min(f, 0.45 * sampleRate) / sampleRate), w2 = w * w, r = std::sqrt(2.0) * w;
        const double a0 = 1.0 + r + w2;
        s.b0 = (high ? 1.0 : w2) / a0;
        s.b1 = (high ? -2.0 : 2.0 * w2) / a0;
        s.b2 = s.b0;
        s.a1 = 2.0 * (w2 - 1.0) / a0;
        s.a2 = (1.0 - r + w2) / a0;
    };
    for (auto& sp : splitters)
    {
        for (int i = 0; i < 2; ++i)
        {
            design(sp.s[static_cast<size_t>(0 + i)], crossoverLow, false);
            design(sp.s[static_cast<size_t>(2 + i)], crossoverLow, true);
            design(sp.s[static_cast<size_t>(4 + i)], crossoverHigh, false);
            design(sp.s[static_cast<size_t>(6 + i)], crossoverHigh, true);
            design(sp.s[static_cast<size_t>(8 + i)], crossoverHigh, false);
            design(sp.s[static_cast<size_t>(10 + i)], crossoverHigh, true);
        }
    }
}

void RevolutionEngine::runLane(int lane, float* x, int numChannels, double q) noexcept
{
    const auto run = [&](float* y)
    {
        if (lane == rev::Motion)
            motionStage(y, numChannels, q);
        else if (lane < rev::kEngines)
            tapeStage(lane, y, numChannels, q);
        else if (lane < rev::kFirstThrow)
            shapeStage(lane - rev::kEngines, y, numChannels, q);
        else
            throwStage(lane - rev::kFirstThrow, y, numChannels, q);
    };
    const int b = band[static_cast<size_t>(lane)];
    if (b == rev::Full)
    {
        if (lane >= rev::kEngines && lane < rev::kFirstThrow && shapes[static_cast<size_t>(lane - rev::kEngines)].empty)
            return; // an empty SHAPE lane: exactly dry (an empty THROW lane decides itself: its tail may still ring)
        run(x);
        return;
    }
    updateCrossovers();
    auto& sp = splitters[static_cast<size_t>(lane)];
    float bands[3][2] {};
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const double in = x[ch];
        double low = sp.s[1].run(ch, sp.s[0].run(ch, in));
        const double rest = sp.s[3].run(ch, sp.s[2].run(ch, in));
        const double mid = sp.s[5].run(ch, sp.s[4].run(ch, rest));
        const double high = sp.s[7].run(ch, sp.s[6].run(ch, rest));
        low = sp.s[9].run(ch, sp.s[8].run(ch, low)) + sp.s[11].run(ch, sp.s[10].run(ch, low));
        // LR4's high pass is in phase with its low pass: low + mid + high = an all-pass of the input
        bands[0][ch] = static_cast<float>(low);
        bands[1][ch] = static_cast<float>(mid);
        bands[2][ch] = static_cast<float>(high);
    }
    if (b == rev::Tops)
    {
        // R4: MID + HIGH as one signal, LOW added back
        float tops[2] {};
        for (int ch = 0; ch < numChannels; ++ch)
            tops[ch] = bands[1][ch] + bands[2][ch];
        run(tops);
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] = bands[0][ch] + tops[ch];
        return;
    }
    run(bands[b - 1]);
    for (int ch = 0; ch < numChannels; ++ch)
        x[ch] = bands[0][ch] + bands[1][ch] + bands[2][ch];
}

double RevolutionEngine::shapeLevel(int shape, double q) noexcept
{
    auto& sh = shapes[static_cast<size_t>(shape)];
    const int lane = rev::kEngines + shape;
    if (!sh.stepValid || sh.stamp != stamp || q + kEps * dq >= sh.step.end || q + kEps * dq < sh.step.start)
    {
        sh.step = stepAt(q, laneQ[static_cast<size_t>(lane)]);
        sh.stepValid = true;
        sh.stamp = stamp;
    }
    const auto level = [this, lane, &sh](long long k) { return sh.depth * value(lane, k) / 15.0; };
    const double here = level(sh.step.k);
    const double x = (q - sh.step.start) / std::max(1.0e-12, sh.step.end - sh.step.start);
    if (sh.curve)
    {
        // Catmull-Rom through the step levels (at the step starts), kept inside 0 .. depth
        const double p0 = level(sh.step.k - 1), p1 = here, p2 = level(sh.step.k + 1), p3 = level(sh.step.k + 2);
        const double t = std::clamp(x, 0.0, 1.0), t2 = t * t, t3 = t2 * t;
        const double c = 0.5 * ((2.0 * p1) + (-p0 + p2) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
        return std::clamp(c, 0.0, sh.depth);
    }
    if (sh.smooth > 0.0 && x > 1.0 - sh.smooth)
        return here + (level(sh.step.k + 1) - here) * (x - (1.0 - sh.smooth)) / sh.smooth;
    const double u = (q - sh.step.start) / dq;
    if (sh.smooth <= 0.0 && u < fadeSamples)
    {
        const double before = level(sh.step.k - 1);
        return before + (here - before) * raisedCos((u + 1.0) / (fadeSamples + 1.0));
    }
    return here;
}

void RevolutionEngine::shapeStage(int shape, float* x, int numChannels, double q) noexcept
{
    auto& sh = shapes[static_cast<size_t>(shape)];
    const double target = sh.on ? sh.mixTarget : 0.0;
    const double ramp = 1.0 / (0.01 * sampleRate);
    sh.mix = sh.mix < target ? std::min(target, sh.mix + ramp) : std::max(target, sh.mix - ramp);
    if (sh.mix <= 0.0 && target <= 0.0)
        return;
    const double m = shapeLevel(shape, q);
    float wet[2] { x[0], numChannels > 1 ? x[1] : 0.0f };
    const auto blend = static_cast<float>(std::min(1.0, m / 0.1)); // filters and drive fade in from exactly dry
    switch (sh.target)
    {
        case rev::ShapeVolume:
            for (int ch = 0; ch < numChannels; ++ch)
                wet[ch] = x[ch] * static_cast<float>(1.0 - m);
            break;
        case rev::ShapeLowPass:
        case rev::ShapeHighPass:
        case rev::ShapeBandPass:
        {
            if (sh.svfG <= 0.0 || n % 16 == 0)
            {
                const double f = sh.target == rev::ShapeLowPass    ? 20000.0 * std::pow(150.0 / 20000.0, m)
                                 : sh.target == rev::ShapeHighPass ? 20.0 * std::pow(8000.0 / 20.0, m)
                                                                   : 200.0 * std::pow(8000.0 / 200.0, m);
                sh.svfG = std::tan(kPiD * std::min(f, 0.45 * sampleRate) / sampleRate);
            }
            const double k = 1.4142135623730951 * (1.0 - 0.9 * sh.res); // RES: Q 0.7 .. 7
            const double a1 = 1.0 / (1.0 + sh.svfG * (sh.svfG + k)), a2 = sh.svfG * a1, a3 = sh.svfG * a2;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& s1 = sh.ic1[static_cast<size_t>(ch)];
                auto& s2 = sh.ic2[static_cast<size_t>(ch)];
                const double v3 = x[ch] - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
                s1 = 2.0 * v1 - s1;
                s2 = 2.0 * v2 - s2;
                const double y = sh.target == rev::ShapeLowPass ? v2 : sh.target == rev::ShapeHighPass ? x[ch] - k * v1 - v2 : k * v1;
                wet[ch] = x[ch] + blend * (static_cast<float>(y) - x[ch]);
            }
            break;
        }
        case rev::ShapePan:
            if (numChannels > 1 && m > 0.0)
            {
                // alternating: even steps of the lane to the left, odd ones to the right
                const double pan = (sh.step.k % 2 == 0 ? -1.0 : 1.0) * m;
                wet[0] = x[0] * static_cast<float>(pan > 0.0 ? 1.0 - pan : 1.0);
                wet[1] = x[1] * static_cast<float>(pan < 0.0 ? 1.0 + pan : 1.0);
            }
            break;
        case rev::ShapeWidth:
            if (numChannels > 1 && m > 0.0)
            {
                const float mid = 0.5f * (x[0] + x[1]), side = 0.5f * (x[0] - x[1]) * static_cast<float>(1.0 - m);
                wet[0] = mid + side;
                wet[1] = mid - side;
            }
            break;
        case rev::ShapeDrive:
            if (m > 0.0)
            {
                // tanh with the gain that keeps a -10 dBFS peak where it was
                const double g = 1.0 + 15.0 * m, ref = 0.31622776601683794;
                const double norm = ref / std::tanh(g * ref);
                for (int ch = 0; ch < numChannels; ++ch)
                    wet[ch] = x[ch] + blend * (static_cast<float>(norm * std::tanh(g * x[ch])) - x[ch]);
            }
            break;
        case rev::ShapeCrush:
            if (m > 0.0)
            {
                const long long hold = 1 + static_cast<long long>(15.0 * m * m);
                const auto scale = static_cast<float>(std::pow(2.0, 15.0 - 12.0 * m));
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    if (n % hold == 0)
                        sh.held[static_cast<size_t>(ch)] = x[ch];
                    const float y = std::round(sh.held[static_cast<size_t>(ch)] * scale) / scale;
                    wet[ch] = x[ch] + blend * (y - x[ch]);
                }
            }
            else
                for (int ch = 0; ch < numChannels; ++ch)
                    sh.held[static_cast<size_t>(ch)] = x[ch];
            break;
        default: break;
    }
    if (sh.mix >= 1.0)
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] = wet[ch];
    else
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] += static_cast<float>(sh.mix) * (wet[ch] - x[ch]);
}

// ---------------------------------------------------------------------------------------------- R3: THROW
void RevolutionEngine::clearThrow(ThrowLane& th) noexcept
{
    for (auto& l : th.line)
        std::fill(l.begin(), l.end(), 0.0f);
    for (auto& l : th.fdn)
        std::fill(l.begin(), l.end(), 0.0f);
    for (auto& l : th.diffuser)
        std::fill(l.begin(), l.end(), 0.0f);
    th.write = 0;
    th.fdnPos = {};
    th.diffuserPos = {};
    th.fdnLow = {};
    th.delay = -1.0;
    th.damp = {};
    th.feedback = {};
    th.hp = {};
    th.lp1 = {};
    th.lp2 = {};
    th.allpass = {};
    th.coefValid = false;
    th.brakeAge = th.fromAge = 0.0;
    th.wowDepth = 0.0;
    th.eqLevel = -1.0;
    th.fade = 0;
    th.brakeValid = false;
    th.phase = 0.0;
    th.held = {};
    th.quiet = 0;
}

float RevolutionEngine::lineAt(const ThrowLane& th, int ch, double age) const noexcept
{
    const auto& l = th.line[static_cast<size_t>(ch)];
    const int size = static_cast<int>(l.size());
    const double pos = static_cast<double>(th.write - 1) - std::clamp(age, 0.0, static_cast<double>(size - 4));
    const double base = std::floor(pos);
    const auto frac = static_cast<float>(pos - base);
    const int i0 = wrap(static_cast<long long>(base), size), i1 = i0 + 1 == size ? 0 : i0 + 1;
    return frac <= 0.0f ? l[static_cast<size_t>(i0)] : l[static_cast<size_t>(i0)] + frac * (l[static_cast<size_t>(i1)] - l[static_cast<size_t>(i0)]);
}

double RevolutionEngine::throwLevel(int lane, double q) noexcept
{
    auto& th = throws[static_cast<size_t>(lane)];
    const int l = rev::kFirstThrow + lane;
    if (!th.stepValid || th.stamp != stamp || q + kEps * dq >= th.step.end || q + kEps * dq < th.step.start)
    {
        th.step = stepAt(q, laneQ[static_cast<size_t>(l)]);
        th.stepValid = true;
        th.stamp = stamp;
    }
    const auto level = [this, l, &th](long long k) { return th.depth * value(l, k) / 15.0; };
    const double here = level(th.step.k), u = (q - th.step.start) / dq;
    if (u < fadeSamples)
    {
        const double before = level(th.step.k - 1);
        return before + (here - before) * raisedCos((u + 1.0) / (fadeSamples + 1.0));
    }
    return here;
}

void RevolutionEngine::throwStage(int lane, float* x, int numChannels, double q) noexcept
{
    auto& th = throws[static_cast<size_t>(lane)];
    const double target = th.on ? th.mixTarget : 0.0;
    const double ramp = 1.0 / (0.01 * sampleRate);
    th.mix = th.mix < target ? std::min(target, th.mix + ramp) : std::max(target, th.mix - ramp);
    const bool send = rev::throwIsSend(th.target);
    // a send's return can be quiet for up to its delay between two echoes
    const double idleAfter = th.target == rev::ThrowReverb ? 0.2 * sampleRate : std::max(0.0, th.delay) + 0.1 * sampleRate;
    if ((th.mix <= 0.0 && target <= 0.0) || (th.empty && (!send || static_cast<double>(th.quiet) > idleAfter)))
    {
        th.asleep = true; // cleared when it wakes up
        return;
    }
    if (th.asleep || th.applied != th.target)
    {
        clearThrow(th);
        th.asleep = false;
        th.applied = th.target;
    }
    const double m = throwLevel(lane, q);
    const bool stereo = numChannels > 1;
    // the musical clock for TIME (in FREE mode a quarter note = 4 free steps) and the LFO phase
    const double timeQ = rev::throwTimeQ(th.time) * (freeMode ? 4.0 : 1.0);
    const double lfo = q / timeQ - std::floor(q / timeQ);
    float wet[2] { 0.0f, 0.0f };
    const auto onePole = [this](double hz) { return std::exp(-2.0 * kPiD * std::min(hz, 0.45 * sampleRate) / sampleRate); };
    const bool refresh = !th.coefValid || n % 16 == 0;
    th.coefValid = true;
    switch (th.target)
    {
        case rev::ThrowDelay:
        case rev::ThrowTonal:
        {
            const bool tonal = th.target == rev::ThrowTonal;
            const double size = static_cast<double>(th.line[0].size()) - 8.0;
            const double want = std::clamp(tonal ? sampleRate / rev::noteHz(th.note) : timeQ / dq, 1.0, size);
            // the delay follows tempo and NOTE changes over about 50 ms (no zipper); exact while they hold
            th.delay = th.delay < 0.0 ? want : th.delay + (want - th.delay) * (1.0 - std::exp(-1.0 / (0.05 * sampleRate)));
            if (std::abs(th.delay - want) < 1.0e-6)
                th.delay = want;
            if (refresh)
                th.coefA = onePole(1000.0 * std::pow(18.0, th.tone)); // damping in the loop, 1 .. 18 kHz
            const double fb = (tonal ? 0.97 : 0.9) * th.tail;
            float echo[2] {};
            for (int ch = 0; ch < numChannels; ++ch)
            {
                echo[ch] = lineAt(th, ch, th.delay - 1.0); // read before this sample is written
                auto& d = th.damp[static_cast<size_t>(ch)];
                d = echo[ch] + th.coefA * (d - echo[ch]);
            }
            if (tonal || !stereo)
                for (int ch = 0; ch < numChannels; ++ch)
                    th.line[static_cast<size_t>(ch)][static_cast<size_t>(th.write)] = static_cast<float>(m * x[ch] + fb * th.damp[static_cast<size_t>(ch)]);
            else
            {
                // ping-pong: the send enters on the left, every echo crosses to the other side
                th.line[0][static_cast<size_t>(th.write)] = static_cast<float>(m * 0.5 * (x[0] + x[1]) + fb * th.damp[1]);
                th.line[1][static_cast<size_t>(th.write)] = static_cast<float>(fb * th.damp[0]);
            }
            th.write = th.write + 1 == static_cast<int>(th.line[0].size()) ? 0 : th.write + 1;
            wet[0] = echo[0];
            wet[1] = echo[1];
            break;
        }
        case rev::ThrowReverb:
        {
            if (refresh)
            {
                const double decay = 0.3 + 7.7 * th.tail * th.tail; // RT60 in seconds
                for (int i = 0; i < 8; ++i)
                    th.fdnGain[static_cast<size_t>(i)]
                        = std::pow(10.0, -3.0 * static_cast<double>(th.fdn[static_cast<size_t>(i)].size()) / (decay * sampleRate));
                th.coefA = onePole(2000.0 * std::pow(8.0, th.tone)); // damping 2 .. 16 kHz
            }
            double in = m * (stereo ? 0.5 * (x[0] + x[1]) : x[0]);
            for (int i = 0; i < 4; ++i)
            {
                auto& d = th.diffuser[static_cast<size_t>(i)];
                auto& pos = th.diffuserPos[static_cast<size_t>(i)];
                const double delayed = d[static_cast<size_t>(pos)], g = 0.6;
                const double v = in + g * delayed;
                d[static_cast<size_t>(pos)] = static_cast<float>(v);
                in = delayed - g * v;
                pos = pos + 1 == static_cast<int>(d.size()) ? 0 : pos + 1;
            }
            std::array<double, 8> out {};
            double sum = 0.0;
            for (int i = 0; i < 8; ++i)
            {
                const auto s = static_cast<size_t>(i);
                const double o = th.fdn[s][static_cast<size_t>(th.fdnPos[s])];
                th.fdnLow[s] = o + th.coefA * (th.fdnLow[s] - o);
                out[s] = o;
                sum += th.fdnGain[s] * th.fdnLow[s];
            }
            sum *= 0.25; // Householder: v - (2 / 8) sum(v)
            for (int i = 0; i < 8; ++i)
            {
                const auto s = static_cast<size_t>(i);
                const double v = th.fdnGain[s] * th.fdnLow[s] - sum + (i % 2 == 0 ? in : -in);
                th.fdn[s][static_cast<size_t>(th.fdnPos[s])] = static_cast<float>(v);
                th.fdnPos[s] = th.fdnPos[s] + 1 == static_cast<int>(th.fdn[s].size()) ? 0 : th.fdnPos[s] + 1;
            }
            const double l = 0.35 * (out[0] - out[2] + out[4] - out[6]), r = 0.35 * (out[1] - out[3] + out[5] - out[7]);
            wet[0] = static_cast<float>(stereo ? l : 0.5 * (l + r));
            wet[1] = static_cast<float>(r);
            break;
        }
        case rev::ThrowPhaser:
        {
            const double fb = 0.85 * th.tail, centre = 200.0 * std::pow(10.0, th.tone);
            for (int ch = 0; ch < numChannels; ++ch)
            {
                if (refresh)
                {
                    const double f = centre * std::pow(4.0, std::sin(2.0 * kPiD * (lfo + 0.25 * ch)));
                    const double t = std::tan(kPiD * std::min(f, 0.45 * sampleRate) / sampleRate);
                    (ch == 0 ? th.coefA : th.coefB) = (t - 1.0) / (t + 1.0);
                }
                const double a = ch == 0 ? th.coefA : th.coefB;
                auto& st = th.allpass[static_cast<size_t>(ch)];
                double y = x[ch] + fb * th.feedback[static_cast<size_t>(ch)];
                for (auto& z : st)
                {
                    const double o = a * y + z;
                    z = y - a * o;
                    y = o;
                }
                th.feedback[static_cast<size_t>(ch)] = y;
                const double fx = 0.5 * (x[ch] + y);
                wet[ch] = static_cast<float>(x[ch] + m * (fx - x[ch]));
            }
            break;
        }
        case rev::ThrowFlanger:
        {
            const double fb = 0.9 * th.tail, ms = 1.0e-3 * sampleRate;
            for (int ch = 0; ch < numChannels; ++ch)
                th.line[static_cast<size_t>(ch)][static_cast<size_t>(th.write)] = static_cast<float>(x[ch] + fb * th.feedback[static_cast<size_t>(ch)]);
            th.write = th.write + 1 == static_cast<int>(th.line[0].size()) ? 0 : th.write + 1;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const double sweep = 0.5 + 0.5 * std::sin(2.0 * kPiD * (lfo + 0.25 * ch));
                const double d = lineAt(th, ch, ms * (0.3 + (1.0 + 4.0 * th.tone) * sweep));
                th.feedback[static_cast<size_t>(ch)] = d;
                const double fx = 0.5 * (x[ch] + d);
                wet[ch] = static_cast<float>(x[ch] + m * (fx - x[ch]));
            }
            break;
        }
        case rev::ThrowVinyl:
        {
            // the head: wow (a slow sine on the delay) + the brake's age, which grows as the speed falls to
            // 1 - TAIL x the step's level over the step; at the next step the old head fades out (fadeSamples)
            const double stepLevel = th.depth * value(rev::kFirstThrow + lane, th.step.k) / 15.0;
            const double stepSamples = (th.step.end - th.step.start) / dq;
            const double xs = std::clamp((q - th.step.start) / std::max(1.0e-12, th.step.end - th.step.start), 0.0, 1.0);
            const double brake = std::min(0.5 * th.tail * stepLevel * stepSamples * xs * xs, 2.0 * sampleRate);
            if (!th.brakeValid || th.brakeStep != th.step.k)
            {
                if (th.brakeValid && th.brakeAge > 0.5)
                {
                    th.fromAge = th.brakeAge; // the old head plays on at speed 1 and fades out
                    th.fade = static_cast<int>(fadeSamples);
                }
                th.brakeStep = th.step.k;
                th.brakeValid = true;
            }
            th.brakeAge = brake;
            for (int ch = 0; ch < numChannels; ++ch)
                th.line[static_cast<size_t>(ch)][static_cast<size_t>(th.write)] = x[ch];
            th.write = th.write + 1 == static_cast<int>(th.line[0].size()) ? 0 : th.write + 1;
            if (m != th.eqLevel)
            {
                // every sample while the level moves (a 16-sample staircase in a 2 ms fade is a row of kinks)
                th.eqLevel = m;
                th.coefA = onePole(30.0 * std::pow(5.0, m));           // record EQ: high pass 30 .. 150 Hz
                th.coefB = onePole(18000.0 * std::pow(4.0 / 18.0, m)); // low pass 18 .. 4 kHz
            }
            // the depth glides: following the 2 ms step fade would move the head by up to 2 ms in 2 ms (a speed jump)
            th.wowDepth += (m - th.wowDepth) * (1.0 - std::exp(-1.0 / (0.05 * sampleRate)));
            const double wow = th.wowDepth * 2.0e-3 * sampleRate * (0.5 + 0.5 * std::sin(2.0 * kPiD * lfo));
            const double blend = smoothBlend(m);
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const auto c = static_cast<size_t>(ch);
                double y = lineAt(th, ch, wow + brake);
                if (th.fade > 0)
                {
                    const double old = lineAt(th, ch, wow + th.fromAge);
                    y = old + (y - old) * raisedCos(1.0 - static_cast<double>(th.fade) / (fadeSamples + 1.0));
                }
                th.hp[c] = y + th.coefA * (th.hp[c] - y); // the low part, taken away
                y -= th.hp[c];
                th.lp1[c] = y + th.coefB * (th.lp1[c] - y);
                th.lp2[c] = th.lp1[c] + th.coefB * (th.lp2[c] - th.lp1[c]);
                y = th.lp2[c];
                if (th.tone > 0.0 && m > 0.0)
                {
                    // crackle: sparse single-sample ticks from the sample counter (the same on every playback)
                    const double u = unitHash(0xC4AC41E5u + static_cast<std::uint32_t>(ch + 2 * lane), n);
                    if (u < 5.0e-4 * th.tone * m * 48000.0 / sampleRate)
                        y += 0.25 * m * (2.0 * unitHash(0x5EED0001u + static_cast<std::uint32_t>(ch + 2 * lane), n) - 1.0);
                }
                wet[ch] = static_cast<float>(x[ch] + blend * (y - x[ch]));
            }
            if (th.fade > 0)
                --th.fade;
            break;
        }
        case rev::ThrowDownsample:
        {
            const double factor = 1.0 + 31.0 * m * m;
            th.phase += 1.0;
            if (th.phase >= factor)
            {
                th.phase -= factor;
                for (int ch = 0; ch < numChannels; ++ch)
                    th.held[static_cast<size_t>(ch)] = x[ch];
            }
            const auto blend = static_cast<float>(smoothBlend(m));
            for (int ch = 0; ch < numChannels; ++ch)
                wet[ch] = x[ch] + blend * (th.held[static_cast<size_t>(ch)] - x[ch]);
            break;
        }
        default: break;
    }
    if (send)
    {
        const float peak = std::max(std::abs(wet[0]), stereo ? std::abs(wet[1]) : 0.0f);
        th.quiet = peak < 1.0e-6f ? th.quiet + 1 : 0;
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] += static_cast<float>(th.mix) * wet[ch];
    }
    else if (th.mix >= 1.0)
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] = wet[ch];
    else
        for (int ch = 0; ch < numChannels; ++ch)
            x[ch] += static_cast<float>(th.mix) * (wet[ch] - x[ch]);
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
            runLane(e, x, numChannels, q);
        for (int s = 0; s < rev::kShapes; ++s)
            runLane(rev::kEngines + s, x, numChannels, q);
        for (int t = 0; t < rev::kThrows; ++t)
            runLane(rev::kFirstThrow + t, x, numChannels, q);
        analyse(in, numChannels, wrap(motionStep.k, steps));
        listenTiming(in, numChannels, q);
        float inPeak = 0.0f, outPeak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            channels[ch][i] = orderGain >= 1.0 ? x[ch] : in[ch] + static_cast<float>(orderGain) * (x[ch] - in[ch]);
            inPeak = std::max(inPeak, std::abs(in[ch]));
            outPeak = std::max(outPeak, std::abs(channels[ch][i]));
        }
        // wave screen: the bin of this sample (1/8 of its step), peaks stored when the bin changes
        const double qe = q + kEps * dq;
        if (!waveStepValid || waveStamp != stamp || qe < waveStep.start || qe >= waveStep.end) // only at step edges
        {
            waveStep = stepAt(q);
            waveStepValid = true;
            waveStamp = stamp;
            waveStepIndex = std::clamp(wrap(waveStep.k, steps), 0, kFeelSteps - 1) * 8;
            waveStepScale = 8.0 / std::max(1.0e-12, waveStep.end - waveStep.start);
        }
        const int bin = waveStepIndex + std::clamp(static_cast<int>((q - waveStep.start) * waveStepScale), 0, 7);
        if (bin != waveBin)
        {
            if (waveBin >= 0)
            {
                waveIn[static_cast<size_t>(waveBin)].store(waveInPeak, std::memory_order_relaxed);
                waveOut[static_cast<size_t>(waveBin)].store(waveOutPeak, std::memory_order_relaxed);
            }
            waveBin = bin;
            waveInPeak = waveOutPeak = 0.0f;
        }
        waveInPeak = std::max(waveInPeak, inPeak);
        waveOutPeak = std::max(waveOutPeak, outPeak);
    }
    shownStep.store(wrap(stepAt(q).k, steps), std::memory_order_relaxed);
    const double stepSamples = stepQ / dq;
    warming.store(static_cast<double>(std::min(tapes[0].written, tapes[1].written)) < std::min(2.0 * stepSamples, static_cast<double>(tapes[1].size - 1)),
                  std::memory_order_relaxed);
}
} // namespace hpeq::dsp
