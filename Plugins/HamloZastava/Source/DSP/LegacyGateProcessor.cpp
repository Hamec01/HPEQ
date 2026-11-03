#include "DSP/LegacyGateProcessor.h"
#include <algorithm>
#include <cmath>
namespace hpeq::dsp
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
}
bool LegacyGateProcessor::usesZastava(const FxSlotParams& p) noexcept
{
    for (int k : { slot::kGateStyle, slot::kGateLookahead, slot::kGateKeyHpHz, slot::kGateKeyLpHz, slot::kGateKeyListen, slot::kGateFlip, slot::kGateOutputDb })
        if (p.values[static_cast<size_t>(k)] != 0.0f)
            return true;
    return false;
}

int LegacyGateProcessor::lookaheadSamples(const FxSlotParams& p, double sampleRate) noexcept
{
    return usesZastava(p) ? ZastavaCore::lookaheadSamples(static_cast<int>(std::lround(p.values[slot::kGateLookahead])), sampleRate) : 0;
}

void LegacyGateProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    if (usesZastava(p))
    {
        if (!coreActive)
        {
            core.reset();
            coreActive = true;
        }
        core.process(channels, numChannels, numSamples, p);
        return;
    }
    if (coreActive)
    {
        coreActive = false;
        open = false;
        holdLeft = 0;
        attenuationDb = detector = 0.0;
    }
    const double threshold = std::clamp(p.values[slot::kGateThresholdDb], -90.0f, 0.0f);
    const double ratio = std::clamp(p.values[slot::kGateRatio], 1.0f, 100.0f);
    const double range = std::clamp(p.values[slot::kGateRangeDb], 0.0f, 90.0f);
    const double attack = coefficientFor(p.values[slot::kGateAttackMs], sampleRate_);
    const double release = coefficientFor(p.values[slot::kGateReleaseMs], sampleRate_);
    const int hold = static_cast<int>(std::max(0.0f, p.values[slot::kGateHoldMs]) * 1.0e-3 * sampleRate_);
    const double hysteresis = std::clamp(p.values[slot::kGateHysteresisDb], 0.0f, 24.0f);
    const double detectorRelease = coefficientFor(kDetectorReleaseMs, sampleRate_);
    for (int i = 0; i < numSamples; ++i)
    {
        // Peak detector: instant attack, 10 ms release, so zero crossings of a tone are not "silence".
        const double level = linkedPeak(channels, numChannels, i);
        detector = std::max(level, detector * detectorRelease);
        const double x = 20.0 * std::log10(detector + 1.0e-12);
        const bool wasOpen = open;
        if (x > threshold)
        {
            open = true;
            holdLeft = hold;
        }
        else if (x < threshold - hysteresis)
        {
            if (holdLeft > 0)
                --holdLeft;
            else
                open = false;
        }
        transitions += wasOpen != open ? 1 : 0;
        const double target = open ? 0.0 : std::min(range, (threshold - x) * (ratio - 1.0));
        attenuationDb = target + (target < attenuationDb ? attack : release) * (attenuationDb - target);
        if (attenuationDb < 1.0e-9)
            attenuationDb = 0.0;
        if (attenuationDb == 0.0)
            continue;
        const auto g = static_cast<float>(std::pow(10.0, -attenuationDb / 20.0));
        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][i] *= g;
    }
}
}
