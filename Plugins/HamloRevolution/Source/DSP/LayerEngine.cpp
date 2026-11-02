#include "DSP/LayerEngine.h"

#include <algorithm>

#include <juce_audio_basics/juce_audio_basics.h>

namespace hpeq::dsp
{
void LayerEngine::prepare(double newSampleRate, int maxBlockSize, LayerPools* sharedPools)
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
        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            auto& v = deltas[static_cast<size_t>(j)][static_cast<size_t>(ch)];
            v.assign(size, 0.0f);
            deltaPtr[static_cast<size_t>(j)][static_cast<size_t>(ch)] = v.data();
        }
        deltaList[static_cast<size_t>(j)] = deltaPtr[static_cast<size_t>(j)].data();
        presence[static_cast<size_t>(j)].assign(size, 0.0f);
        presencePtr[static_cast<size_t>(j)] = presence[static_cast<size_t>(j)].data();
    }

    preEq.prepare(sampleRate);
    if (sharedPools != nullptr)
    {
        ownPools.reset();
        pools = sharedPools;
    }
    else
    {
        ownPools = std::make_unique<LayerPools>();
        ownPools->prepare(sampleRate);
        pools = ownPools.get();
    }
    dryAlign.prepare(FxNodeProcessor::maxNodeLatency(sampleRate));
    for (auto& d : deltaAlign)
        d.prepare(FxNodeProcessor::maxNodeLatency(sampleRate));
    for (auto& d : soloBandAlign)
        d.prepare(FxNodeProcessor::maxNodeLatency(sampleRate));
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        soloOut[static_cast<size_t>(ch)].assign(size, 0.0f);
        soloBand[static_cast<size_t>(ch)].assign(size, 0.0f);
    }
    for (auto& node : nodes)
        node.prepare(sampleRate, maxBlock, pools);
    mixNode.prepare(sampleRate, maxBlock, pools);
    mixDryAlign.prepare(FxNodeProcessor::maxNodeLatency(sampleRate));
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        mixBand[static_cast<size_t>(ch)].assign(size, 0.0f);
        mixDelta[static_cast<size_t>(ch)].assign(size, 0.0f);
    }
    mixPresence.assign(size, 1.0f);
    tree.prepare(sampleRate, maxBlock);
    reset();
}

void LayerEngine::reset() noexcept
{
    controlPhase = 0;
    preEq.reset();
    for (auto& node : nodes)
        node.reset();
    mixNode.reset();
    mixDryAlign.reset();
    tree.reset();
    dryAlign.reset();
    for (auto& d : deltaAlign)
        d.reset();
    applySnapshot(); // re-apply the current targets so a reset engine resumes the same sound
}

void LayerEngine::publish(const LayerSnapshot& snapshot) noexcept
{
    exchange.publish(snapshot);
}

int LayerEngine::latencyFor(const LayerSnapshot& snapshot, double sampleRate)
{
    int latency = 0;
    for (const auto& node : snapshot.fx)
        latency = std::max(latency, FxNodeProcessor::nodeLatencyFor(node, sampleRate));
    return latency + FxNodeProcessor::nodeLatencyFor(snapshot.mix, sampleRate);
}

void LayerEngine::applySnapshot() noexcept
{
    const auto& snapshot = exchange.current();
    preEq.setTargets(snapshot.eq);
    hasEq = false;
    for (const auto& band : snapshot.eq)
        hasEq = hasEq || band.active;
    for (size_t i = 0; i < nodes.size(); ++i)
        nodes[i].setTargets(snapshot.fx[i]);
    mixNode.setTargets(snapshot.mix);
    anySolo = false;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        solo[i] = FxNodeProcessor::wantsSolo(snapshot.fx[i]);
        anySolo = anySolo || solo[i];
    }
    layerLatency = 0;
    for (const auto& node : nodes)
        layerLatency = std::max(layerLatency, node.latencySamples());
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

