#include "DSP/LayerEngine.h"

#include <algorithm>

#include <juce_audio_basics/juce_audio_basics.h>

namespace hpeq::dsp
{
void LayerEngine::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    maxBlock = std::max(1, maxBlockSize);
    const auto size = static_cast<size_t>(maxBlock);
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        xeCopy[static_cast<size_t>(ch)].assign(size, 0.0f);
        dry[static_cast<size_t>(ch)].assign(size, 0.0f);
        dryPtr[static_cast<size_t>(ch)] = dry[static_cast<size_t>(ch)].data();
    }
    for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
    {
        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            auto& v = bands[static_cast<size_t>(j)][static_cast<size_t>(ch)];
            v.assign(size, 0.0f);
            bandPtr[static_cast<size_t>(j)][static_cast<size_t>(ch)] = v.data();
        }
        bandList[static_cast<size_t>(j)] = bandPtr[static_cast<size_t>(j)].data();
        presence[static_cast<size_t>(j)].assign(size, 0.0f);
        presencePtr[static_cast<size_t>(j)] = presence[static_cast<size_t>(j)].data();
    }

    preEq.prepare(sampleRate);
    reverbs.prepare(sampleRate);
    for (auto& node : nodes)
        node.prepare(sampleRate, maxBlock, &reverbs);
    tree.prepare(sampleRate);
    reset();
}

void LayerEngine::reset() noexcept
{
    controlPhase = 0;
    preEq.reset();
    for (auto& node : nodes)
        node.reset();
    tree.reset();
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

void LayerEngine::updateTree() noexcept
{
    std::array<bool, kMaxFxNodesPerLayer> running {};
    std::array<double, kMaxFxNodesPerLayer> low {}, high {};
    for (size_t j = 0; j < nodes.size(); ++j)
    {
        running[j] = nodes[j].isRunning();
        low[j] = nodes[j].targetLowHz();
        high[j] = nodes[j].targetHighHz();
    }
    tree.update(running, low, high);
}

void LayerEngine::process(float* const* channels, int numChannels, int numSamples) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    numChannels = std::min(numChannels, kMaxChannels);
    if (exchange.acquire())
        applySnapshot();
    updateTree();

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

        if (tree.isActive())
        {
            for (int ch = 0; ch < numChannels; ++ch)
                std::copy(chunk[ch], chunk[ch] + n, xe[ch]);
            tree.process(xe, numChannels, n, controlPhase, dryPtr.data(), bandList.data(), presencePtr.data());
            for (int ch = 0; ch < numChannels; ++ch)
                std::copy(dryPtr[static_cast<size_t>(ch)], dryPtr[static_cast<size_t>(ch)] + n, chunk[ch]);
            for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
                if (tree.hasBand(j))
                    nodes[static_cast<size_t>(j)].process(bandList[static_cast<size_t>(j)], presencePtr[static_cast<size_t>(j)],
                                                          chunk, numChannels, n);
        }
        controlPhase = (controlPhase + n) % kControlInterval;
    }
}
} // namespace hpeq::dsp
