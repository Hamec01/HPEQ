#include "DSP/Limiter.h"

#include <algorithm>
#include <cmath>

namespace hamlo::dsp
{
namespace
{
double coefficientFor(double ms, double sampleRate) noexcept
{
    return std::exp(-1.0 / (std::max(0.01, ms) * 1.0e-3 * sampleRate));
}

float linkedPeak(float* const* channels, int numChannels, int i) noexcept
{
    float level = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        level = std::max(level, std::abs(channels[ch][i]));
    return level;
}
} // namespace

int LimiterProcessor::maxLookaheadSamples(double sampleRate) noexcept
{
    return std::clamp(static_cast<int>(std::ceil(kMaxLookaheadMs * 1.0e-3 * sampleRate)) + 1, 2, kMaxLookahead);
}

int LimiterProcessor::baseLookaheadSamples(const LimiterSettings& p, double sampleRate) noexcept
{
    const double ms = std::clamp(static_cast<double>(p.values[slot::kLookaheadMs]), 0.1, kMaxLookaheadMs);
    return std::clamp(static_cast<int>(std::lround(ms * 1.0e-3 * sampleRate)), 1, maxLookaheadSamples(sampleRate));
}

int LimiterProcessor::lookaheadSamples(const LimiterSettings& p, double sampleRate) noexcept
{
    return baseLookaheadSamples(p, sampleRate) + (p.values[slot::kLimTruePeak] > 0.5f ? RedWallCore::kTruePeakDelay : 0);
}

bool LimiterProcessor::usesRedWall(const LimiterSettings& p) noexcept
{
    return p.values[slot::kLimThresholdDb] != 0.0f || p.values[slot::kLimTruePeak] > 0.5f || p.values[slot::kLimAutoRelease] > 0.5f
        || std::lround(p.values[slot::kLimStyle]) != 0 || p.values[slot::kLimLink] != 1.0f || std::lround(p.values[slot::kLimMonitor]) != 0
        || p.values[slot::kLimOutputDb] != 0.0f;
}

void LimiterProcessor::prepare(double sampleRate)
{
    sampleRate_ = sampleRate;
    const int maxL = maxLookaheadSamples(sampleRate);
    for (auto& a : audio)
        a.assign(static_cast<size_t>(maxL + 1), 0.0f);
    averaged.assign(static_cast<size_t>(maxL + 1), 1.0);
    capacity = maxL + 2;
    dequeValue.assign(static_cast<size_t>(capacity), 1.0f);
    dequeIndex.assign(static_cast<size_t>(capacity), 0);
    lookahead = -1;
    core.prepare(sampleRate, maxL);
    redWallActive = false;
    reset();
}

void LimiterProcessor::reset() noexcept
{
    for (auto& a : audio)
        std::fill(a.begin(), a.end(), 0.0f);
    std::fill(averaged.begin(), averaged.end(), 1.0);
    dequeHead = dequeTail = 0;
    counter = 0;
    writePos = 0;
    released = 1.0;
    runningSum = lookahead > 0 ? static_cast<double>(lookahead) : 0.0;
    meter.store(0.0f, std::memory_order_relaxed);
    meterOutput.store(-120.0f, std::memory_order_relaxed);
    core.reset();
}

void LimiterProcessor::process(float* const* channels, int numChannels, int numSamples, const LimiterSettings& p) noexcept
{
    const bool wantRedWall = usesRedWall(p);
    if (wantRedWall != redWallActive)
    {
        redWallActive = wantRedWall; // switching between the two paths starts clean
        lookahead = -1;
        reset();
    }
    if (redWallActive)
    {
        core.process(channels, numChannels, numSamples, p, baseLookaheadSamples(p, sampleRate_));
        return;
    }
    if (averaged.empty())
        return; // not prepared
    const int l = lookaheadSamples(p, sampleRate_);
    if (l != lookahead)
    {
        lookahead = l;
        reset();
    }
    const float ceiling = std::pow(10.0f, std::clamp(p.values[slot::kCeilingDb], -60.0f, 0.0f) / 20.0f);
    const double releaseCoefficient = coefficientFor(std::max(1.0f, p.values[slot::kLimiterReleaseMs]), sampleRate_);
    const int ring = lookahead + 1;
    float minGain = 1.0f, outPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float level = linkedPeak(channels, numChannels, i);
        const float target = level > ceiling ? ceiling / level : 1.0f;

        // Sliding minimum over the last L+1 targets (monotonic deque in fixed arrays).
        while (dequeTail != dequeHead && dequeValue[static_cast<size_t>((dequeTail - 1 + capacity) % capacity)] >= target)
            dequeTail = (dequeTail - 1 + capacity) % capacity;
        dequeIndex[static_cast<size_t>(dequeTail)] = counter;
        dequeValue[static_cast<size_t>(dequeTail)] = target;
        dequeTail = (dequeTail + 1) % capacity;
        while (counter - dequeIndex[static_cast<size_t>(dequeHead)] > lookahead) // older than L samples
            dequeHead = (dequeHead + 1) % capacity;
        const double held = dequeValue[static_cast<size_t>(dequeHead)];

        // Release on rising gain only; never above the held minimum.
        released = held < released ? held : released + (held - released) * (1.0 - releaseCoefficient);

        // Moving average over the last L values.
        const int avgPos = static_cast<int>(counter % lookahead);
        runningSum += released - averaged[static_cast<size_t>(avgPos)];
        averaged[static_cast<size_t>(avgPos)] = released;
        if (avgPos == 0 && counter % (1LL << 16) == 0)
        {
            runningSum = 0.0; // re-sum now and then: no slow floating-point drift
            for (int k = 0; k < lookahead; ++k)
                runningSum += averaged[static_cast<size_t>(k)];
        }
        const float gain = static_cast<float>(std::min(1.0, runningSum / lookahead));
        minGain = std::min(minGain, gain);

        // Delay the audio by L and apply the gain.
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& a = audio[static_cast<size_t>(ch)];
            const float delayed = a[static_cast<size_t>((writePos + 1) % ring)];
            a[static_cast<size_t>(writePos)] = channels[ch][i];
            channels[ch][i] = delayed * gain;
            outPeak = std::max(outPeak, std::abs(channels[ch][i]));
        }
        writePos = (writePos + 1) % ring;
        ++counter;
    }
    meter.store(-20.0f * std::log10(std::max(minGain, 1.0e-6f)), std::memory_order_relaxed);
    meterOutput.store(outPeak > 0.0f ? 20.0f * std::log10(outPeak) : -120.0f, std::memory_order_relaxed);
}
} // namespace hamlo::dsp
