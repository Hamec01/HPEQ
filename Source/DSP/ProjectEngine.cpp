#include "DSP/ProjectEngine.h"

#include <algorithm>
#include <cmath>

namespace hpeq::dsp
{
namespace
{
ChannelMode layerMode(ChannelMode m) noexcept
{
    return m == ChannelMode::LinkedStereo || m == ChannelMode::DualMono ? ChannelMode::Stereo : m;
}
} // namespace

ProjectSnapshot makeProjectSnapshot(const FxProjectState& project)
{
    ProjectSnapshot s;
    s.routing = project.routing;
    bool anySolo = false;
    for (const auto& layer : project.layers)
        anySolo = anySolo || (layer.enabled && layer.solo);
    for (size_t j = 0; j < project.layers.size(); ++j)
    {
        const auto& layer = project.layers[j];
        auto& p = s.layers[j];
        const bool heard = !layer.mute && (!anySolo || layer.solo);
        p.enabled = layer.enabled;
        p.weight = heard ? juce::jlimit(0.0f, 1.0f, layer.mix) : 0.0f;
        p.gain = std::pow(10.0f, juce::jlimit(-60.0f, 24.0f, layer.gainDb) / 20.0f);
        p.pan = juce::jlimit(-1.0f, 1.0f, layer.pan);
        p.width = juce::jlimit(0.0f, 2.0f, layer.width);
        p.channel = layerMode(layer.channel);
        p.identityControls = layer.gainDb == 0.0f && p.pan == 0.0f && p.width == 1.0f && p.channel == ChannelMode::Stereo;
    }
    return s;
}

int ProjectEngine::latencyFor(const FxProjectState& project, double sampleRate)
{
    int sum = 0, max = 0;
    for (const auto& layer : project.layers)
    {
        const int l = LayerEngine::latencyFor(makeSnapshot(layer), sampleRate);
        sum += l;
        max = std::max(max, l);
    }
    return project.routing == LayerRouting::Serial ? sum : max;
}

ProjectEngine::ProjectEngine()
{
    // Engines exist from the start so a project published before prepare() is not lost.
    for (auto& layer : layers)
        layer.engine = std::make_unique<LayerEngine>();
}

void ProjectEngine::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    maxBlock = std::max(1, maxBlockSize);
    rampSteps = stepsFor(kMixRampSeconds, sampleRate);
    pools = std::make_unique<LayerPools>();
    pools->prepare(sampleRate);
    for (auto& layer : layers)
    {
        layer.engine->prepare(sampleRate, maxBlock, pools.get());
        layer.outAlign.prepare(kMaxLayerLatency);
        layer.restAlign.prepare(kMaxLayerLatency);
    }
    inputAlign.prepare(kMaxLayerLatency);
    for (auto* group : { &dryAligned, &work, &current, &component, &rest })
        for (auto& channel : *group)
            channel.assign(static_cast<size_t>(maxBlock), 0.0f);
    reset();
}

void ProjectEngine::reset() noexcept
{
    inputAlign.reset();
    for (auto& layer : layers)
    {
        layer.engine->reset();
        layer.outAlign.reset();
        layer.restAlign.reset();
        layer.live = false;
        layer.modePending = false;
        layer.processedLastBlock = false;
    }
    applySnapshot();
    for (auto& layer : layers)
    {
        layer.mode = layer.params.channel;
        layer.weight.reset(layer.params.enabled ? layer.params.weight : 0.0);
        layer.gain.reset(layer.params.gain);
        layer.pan.reset(layer.params.pan);
        layer.width.reset(layer.params.width);
        layer.live = layer.params.enabled;
    }
    freshStart = true;
}

void ProjectEngine::publish(const FxProjectState& project) noexcept
{
    for (size_t j = 0; j < layers.size(); ++j)
        layers[j].engine->publish(makeSnapshot(project.layers[j]));
    exchange.publish(makeProjectSnapshot(project));
}

void ProjectEngine::applySnapshot() noexcept
{
    const auto& s = exchange.current();
    if (s.routing != routing)
    {
        // Routing is a set-up choice (the latency changes with it): restart the alignment lines.
        routing = s.routing;
        inputAlign.reset();
        for (auto& layer : layers)
            layer.outAlign.reset();
    }
    for (size_t j = 0; j < layers.size(); ++j)
        layers[j].params = s.layers[j];
}

int ProjectEngine::layersProcessedLastBlock() const noexcept
{
    int n = 0;
    for (const auto& layer : layers)
        n += layer.processedLastBlock ? 1 : 0;
    return n;
}

