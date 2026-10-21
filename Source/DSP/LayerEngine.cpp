#include "DSP/LayerEngine.h"

#include <algorithm>

#include <juce_audio_basics/juce_audio_basics.h>

namespace hpeq::dsp
{
void LayerEngine::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    maxBlock = std::max(1, maxBlockSize);
    for (auto& channel : xeCopy)
        channel.assign(static_cast<size_t>(maxBlock), 0.0f);
    preEq.prepare(sampleRate);
    for (auto& node : nodes)
        node.prepare(sampleRate, maxBlock);
    reset();
}

void LayerEngine::reset() noexcept
{
    controlPhase = 0;
    preEq.reset();
    for (auto& node : nodes)
        node.reset();
    applySnapshot(); // re-apply the current targets so a reset engine resumes the same sound
}

void LayerEngine::publish(const LayerSnapshot& snapshot) noexcept
{
    exchange.publish(snapshot);
}

void LayerEngine::applySnapshot() noexcept
{
    const auto& snapshot = exchange.current();
    preEq.setTargets(snapshot.eq);
    for (size_t i = 0; i < nodes.size(); ++i)
        nodes[i].setTargets(snapshot.fx[i]);
}

void LayerEngine::process(float* const* channels, int numChannels, int numSamples) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    numChannels = std::min(numChannels, kMaxChannels);
    if (exchange.acquire())
        applySnapshot();

    for (int start = 0; start < numSamples; start += maxBlock)
    {
        const int n = std::min(maxBlock, numSamples - start);
        float* chunk[kMaxChannels] {};
        float* xe[kMaxChannels] {};
        for (int ch = 0; ch < numChannels; ++ch)
        {
            chunk[ch] = channels[ch] + start;
            xe[ch] = xeCopy[static_cast<size_t>(ch)].data();
        }

        preEq.process(chunk, numChannels, n, controlPhase);

        bool anyNode = false;
        for (const auto& node : nodes)
            anyNode = anyNode || node.isRunning();
        if (anyNode)
        {
            // Every node reads the same xE; deltas accumulate into the output in node order.
            for (int ch = 0; ch < numChannels; ++ch)
                std::copy(chunk[ch], chunk[ch] + n, xe[ch]);
            for (auto& node : nodes)
                node.process(xe, chunk, numChannels, n, controlPhase);
        }
        controlPhase = (controlPhase + n) % kControlInterval;
    }
}
} // namespace hpeq::dsp
