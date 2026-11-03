#include "DSP/FaderGain.h"
#include <algorithm>
#include <cmath>
namespace hpeq::dsp
{
void GainProcessor::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    for (auto& l : lines)
        l.assign(static_cast<size_t>(std::ceil(kMaxDelayMs * 1.0e-3 * sampleRate)) + 4, 0.0f);
    reset();
}

void GainProcessor::reset() noexcept
{
    for (auto& l : lines)
        std::fill(l.begin(), l.end(), 0.0f);
    writePos = 0;
    primed = false;
    msIn = msInLong = msOut = 0.0;
    meterIn.store(-120.0f, std::memory_order_relaxed);
    meterInLong.store(-120.0f, std::memory_order_relaxed);
    meterOut.store(-120.0f, std::memory_order_relaxed);
}

void GainProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    const float gainDb = p.values[slot::kGainDb];
    const bool mute = p.values[slot::kFadMute] >= 0.5f;
    const int pol = std::clamp(static_cast<int>(std::lround(p.values[slot::kFadPolarity])), 0, 3);
    const double gainTarget = std::pow(10.0, gainDb / 20.0), muteTarget = mute ? 0.0 : 1.0;
    const std::array<double, kMaxChannels> polTarget { (pol & 1) != 0 ? -1.0 : 1.0, (pol & 2) != 0 ? -1.0 : 1.0 };
    const int lineSize = lines[0].empty() ? 0 : static_cast<int>(lines[0].size());
    const double delayTarget = lineSize == 0 ? 0.0
                                             : std::clamp(static_cast<double>(p.values[slot::kFadDelayMs]), 0.0, kMaxDelayMs) * 1.0e-3 * sampleRate;
    if (!primed)
    {
        gain = gainTarget;
        muteGain = muteTarget;
        delay = delayTarget;
        polarity = polTarget;
        primed = true;
    }

    // band input meters (before the fader)
    const double fast = 1.0 - std::exp(-1.0 / (0.3 * sampleRate)), slow = 1.0 - std::exp(-1.0 / (3.0 * sampleRate));
    for (int i = 0; i < numSamples; ++i)
    {
        double s = 0.0;
        for (int ch = 0; ch < numChannels; ++ch)
            s += static_cast<double>(channels[ch][i]) * channels[ch][i];
        s /= std::max(1, numChannels);
        msIn += fast * (s - msIn);
        msInLong += slow * (s - msInLong);
    }

    const bool settled = gain == gainTarget && muteGain == 1.0 && delay == 0.0 && delayTarget == 0.0 && polarity == polTarget && polTarget[0] > 0.0
                         && polTarget[1] > 0.0 && !mute;
    if (settled)
    {
        if (lineSize > 0) // keep the delay lines current, so a delay can start from real history
            for (int i = 0; i < numSamples; ++i)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                    lines[static_cast<size_t>(ch)][static_cast<size_t>(writePos)] = channels[ch][i];
                if (++writePos == lineSize)
                    writePos = 0;
            }
        // the old law, bit for bit
        if (gainDb != 0.0f)
        {
            const float g = std::pow(10.0f, gainDb / 20.0f);
            for (int ch = 0; ch < numChannels; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    channels[ch][i] *= g;
        }
    }
    else
    {
        const double gainK = 1.0 - std::exp(-1.0 / (0.02 * sampleRate)), delayK = 1.0 - std::exp(-1.0 / (0.05 * sampleRate));
        const double polStep = 2.0 / (0.01 * sampleRate), muteStep = 1.0 / (0.01 * sampleRate);
        for (int i = 0; i < numSamples; ++i)
        {
            if (gain != gainTarget)
            {
                gain += gainK * (gainTarget - gain);
                if (std::abs(gainTarget - gain) < 1.0e-7)
                    gain = gainTarget;
            }
            if (muteGain != muteTarget)
                muteGain = muteTarget > muteGain ? std::min(muteTarget, muteGain + muteStep) : std::max(muteTarget, muteGain - muteStep);
            if (delay != delayTarget)
            {
                delay += delayK * (delayTarget - delay);
                if (std::abs(delayTarget - delay) < 1.0e-4)
                    delay = delayTarget;
            }
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& pc = polarity[static_cast<size_t>(ch)];
                const double pt = polTarget[static_cast<size_t>(ch)];
                if (pc != pt)
                    pc = pt > pc ? std::min(pt, pc + polStep) : std::max(pt, pc - polStep);
                double x = channels[ch][i];
                if (lineSize > 0)
                {
                    auto& line = lines[static_cast<size_t>(ch)];
                    line[static_cast<size_t>(writePos)] = static_cast<float>(x);
                    if (delay > 0.0)
                    {
                        const int whole = static_cast<int>(delay);
                        const double f = delay - whole;
                        int i0 = writePos - whole;
                        if (i0 < 0)
                            i0 += lineSize;
                        const int i1 = i0 == 0 ? lineSize - 1 : i0 - 1;
                        x = line[static_cast<size_t>(i0)] + f * (line[static_cast<size_t>(i1)] - line[static_cast<size_t>(i0)]);
                    }
                }
                channels[ch][i] = static_cast<float>(pc * gain * muteGain * x);
            }
            if (lineSize > 0 && ++writePos == lineSize)
                writePos = 0;
        }
    }

    for (int i = 0; i < numSamples; ++i)
    {
        double s = 0.0;
        for (int ch = 0; ch < numChannels; ++ch)
            s += static_cast<double>(channels[ch][i]) * channels[ch][i];
        msOut += fast * (s / std::max(1, numChannels) - msOut);
    }
    meterIn.store(static_cast<float>(10.0 * std::log10(msIn + 1.0e-12)), std::memory_order_relaxed);
    meterInLong.store(static_cast<float>(10.0 * std::log10(msInLong + 1.0e-12)), std::memory_order_relaxed);
    meterOut.store(static_cast<float>(10.0 * std::log10(msOut + 1.0e-12)), std::memory_order_relaxed);
}

}
