#include "DSP/VerbV2Processor.h"
#include "DSP/VerbTemporalEffects.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <vector>

namespace hpeq::dsp
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr int linesCount = 16, controlInterval = 32;
using Pair = std::array<double, 2>;

double bounded(double x, double lo, double hi, double fallback = 0.0) noexcept
{
    return std::clamp(std::isfinite(x) ? x : fallback, lo, hi);
}
double dbGain(double db) noexcept { return std::pow(10.0, db / 20.0); }
double smoothing(double seconds, double fs) noexcept { return 1.0 - std::exp(-1.0 / (seconds * fs)); }
double glide(double& value, double target, double coefficient, double snap = 1.0e-9) noexcept
{
    value += coefficient * (target - value);
    if (std::abs(target - value) < snap) value = target;
    return value;
}

struct Coeff
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
};

Coeff design(int type, double hz, double gainDb, double q, double fs) noexcept
{
    hz = bounded(hz, 10.0, fs * 0.45, 1000.0);
    q = bounded(q, 0.15, 8.0, 0.7071067811865476);
    if (type < 3 && std::abs(gainDb) < 1.0e-12) return {};
    const double w = 2.0 * pi * hz / fs, c = std::cos(w), s = std::sin(w);
    const double A = std::pow(10.0, bounded(gainDb, -120.0, 24.0) / 40.0);
    const double alpha = s / (2.0 * q);
    double b0, b1, b2, a0, a1, a2;
    if (type == 1 || type == 2)
    {
        // Monotonic shelves (S=1); Q belongs to bells/cuts only.
        const double t = std::sqrt(2.0 * A) * s, ap = A + 1.0, am = A - 1.0;
        if (type == 1)
        {
            b0 = A * (ap - am * c + t); b1 = 2.0 * A * (am - ap * c); b2 = A * (ap - am * c - t);
            a0 = ap + am * c + t; a1 = -2.0 * (am + ap * c); a2 = ap + am * c - t;
        }
        else
        {
            b0 = A * (ap + am * c + t); b1 = -2.0 * A * (am + ap * c); b2 = A * (ap + am * c - t);
            a0 = ap - am * c + t; a1 = 2.0 * (am - ap * c); a2 = ap - am * c - t;
        }
    }
    else if (type == 3 || type == 4)
    {
        const bool highPass = type == 3;
        b0 = (1.0 + (highPass ? c : -c)) * 0.5;
        b1 = highPass ? -2.0 * b0 : 2.0 * b0; b2 = b0;
        a0 = 1.0 + alpha; a1 = -2.0 * c; a2 = 1.0 - alpha;
    }
    else
    {
        b0 = 1.0 + alpha * A; b1 = -2.0 * c; b2 = 1.0 - alpha * A;
        a0 = 1.0 + alpha / A; a1 = -2.0 * c; a2 = 1.0 - alpha / A;
    }
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

double magnitude(const Coeff& c, double hz, double fs) noexcept
{
    const auto z = std::polar(1.0, -2.0 * pi * hz / fs);
    return std::abs((c.b0 + c.b1 * z + c.b2 * z * z) / (1.0 + c.a1 * z + c.a2 * z * z));
}

struct Filter
{
    Coeff c;
    double z1 = 0.0, z2 = 0.0;
    bool enabled = false;
    double tick(double x) noexcept
    {
        if (!enabled) return x;
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }
    void clear() noexcept { z1 = z2 = 0.0; }
};

using detail::Delay;

struct Allpass
{
    Delay delay;
    int length = 1;
    double tick(double x, double g) noexcept
    {
        const double z = delay.linear(static_cast<double>(length - 1));
        const double w = x + g * z;
        delay.push(w);
        return z - g * w;
    }
};

constexpr std::array<double, 16> delayMs {
    7.31, 13.57, 19.19, 10.43, 27.11, 16.73, 32.93, 23.89,
    8.81, 14.87, 21.37, 11.93, 29.83, 18.61, 36.17, 25.79
};
constexpr std::array<double, 8> diffusionMs { 1.137, 2.381, 3.769, 5.113, 0.683, 1.811, 2.977, 4.433 };
constexpr std::array<double, 12> syncBeats { 0, 0.0625, 0.125, 1.0 / 6, 0.25, 1.0 / 3, 0.375, 0.5, 2.0 / 3, 0.75, 1, 2 };

double predelaySeconds(const VerbV2Settings& s, double bpm) noexcept
{
    const int sync = static_cast<int>(bounded(std::round(s.base.values[slot::kRevSync]), 0, 11));
    return sync == 0 ? bounded(s.base.values[slot::kPredelayMs], 0, 500) * 0.001
                    : std::min(0.5, syncBeats[static_cast<size_t>(sync)] * 60.0 / bounded(bpm, 20, 999, 120));
}
double shapeSeconds(const VerbV2Settings& s, double bpm) noexcept
{
    const int sync = std::clamp(s.shapeSync, 0, 11);
    return sync == 0 ? bounded(s.shapeTimeMs, 50, 2000, 250) * 0.001
                    : std::clamp(syncBeats[static_cast<size_t>(sync)] * 60.0 / bounded(bpm, 20, 999, 120), 0.05, 2.0);
}

void hadamard(std::array<double, linesCount>& v) noexcept
{
    for (int h = 1; h < linesCount; h <<= 1)
        for (int start = 0; start < linesCount; start += h * 2)
            for (int k = start; k < start + h; ++k)
            {
                const double a = v[static_cast<size_t>(k)], b = v[static_cast<size_t>(k + h)];
                v[static_cast<size_t>(k)] = a + b; v[static_cast<size_t>(k + h)] = a - b;
            }
    for (auto& x : v) x *= 0.25;
}

// Orthogonal pair rotation followed by a signed permutation: distinct from the
// all-to-all Hadamard tank, with no gain in a frozen loop.
void coupled(std::array<double, linesCount>& v) noexcept
{
    std::array<double, linesCount> w {};
    constexpr double c = 0.7071067811865475244;
    for (int k = 0; k < linesCount; k += 2)
    {
        const double a = v[static_cast<size_t>(k)], b = v[static_cast<size_t>(k + 1)];
        w[static_cast<size_t>((k * 5) % linesCount)] = (a + b) * c;
        w[static_cast<size_t>(((k + 1) * 5) % linesCount)] = (a - b) * c;
    }
    v = w;
}

using detail::ReverseFragments;
using detail::PitchFeedback;

struct TankLine
{
    Delay delay;
    Allpass diffuseA, diffuseB;
    std::array<Filter, 8> damping;
    double length = 1, lengthTarget = 1, frozenLength = 1, gain = 0.5, gainTarget = 0.5;
    double phaseSin = 0, phaseCos = 1, rotateSin = 0, rotateCos = 1;
};

struct Path
{
    double fs = 48000.0;
    int engine = 1, tickCounter = 0;
    bool primed = false, coefficientsDirty = true;
    VerbV2Settings settings, designedSettings;
    double bpm = 120.0;
    std::array<TankLine, linesCount> lines;
    std::array<Delay, 2> pre, early, buildup;
    std::array<std::array<Allpass, 8>, 2> inputDiff;
    std::array<std::array<Allpass, 2>, 2> outputDiff;
    std::array<std::array<Filter, 8>, 2> post;
    ReverseFragments reverse;
    PitchFeedback pitch;
    Pair pitchLast {}, warmState {};
    double predelay = 0, predelayTarget = 0, freezeMix = 0, freezeTarget = 0;
    double inputFade = 0, inputTarget = 1, modDepth = 0, modTarget = 0;
    double earlyGain = 0, earlyTarget = 0, lateGain = 1, lateTarget = 1;
    double width = 1, widthTarget = 1, duckEnvelope = 0, duckGain = 1;
    double gateGain = 0, shapeMix = 0, shapeTarget = 0, shimmer = 0, shimmerTarget = 0;
    int gateHold = 0, swellAge = -1, reverseLength = 12000, shape = 0;
    bool triggerArmed = true;
    double peakEnvelope = 0, lastDuck = 0;
    double fastK = 0, slowK = 0, warmK = 0, peakRelease = 0, detectorAttack = 0, detectorRelease = 0;
    double duckAttack = 0, duckRelease = 0, gateAttack = 0, gateRelease = 0;
    double size = 1, earlyDiffusion = 0.7, lateDiffusion = 0.7, frozenLateDiffusion = 0.7, attack = 0.2, release = 0.03, threshold = 0.03;
    double pitchRatio = 2, pitchFeedback = 0.35, duckAmount = 0;
    std::array<std::array<double, 20>, 2> earlyDelay {}, earlyWeight {};

    void prepare(double rate)
    {
        fs = rate;
        fastK = smoothing(0.025, fs); slowK = smoothing(0.15, fs);
        warmK = 1.0 - std::exp(-2.0 * pi * 7200 / fs);
        peakRelease = std::exp(-1.0 / (0.025 * fs));
        detectorAttack = smoothing(0.002, fs); detectorRelease = smoothing(0.050, fs);
        gateAttack = smoothing(0.001, fs);
        for (auto& l : lines)
        {
            l.delay.prepare(static_cast<int>(fs * 0.6) + 64);
            l.diffuseA.delay.prepare(static_cast<int>(fs * 0.10) + 16);
            l.diffuseB.delay.prepare(static_cast<int>(fs * 0.08) + 16);
        }
        for (int ch = 0; ch < 2; ++ch)
        {
            pre[static_cast<size_t>(ch)].prepare(static_cast<int>(fs * 0.51) + 16);
            early[static_cast<size_t>(ch)].prepare(static_cast<int>(fs * 0.30) + 16);
            buildup[static_cast<size_t>(ch)].prepare(static_cast<int>(fs * 0.60) + 16);
            for (auto& a : inputDiff[static_cast<size_t>(ch)]) a.delay.prepare(static_cast<int>(fs * 0.025) + 16);
            for (auto& a : outputDiff[static_cast<size_t>(ch)]) a.delay.prepare(static_cast<int>(fs * 0.025) + 16);
        }
        reverse.prepare(fs); pitch.prepare(fs); clear();
    }

    void clear() noexcept
    {
        for (auto& l : lines)
        {
            l.delay.clear(); l.diffuseA.delay.clear(); l.diffuseB.delay.clear();
            for (auto& f : l.damping) { f.clear(); f.enabled = false; f.c = {}; }
            l.phaseSin = 0; l.phaseCos = 1;
        }
        for (int ch = 0; ch < 2; ++ch)
        {
            pre[static_cast<size_t>(ch)].clear(); early[static_cast<size_t>(ch)].clear(); buildup[static_cast<size_t>(ch)].clear();
            for (auto& a : inputDiff[static_cast<size_t>(ch)]) a.delay.clear();
            for (auto& a : outputDiff[static_cast<size_t>(ch)]) a.delay.clear();
            for (auto& f : post[static_cast<size_t>(ch)]) { f.clear(); f.enabled = false; f.c = {}; }
        }
        reverse.clear(); pitch.clear(); pitchLast = {}; warmState = {};
        tickCounter = 0; primed = false; coefficientsDirty = true;
        inputFade = 0; inputTarget = 1; freezeMix = freezeTarget = 0;
        duckEnvelope = 0; duckGain = 1; gateGain = 0; gateHold = 0; swellAge = -1;
        triggerArmed = true; peakEnvelope = 0; shapeMix = 0; shimmer = 0;
    }

    void configure(int e) noexcept
    {
        engine = std::clamp(e, 1, 6);
        for (int i = 0; i < linesCount; ++i)
        { lines[static_cast<size_t>(i)].phaseSin = std::sin(i * 0.71); lines[static_cast<size_t>(i)].phaseCos = std::cos(i * 0.71); }
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int k = 0; k < 8; ++k)
                inputDiff[static_cast<size_t>(ch)][static_cast<size_t>(k)].length = std::max(1, static_cast<int>(std::round(
                    diffusionMs[static_cast<size_t>(k)] * (1.0 + ch * 0.079) * fs * 0.001)));
            for (int k = 0; k < 2; ++k)
                outputDiff[static_cast<size_t>(ch)][static_cast<size_t>(k)].length = std::max(1, static_cast<int>(std::round(
                    (1.93 + 2.17 * k + 0.37 * ch) * fs * 0.001)));
        }
    }

    void set(const VerbV2Settings& s, double newBpm, bool outgoing = false) noexcept
    {
        settings = s; settings.engine = engine; bpm = bounded(newBpm, 20, 999, 120);
        if (outgoing) { settings.base.values[slot::kRevFreeze] = 0; settings.shimmerAmount = 0; inputTarget = 0; }
        else inputTarget = settings.base.active ? 1 : 0;
        predelayTarget = predelaySeconds(settings, bpm) * fs;
        const bool wasFrozen = freezeTarget >= 0.5;
        freezeTarget = settings.base.values[slot::kRevFreeze] >= 0.5f ? 1.0 : 0.0;
        if (!wasFrozen && freezeTarget > 0 && primed)
        {
            frozenLateDiffusion = lateDiffusion;
            for (auto& line : lines) line.frozenLength = std::round(line.length);
        }
        const double distance = bounded(settings.base.values[slot::kRevDistance], 0, 1, 0.4);
        earlyTarget = dbGain(bounded(settings.earlyDb, -60, 12)) * (1.1 - 0.8 * distance) * (engine == 5 ? 0.38 : 1.0);
        lateTarget = dbGain(bounded(settings.lateDb, -60, 12)) * (0.8 + 0.4 * distance);
        widthTarget = bounded(settings.base.values[slot::kWidth], 0, 1.5, 1);
        modTarget = bounded(settings.base.values[slot::kRevModDepth], 0, 1) * fs * 0.0006;
        shapeTarget = settings.timeShape == 0 || freezeTarget > 0 ? 0 : 1;
        shimmerTarget = bounded(settings.shimmerAmount, 0, 1) * (1.0 - freezeTarget);
        shape = std::clamp(settings.timeShape, 0, 3);
        reverseLength = std::max(8, static_cast<int>(std::round(shapeSeconds(settings, bpm) * fs)));
        size = bounded(settings.base.values[slot::kSize], 0.3, 1.5, 1);
        for (int ch = 0; ch < 2; ++ch)
            for (int j = 0; j < 20; ++j)
            {
                earlyDelay[static_cast<size_t>(ch)][static_cast<size_t>(j)] =
                    (2.7 + 1.93 * j + 0.173 * j * j + ch * (0.59 + j * 0.109)) * size * 0.001 * fs;
                earlyWeight[static_cast<size_t>(ch)][static_cast<size_t>(j)] =
                    ((j + ch) % 5 == 0 ? -1.0 : 1.0) * std::exp(-0.13 * j);
            }
        earlyDiffusion = bounded(settings.base.values[slot::kRevEarlyDiff], 0, 1, 0.7);
        lateDiffusion = freezeTarget > 0 && primed ? frozenLateDiffusion : bounded(settings.base.values[slot::kRevLateDiff], 0, 1, 0.7);
        attack = bounded(settings.base.values[slot::kRevAttack], 0, 1, 0.2);
        release = bounded(settings.gateReleaseMs, 5, 1000, 30) * 0.001;
        threshold = dbGain(bounded(settings.gateThresholdDb, -60, 0, -30));
        gateRelease = smoothing(release * 0.145, fs);
        pitchRatio = std::exp2(bounded(settings.shimmerPitch, -12, 24, 12) / 12.0);
        pitchFeedback = bounded(settings.shimmerFeedback, 0, 0.85, 0.35);
        duckAmount = bounded(settings.base.values[slot::kRevDuck], 0, 1);
        duckAttack = smoothing(bounded(settings.duckAttackMs, 0.1, 100, 5) * 0.001, fs);
        duckRelease = smoothing(bounded(settings.duckReleaseMs, 10, 2000, 250) * 0.001, fs);
        const double rate = bounded(settings.base.values[slot::kRevModRate], 0.01, 10, 0.6);
        for (int i = 0; i < linesCount; ++i)
        {
            auto& l = lines[static_cast<size_t>(i)];
            const double w = 2.0 * pi * rate * (0.61 + 0.057 * i) / fs;
            l.rotateSin = std::sin(w); l.rotateCos = std::cos(w);
        }
        bool dirty = !primed || wasFrozen != (freezeTarget > 0);
        constexpr std::array<int, 8> relevant { slot::kDecaySeconds, slot::kSize, slot::kRevBassMult, slot::kHfRatio,
            slot::kRevBassXover, slot::kRevHighXover, slot::kLowCutHz, slot::kRevHighCut };
        for (const auto i : relevant) dirty = dirty || settings.base.values[static_cast<size_t>(i)] != designedSettings.base.values[static_cast<size_t>(i)];
        const auto different = [](const VerbEqBand& a, const VerbEqBand& b)
        { return a.enabled != b.enabled || a.type != b.type || a.hz != b.hz || a.value != b.value || a.q != b.q; };
        for (int i = 0; i < 6; ++i)
            dirty = dirty || different(settings.decayEq[static_cast<size_t>(i)], designedSettings.decayEq[static_cast<size_t>(i)])
                          || different(settings.postEq[static_cast<size_t>(i)], designedSettings.postEq[static_cast<size_t>(i)]);
        coefficientsDirty = coefficientsDirty || dirty;
        if (!primed)
        {
            predelay = predelayTarget; freezeMix = freezeTarget; modDepth = modTarget * (1.0 - freezeMix);
            earlyGain = earlyTarget; lateGain = lateTarget; width = widthTarget;
            shapeMix = shapeTarget; shimmer = shimmerTarget;
            updateCoefficients();
            for (auto& l : lines) { l.length = l.lengthTarget; l.frozenLength = l.length; l.gain = l.gainTarget; }
            primed = true;
        }
    }

    void updateCoefficients() noexcept
    {
        const double size = bounded(settings.base.values[slot::kSize], 0.3, 1.5, 1.0);
        const double t60 = bounded(settings.base.values[slot::kDecaySeconds], 0.1, 30, 2.4);
        const double bass = bounded(settings.base.values[slot::kRevBassMult], 0.25, 4, 1);
        const double high = bounded(settings.base.values[slot::kHfRatio], 0.05, 1, 1);
        const double shapeScale = engine == 4 ? 0.67 : engine == 5 ? 2.2 : engine == 6 ? 1.6 : 1.0;
        for (int i = 0; i < linesCount; ++i)
        {
            auto& l = lines[static_cast<size_t>(i)];
            l.lengthTarget = std::max(4.0, std::round(delayMs[static_cast<size_t>(i)] * size * shapeScale * fs * 0.001));
            if (freezeTarget < .5 || !primed)
            {
                l.diffuseA.length = std::max(1, static_cast<int>(std::round((1.17 + (i % 7) * 0.617) * size * fs * 0.001)));
                l.diffuseB.length = std::max(1, static_cast<int>(std::round((0.67 + (i % 5) * 0.431) * size * fs * 0.001)));
            }
            const double added = engine == 3 ? l.diffuseA.length + l.diffuseB.length
                               : (engine == 2 || engine == 4) ? l.diffuseA.length : 0;
            const double loopSeconds = (l.lengthTarget + added) / fs;
            l.gainTarget = std::pow(10.0, -3.0 * loopSeconds / t60);
            const double lowDb = -60.0 * loopSeconds * (1.0 / std::clamp(t60 * bass, 0.1, 120.0) - 1.0 / t60);
            const double highDb = -60.0 * loopSeconds * (1.0 / std::clamp(t60 * high, 0.1, 120.0) - 1.0 / t60);
            l.damping[0].c = design(1, settings.base.values[slot::kRevBassXover], lowDb, 0.707, fs);
            l.damping[0].enabled = std::abs(lowDb) > 1.0e-12;
            l.damping[1].c = design(2, settings.base.values[slot::kRevHighXover], highDb, 0.707, fs);
            l.damping[1].enabled = std::abs(highDb) > 1.0e-12;
            for (int k = 0; k < 6; ++k)
            {
                const auto& band = settings.decayEq[static_cast<size_t>(k)];
                const double mult = std::exp2(bounded(band.value, -2, 2));
                const double bandDb = -60.0 * loopSeconds * (1.0 / std::clamp(t60 * mult, 0.1, 120.0) - 1.0 / t60);
                auto& f = l.damping[static_cast<size_t>(k + 2)];
                f.c = design(std::clamp(band.type, 0, 2), band.hz, bandDb, band.q, fs);
                f.enabled = band.enabled && std::abs(bandDb) > 1.0e-12;
            }
            // Bound combined loop magnitude, including overlapping positive
            // Decay EQ bands. The effective decay never exceeds 120 seconds.
            double peak = 1.0;
            for (int k = 0; k < 96; ++k)
            {
                const double hz = k == 0 ? 0 : 10.0 * std::pow((fs * 0.5) / 10.0, (k - 1) / 94.0);
                double mag = 1.0;
                for (const auto& f : l.damping) if (f.enabled) mag *= magnitude(f.c, hz, fs);
                peak = std::max(peak, mag);
            }
            // Include all band centres, where narrow bells can peak between grid points.
            for (const auto& band : settings.decayEq)
            {
                double mag = 1.0;
                for (const auto& f : l.damping) if (f.enabled) mag *= magnitude(f.c, bounded(band.hz, 10, fs * 0.45, 1000), fs);
                peak = std::max(peak, mag);
            }
            l.gainTarget = std::min(l.gainTarget, std::pow(10.0, -3.0 * loopSeconds / 120.0) / (peak * 1.000001));
        }
        for (int ch = 0; ch < 2; ++ch)
        {
            auto& p = post[static_cast<size_t>(ch)];
            p[0].c = design(3, settings.base.values[slot::kLowCutHz], 0, 0.7071067811865476, fs);
            p[0].enabled = true;
            p[1].c = design(4, settings.base.values[slot::kRevHighCut], 0, 0.7071067811865476, fs);
            p[1].enabled = settings.base.values[slot::kRevHighCut] < std::min(19900.0, fs * 0.45);
            for (int i = 0; i < 6; ++i)
            {
                const auto& band = settings.postEq[static_cast<size_t>(i)];
                p[static_cast<size_t>(i + 2)].c = design(std::clamp(band.type, 0, 4), band.hz, bounded(band.value, -18, 18), band.q, fs);
                p[static_cast<size_t>(i + 2)].enabled = band.enabled;
            }
        }
        designedSettings = settings; coefficientsDirty = false;
    }

    Pair tick(Pair input) noexcept
    {
        if (tickCounter == 0 && coefficientsDirty) updateCoefficients();
        if (++tickCounter == controlInterval) tickCounter = 0;
        const double k = fastK, slow = slowK;
        glide(inputFade, inputTarget, k); glide(freezeMix, freezeTarget, k, 1.0e-10);
        glide(predelay, predelayTarget, slow, 1.0e-6);
        glide(modDepth, modTarget * (1.0 - freezeMix), k, 1.0e-8);
        glide(earlyGain, earlyTarget, k); glide(lateGain, lateTarget, k); glide(width, widthTarget, k);
        glide(shapeMix, shapeTarget, k); glide(shimmer, shimmerTarget, k);
        const double inputGate = inputFade * (1.0 - freezeMix);
        Pair delayed {};
        for (int ch = 0; ch < 2; ++ch)
        {
            pre[static_cast<size_t>(ch)].push(input[static_cast<size_t>(ch)] * inputGate);
            delayed[static_cast<size_t>(ch)] = pre[static_cast<size_t>(ch)].linear(predelay);
            early[static_cast<size_t>(ch)].push(delayed[static_cast<size_t>(ch)]);
        }
        Pair er {};
        const int taps = engine == 5 ? 6 : engine == 4 ? 20 : 12;
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int j = 0; j < taps; ++j)
            {
                er[static_cast<size_t>(ch)] += earlyWeight[static_cast<size_t>(ch)][static_cast<size_t>(j)]
                    * early[static_cast<size_t>(ch)].linear(earlyDelay[static_cast<size_t>(ch)][static_cast<size_t>(j)]);
            }
            er[static_cast<size_t>(ch)] *= 0.22;
            // A separate early diffuser remains outside the lossless tank.
            er[static_cast<size_t>(ch)] = outputDiff[static_cast<size_t>(ch)][0].tick(er[static_cast<size_t>(ch)], 0.6 * earlyDiffusion);
        }
        Pair tankIn = delayed;
        for (int ch = 0; ch < 2; ++ch)
        {
            auto& b = buildup[static_cast<size_t>(ch)];
            b.push(tankIn[static_cast<size_t>(ch)]);
            if (engine == 6)
            {
                // Cascaded input arrivals produce a long density build-up,
                // independent of the diffusion and late decay controls.
                tankIn[static_cast<size_t>(ch)] = 0.10 * b.linear(0) + 0.24 * b.linear(fs * (0.035 + attack * 0.04))
                    + 0.40 * b.linear(fs * (0.085 + attack * 0.09)) + 0.55 * b.linear(fs * (0.16 + attack * 0.18));
            }
            else if (attack > 0)
                tankIn[static_cast<size_t>(ch)] = (1.0 - attack * 0.75) * b.linear(0)
                    + attack * 0.50 * b.linear(fs * 0.057 * size) + attack * 0.35 * b.linear(fs * 0.113 * size);
            if (engine == 2)
            {
                warmState[static_cast<size_t>(ch)] += warmK
                    * (tankIn[static_cast<size_t>(ch)] - warmState[static_cast<size_t>(ch)]);
                tankIn[static_cast<size_t>(ch)] = warmState[static_cast<size_t>(ch)];
            }
            const int diffusers = engine == 4 ? 8 : engine == 3 ? 8 : engine == 5 ? 4 : 6;
            for (int j = 0; j < diffusers; ++j)
                tankIn[static_cast<size_t>(ch)] = inputDiff[static_cast<size_t>(ch)][static_cast<size_t>(j)].tick(
                    tankIn[static_cast<size_t>(ch)], (j % 2 == 0 ? 1.0 : -1.0) * (0.42 + 0.30 * lateDiffusion));
        }
        std::array<double, linesCount> z {};
        Pair late {};
        for (int i = 0; i < linesCount; ++i)
        {
            auto& l = lines[static_cast<size_t>(i)];
            glide(l.length, freezeTarget > 0 ? l.frozenLength : l.lengthTarget, slow, 1.0e-5); glide(l.gain, l.gainTarget, k, 1.0e-12);
            if (freezeTarget < 1.0)
            {
                const double sn = l.phaseSin * l.rotateCos + l.phaseCos * l.rotateSin;
                const double cs = l.phaseCos * l.rotateCos - l.phaseSin * l.rotateSin;
                const double norm = tickCounter == 0 ? 1.5 - 0.5 * (sn * sn + cs * cs) : 1.0;
                l.phaseSin = sn * norm; l.phaseCos = cs * norm;
            }
            const double read = l.length - 1.0 + modDepth * l.phaseSin;
            double v = l.delay.cubic(std::max(1.0, read));
            if (engine == 2 || engine == 4) v = l.diffuseA.tick(v, (engine == 4 ? -0.5 : 0.47) * lateDiffusion);
            else if (engine == 3)
            {
                v = l.diffuseA.tick(v, 0.63 * lateDiffusion);
                v = l.diffuseB.tick(v, -0.53 * lateDiffusion);
            }
            double filtered = v;
            for (auto& filter : l.damping) filtered = filter.tick(filtered);
            // The frozen path bypasses all decay filters rather than retuning
            // poles in a live lossless loop. Its memory remains clocked.
            v = (1.0 - freezeMix) * l.gain * filtered + freezeMix * v;
            if (engine == 2 && freezeMix < 1.0) v /= 1.0 + 0.035 * (1.0 - freezeMix) * v * v;
            z[static_cast<size_t>(i)] = v;
            late[0] += ((i & 1) == 0 ? 1.0 : -1.0) * v * 0.25;
            late[1] += ((i & 2) == 0 ? 1.0 : -1.0) * v * 0.25;
        }
        if (engine == 2 || engine == 4) coupled(z);
        else if (engine == 5)
        {
            double sum = 0; for (double x : z) sum += x;
            for (auto& x : z) x -= sum * 0.125; // Householder, distinct sparse-room field.
        }
        else hadamard(z);
        for (int i = 0; i < linesCount; ++i)
        {
            const double sign = (i % 3 == 0) ? -1.0 : 1.0;
            const int ch = i & 1;
            // Regeneration is confined to PitchFeedback. Its bounded return
            // enters the tank proportional to the ordinary loop loss, avoiding
            // a new unstable feedback pole at long decay times.
            const double injection = tankIn[static_cast<size_t>(ch)] + shimmer * 0.5
                * (1.0 - lines[static_cast<size_t>(i)].gainTarget) * std::tanh(pitchLast[static_cast<size_t>(ch)]) * (1.0 - freezeMix);
            lines[static_cast<size_t>(i)].delay.push(z[static_cast<size_t>(i)] + sign * 0.25 * injection);
        }
        if (shimmer > 0) pitchLast = pitch.tick(late, pitchRatio, pitchFeedback * (1.0 - freezeMix), fs);
        else pitchLast = {};
        Pair output { earlyGain * er[0] + lateGain * (late[0] + shimmer * pitchLast[0]),
                      earlyGain * er[1] + lateGain * (late[1] + shimmer * pitchLast[1]) };

        const double level = std::max(std::abs(input[0]), std::abs(input[1]));
        peakEnvelope = std::max(level, peakEnvelope * peakRelease);
        if (peakEnvelope < threshold * 0.65) triggerArmed = true;
        if (triggerArmed && level >= threshold) { swellAge = 0; triggerArmed = false; }
        if (level >= threshold) gateHold = reverseLength;
        else if (gateHold > 0) --gateHold;
        const double gateTarget = gateHold > 0 ? 1.0 : 0.0;
        glide(gateGain, gateTarget, gateTarget > gateGain ? gateAttack : gateRelease, 1.0e-9);
        const Pair reversed = shape == 3 ? reverse.tick(output, reverseLength) : Pair {};
        double envelope = 1.0;
        if (shape == 1) envelope = gateGain;
        else if (shape == 2)
        {
            if (swellAge < 0) envelope = 0;
            else if (swellAge <= reverseLength)
            { const double phase = static_cast<double>(swellAge) / reverseLength; envelope = phase * phase; }
            else envelope = std::max(0.0, 1.0 - (swellAge - reverseLength) / (release * fs));
        }
        if (swellAge >= 0 && swellAge < reverseLength + static_cast<int>(release * fs) + 1) ++swellAge;
        for (int ch = 0; ch < 2; ++ch)
        {
            const double shaped = shape == 3 ? reversed[static_cast<size_t>(ch)] : output[static_cast<size_t>(ch)] * envelope;
            output[static_cast<size_t>(ch)] += shapeMix * (shaped - output[static_cast<size_t>(ch)]);
            for (auto& f : post[static_cast<size_t>(ch)]) output[static_cast<size_t>(ch)] = f.tick(output[static_cast<size_t>(ch)]);
        }
        const double mid = (output[0] + output[1]) * 0.5, side = (output[0] - output[1]) * 0.5 * width;
        output = { mid + side, mid - side };
        duckEnvelope += (level > duckEnvelope ? detectorAttack : detectorRelease) * (level - duckEnvelope);
        double target = 1.0;
        if (duckAmount > 0)
        {
            const double envDb = 20.0 * std::log10(duckEnvelope + 1.0e-12);
            target = dbGain(-duckAmount * 30.0 * std::clamp((envDb + 40.0) / 34.0, 0.0, 1.0));
        }
        glide(duckGain, target, target < duckGain ? duckAttack : duckRelease);
        if (tickCounter == 0) lastDuck = -20.0 * std::log10(std::max(1.0e-12, duckGain));
        for (auto& x : output) x *= duckGain;
        return output;
    }

    double energy() const noexcept
    {
        double sum = 0;
        for (const auto& l : lines)
        {
            sum += l.delay.recentEnergy(static_cast<int>(std::ceil(l.length)));
            if (engine == 2 || engine == 3 || engine == 4)
            {
                const double g = (engine == 3 ? 0.63 : engine == 4 ? -0.5 : 0.47) * lateDiffusion;
                sum += (1.0 - g * g) * l.diffuseA.delay.recentEnergy(l.diffuseA.length);
            }
            if (engine == 3)
            {
                const double g = -0.53 * lateDiffusion;
                sum += (1.0 - g * g) * l.diffuseB.delay.recentEnergy(l.diffuseB.length);
            }
        }
        return sum;
    }
};
} // namespace

