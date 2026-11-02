#include "DSP/HamloEcho.h"

#include <algorithm>
#include <cmath>
#include <complex>

#include "DSP/DspConfig.h"

namespace hpeq::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kTimeGlideSeconds = 0.15;  // TAPE: the read glides like a tape speed change
constexpr double kFadeSeconds = 0.04;       // FADE: head cross-fade
constexpr double kSmoothSeconds = 0.03;     // feedback, cross, width, drive, depth, diffusion
constexpr double kFreezeSeconds = 0.02;
constexpr double kGrainSeconds = 0.05;      // pitch shifter grain
constexpr double kSearchSeconds = 0.008;    // a new grain may start up to this much later, to match the playing one
constexpr double kMatchSeconds = 0.003;     // compared stretch
constexpr std::array<double, 4> kDiffuserMs { 3.1, 4.7, 7.3, 11.9 };
constexpr std::array<double, echo::kSyncDivisions> kSyncBeats { 0.0, 0.125, 0.25, 0.375, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0, 4.0 };

double smoothing(double seconds, double fs) noexcept
{
    return 1.0 - std::exp(-1.0 / (seconds * fs));
}

double onePole(double hz, double fs) noexcept
{
    return 1.0 - std::exp(-2.0 * kPi * std::clamp(hz, 1.0, 0.49 * fs) / fs);
}

double resonanceQ(double res) noexcept
{
    return 0.7071067811865476 + std::clamp(res, 0.0, 1.0) * 5.3;
}

// RBJ cookbook designs. kind: 0 high-pass, 1 low-pass, 2 peaking (gain in dB).
struct Coefficients
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
};

Coefficients design(int kind, double hz, double q, double gainDb, double fs) noexcept
{
    const double w = 2.0 * kPi * std::clamp(hz, 5.0, 0.45 * fs) / fs;
    const double cw = std::cos(w), alpha = std::sin(w) / (2.0 * q);
    double b0, b1, b2, a0, a1, a2;
    if (kind == 0)
    {
        b0 = (1.0 + cw) * 0.5;
        b1 = -(1.0 + cw);
        b2 = b0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cw;
        a2 = 1.0 - alpha;
    }
    else if (kind == 1)
    {
        b0 = (1.0 - cw) * 0.5;
        b1 = 1.0 - cw;
        b2 = b0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cw;
        a2 = 1.0 - alpha;
    }
    else
    {
        const double A = std::pow(10.0, gainDb / 40.0);
        b0 = 1.0 + alpha * A;
        b1 = -2.0 * cw;
        b2 = 1.0 - alpha * A;
        a0 = 1.0 + alpha / A;
        a1 = -2.0 * cw;
        a2 = 1.0 - alpha / A;
    }
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

double magnitude(const Coefficients& c, double w) noexcept
{
    const auto z1 = std::polar(1.0, -w), z2 = std::polar(1.0, -2.0 * w);
    return std::abs((c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2));
}

// The character's own bandwidth (one-pole low-passes in the loop): cutoff and how many poles.
std::pair<double, int> characterTone(int character) noexcept
{
    switch (character)
    {
        case echo::Tape: return { 11000.0, 1 };
        case echo::Bbd: return { 4800.0, 2 };
        case echo::LoFi: return { 7000.0, 1 };
        default: return { 0.0, 0 };
    }
}

double characterWobbleMs(int character) noexcept
{
    switch (character)
    {
        case echo::Tape: return 2.5;
        case echo::Bbd: return 3.0;
        default: return 1.5;
    }
}

int characterOf(const FxSlotParams& p) noexcept
{
    return std::clamp(static_cast<int>(std::lround(p.values[slot::kDlyCharacter])), 1, echo::NumCharacters - 1);
}

inline double readCubic(const float* line, int size, int writePos, double delay) noexcept
{
    const int whole = static_cast<int>(delay);
    const double f = delay - whole;
    int i0 = writePos - whole;
    if (i0 < 0)
        i0 += size;
    if (f == 0.0)
        return line[i0];
    int im1 = i0 + 1;
    if (im1 >= size)
        im1 -= size;
    int i1 = i0 - 1;
    if (i1 < 0)
        i1 += size;
    int i2 = i1 - 1;
    if (i2 < 0)
        i2 += size;
    const double xm1 = line[im1], x0 = line[i0], x1 = line[i1], x2 = line[i2];
    const double c1 = 0.5 * (x1 - xm1);
    const double c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
    const double c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}
} // namespace

