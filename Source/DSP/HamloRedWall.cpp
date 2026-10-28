#include "DSP/HamloRedWall.h"

#include <algorithm>
#include <cmath>

namespace hamlo::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

double besselI0(double x) noexcept
{
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 40; ++k)
    {
        term *= (x / (2.0 * k)) * (x / (2.0 * k));
        sum += term;
    }
    return sum;
}

double coefficientFor(double ms, double sampleRate) noexcept
{
    return std::exp(-1.0 / (std::max(0.01, ms) * 1.0e-3 * sampleRate));
}
} // namespace

void RedWallCore::prepare(double newSampleRate, int maxLookahead)
{
    sampleRate = newSampleRate;
    capacity = maxLookahead + kTruePeakDelay + 4;
    for (auto& c : ch)
    {
        c.preAudio.assign(static_cast<size_t>(capacity), 0.0f);
        c.preLevel.assign(static_cast<size_t>(capacity), 0.0f);
        c.audio.assign(static_cast<size_t>(capacity), 0.0f);
        c.averaged.assign(static_cast<size_t>(capacity), 1.0);
        c.dequeValue.assign(static_cast<size_t>(capacity + 1), 1.0f);
        c.dequeIndex.assign(static_cast<size_t>(capacity + 1), 0);
    }
    // Interpolation between history[7] and history[8] at fractions p / 4 (centre 7 + p / 4).
    for (int p = 1; p <= 3; ++p)
    {
        const double frac = p / 4.0, fc = 0.5, beta = 9.0;
        double sum = 0.0;
        std::array<double, 16> taps {};
        for (int k = 0; k < 16; ++k)
        {
            const double d = k - (7.0 + frac);
            const double x = 2.0 * fc * d;
            const double sinc = std::abs(x) < 1.0e-12 ? 1.0 : std::sin(kPi * x) / (kPi * x);
            const double r = d / 8.5;
            const double w = std::abs(r) < 1.0 ? besselI0(beta * std::sqrt(1.0 - r * r)) / besselI0(beta) : 0.0;
            taps[static_cast<size_t>(k)] = 2.0 * fc * sinc * w;
            sum += taps[static_cast<size_t>(k)];
        }
        for (int k = 0; k < 16; ++k)
            phaseTaps[static_cast<size_t>(p - 1)][static_cast<size_t>(k)] = static_cast<float>(taps[static_cast<size_t>(k)] / sum);
    }
    for (int p = 1; p <= 7; ++p) // v2: the same design on an 8x grid
    {
        const double frac = p / 8.0, fc = 0.5, beta = 9.0;
        double sum = 0.0;
        std::array<double, 16> taps {};
        for (int k = 0; k < 16; ++k)
        {
            const double d = k - (7.0 + frac);
            const double x = 2.0 * fc * d;
            const double sinc = std::abs(x) < 1.0e-12 ? 1.0 : std::sin(kPi * x) / (kPi * x);
            const double r = d / 8.5;
            const double w = std::abs(r) < 1.0 ? besselI0(beta * std::sqrt(1.0 - r * r)) / besselI0(beta) : 0.0;
            taps[static_cast<size_t>(k)] = 2.0 * fc * sinc * w;
            sum += taps[static_cast<size_t>(k)];
        }
        for (int k = 0; k < 16; ++k)
            phaseTaps8[static_cast<size_t>(p - 1)][static_cast<size_t>(k)] = static_cast<float>(taps[static_cast<size_t>(k)] / sum);
    }
    coreLookahead = preDelay = -1;
    reset();
}

void RedWallCore::reset() noexcept
{
    for (auto& c : ch)
    {
        c.history = {};
        c.previousInterSample = 0.0f;
        std::fill(c.preAudio.begin(), c.preAudio.end(), 0.0f);
        std::fill(c.preLevel.begin(), c.preLevel.end(), 0.0f);
        std::fill(c.audio.begin(), c.audio.end(), 0.0f);
        std::fill(c.averaged.begin(), c.averaged.end(), 1.0);
        c.dequeHead = c.dequeTail = 0;
        c.released = 1.0;
        c.hold = 0.0;
        c.runningSum = coreLookahead > 0 ? static_cast<double>(coreLookahead) : 0.0;
    }
    counter = 0;
    prePos = corePos = 0;
    meterReduction.store(0.0f, std::memory_order_relaxed);
    meterOutput.store(-120.0f, std::memory_order_relaxed);
}