struct VerbV2Processor::Impl
{
    std::array<Path, 2> paths;
    double fs = 48000.0;
    int active = 0, tail = -1, pendingEngine = 0;
    std::int64_t tailAge = 0, tailLimit = 0;
    int retireRemaining = 0, retireLength = 1;
    bool started = false;
    VerbV2Settings tailSettings;
    double tailBpm = 120.0;

    void reset() noexcept
    {
        for (auto& p : paths) p.clear();
        active = 0; tail = -1; pendingEngine = 0; tailAge = tailLimit = 0; retireRemaining = 0; started = false;
    }
    void handover(const VerbV2Settings& s, double bpm) noexcept
    {
        tail = active; tailSettings = paths[static_cast<size_t>(tail)].settings;
        tailSettings.base.values[slot::kRevFreeze] = 0; tailSettings.shimmerAmount = 0;
        tailBpm = paths[static_cast<size_t>(tail)].bpm;
        tailLimit = static_cast<std::int64_t>(std::min(120.0, VerbV2Processor::tailSeconds(tailSettings, tailBpm) * 1.4 + 0.1) * fs);
        tailAge = 0; active = 1 - active;
        auto& next = paths[static_cast<size_t>(active)]; next.clear(); next.configure(s.engine); next.set(s, bpm);
        paths[static_cast<size_t>(tail)].set(tailSettings, tailBpm, true);
        pendingEngine = 0; retireRemaining = 0;
    }
};