namespace echo
{
const char* characterName(int c) noexcept
{
    static constexpr const char* names[] { "Classic", "Clean", "Tape", "BBD", "Lo-Fi" };
    return names[std::clamp(c, 0, NumCharacters - 1)];
}

const char* syncName(int d) noexcept
{
    static constexpr const char* names[] { "Free", "1/32", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1" };
    return names[std::clamp(d, 0, kSyncDivisions - 1)];
}

double syncBeats(int d) noexcept
{
    return kSyncBeats[static_cast<size_t>(std::clamp(d, 0, kSyncDivisions - 1))];
}

double timeSeconds(const FxSlotParams& p, bool right, double bpm) noexcept
{
    const bool useRight = right && p.values[slot::kDlyLink] < 0.5f;
    const int sync = std::clamp(static_cast<int>(std::lround(p.values[useRight ? slot::kDlySyncR : slot::kDelaySync])), 0, kSyncDivisions - 1);
    const double ms = p.values[useRight ? slot::kDlyTimeRMs : slot::kDelayTimeMs];
    const double seconds = sync == 0 ? ms * 1.0e-3 : syncBeats(sync) * 60.0 / std::clamp(bpm, 20.0, 999.0);
    return std::clamp(seconds, 0.001, kMaxSeconds - 0.05);
}

double loopResponseDb(double hz, const FxSlotParams& p, double fs) noexcept
{
    const double w = 2.0 * kPi * std::clamp(hz, 1.0, 0.49 * fs) / fs;
    double m = magnitude(design(0, std::clamp(static_cast<double>(p.values[slot::kDelayLowCutHz]), 10.0, 2000.0),
                                resonanceQ(p.values[slot::kDlyLowRes]), 0.0, fs), w);
    if (p.values[slot::kDlyBandGainDb] != 0.0f)
        m *= magnitude(design(2, p.values[slot::kDlyBandHz], p.values[slot::kDlyBandQ], p.values[slot::kDlyBandGainDb], fs), w);
    const double high = std::clamp(static_cast<double>(p.values[slot::kDampingHz]), 500.0, 20000.0);
    if (high < std::min(19900.0, 0.45 * fs))
        m *= magnitude(design(1, high, resonanceQ(p.values[slot::kDlyHighRes]), 0.0, fs), w);
    const auto [toneHz, poles] = characterTone(characterOf(p));
    if (poles > 0)
    {
        const double a = onePole(toneHz, fs);
        const double tone = std::abs(a / (1.0 - (1.0 - a) * std::polar(1.0, -w)));
        m *= std::pow(tone, poles);
    }
    return 20.0 * std::log10(std::max(1.0e-9, m));
}
} // namespace echo

float EchoProcessor::Allpass::process(float x, float g) noexcept
{
    int r = write - length;
    if (r < 0)
        r += static_cast<int>(buffer.size());
    const float d = buffer[static_cast<size_t>(r)];
    const float w = x + g * d;
    buffer[static_cast<size_t>(write)] = w;
    if (++write == static_cast<int>(buffer.size()))
        write = 0;
    return d - g * w;
}

void EchoProcessor::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    lineSize = static_cast<int>(std::ceil(echo::kMaxSeconds * sampleRate)) + 8;
    pitchSize = static_cast<int>(std::ceil((2.5 * kGrainSeconds + kSearchSeconds + kMatchSeconds) * sampleRate)) + 8;
    for (int c = 0; c < 2; ++c)
    {
        auto& ch = channels[static_cast<size_t>(c)];
        ch.line.assign(static_cast<size_t>(lineSize), 0.0f);
        ch.pitchRing.assign(static_cast<size_t>(pitchSize), 0.0f);
        for (size_t i = 0; i < ch.diffusers.size(); ++i)
        {
            auto& a = ch.diffusers[i];
            a.length = std::max(1, static_cast<int>(std::lround(kDiffuserMs[i] * (c == 0 ? 1.0 : 1.13) * 1.0e-3 * sampleRate)));
            a.buffer.assign(static_cast<size_t>(a.length) + 2, 0.0f);
        }
    }
    reset();
}

