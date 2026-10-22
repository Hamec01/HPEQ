#include "DSP/Processors.h"

#include <algorithm>
#include <cmath>

namespace hpeq::dsp
{
void GainProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    const float gainDb = p.values[slot::kGainDb];
    if (gainDb == 0.0f)
        return;
    const float g = std::pow(10.0f, gainDb / 20.0f);
    for (int ch = 0; ch < numChannels; ++ch)
        for (int i = 0; i < numSamples; ++i)
            channels[ch][i] *= g;
}

void CompressorProcessor::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    reset();
}

void CompressorProcessor::reset() noexcept
{
    smoothedReductionDb = {};
    meanSquare = {};
    hold = {};
    autoMakeupDb = {};
    meterGainReductionDb.store(0.0f, std::memory_order_relaxed);
    meterInputDb.store(-120.0f, std::memory_order_relaxed);
    meterAutoMakeupDb.store(0.0f, std::memory_order_relaxed);
}

double CompressorProcessor::staticGainReductionDb(double x, double t, double ratio, double kneeDb) noexcept
{
    const double slope = 1.0 / ratio - 1.0; // <= 0
    const double over = x - t;
    if (kneeDb > 0.0 && std::abs(over) <= kneeDb * 0.5)
    {
        const double a = over + kneeDb * 0.5;
        return -slope * a * a / (2.0 * kneeDb);
    }
    return over > 0.0 ? -slope * over : 0.0;
}

void CompressorProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p, bool linked) noexcept
{
    // Same law and ballistics as documented in the header; the work is split into loops the compiler
    // can vectorise (level, dB conversion, gain computer, dB -> gain), around the one inherently
    // serial step (the attack/release recursion, kept in double).
    const float threshold = p.values[slot::kThresholdDb];
    const float ratio = std::max(1.0f, p.values[slot::kRatio]);
    const float knee = std::max(0.0f, p.values[slot::kKneeDb]);
    const double makeup = p.values[slot::kMakeupDb];
    const double attack = std::exp(-1.0 / (std::max(0.05f, p.values[slot::kAttackMs]) * 1.0e-3 * sampleRate));
    const double release = std::exp(-1.0 / (std::max(1.0f, p.values[slot::kReleaseMs]) * 1.0e-3 * sampleRate));
    const float kneeFloorLinear = std::pow(10.0f, (threshold - knee * 0.5f) / 20.0f);
    const float slope = 1.0f / ratio - 1.0f;
    const float halfKnee = knee * 0.5f;
    const float kneeScale = knee > 0.0f ? -slope / (2.0f * knee) : 0.0f;
    constexpr float kDbToLog2 = 0.16609640474436813f; // log2(10) / 20
    const int detectors = linked ? 1 : numChannels;
    const bool rms = p.values[slot::kCompDetector] > 0.5f;
    const int character = static_cast<int>(std::lround(std::clamp(p.values[slot::kCompCharacter], 0.0f, 2.0f)));
    const bool autoMakeup = p.values[slot::kCompAutoMakeup] > 0.5f;
    const double rmsCoefficient = std::exp(-1.0 / (0.010 * sampleRate));
    const double holdCoefficient = std::exp(-1.0 / (1.0 * sampleRate));
    const double autoCoefficient = std::exp(-1.0 / (2.0 * sampleRate));
    const double releaseFast = std::exp(-1.0 / (std::max(1.0f, p.values[slot::kReleaseMs]) * 0.25e-3 * sampleRate));
    const double releaseSlow = std::exp(-1.0 / (std::max(1.0f, p.values[slot::kReleaseMs]) * 2.0e-3 * sampleRate));

    double peakReduction = 0.0;
    float peakLevel = 0.0f;
    const float punchScale = std::pow(10.0f, threshold / 20.0f);
    for (int start = 0; start < numSamples; start += kChunk)
    {
        const int n = std::min(kChunk, numSamples - start);
        for (int d = 0; d < detectors; ++d)
        {
            // 1. detector level (sample peak; linked: max over channels)
            if (linked)
            {
                std::fill(level.begin(), level.begin() + n, 0.0f);
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const float* x = channels[ch] + start;
                    for (int i = 0; i < n; ++i)
                        level[static_cast<size_t>(i)] = std::max(level[static_cast<size_t>(i)], std::abs(x[i]));
                }
            }
            else
            {
                const float* x = channels[d] + start;
                for (int i = 0; i < n; ++i)
                    level[static_cast<size_t>(i)] = std::abs(x[i]);
            }
            if (rms)
            {
                // RMS: one-pole mean square of the (linked) peak square, serial.
                auto& m = meanSquare[static_cast<size_t>(d)];
                for (int i = 0; i < n; ++i)
                {
                    const double l = level[static_cast<size_t>(i)];
                    m = l * l + rmsCoefficient * (m - l * l);
                    level[static_cast<size_t>(i)] = static_cast<float>(std::sqrt(m));
                }
            }
            for (int i = 0; i < n; ++i)
                peakLevel = std::max(peakLevel, level[static_cast<size_t>(i)]);

            // 2. static gain computer in dB (0 below the knee, exactly)
            for (int i = 0; i < n; ++i)
            {
                const float l = level[static_cast<size_t>(i)];
                const float xDb = 20.0f * std::log10(std::max(l, 1.0e-30f));
                const float over = xDb - threshold;
                const float a = over + halfKnee;
                const float inKnee = kneeScale * a * a;
                const float above = -slope * over;
                const float reduction = (knee > 0.0f && std::abs(over) <= halfKnee) ? inKnee : (over > 0.0f ? above : 0.0f);
                target[static_cast<size_t>(i)] = l > kneeFloorLinear ? reduction : 0.0f;
            }

            // 3. branching attack/release recursion in dB (serial)
            auto& smoothed = smoothedReductionDb[static_cast<size_t>(d)];
            auto& held = hold[static_cast<size_t>(d)];
            auto& autoDb = autoMakeupDb[static_cast<size_t>(d)];
            for (int i = 0; i < n; ++i)
            {
                const double t = target[static_cast<size_t>(i)];
                double releaseNow = release;
                if (character == 2)
                {
                    // Glue: program-dependent release (see the header).
                    held = (smoothed > 1.0 ? 1.0 : 0.0) + holdCoefficient * (held - (smoothed > 1.0 ? 1.0 : 0.0));
                    releaseNow = releaseFast + (releaseSlow - releaseFast) * held;
                }
                smoothed = t + (t > smoothed ? attack : releaseNow) * (smoothed - t);
                if (smoothed < 1.0e-9)
                    smoothed = 0.0; // settle exactly, so a recovered compressor is an identity again
                peakReduction = std::max(peakReduction, smoothed);
                reductionDb[static_cast<size_t>(i)] = static_cast<float>(smoothed);
                if (autoMakeup)
                {
                    autoDb = smoothed + autoCoefficient * (autoDb - smoothed);
                    gainDb[static_cast<size_t>(i)] = static_cast<float>(makeup + autoDb - smoothed);
                }
                else
                    gainDb[static_cast<size_t>(i)] = static_cast<float>(makeup - smoothed);
            }

            // 4. dB -> linear gain; 0 dB stays an exact 1
            for (int i = 0; i < n; ++i)
            {
                const float g = gainDb[static_cast<size_t>(i)];
                gain[static_cast<size_t>(i)] = g == 0.0f ? 1.0f : std::exp2(g * kDbToLog2);
            }

            // 5. apply
            for (int ch = linked ? 0 : d; ch < (linked ? numChannels : d + 1); ++ch)
            {
                float* x = channels[ch] + start;
                if (character == 1)
                    for (int i = 0; i < n; ++i)
                    {
                        // Punch: saturation relative to the threshold, before the gain, growing with the
                        // gain reduction (see the header).
                        const float m = 0.35f * std::min(1.0f, reductionDb[static_cast<size_t>(i)] * (1.0f / 12.0f));
                        if (m > 0.0f)
                            x[i] += m * (punchScale * std::tanh(x[i] / punchScale) - x[i]);
                    }
                for (int i = 0; i < n; ++i)
                    x[i] *= gain[static_cast<size_t>(i)];
            }
        }
    }
    meterGainReductionDb.store(static_cast<float>(peakReduction), std::memory_order_relaxed);
    meterInputDb.store(peakLevel > 0.0f ? 20.0f * std::log10(peakLevel) : -120.0f, std::memory_order_relaxed);
    meterAutoMakeupDb.store(static_cast<float>(autoMakeup ? autoMakeupDb[0] : 0.0), std::memory_order_relaxed);
}
} // namespace hpeq::dsp