bool ProjectEngine::layerNeedsDsp(Layer& layer) const noexcept
{
    if (!layer.params.enabled)
        return layer.live && (layer.weight.value() > 0.0 || layer.weight.isRamping());
    return layer.engine->needsProcessing() || !layer.params.identityControls || layer.modePending
        || layer.gain.isRamping() || layer.pan.isRamping() || layer.width.isRamping() || layer.mode != ChannelMode::Stereo;
}

void ProjectEngine::runLayer(Layer& layer, float* const* io, int numChannels, int n, const Transport& transport, int blockOffset) noexcept
{
    Transport t = transport;
    if (t.hasPosition && blockOffset > 0)
        t.ppqAtBlockStart += blockOffset * t.bpm / (60.0 * sampleRate);
    const int lj = layer.engine->getLatencySamples();
    const auto mode = numChannels < 2 ? ChannelMode::Stereo : layer.mode;

    switch (mode)
    {
        case ChannelMode::Left:
        case ChannelMode::Right:
        {
            const int processed = mode == ChannelMode::Left ? 0 : 1, other = 1 - processed;
            float* one[] { io[processed] };
            layer.engine->process(one, 1, n, t);
            float* untouched[] { io[other] };
            layer.restAlign.process(untouched, untouched, 1, n, lj);
            break;
        }
        case ChannelMode::Mid:
        case ChannelMode::Side:
        {
            auto* c = component[0].data();
            auto* r = rest[0].data();
            const bool mid = mode == ChannelMode::Mid;
            for (int i = 0; i < n; ++i)
            {
                const float m = 0.5f * (io[0][i] + io[1][i]), s = 0.5f * (io[0][i] - io[1][i]);
                c[i] = mid ? m : s;
                r[i] = mid ? s : m;
            }
            float* one[] { c };
            layer.engine->process(one, 1, n, t);
            float* other[] { r };
            layer.restAlign.process(other, other, 1, n, lj);
            for (int i = 0; i < n; ++i)
            {
                const float m = mid ? c[i] : r[i], s = mid ? r[i] : c[i];
                io[0][i] = m + s;
                io[1][i] = m - s;
            }
            break;
        }
        case ChannelMode::Stereo:
        case ChannelMode::LinkedStereo:
        case ChannelMode::DualMono:
            layer.engine->process(io, numChannels, n, t);
            break;
    }

    // Controls: gain, then M/S width, then linear balance. Exact no-op while all are neutral.
    const bool ramping = layer.gain.isRamping() || layer.pan.isRamping() || layer.width.isRamping();
    if (!ramping && layer.gain.value() == 1.0 && layer.pan.value() == 0.0 && layer.width.value() == 1.0)
        return;
    for (int i = 0; i < n; ++i)
    {
        const auto g = static_cast<float>(layer.gain.next());
        const auto w = static_cast<float>(layer.width.next());
        const auto p = static_cast<float>(layer.pan.next());
        if (numChannels < 2)
        {
            io[0][i] *= g;
            continue;
        }
        const float l = io[0][i] * g, r = io[1][i] * g;
        const float m = 0.5f * (l + r), s = 0.5f * (l - r) * w;
        io[0][i] = (m + s) * (p > 0.0f ? 1.0f - p : 1.0f);
        io[1][i] = (m - s) * (p < 0.0f ? 1.0f + p : 1.0f);
    }
}