void EchoProcessor::reset() noexcept
{
    for (auto& ch : channels)
    {
        std::fill(ch.line.begin(), ch.line.end(), 0.0f);
        std::fill(ch.pitchRing.begin(), ch.pitchRing.end(), 0.0f);
        for (auto& a : ch.diffusers)
        {
            std::fill(a.buffer.begin(), a.buffer.end(), 0.0f);
            a.write = 0;
        }
        ch.lowCut.z1 = ch.lowCut.z2 = ch.band.z1 = ch.band.z2 = ch.highCut.z1 = ch.highCut.z2 = 0.0;
        ch.pitchWrite = 0;
        ch.grainDelay = { 2.0, 2.0 };
        ch.grainPos = { 0.0, 0.5 };
        ch.tone1 = ch.tone2 = ch.held = 0.0;
        ch.delay = -1.0;
        ch.fade = 1.0;
    }
    writePos = 0;
    controlCounter = holdCounter = 0;
    wowCos = flutterCos = 1.0;
    wowSin = flutterSin = 0.0;
    duckEnvelope = 0.0;
    duckGain = 1.0;
    freezeMix = 0.0;
    inputGate = 1.0;
    blockPeak = 0.0f;
    meterOutDb.store(-120.0f, std::memory_order_relaxed);
    meterDuckDb.store(0.0f, std::memory_order_relaxed);
    primed = false;
}

double EchoProcessor::tailSeconds(const FxSlotParams& p, double bpm) noexcept
{
    const double t = std::max(echo::timeSeconds(p, false, bpm), echo::timeSeconds(p, true, bpm));
    const double fb = std::clamp(static_cast<double>(p.values[slot::kFeedback]), 0.0, 0.98);
    // a resonant or boosted loop rings longer: take the loop's largest gain into account
    const double boost = std::max({ 1.0, std::pow(10.0, p.values[slot::kDlyBandGainDb] / 20.0),
                                    resonanceQ(p.values[slot::kDlyLowRes]) / 0.7071067811865476,
                                    resonanceQ(p.values[slot::kDlyHighRes]) / 0.7071067811865476 });
    const double loopGain = fb * boost;
    double tail = loopGain < 0.01 ? t : loopGain >= 0.999 ? 30.0 : t + t * std::log(1.0e-3) / std::log(loopGain);
    tail = std::min(tail, 30.0) + 0.1; // diffusion and pitch grains
    return p.values[slot::kDlyFreeze] >= 0.5f ? std::max(tail, 30.0) : tail;
}