VerbV2Processor::VerbV2Processor() : impl(std::make_unique<Impl>()) {}
VerbV2Processor::~VerbV2Processor() = default;

void VerbV2Processor::prepare(double sampleRate, int maximumBlockSize)
{
    (void) maximumBlockSize;
    impl->fs = bounded(sampleRate, 8000, 384000, 48000);
    for (auto& p : impl->paths) p.prepare(impl->fs);
    impl->retireLength = std::max(1, static_cast<int>(std::lround(impl->fs * 0.05)));
    reset();
}

void VerbV2Processor::reset() noexcept
{
    impl->reset(); outputDb.store(-120.0f, std::memory_order_relaxed); duckDb.store(0.0f, std::memory_order_relaxed);
}

void VerbV2Processor::process(const float* const* input, float* const* output, int channels, int count,
                              const VerbV2Settings& s, const Transport& transport) noexcept
{
    if (count <= 0 || channels <= 0) return;
    channels = std::min(2, channels);
    auto& state = *impl;
    const int wanted = std::clamp(s.engine, 1, 6);
    if (!state.started)
    {
        state.paths[static_cast<size_t>(state.active)].configure(wanted);
        state.paths[static_cast<size_t>(state.active)].inputFade = 1.0;
        state.started = true;
    }
    auto& active = state.paths[static_cast<size_t>(state.active)];
    if (wanted != active.engine)
    {
        if (state.tail < 0) state.handover(s, transport.bpm);
        else
        {
            if (state.pendingEngine == 0) state.retireRemaining = state.retireLength;
            state.pendingEngine = wanted;
        }
    }
    else if (state.pendingEngine != 0) state.pendingEngine = 0;
    state.paths[static_cast<size_t>(state.active)].set(s, transport.bpm);
    double energySum = 0, duckSum = 0;
    for (int n = 0; n < count; ++n)
    {
        if (state.tail >= 0 && state.retireRemaining == 0 && state.pendingEngine != 0)
        {
            state.tail = -1;
            state.handover(s, transport.bpm);
        }
        Pair in { input[0][n], channels > 1 ? input[1][n] : input[0][n] };
        auto& current = state.paths[static_cast<size_t>(state.active)];
        Pair wet = current.tick(in); duckSum += current.lastDuck;
        if (state.tail >= 0)
        {
            const auto old = state.paths[static_cast<size_t>(state.tail)].tick({ 0, 0 });
            double gain = 1.0;
            const bool retiring = state.retireRemaining > 0;
            if (state.retireRemaining > 0) gain = static_cast<double>(state.retireRemaining--) / state.retireLength;
            else if (state.tailAge > state.tailLimit - state.retireLength)
                gain = std::max(0.0, static_cast<double>(state.tailLimit - state.tailAge) / state.retireLength);
            wet[0] += gain * old[0]; wet[1] += gain * old[1];
            if (++state.tailAge >= state.tailLimit) state.tail = -1;
            if (retiring && state.retireRemaining == 0 && state.pendingEngine == 0) state.tail = -1;
        }
        if (channels == 1)
        {
            output[0][n] = static_cast<float>((wet[0] + wet[1]) * 0.5);
            energySum += static_cast<double>(output[0][n]) * output[0][n];
        }
        else
            for (int ch = 0; ch < 2; ++ch)
            {
                output[ch][n] = static_cast<float>(wet[static_cast<size_t>(ch)]);
                energySum += wet[static_cast<size_t>(ch)] * wet[static_cast<size_t>(ch)];
            }
    }
    outputDb.store(static_cast<float>(10.0 * std::log10(energySum / (count * channels) + 1.0e-12)), std::memory_order_relaxed);
    duckDb.store(static_cast<float>(duckSum / count), std::memory_order_relaxed);
}