void RedWallCore::process(float* const* channels, int numChannels, int numSamples, const LimiterSettings& p, int lookahead) noexcept
{
    if (capacity <= 0)
        return;
    numChannels = std::min(numChannels, kMaxChannels);
    const int style = static_cast<int>(std::lround(std::clamp(p.values[slot::kLimStyle], 0.0f, 2.0f)));
    const bool tp = p.values[slot::kLimTruePeak] > 0.5f;
    const int le = style == 1 ? std::max(1, lookahead / 4) : std::max(1, lookahead);
    const int pre = std::max(0, lookahead - le);
    if (le != coreLookahead || pre != preDelay || tp != truePeak)
    {
        coreLookahead = le;
        preDelay = pre;
        truePeak = tp;
        reset();
    }

    const float ceiling = std::pow(10.0f, std::clamp(p.values[slot::kCeilingDb], -60.0f, 0.0f) / 20.0f);
    const float thresholdDb = std::clamp(p.values[slot::kLimThresholdDb], -30.0f, 0.0f);
    const float push = std::pow(10.0f, -thresholdDb / 20.0f);
    const float clipScale = 1.25f * ceiling;
    const double link = std::clamp(static_cast<double>(p.values[slot::kLimLink]), 0.0, 1.0);
    const bool autoRelease = p.values[slot::kLimAutoRelease] > 0.5f;
    const int monitor = static_cast<int>(std::lround(std::clamp(p.values[slot::kLimMonitor], 0.0f, 2.0f)));
    const double releaseMs = std::max(1.0f, p.values[slot::kLimiterReleaseMs]) * (style == 1 ? 0.5 : 1.0);
    const double releaseManual = coefficientFor(releaseMs, sampleRate);
    const double releaseFast = coefficientFor(releaseMs * 0.25, sampleRate), releaseSlow = coefficientFor(releaseMs * 6.0, sampleRate);
    const double holdCoefficient = std::exp(-1.0 / (0.3 * sampleRate));
    const bool v2 = p.values[slot::kLimEngine] > 0.5f;
    const float unity = std::pow(10.0f, thresholdDb / 20.0f);
    const float outputGain = std::pow(10.0f, std::clamp(p.values[slot::kLimOutputDb], -24.0f, 0.0f) / 20.0f);
    const int ring = le + 1, dequeCapacity = capacity + 1;

    float minGain = 1.0f, outPeak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        std::array<float, kMaxChannels> v {}, level {}, target {};
        for (int c = 0; c < numChannels; ++c)
        {
            auto& s = ch[static_cast<size_t>(c)];
            float u = channels[c][i] * push;
            if (style == 2)
                u = clipScale * std::tanh(u / clipScale);
            if (truePeak)
            {
                std::copy(s.history.begin() + 1, s.history.end(), s.history.begin());
                s.history[15] = u;
                float isp = 0.0f;
                const auto interpolate = [&](const auto& phases)
                {
                    for (const auto& taps : phases)
                    {
                        float y = 0.0f;
                        for (int k = 0; k < 16; ++k)
                            y += taps[static_cast<size_t>(k)] * s.history[static_cast<size_t>(k)];
                        isp = std::max(isp, std::abs(y));
                    }
                };
                if (v2)
                    interpolate(phaseTaps8);
                else
                    interpolate(phaseTaps);
                v[static_cast<size_t>(c)] = s.history[7];
                level[static_cast<size_t>(c)] = std::max({ std::abs(s.history[7]), isp, s.previousInterSample });
                s.previousInterSample = isp; // the points between history[7] and [8] are the next sample's left side
            }
            else
            {
                v[static_cast<size_t>(c)] = u;
                level[static_cast<size_t>(c)] = std::abs(u);
            }
            if (preDelay > 0)
            {
                const int size = preDelay + 1;
                const int read = (prePos + 1) % size;
                const float a = s.preAudio[static_cast<size_t>(read)], l = s.preLevel[static_cast<size_t>(read)];
                s.preAudio[static_cast<size_t>(prePos)] = v[static_cast<size_t>(c)];
                s.preLevel[static_cast<size_t>(prePos)] = level[static_cast<size_t>(c)];
                v[static_cast<size_t>(c)] = a;
                level[static_cast<size_t>(c)] = l;
            }
            target[static_cast<size_t>(c)] = level[static_cast<size_t>(c)] > ceiling ? ceiling / level[static_cast<size_t>(c)] : 1.0f;
        }
        if (preDelay > 0)
            prePos = (prePos + 1) % (preDelay + 1);
        float linkedTarget = 1.0f;
        for (int c = 0; c < numChannels; ++c)
            linkedTarget = std::min(linkedTarget, target[static_cast<size_t>(c)]);

        for (int c = 0; c < numChannels; ++c)
        {
            auto& s = ch[static_cast<size_t>(c)];
            const float t = static_cast<float>(link * linkedTarget + (1.0 - link) * target[static_cast<size_t>(c)]);
            // sliding minimum over the last Le + 1 targets
            while (s.dequeTail != s.dequeHead && s.dequeValue[static_cast<size_t>((s.dequeTail - 1 + dequeCapacity) % dequeCapacity)] >= t)
                s.dequeTail = (s.dequeTail - 1 + dequeCapacity) % dequeCapacity;
            s.dequeIndex[static_cast<size_t>(s.dequeTail)] = counter;
            s.dequeValue[static_cast<size_t>(s.dequeTail)] = t;
            s.dequeTail = (s.dequeTail + 1) % dequeCapacity;
            while (counter - s.dequeIndex[static_cast<size_t>(s.dequeHead)] > le)
                s.dequeHead = (s.dequeHead + 1) % dequeCapacity;
            const double held = s.dequeValue[static_cast<size_t>(s.dequeHead)];

            double coefficient = releaseManual;
            if (autoRelease)
            {
                const double reducing = s.released < 0.8912509381337456 ? 1.0 : 0.0; // > 1 dB
                s.hold = reducing + holdCoefficient * (s.hold - reducing);
                coefficient = releaseFast + (releaseSlow - releaseFast) * s.hold;
            }
            s.released = held < s.released ? held : s.released + (held - s.released) * (1.0 - coefficient);

            const int avgPos = static_cast<int>(counter % le);
            s.runningSum += s.released - s.averaged[static_cast<size_t>(avgPos)];
            s.averaged[static_cast<size_t>(avgPos)] = s.released;
            if (avgPos == 0 && counter % (1LL << 16) == 0)
            {
                s.runningSum = 0.0;
                for (int k = 0; k < le; ++k)
                    s.runningSum += s.averaged[static_cast<size_t>(k)];
            }
            const float gain = static_cast<float>(std::min(1.0, s.runningSum / le));
            minGain = std::min(minGain, gain);

            const float delayed = s.audio[static_cast<size_t>((corePos + 1) % ring)];
            s.audio[static_cast<size_t>(corePos)] = v[static_cast<size_t>(c)];
            float y = delayed * gain;
            if (monitor == 1)
                y *= unity;
            else if (monitor == 2)
                y = delayed - y;
            if (outputGain != 1.0f)
                y *= outputGain;
            outPeak = std::max(outPeak, std::abs(y));
            channels[c][i] = y;
        }
        corePos = (corePos + 1) % ring;
        ++counter;
    }
    meterReduction.store(-20.0f * std::log10(std::max(minGain, 1.0e-6f)), std::memory_order_relaxed);
    meterOutput.store(outPeak > 0.0f ? 20.0f * std::log10(outPeak) : -120.0f, std::memory_order_relaxed);
}
} // namespace hamlo::dsp