void EchoProcessor::updateTargets(const FxSlotParams& p, const Transport& transport) noexcept
{
    const double fs = sampleRate;
    character = characterOf(p);
    freeze = p.values[slot::kDlyFreeze] >= 0.5f;
    pingPong = p.values[slot::kPingPong] > 0.5f;
    const int mode = std::clamp(static_cast<int>(std::lround(p.values[slot::kDlyTimeMode])), 0, 1);
    const double maxDelay = static_cast<double>(lineSize - 8) - 2.0 * (3.0 + 0.2) * 1.0e-3 * fs; // room for the wobble
    for (int c = 0; c < 2; ++c)
    {
        auto& ch = channels[static_cast<size_t>(c)];
        // whole samples: a settled read is exact, which keeps a frozen loop lossless
        ch.target = std::clamp(std::round(echo::timeSeconds(p, c == 1, transport.bpm) * fs), 2.0, maxDelay);
        if (primed && mode != timeMode)
        {
            if (mode == echo::FadeTime) // TAPE -> FADE: the fade starts from where the tape read is
            {
                ch.fadeTo = std::round(ch.delay);
                ch.fade = 1.0;
            }
            else
                ch.delay = ch.fadeTo;
        }
    }
    timeMode = mode;

    feedbackTarget = std::clamp(static_cast<double>(p.values[slot::kFeedback]), 0.0, 0.98);
    // PING-PONG feeds the left line only and bounces every echo to the other side (owner's bug 2026-10-26:
    // with CROSS at 0 the echoes stayed in the left ear); CROSS acts while ping-pong is off.
    crossTarget = pingPong ? 1.0 : std::clamp(static_cast<double>(p.values[slot::kDlyCross]), 0.0, 1.0);
    widthTarget = std::clamp(static_cast<double>(p.values[slot::kDelayWidth]), 0.0, 1.5);
    driveTarget = std::pow(10.0, std::clamp(static_cast<double>(p.values[slot::kDlyDriveDb]), 0.0, 24.0) / 20.0);
    modDepthTarget = freeze ? 0.0 : std::clamp(static_cast<double>(p.values[slot::kDlyModDepth]), 0.0, 1.0) * characterWobbleMs(character) * 1.0e-3 * fs;
    modRate = std::clamp(static_cast<double>(p.values[slot::kDlyModRate]), 0.05, 10.0);
    wowRotCos = std::cos(2.0 * kPi * modRate / fs);
    wowRotSin = std::sin(2.0 * kPi * modRate / fs);
    flutterRotCos = std::cos(2.0 * kPi * 6.8 / fs);
    flutterRotSin = std::sin(2.0 * kPi * 6.8 / fs);
    diffusionTarget = std::clamp(static_cast<double>(p.values[slot::kDlyDiffusion]), 0.0, 1.0);
    pitchRatio = std::pow(2.0, std::clamp(static_cast<double>(p.values[slot::kDlyPitch]), -12.0, 12.0) / 12.0);
    lofi = std::clamp(static_cast<double>(p.values[slot::kDlyLofi]), 0.0, 1.0);
    if (character == echo::LoFi)
        lofi = std::max(lofi, 0.5);
    lowHzTarget = std::clamp(static_cast<double>(p.values[slot::kDelayLowCutHz]), 10.0, 2000.0);
    highHzTarget = std::clamp(static_cast<double>(p.values[slot::kDampingHz]), 500.0, 20000.0);
    lowQTarget = resonanceQ(p.values[slot::kDlyLowRes]);
    highQTarget = resonanceQ(p.values[slot::kDlyHighRes]);
    bandHzTarget = std::clamp(static_cast<double>(p.values[slot::kDlyBandHz]), 100.0, 10000.0);
    bandGainTarget = std::clamp(static_cast<double>(p.values[slot::kDlyBandGainDb]), -18.0, 12.0);
    bandQTarget = std::clamp(static_cast<double>(p.values[slot::kDlyBandQ]), 0.3, 6.0);
    const auto tone = characterTone(character);
    toneCoefficient = tone.second > 0 ? onePole(tone.first, fs) : 1.0;
    duckAmount = std::clamp(static_cast<double>(p.values[slot::kDlyDuck]), 0.0, 1.0);

    if (!primed)
    {
        for (auto& ch : channels)
        {
            ch.delay = ch.target;
            ch.fadeFrom = ch.fadeTo = ch.target;
            ch.fade = 1.0;
        }
        feedback = feedbackTarget;
        cross = crossTarget;
        width = widthTarget;
        driveGain = driveTarget;
        modDepthSamples = modDepthTarget;
        diffusion = diffusionTarget;
        lowHz = lowHzTarget;
        highHz = highHzTarget;
        lowQ = lowQTarget;
        highQ = highQTarget;
        bandHz = bandHzTarget;
        bandGainDb = bandGainTarget;
        bandQ = bandQTarget;
        freezeMix = freeze ? 1.0 : 0.0;
        inputGate = 1.0 - freezeMix;
        designFilters();
        controlCounter = 0;
        primed = true;
    }
}

void EchoProcessor::designFilters() noexcept
{
    const double fs = sampleRate;
    const auto lo = design(0, lowHz, lowQ, 0.0, fs), bd = design(2, bandHz, bandQ, bandGainDb, fs);
    highOpen = highHz >= std::min(19900.0, 0.45 * fs);
    const auto hi = design(1, highHz, highQ, 0.0, fs);
    for (auto& ch : channels)
    {
        ch.lowCut.b0 = lo.b0, ch.lowCut.b1 = lo.b1, ch.lowCut.b2 = lo.b2, ch.lowCut.a1 = lo.a1, ch.lowCut.a2 = lo.a2;
        ch.band.b0 = bd.b0, ch.band.b1 = bd.b1, ch.band.b2 = bd.b2, ch.band.a1 = bd.a1, ch.band.a2 = bd.a2;
        ch.highCut.b0 = hi.b0, ch.highCut.b1 = hi.b1, ch.highCut.b2 = hi.b2, ch.highCut.a1 = hi.a1, ch.highCut.a2 = hi.a2;
    }
}

double EchoProcessor::readLine(const Channel& ch, double delay) const noexcept
{
    return readCubic(ch.line.data(), lineSize, writePos, std::clamp(delay, 2.0, static_cast<double>(lineSize - 4)));
}

