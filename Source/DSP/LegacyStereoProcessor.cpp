#include "DSP/LegacyStereoProcessor.h"
#include <algorithm>

namespace hpeq::dsp
{
bool LegacyStereoProcessor::usesShirota(const FxSlotParams& p) noexcept
{
    for (int k = slot::kStStyle; k <= slot::kStOutputDb; ++k)
        if (p.values[static_cast<size_t>(k)] != 0.0f)
            return true;
    return false;
}

void LegacyStereoProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    if (usesShirota(p))
    {
        if (!coreActive)
        {
            core.reset();
            coreActive = true;
        }
        core.process(channels, numChannels, numSamples, p);
        return;
    }
    coreActive = false;
    const float width = std::clamp(p.values[slot::kStereoWidth], 0.0f, 2.0f);
    const float balance = std::clamp(p.values[slot::kStereoBalance], -1.0f, 1.0f);
    if (numChannels < 2 || (width == 1.0f && balance == 0.0f))
        return;
    const float gl = balance > 0.0f ? 1.0f - balance : 1.0f;
    const float gr = balance < 0.0f ? 1.0f + balance : 1.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float m = 0.5f * (channels[0][i] + channels[1][i]);
        const float sd = 0.5f * (channels[0][i] - channels[1][i]) * width;
        channels[0][i] = (m + sd) * gl;
        channels[1][i] = (m - sd) * gr;
    }
}

}
