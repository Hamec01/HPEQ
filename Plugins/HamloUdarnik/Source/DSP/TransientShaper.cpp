#include "DSP/TransientShaper.h"

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

bool TransientProcessor::usesUdarnik(const TransientSettings& p) noexcept
{
    return std::lround(p.values[slot::kTrStyle]) != 0 || std::lround(p.values[slot::kTrLookahead]) != 0 || p.values[slot::kTrRiseLog2] != 0.0f
        || p.values[slot::kTrDecayLog2] != 0.0f || p.values[slot::kTrStrictness] != 0.0f || p.values[slot::kTrPreserve] != 0.0f
        || std::lround(p.values[slot::kTrListen]) != 0 || p.values[slot::kTrOutputDb] != 0.0f;
}

int TransientProcessor::lookaheadSamples(const TransientSettings& p, double sampleRate) noexcept
{
    if (usesV2(p))
        return UdarnikCore::lookaheadSamples(static_cast<int>(std::lround(p.values[slot::kTrLookahead])), sampleRate);
    return usesUdarnik(p) ? UdarnikCore::lookaheadSamples(static_cast<int>(std::lround(p.values[slot::kTrLookahead])), sampleRate) : 0;
}

void TransientProcessor::process(float* const* channels, int numChannels, int numSamples, const TransientSettings& p) noexcept
{
    if (usesV2(p))
    {
        if (!v2On)
        {
            v2.reset();
            v2On = true;
        }
        v2.process(channels, numChannels, numSamples, p);
        return;
    }
    v2On = false;
    if (usesUdarnik(p))
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
        fast = slow = 0.0;
    }
    const double attackDb = std::clamp(p.values[slot::kAttackGainDb], -24.0f, 24.0f);
    const double sustainDb = std::clamp(p.values[slot::kSustainGainDb], -24.0f, 24.0f);
    const double s = 0.5 + std::clamp(p.values[slot::kTransientSpeed], 0.0f, 1.0f);
    const double fastAttack = coefficientFor(0.5, sampleRate_), fastRelease = coefficientFor(20.0 * s, sampleRate_);
    const double slowAttack = coefficientFor(20.0 * s, sampleRate_), slowRelease = coefficientFor(250.0 * s, sampleRate_);
    const bool identity = attackDb == 0.0 && sustainDb == 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        const double level = linkedPeak(channels, numChannels, i);
        fast = level + (level > fast ? fastAttack : fastRelease) * (fast - level);
        slow = level + (level > slow ? slowAttack : slowRelease) * (slow - level);
        if (identity)
            continue;
        const double d = 20.0 * std::log10((fast + 1.0e-9) / (slow + 1.0e-9));
        const double gainDb = attackDb * std::clamp(d / 6.0, 0.0, 1.0) + sustainDb * std::clamp(-d / 6.0, 0.0, 1.0);
        if (gainDb == 0.0)
            continue;
        const auto g = static_cast<float>(std::pow(10.0, gainDb / 20.0));
        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][i] *= g;
    }
}
} // namespace hamlo::dsp
