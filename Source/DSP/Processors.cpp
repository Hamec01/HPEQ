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
    smoothedReductionDb = 0.0;
    meterGainReductionDb.store(0.0f, std::memory_order_relaxed);
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

void CompressorProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    const double threshold = p.values[slot::kThresholdDb];
    const double ratio = std::max(1.0f, p.values[slot::kRatio]);
    const double knee = std::max(0.0f, p.values[slot::kKneeDb]);
    const double makeup = p.values[slot::kMakeupDb];
    const double attack = std::exp(-1.0 / (std::max(0.05f, p.values[slot::kAttackMs]) * 1.0e-3 * sampleRate));
    const double release = std::exp(-1.0 / (std::max(1.0f, p.values[slot::kReleaseMs]) * 1.0e-3 * sampleRate));
    // Below the knee the computer returns exactly 0 dB; skip the log for such quiet samples.
    const double kneeFloorDb = threshold - knee * 0.5;
    const double kneeFloorLinear = std::pow(10.0, kneeFloorDb / 20.0);

    double peakReduction = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        double level = 0.0;
        for (int ch = 0; ch < numChannels; ++ch)
            level = std::max(level, static_cast<double>(std::abs(channels[ch][i])));

        const double target = level > kneeFloorLinear
            ? staticGainReductionDb(20.0 * std::log10(level), threshold, ratio, knee)
            : 0.0;
        const double coefficient = target > smoothedReductionDb ? attack : release;
        smoothedReductionDb = target + coefficient * (smoothedReductionDb - target);
        if (smoothedReductionDb < 1.0e-9)
            smoothedReductionDb = 0.0; // settle exactly, so a recovered compressor is an identity again
        peakReduction = std::max(peakReduction, smoothedReductionDb);

        const double gainDb = makeup - smoothedReductionDb;
        if (gainDb != 0.0)
        {
            const auto g = static_cast<float>(std::pow(10.0, gainDb / 20.0));
            for (int ch = 0; ch < numChannels; ++ch)
                channels[ch][i] *= g;
        }
    }
    meterGainReductionDb.store(static_cast<float>(peakReduction), std::memory_order_relaxed);
}
} // namespace hpeq::dsp