double VerbV2Processor::energy() const noexcept
{
    double e = impl->paths[static_cast<size_t>(impl->active)].energy();
    if (impl->tail >= 0) e += impl->paths[static_cast<size_t>(impl->tail)].energy();
    return e;
}

double VerbV2Processor::decaySecondsAt(const VerbV2Settings& s, double hz) noexcept
{
    const double t = bounded(s.base.values[slot::kDecaySeconds], 0.1, 30, 2.4);
    hz = bounded(hz, 1, 192000, 1000);
    const double lowHz = bounded(s.base.values[slot::kRevBassXover], 50, 2000, 300);
    const double highHz = bounded(s.base.values[slot::kRevHighXover], 1000, 16000, 4000);
    double logRatio = std::log2(bounded(s.base.values[slot::kRevBassMult], 0.25, 4, 1)) / (1.0 + std::pow(hz / lowHz, 2.0))
        + std::log2(bounded(s.base.values[slot::kHfRatio], 0.05, 1, 1)) / (1.0 + std::pow(highHz / hz, 2.0));
    for (const auto& band : s.decayEq)
    {
        if (!band.enabled) continue;
        const double centre = bounded(band.hz, 10, 20000, 1000), q = bounded(band.q, 0.15, 8, 0.707);
        const double ratio = hz / centre;
        const double shape = band.type == 1 ? 1.0 / (1.0 + ratio * ratio)
                           : band.type == 2 ? 1.0 / (1.0 + 1.0 / (ratio * ratio))
                           : 1.0 / (1.0 + std::pow(std::log2(ratio) * q, 2.0));
        logRatio += bounded(band.value, -2, 2) * shape;
    }
    return std::clamp(t * std::exp2(logRatio), 0.1, 120.0);
}