double EchoProcessor::loop(Channel& ch, double x) noexcept
{
    // loop filters
    x = ch.lowCut.process(x);
    if (bandGainDb != 0.0)
        x = ch.band.process(x);
    if (!highOpen)
        x = ch.highCut.process(x);
    // diffusion
    if (diffusion > 1.0e-6)
    {
        const auto g = static_cast<float>(0.7 * diffusion);
        auto v = static_cast<float>(x);
        for (auto& a : ch.diffusers)
            v = a.process(v, g);
        x = v;
    }
    if (pitchRatio != 1.0)
        x = pitchShift(ch, x);
    // character: drive lowers the clip ceiling, small signals keep their level
    const double g = driveGain;
    switch (character)
    {
        case echo::Tape:
        {
            constexpr double bias = 0.08;
            const double t0 = std::tanh(bias), slope = 1.0 - t0 * t0;
            x = (std::tanh(g * x + bias) - t0) / (g * slope);
            break;
        }
        case echo::Bbd: x = x / (1.0 + std::abs(g * x)); break;
        default:
            if (g > 1.0001)
                x = std::tanh(g * x) / g;
            break;
    }
    if (toneCoefficient < 1.0)
    {
        ch.tone1 += toneCoefficient * (x - ch.tone1);
        x = ch.tone1;
        if (character == echo::Bbd)
        {
            ch.tone2 += toneCoefficient * (x - ch.tone2);
            x = ch.tone2;
        }
    }
    // lo-fi: sample-and-hold and fewer bits
    if (lofi > 0.0)
    {
        if (holdCounter == 0)
            ch.held = x;
        x = ch.held;
        const double step = std::pow(2.0, 1.0 - (16.0 - 12.0 * lofi));
        x = std::round(x / step) * step;
    }
    return 4.0 * std::tanh(0.25 * x); // safety: any feedback, resonance or boost stays bounded
}

double EchoProcessor::readPitch(const Channel& ch, double delay) const noexcept
{
    delay = std::clamp(delay, 0.0, static_cast<double>(pitchSize - 2)); // a ratio change mid-grain cannot leave the ring
    const int whole = static_cast<int>(delay);
    const double f = delay - whole;
    int i0 = ch.pitchWrite - whole;
    if (i0 < 0)
        i0 += pitchSize;
    const int i1 = i0 == 0 ? pitchSize - 1 : i0 - 1;
    return ch.pitchRing[static_cast<size_t>(i0)] + f * (ch.pitchRing[static_cast<size_t>(i1)] - ch.pitchRing[static_cast<size_t>(i0)]);
}

// Two voices read the ring at `ratio` under raised-sine windows half a grain apart (the weights sum to 1).
// A voice that finishes its window starts again at the delay that keeps it inside the ring for a whole
// grain, moved by up to 8 ms to where the recent past best matches what the other voice is reading:
// the cross-fade then joins two waveforms in phase instead of cancelling them.
double EchoProcessor::pitchShift(Channel& ch, double x) noexcept
{
    ch.pitchRing[static_cast<size_t>(ch.pitchWrite)] = static_cast<float>(x);
    const double grain = kGrainSeconds * sampleRate;
    const double start = 2.0 + std::max(0.0, pitchRatio - 1.0) * grain;
    double out = 0.0;
    for (int v = 0; v < 2; ++v)
    {
        auto& pos = ch.grainPos[static_cast<size_t>(v)];
        auto& delay = ch.grainDelay[static_cast<size_t>(v)];
        if (pos >= 1.0)
        {
            pos -= 1.0;
            // integer reads: coarse search every 4 samples, then +-3 around the best
            const int other = static_cast<int>(std::lround(ch.grainDelay[static_cast<size_t>(1 - v)]));
            const int base = static_cast<int>(std::lround(start));
            const int search = static_cast<int>(kSearchSeconds * sampleRate), length = static_cast<int>(kMatchSeconds * sampleRate);
            const float* ring = ch.pitchRing.data();
            const auto at = [&](int delayed)
            {
                int i = ch.pitchWrite - std::clamp(delayed, 0, pitchSize - 2);
                return ring[i < 0 ? i + pitchSize : i];
            };
            const auto score = [&](int offset)
            {
                double dot = 0.0, energy = 1.0e-12;
                for (int j = 0; j < length; j += 3)
                {
                    const double a = at(base + offset + j), b = at(other + j);
                    dot += a * b;
                    energy += a * a;
                }
                return dot / std::sqrt(energy);
            };
            int bestOffset = 0;
            double bestScore = -1.0e300;
            for (int offset = 0; offset <= search; offset += 4)
                if (const double sc = score(offset); sc > bestScore)
                {
                    bestScore = sc;
                    bestOffset = offset;
                }
            const int coarse = bestOffset;
            for (int offset = std::max(0, coarse - 3); offset <= coarse + 3; ++offset)
                if (const double sc = score(offset); sc > bestScore)
                {
                    bestScore = sc;
                    bestOffset = offset;
                }
            const double best = start + bestOffset;
            delay = best;
        }
        const double s = std::sin(kPi * pos);
        out += s * s * readPitch(ch, delay);
        delay += 1.0 - pitchRatio;
        pos += 1.0 / grain;
    }
    if (++ch.pitchWrite == pitchSize)
        ch.pitchWrite = 0;
    return out;
}