void ProjectEngine::process(float* const* channels, int numChannels, int numSamples, const Transport& transport) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    if (maxBlock <= 0)
        return; // not prepared: leave the input untouched
    numChannels = std::min(numChannels, kMaxChannels);
    if (exchange.acquire())
        applySnapshot();

    // Targets, mode changes, enable / disable, latencies.
    std::array<bool, kNumLayers> dsp {};
    std::array<int, kNumLayers> lj {};
    int sum = 0, max = 0;
    for (size_t j = 0; j < layers.size(); ++j)
    {
        auto& layer = layers[j];
        layer.engine->update();
        if (freshStart)
        {
            // Nothing has been heard yet: start in the published state, no fades.
            layer.live = layer.params.enabled;
            layer.mode = layer.params.channel;
            layer.modePending = false;
            layer.weight.reset(layer.params.enabled ? layer.params.weight : 0.0);
            layer.gain.reset(layer.params.gain);
            layer.pan.reset(layer.params.pan);
            layer.width.reset(layer.params.width);
        }
        if (layer.params.enabled && !layer.live)
        {
            // Re-enabled: start clean and fade in.
            layer.engine->reset();
            layer.outAlign.reset();
            layer.restAlign.reset();
            layer.weight.reset(0.0);
            layer.mode = layer.params.channel;
            layer.modePending = false;
            layer.live = true;
        }
        if (layer.params.channel != layer.mode)
            layer.modePending = true;
        if (layer.modePending && layer.weight.value() == 0.0 && !layer.weight.isRamping())
        {
            layer.mode = layer.params.channel;
            layer.modePending = false;
            layer.engine->reset();
            layer.restAlign.reset();
        }
        const double target = layer.params.enabled && !layer.modePending ? layer.params.weight : 0.0;
        layer.weight.setTarget(target, rampSteps);
        layer.gain.setTarget(layer.params.gain, rampSteps);
        layer.pan.setTarget(layer.params.pan, rampSteps);
        layer.width.setTarget(layer.params.width, rampSteps);
        dsp[j] = layerNeedsDsp(layer);
        if (!dsp[j])
        {
            // Skipped: its delta is exactly 0 whatever the weight, so the ramps may land at once.
            layer.weight.reset(layer.weight.targetValue());
            layer.gain.reset(layer.gain.targetValue());
            layer.pan.reset(layer.pan.targetValue());
            layer.width.reset(layer.width.targetValue());
        }
        layer.processedLastBlock = dsp[j];
        lj[j] = layer.engine->getLatencySamples();
        sum += lj[j];
        max = std::max(max, lj[j]);
    }
    latency = routing == LayerRouting::Serial ? std::min(sum, kMaxLatency) : max;
    freshStart = false;

    for (int start = 0; start < numSamples; start += maxBlock)
    {
        const int n = std::min(maxBlock, numSamples - start);
        float* x[kMaxChannels] {};
        float* w[kMaxChannels] {};
        for (int ch = 0; ch < numChannels; ++ch)
        {
            x[ch] = channels[ch] + start;
            w[ch] = work[static_cast<size_t>(ch)].data();
        }

        if (routing == LayerRouting::Parallel)
        {
            float* d[kMaxChannels] {};
            float* acc[kMaxChannels] {};
            for (int ch = 0; ch < numChannels; ++ch)
            {
                d[ch] = dryAligned[static_cast<size_t>(ch)].data();
                acc[ch] = current[static_cast<size_t>(ch)].data();
            }
            inputAlign.process(x, d, numChannels, n, max);
            for (int ch = 0; ch < numChannels; ++ch)
                std::copy(d[ch], d[ch] + n, acc[ch]);
            for (size_t j = 0; j < layers.size(); ++j)
            {
                if (!dsp[j])
                    continue;
                auto& layer = layers[j];
                for (int ch = 0; ch < numChannels; ++ch)
                    std::copy(x[ch], x[ch] + n, w[ch]);
                runLayer(layer, w, numChannels, n, transport, start);
                if (max - lj[j] > 0)
                    layer.outAlign.process(w, w, numChannels, n, max - lj[j]);
                for (int i = 0; i < n; ++i)
                {
                    const auto g = static_cast<float>(layer.weight.next());
                    for (int ch = 0; ch < numChannels; ++ch)
                        acc[ch][i] += g * (w[ch][i] - d[ch][i]);
                }
            }
            for (int ch = 0; ch < numChannels; ++ch)
                std::copy(acc[ch], acc[ch] + n, x[ch]);
        }
        else
        {
            for (size_t j = 0; j < layers.size(); ++j)
            {
                auto& layer = layers[j];
                if (!dsp[j])
                {
                    if (lj[j] > 0)
                        layer.outAlign.process(x, x, numChannels, n, lj[j]); // disabled layer: pure delay
                    continue;
                }
                for (int ch = 0; ch < numChannels; ++ch)
                    std::copy(x[ch], x[ch] + n, w[ch]);
                runLayer(layer, w, numChannels, n, transport, start);
                if (lj[j] > 0)
                    layer.outAlign.process(x, x, numChannels, n, lj[j]);
                for (int i = 0; i < n; ++i)
                {
                    const auto g = static_cast<float>(layer.weight.next());
                    for (int ch = 0; ch < numChannels; ++ch)
                        x[ch][i] = g == 1.0f ? w[ch][i] : x[ch][i] + g * (w[ch][i] - x[ch][i]); // w = 1: exactly L_j
                }
            }
        }
    }

    // A disabled layer that has faded out stops here; it is reset when enabled again.
    for (auto& layer : layers)
        if (!layer.params.enabled && layer.live && layer.weight.value() == 0.0 && !layer.weight.isRamping())
            layer.live = false;
}
} // namespace hpeq::dsp
