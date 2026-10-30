#include "DSP/LegacyEchoCore.h"
#include <algorithm>

namespace hpeq::dsp
{
void LegacyEchoCore::prepare(double sampleRate, int maximumBlockSize)
{
    delay.prepare(sampleRate);
    for (auto* group : { &work, &wet, &chain })
        for (auto& channel : *group)
            channel.assign(static_cast<size_t>(maximumBlockSize), 0.0f);
    mixSteps = stepsFor(kMixRampSeconds, sampleRate);
    silenceSamples = stepsFor(0.05, sampleRate);
    nodeMix.reset(0.0);
    mix.reset(0.0);
    inputGate.reset(0.0);
    silentSamples = 0;
    running = false;
}

void LegacyEchoCore::setTargets(const FxSlotParams& p) noexcept
{
    params = p;
    nodeMix.setTarget(1.0, mixSteps);
    if (p.active && !running)
    {
        // This is the old pool's acquire/reset and send-slot start, without a pool.
        delay.reset();
        running = true;
        silentSamples = 0;
        mix.reset(p.mix);
        inputGate.reset(0.0);
        inputGate.setTarget(1.0, mixSteps);
    }
    if (running)
    {
        mix.setTarget(p.mix, mixSteps);
        inputGate.setTarget(p.active ? 1.0 : 0.0, mixSteps);
    }
}

void LegacyEchoCore::process(float* const* channels, int numChannels, int samples, const Transport& transport) noexcept
{
    float* input[kMaxChannels] {};
    float* output[kMaxChannels] {};
    for (int ch = 0; ch < numChannels; ++ch)
    {
        input[ch] = work[static_cast<size_t>(ch)].data();
        output[ch] = wet[static_cast<size_t>(ch)].data();
        std::copy(channels[ch], channels[ch] + samples, chain[static_cast<size_t>(ch)].data());
    }
    if (running)
    {
        for (int i = 0; i < samples; ++i)
        {
            const auto g = static_cast<float>(inputGate.next());
            for (int ch = 0; ch < numChannels; ++ch)
                input[ch][i] = g * chain[static_cast<size_t>(ch)][static_cast<size_t>(i)];
        }
        delay.process(input, output, numChannels, samples, params, transport);
        for (int i = 0; i < samples; ++i)
        {
            const auto m = static_cast<float>(mix.next());
            for (int ch = 0; ch < numChannels; ++ch)
                chain[static_cast<size_t>(ch)][static_cast<size_t>(i)] += m * output[ch][i];
        }
        const bool closed = !inputGate.isRamping() && inputGate.value() == 0.0;
        silentSamples = closed && delay.lastBlockPeak() < 1.0e-6f ? silentSamples + samples : 0;
        if (silentSamples >= silenceSamples)
        {
            running = false;
            silentSamples = 0;
            mix.reset(0.0);
            inputGate.reset(0.0);
        }
    }
    for (int i = 0; i < samples; ++i)
    {
        // Preserve the node delta, layer addition, then the outer plugin bypass blend.
        const auto g = static_cast<float>(nodeMix.next()) * 1.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float delta = 0.0f;
            delta += g * (chain[static_cast<size_t>(ch)][static_cast<size_t>(i)] - channels[ch][i]);
            channels[ch][i] += delta;
        }
    }
}
}