void EchoProcessor::process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams& p,
                            const Transport& transport) noexcept
{
    updateTargets(p, transport);
    const double fs = sampleRate;
    const double glideK = smoothing(kTimeGlideSeconds, fs), smoothK = smoothing(kSmoothSeconds, fs), freezeK = smoothing(kFreezeSeconds, fs);
    const double filterTickK = 1.0 - std::pow(1.0 - smoothK, kControlInterval);
    const double fadeStep = 1.0 / (kFadeSeconds * fs);
    const double duckAttack = smoothing(0.002, fs), duckDetectorRelease = smoothing(0.05, fs);
    const double duckGainAttack = smoothing(0.010, fs), duckRelease = smoothing(0.150, fs);
    const int holdLength = 1 + static_cast<int>(std::lround(lofi * 11.0));
    const double flutterDepth = character == echo::Tape ? 0.06 : 0.0;
    float peak = 0.0f;
    double energy = 0.0, duckSum = 0.0;

    for (int n = 0; n < numSamples; ++n)
    {
        if (controlCounter == 0)
        {
            const auto glide = [filterTickK](double& v, double target, double snap)
            {
                if (v == target)
                    return false;
                v += filterTickK * (target - v);
                if (std::abs(target - v) < snap)
                    v = target;
                return true;
            };
            bool moved = glide(lowHz, lowHzTarget, 1.0e-3);
            moved |= glide(highHz, highHzTarget, 1.0e-3);
            moved |= glide(bandHz, bandHzTarget, 1.0e-3);
            moved |= glide(lowQ, lowQTarget, 1.0e-6);
            moved |= glide(highQ, highQTarget, 1.0e-6);
            moved |= glide(bandGainDb, bandGainTarget, 1.0e-6);
            moved |= glide(bandQ, bandQTarget, 1.0e-6);
            if (moved)
                designFilters();
        }
        if (++controlCounter == kControlInterval)
            controlCounter = 0;

        const auto follow = [smoothK](double& v, double target)
        {
            if (v != target)
            {
                v += smoothK * (target - v);
                if (std::abs(target - v) < 1.0e-9)
                    v = target;
            }
        };
        follow(feedback, feedbackTarget);
        follow(cross, crossTarget);
        follow(width, widthTarget);
        follow(driveGain, driveTarget);
        follow(modDepthSamples, modDepthTarget);
        follow(diffusion, diffusionTarget);
        const double freezeTarget = freeze ? 1.0 : 0.0;
        if (freezeMix != freezeTarget)
        {
            freezeMix += freezeK * (freezeTarget - freezeMix);
            if (std::abs(freezeTarget - freezeMix) < 1.0e-9)
                freezeMix = freezeTarget;
            inputGate = 1.0 - freezeMix;
        }

        // LFOs (rotation, renormalised: no drift)
        {
            const double c = wowCos * wowRotCos - wowSin * wowRotSin, s = wowSin * wowRotCos + wowCos * wowRotSin;
            const double k = 1.5 - 0.5 * (c * c + s * s);
            wowCos = c * k;
            wowSin = s * k;
            const double fc = flutterCos * flutterRotCos - flutterSin * flutterRotSin, fsn = flutterSin * flutterRotCos + flutterCos * flutterRotSin;
            const double fk = 1.5 - 0.5 * (fc * fc + fsn * fsn);
            flutterCos = fc * fk;
            flutterSin = fsn * fk;
        }

        const double dryL = in[0][n], dryR = numChannels > 1 ? in[1][n] : in[0][n];
        const double level = std::max(std::abs(dryL), std::abs(dryR));
        duckEnvelope += (level > duckEnvelope ? duckAttack : duckDetectorRelease) * (level - duckEnvelope);
        double duckTarget = 1.0;
        if (duckAmount > 0.0)
        {
            const double envDb = 20.0 * std::log10(duckEnvelope + 1.0e-9);
            duckTarget = std::pow(10.0, -duckAmount * 30.0 * std::clamp((envDb + 40.0) / 34.0, 0.0, 1.0) / 20.0);
        }
        if (duckGain != duckTarget)
            duckGain += (duckTarget < duckGain ? duckGainAttack : duckRelease) * (duckTarget - duckGain);

        // read both lines
        std::array<double, 2> raw {}, e {};
        for (int c = 0; c < 2; ++c)
        {
            auto& ch = channels[static_cast<size_t>(c)];
            const double lfo = c == 0 ? wowSin : wowCos;
            const double wobble = modDepthSamples > 0.0 ? modDepthSamples * (1.0 + lfo + flutterDepth * (1.0 + flutterSin)) : 0.0;
            if (timeMode == echo::TapeTime)
            {
                if (ch.delay != ch.target)
                {
                    ch.delay += glideK * (ch.target - ch.delay);
                    if (std::abs(ch.target - ch.delay) < 1.0e-6)
                        ch.delay = ch.target;
                }
                raw[static_cast<size_t>(c)] = readLine(ch, ch.delay + wobble);
            }
            else
            {
                if (ch.fade >= 1.0 && ch.target != ch.fadeTo)
                {
                    ch.fadeFrom = ch.fadeTo;
                    ch.fadeTo = ch.target;
                    ch.fade = 0.0;
                }
                if (ch.fade < 1.0)
                {
                    ch.fade = std::min(1.0, ch.fade + fadeStep);
                    const double a = 0.5 * kPi * ch.fade;
                    raw[static_cast<size_t>(c)] = std::cos(a) * readLine(ch, ch.fadeFrom + wobble) + std::sin(a) * readLine(ch, ch.fadeTo + wobble);
                }
                else
                    raw[static_cast<size_t>(c)] = readLine(ch, ch.fadeTo + wobble);
            }
            e[static_cast<size_t>(c)] = loop(ch, raw[static_cast<size_t>(c)]);
        }
        if (lofi > 0.0 && ++holdCounter >= holdLength)
            holdCounter = 0;

        // feedback (cross = ping-pong amount), freeze, input
        const double fbL = feedback * ((1.0 - cross) * e[0] + cross * e[1]);
        const double fbR = feedback * ((1.0 - cross) * e[1] + cross * e[0]);
        const double inL = pingPong ? 0.5 * (dryL + dryR) : dryL, inR = pingPong ? 0.0 : dryR;
        channels[0].line[static_cast<size_t>(writePos)] = static_cast<float>(inputGate * inL + (1.0 - freezeMix) * fbL + freezeMix * raw[0]);
        channels[1].line[static_cast<size_t>(writePos)] = static_cast<float>(inputGate * inR + (1.0 - freezeMix) * fbR + freezeMix * raw[1]);
        if (++writePos == lineSize)
            writePos = 0;

        // output: the echoes, width, ducking
        const double mid = 0.5 * (e[0] + e[1]), side = 0.5 * (e[0] - e[1]) * width;
        const double outL = (mid + side) * duckGain, outR = (mid - side) * duckGain;
        if (numChannels > 1)
        {
            wet[0][n] = static_cast<float>(outL);
            wet[1][n] = static_cast<float>(outR);
        }
        else
            wet[0][n] = static_cast<float>(0.5 * (outL + outR));
        peak = std::max({ peak, std::abs(static_cast<float>(outL)), std::abs(static_cast<float>(outR)) });
        energy += 0.5 * (outL * outL + outR * outR);
        duckSum += duckGain;
    }
    blockPeak = peak;
    if (numSamples > 0)
    {
        meterOutDb.store(static_cast<float>(10.0 * std::log10(energy / numSamples + 1.0e-12)), std::memory_order_relaxed);
        meterDuckDb.store(static_cast<float>(-20.0 * std::log10(std::max(1.0e-6, duckSum / numSamples))), std::memory_order_relaxed);
    }
}
} // namespace hpeq::dsp