void LayerEngine::process(float* const* channels, int numChannels, int numSamples, const Transport& transport) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    numChannels = std::min(numChannels, kMaxChannels);
    update();

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
            if (layerLatency > 0)
                dryAlign.process(dryPtr.data(), chunk, numChannels, n, layerLatency);
            else
                for (int ch = 0; ch < numChannels; ++ch)
                    std::copy(dryPtr[static_cast<size_t>(ch)], dryPtr[static_cast<size_t>(ch)] + n, chunk[ch]);
            for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
                if (tree.hasBand(j))
                {
                    auto* delta = deltaPtr[static_cast<size_t>(j)].data();
                    for (int ch = 0; ch < numChannels; ++ch)
                        std::fill(delta[ch], delta[ch] + n, 0.0f);
                    auto& node = nodes[static_cast<size_t>(j)];
                    node.process(bandList[static_cast<size_t>(j)], presencePtr[static_cast<size_t>(j)], delta, numChannels, n, transport, start);
                    const int extra = layerLatency - node.latencySamples();
                    if (extra > 0)
                        deltaAlign[static_cast<size_t>(j)].process(delta, delta, numChannels, n, extra);
                }
            tree.combine(deltaList.data(), chunk, numChannels, n);

            // SOLO: the soloed bands alone (aligned band + delta), cross-faded with the normal output
            const double soloTarget = anySolo ? 1.0 : 0.0;
            if (soloMix != soloTarget || anySolo)
            {
                float* aligned[kMaxChannels] {};
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    aligned[ch] = soloBand[static_cast<size_t>(ch)].data();
                    std::fill(soloOut[static_cast<size_t>(ch)].begin(), soloOut[static_cast<size_t>(ch)].begin() + n, 0.0f);
                }
                for (int j = 0; j < kMaxFxNodesPerLayer; ++j)
                {
                    if (!tree.hasBand(j))
                        continue;
                    soloBandAlign[static_cast<size_t>(j)].process(bandList[static_cast<size_t>(j)], aligned, numChannels, n, layerLatency);
                    if (!solo[static_cast<size_t>(j)])
                        continue;
                    const float* presenceJ = presencePtr[static_cast<size_t>(j)];
                    const auto* delta = deltaPtr[static_cast<size_t>(j)].data();
                    for (int ch = 0; ch < numChannels; ++ch)
                        for (int i = 0; i < n; ++i)
                            soloOut[static_cast<size_t>(ch)][static_cast<size_t>(i)] += presenceJ[i] * aligned[ch][i] + delta[ch][i];
                }
                const double step = 1.0 / (0.01 * sampleRate);
                for (int i = 0; i < n; ++i)
                {
                    soloMix = soloTarget > soloMix ? std::min(soloTarget, soloMix + step) : std::max(soloTarget, soloMix - step);
                    const auto m = static_cast<float>(soloMix);
                    for (int ch = 0; ch < numChannels; ++ch)
                        chunk[ch][i] += m * (soloOut[static_cast<size_t>(ch)][static_cast<size_t>(i)] - chunk[ch][i]);
                }
            }
            else
                for (int j = 0; j < kMaxFxNodesPerLayer; ++j) // keep the solo alignment history current
                    if (tree.hasBand(j))
                    {
                        float* aligned[kMaxChannels] {};
                        for (int ch = 0; ch < numChannels; ++ch)
                            aligned[ch] = soloBand[static_cast<size_t>(ch)].data();
                        soloBandAlign[static_cast<size_t>(j)].process(bandList[static_cast<size_t>(j)], aligned, numChannels, n, layerLatency);
                    }
        }
        else if (layerLatency > 0)
            dryAlign.process(chunk, chunk, numChannels, n, layerLatency);

        // D7: the whole-layer chain on the layer signal: y = D_m(y) + delta (delta = mix (chain - D_m)).
        const int mixLatency = mixNode.latencySamples();
        if (mixNode.isRunning())
        {
            float* band[kMaxChannels] {};
            float* delta[kMaxChannels] {};
            for (int ch = 0; ch < numChannels; ++ch)
            {
                band[ch] = mixBand[static_cast<size_t>(ch)].data();
                delta[ch] = mixDelta[static_cast<size_t>(ch)].data();
                std::copy(chunk[ch], chunk[ch] + n, band[ch]);
                std::fill(delta[ch], delta[ch] + n, 0.0f);
            }
            mixNode.process(band, mixPresence.data(), delta, numChannels, n, transport, start);
            if (mixLatency > 0)
                mixDryAlign.process(chunk, chunk, numChannels, n, mixLatency);
            for (int ch = 0; ch < numChannels; ++ch)
                for (int i = 0; i < n; ++i)
                    chunk[ch][i] += delta[ch][i];
        }
        else if (mixLatency > 0)
            mixDryAlign.process(chunk, chunk, numChannels, n, mixLatency);
        controlPhase = (controlPhase + n) % kControlInterval;
    }
}
} // namespace hpeq::dsp