double VerbV2Processor::postEqMagnitude(const VerbV2Settings& s, double hz, double sampleRate) noexcept
{
    const double fs = bounded(sampleRate, 8000, 384000, 48000);
    hz = bounded(hz, 1, fs * 0.499, 1000);
    double result = magnitude(design(3, s.base.values[slot::kLowCutHz], 0, 0.7071067811865476, fs), hz, fs);
    if (s.base.values[slot::kRevHighCut] < std::min(19900.0, fs * 0.45))
        result *= magnitude(design(4, s.base.values[slot::kRevHighCut], 0, 0.7071067811865476, fs), hz, fs);
    for (const auto& band : s.postEq) if (band.enabled)
        result *= magnitude(design(std::clamp(band.type, 0, 4), band.hz, bounded(band.value, -18, 18), band.q, fs), hz, fs);
    return result;
}

double VerbV2Processor::tailSeconds(const VerbV2Settings& s, double bpm) noexcept
{
    if (s.base.values[slot::kRevFreeze] >= 0.5f) return std::numeric_limits<double>::infinity();
    double longest = 0.1;
    for (int k = 0; k <= 48; ++k) longest = std::max(longest, decaySecondsAt(s, 10.0 * std::pow(2000.0, k / 48.0)));
    if (s.shimmerAmount > 0) longest = std::max(longest, 2.0 + 6.0 * bounded(s.shimmerFeedback, 0, 0.85));
    return predelaySeconds(s, bpm) + longest + (s.engine == 6 ? 0.6 : 0.2)
        + (s.timeShape >= 2 ? shapeSeconds(s, bpm) * 2.0 : 0.0);
}
} // namespace hpeq::dsp
