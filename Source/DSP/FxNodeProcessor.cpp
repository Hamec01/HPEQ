#include "DSP/FxNodeProcessor.h"
#include "DSP/Biquad.h"

#include <algorithm>

namespace hpeq::dsp
{
int FxNodeProcessor::slotLatency(const FxSlotParams& p, double sampleRate)
{
    switch (p.type)
    {
        case FxType::Saturator: return SaturatorProcessor::latencySamples(p, sampleRate);
        case FxType::Limiter:   return LimiterProcessor::lookaheadSamples(p, sampleRate);
        case FxType::TransientShaper: return TransientProcessor::lookaheadSamples(p, sampleRate);
        case FxType::Gain: case FxType::Compressor: case FxType::Reverb: case FxType::Delay:
        case FxType::MonstaFx: case FxType::StereoProcessor: case FxType::GateExpander: break;
    }
    return 0;
}

int FxNodeProcessor::maxSlotLatency(double sampleRate)
{
    return std::max(LimiterProcessor::maxLookaheadSamples(sampleRate) + RedWallCore::kTruePeakDelay,
                    SaturatorProcessor::latencySamples(2) + static_cast<int>(std::lround(mag::kWowBaseSeconds * sampleRate)));
}

int FxNodeProcessor::nodeLatencyFor(const FxNodeParams& p, double sampleRate)
{
    int total = 0;
    for (int k = 0; k < p.numSlots; ++k)
        total += slotLatency(p.slots[static_cast<size_t>(k)], sampleRate);
    return std::min(total, maxNodeLatency(sampleRate));
}

void FxNodeProcessor::prepare(double newSampleRate, int maxBlockSize, LayerPools* layerPools)
{
    sampleRate = newSampleRate;
    pools = layerPools;
    mixSteps = stepsFor(kMixRampSeconds, sampleRate);
    silenceSamples = stepsFor(0.05, sampleRate);
    for (auto* group : { &chain, &work, &bandWork, &gated, &wet, &aligned, &bandDelayed })
        for (auto& channel : *group)
            channel.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    slotLatencyCap = maxSlotLatency(sampleRate);
    nodeLatencyCap = maxNodeLatency(sampleRate);
    bandAlign.prepare(nodeLatencyCap);
    for (auto& slot : slots)
    {
        slot.align.prepare(slotLatencyCap);
        slot.gain.prepare(sampleRate);
        slot.compressor.prepare(sampleRate);
        slot.saturator.prepare(sampleRate, maxBlockSize);
        slot.transient.prepare(sampleRate);
        slot.stereo.prepare(sampleRate);
        slot.gateFx.prepare(sampleRate);
        slot.limiter.prepare(sampleRate);
    }
    reset();
}

void FxNodeProcessor::reset() noexcept
{
    wanted = running = false;
    nodeMix.reset(0.0);
    gate.reset(1.0);
    nodeLatency = 0;
    bandAlign.reset();
    for (auto& slot : slots)
    {
        stopSlot(slot);
        slot.inChain = false;
        slot.latency = 0;
        slot.align.reset();
    }
}

void FxNodeProcessor::releasePooled(Slot& slot) noexcept
{
    if (pools != nullptr && slot.pooled >= 0)
    {
        if (slot.type == FxType::Reverb) pools->reverbs.release(slot.pooled);
        else if (slot.type == FxType::Delay) pools->delays.release(slot.pooled);
        else if (slot.type == FxType::MonstaFx) pools->monstas.release(slot.pooled);
    }
    slot.pooled = -1;
}

void FxNodeProcessor::stopSlot(Slot& slot) noexcept
{
    releasePooled(slot);
    slot.running = false;
    slot.silentSamples = 0;
    slot.mix.reset(0.0);
    slot.inGate.reset(0.0);
    slot.gain.reset();
    slot.compressor.reset();
    slot.saturator.reset();
    slot.transient.reset();
    slot.gateFx.reset();
    slot.limiter.reset();
}

void FxNodeProcessor::startSlot(Slot& slot, const FxSlotParams& sp) noexcept
{
    slot.type = sp.type;
    slot.gain.reset();
    slot.compressor.reset();
    slot.saturator.reset();
    slot.transient.reset();
    slot.gateFx.reset();
    slot.limiter.reset();
    slot.silentSamples = 0;
    slot.starved = false;
    if (isPooled(sp.type) && pools != nullptr)
    {
        slot.pooled = sp.type == FxType::Reverb ? pools->reverbs.acquire()
                    : sp.type == FxType::Delay  ? pools->delays.acquire()
                                                : pools->monstas.acquire();
        slot.starved = slot.pooled < 0;
        if (slot.starved)
            return; // stays off; reported through slotStarved()
    }
    if (isSend(sp.type))
    {
        // A send starts with silence in its memory: open the input, output weight at once.
        slot.mix.reset(sp.mix);
        slot.inGate.reset(0.0);
        slot.inGate.setTarget(1.0, mixSteps);
    }
    else
    {
        slot.mix.reset(0.0);
        slot.inGate.reset(1.0);
    }
    slot.running = true;
}

void FxNodeProcessor::setTargets(const FxNodeParams& p) noexcept
{
    wanted = p.active;
    targetMix = p.mix;
    if (wanted && !running)
    {
        running = true;
        mode = p.channel;
        modeChangePending = false;
        nodeMix.reset(0.0);
        gate.reset(1.0);
        bandAlign.reset();
        for (auto& slot : slots)
        {
            stopSlot(slot);
            slot.align.reset();
        }
    }

    // Latency follows the chain (snapshot), not the running state: stable while slots are bypassed.
    nodeLatency = 0;
    for (int k = 0; k < kMaxFxPerNode; ++k)
    {
        auto& slot = slots[static_cast<size_t>(k)];
        slot.inChain = k < p.numSlots;
        slot.latency = slot.inChain ? std::min(slotLatency(p.slots[static_cast<size_t>(k)], sampleRate), slotLatencyCap) : 0;
        nodeLatency += slot.latency;
    }
    nodeLatency = std::min(nodeLatency, nodeLatencyCap);

    if (!running)
        return;

    lowHz = design::clampFrequency(p.lowHz, sampleRate);
    highHz = design::clampFrequency(p.highHz, sampleRate);
    if (highHz < lowHz)
        std::swap(lowHz, highHz);

    // A channel-mode change re-routes the delta: fade the node out, switch, fade back in.
    if (p.channel != mode)
    {
        modeChangePending = true;
        pendingMode = p.channel;
    }
    nodeMix.setTarget(modeChangePending ? 0.0 : static_cast<double>(p.mix), mixSteps);
    gate.setTarget(wanted ? 1.0 : 0.0, mixSteps);

    for (int k = 0; k < kMaxFxPerNode; ++k)
    {
        auto& slot = slots[static_cast<size_t>(k)];
        const auto& sp = p.slots[static_cast<size_t>(k)];
        slot.wanted = slot.inChain && sp.active;

        if (slot.running && slot.inChain && slot.type != sp.type)
            stopSlot(slot); // a different processor: never reuse its state
        if (slot.wanted && !slot.running)
            startSlot(slot, sp);
        if (!slot.wanted)
            slot.starved = false;
        if (slot.inChain)
            slot.params = sp;
        if (!slot.running)
            continue;
        if (isSend(slot.type))
        {
            slot.mix.setTarget(sp.mix, mixSteps);
            slot.inGate.setTarget(slot.wanted ? 1.0 : 0.0, mixSteps);
        }
        else
            slot.mix.setTarget(slot.wanted ? static_cast<double>(sp.mix) : 0.0, mixSteps);
    }
}

void FxNodeProcessor::runInsert(Slot& slot, float* const* channels, int numChannels, int numSamples, const Transport& transport, int blockOffset) noexcept
{
    switch (slot.type)
    {
        case FxType::Gain:            slot.gain.process(channels, numChannels, numSamples, slot.params); break;
        case FxType::Compressor:      slot.compressor.process(channels, numChannels, numSamples, slot.params, mode != ChannelMode::DualMono); break;
        case FxType::Saturator:       slot.saturator.process(channels, numChannels, numSamples, slot.params); break;
        case FxType::TransientShaper: slot.transient.process(channels, numChannels, numSamples, slot.params); break;
        case FxType::StereoProcessor: slot.stereo.process(channels, numChannels, numSamples, slot.params); break;
        case FxType::GateExpander:    slot.gateFx.process(channels, numChannels, numSamples, slot.params); break;
        case FxType::Limiter:         slot.limiter.process(channels, numChannels, numSamples, slot.params); break;
        case FxType::MonstaFx:
            if (slot.pooled >= 0)
                pools->monstas.get(slot.pooled).process(channels, numChannels, numSamples, slot.params, transport, blockOffset);
            break;
        case FxType::Reverb:
        case FxType::Delay:
            break; // sends are handled in process()
    }
}

void FxNodeProcessor::process(const float* const* bandPtr, const float* presence, float* const* out, int numChannels, int numSamples,
                              const Transport& transport, int blockOffset) noexcept
{
    if (!running)
        return;

    // Route the stereo band into the work domain of the channel mode (M = (L+R)/2, S = (L-R)/2).
    const bool stereo = numChannels > 1;
    int workChannels = numChannels;
    switch (mode)
    {
        case ChannelMode::Stereo:
        case ChannelMode::LinkedStereo:
        case ChannelMode::DualMono: workChannels = numChannels; break;
        case ChannelMode::Left:
        case ChannelMode::Mid:      workChannels = 1; break;
        case ChannelMode::Right:
        case ChannelMode::Side:     workChannels = stereo ? 1 : 0; break; // nothing to process on mono
    }

    if (workChannels == 0)
    {
        // Right/Side on a mono input: nothing to process; only let the ramps and the bypass finish.
        for (int i = 0; i < numSamples; ++i)
        {
            gate.next();
            nodeMix.next();
        }
        for (auto& slot : slots)
            if (!slot.wanted || isSend(slot.type))
                stopSlot(slot);
        if (!wanted && !gate.isRamping() && gate.value() == 0.0)
            running = false;
        return;
    }

    float* bandWorkPtr[kMaxChannels] {};
    float* gatedPtr[kMaxChannels] {};
    float* chainPtr[kMaxChannels] {};
    float* workPtr[kMaxChannels] {};
    float* wetPtr[kMaxChannels] {};
    float* alignedPtr[kMaxChannels] {};
    float* bandDelayedPtr[kMaxChannels] {};
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        bandWorkPtr[ch] = bandWork[static_cast<size_t>(ch)].data();
        gatedPtr[ch] = gated[static_cast<size_t>(ch)].data();
        chainPtr[ch] = chain[static_cast<size_t>(ch)].data();
        workPtr[ch] = work[static_cast<size_t>(ch)].data();
        wetPtr[ch] = wet[static_cast<size_t>(ch)].data();
        alignedPtr[ch] = aligned[static_cast<size_t>(ch)].data();
        bandDelayedPtr[ch] = bandDelayed[static_cast<size_t>(ch)].data();
    }

    const float* const* b = bandPtr; // Stereo-like modes and Left use the band as it is
    if (mode == ChannelMode::Right)
        b = bandPtr + 1;
    else if (stereo && (mode == ChannelMode::Mid || mode == ChannelMode::Side))
    {
        const float sign = mode == ChannelMode::Mid ? 1.0f : -1.0f;
        for (int i = 0; i < numSamples; ++i)
            bandWorkPtr[0][i] = 0.5f * (bandPtr[0][i] + sign * bandPtr[1][i]);
        b = bandWorkPtr;
    }
    // Node input gate (bypass with spill-over). Fully open: the band is used as it is.
    if (gate.isRamping() || gate.value() != 1.0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto g = static_cast<float>(gate.next());
            for (int ch = 0; ch < workChannels; ++ch)
                gatedPtr[ch][i] = g * b[ch][i];
        }
        b = gatedPtr;
    }
    for (int ch = 0; ch < workChannels; ++ch)
        std::copy(b[ch], b[ch] + numSamples, chainPtr[ch]);

    const bool nodeClosed = !gate.isRamping() && gate.value() == 0.0;
    bool sendRunning = false;
    for (auto& slot : slots)
    {
        if (!slot.inChain)
            continue;

        if (isSend(slot.type) && slot.running)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const auto g = static_cast<float>(slot.inGate.next());
                for (int ch = 0; ch < workChannels; ++ch)
                    workPtr[ch][i] = g * chainPtr[ch][i];
            }
            float peak = 0.0f;
            if (slot.type == FxType::Reverb)
            {
                auto& reverb = pools->reverbs.get(slot.pooled);
                reverb.process(workPtr, wetPtr, workChannels, numSamples, slot.params, transport);
                peak = reverb.lastBlockPeak();
            }
            else
            {
                auto& delay = pools->delays.get(slot.pooled);
                delay.process(workPtr, wetPtr, workChannels, numSamples, slot.params, transport);
                peak = delay.lastBlockPeak();
            }
            for (int i = 0; i < numSamples; ++i)
            {
                const auto m = static_cast<float>(slot.mix.next());
                for (int ch = 0; ch < workChannels; ++ch)
                    chainPtr[ch][i] += m * wetPtr[ch][i];
            }
            const bool inputClosed = nodeClosed || (!slot.inGate.isRamping() && slot.inGate.value() == 0.0);
            slot.silentSamples = inputClosed && peak < 1.0e-6f ? slot.silentSamples + numSamples : 0;
            if (slot.silentSamples >= silenceSamples)
                stopSlot(slot); // tail has rung out
            else
                sendRunning = true;
            continue;
        }

        // Insert (or a stopped slot that only keeps its alignment delay).
        const bool delayed = slot.latency > 0;
        const float* const* dryAligned = chainPtr;
        if (delayed)
        {
            slot.align.process(chainPtr, alignedPtr, workChannels, numSamples, slot.latency);
            dryAligned = alignedPtr;
        }
        if (!slot.running || isSend(slot.type))
        {
            if (delayed)
                for (int ch = 0; ch < workChannels; ++ch)
                    std::copy(alignedPtr[ch], alignedPtr[ch] + numSamples, chainPtr[ch]);
            continue;
        }
        for (int ch = 0; ch < workChannels; ++ch)
            std::copy(chainPtr[ch], chainPtr[ch] + numSamples, workPtr[ch]);
        runInsert(slot, workPtr, workChannels, numSamples, transport, blockOffset);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto m = static_cast<float>(slot.mix.next());
            for (int ch = 0; ch < workChannels; ++ch)
            {
                const float d = dryAligned[ch][i];
                chainPtr[ch][i] = m == 1.0f ? workPtr[ch][i] : (m == 0.0f ? d : d + m * (workPtr[ch][i] - d));
            }
        }
        if (!slot.wanted && !slot.mix.isRamping() && slot.mix.value() == 0.0)
            stopSlot(slot);
    }

    // The node's dry reference, delayed by the node latency so the delta stays aligned.
    if (nodeLatency > 0)
    {
        bandAlign.process(b, bandDelayedPtr, workChannels, numSamples, nodeLatency);
        b = bandDelayedPtr;
    }

    if (nodeMix.isRamping() || nodeMix.value() != 0.0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto g = static_cast<float>(nodeMix.next()) * presence[i];
            switch (mode)
            {
                case ChannelMode::Stereo:
                case ChannelMode::LinkedStereo:
                case ChannelMode::DualMono:
                    for (int ch = 0; ch < workChannels; ++ch)
                        out[ch][i] += g * (chainPtr[ch][i] - b[ch][i]);
                    break;
                case ChannelMode::Left:  out[0][i] += g * (chainPtr[0][i] - b[0][i]); break;
                case ChannelMode::Right: out[1][i] += g * (chainPtr[0][i] - b[0][i]); break;
                case ChannelMode::Mid:
                {
                    const float d = g * (chainPtr[0][i] - b[0][i]);
                    out[0][i] += d;
                    if (stereo)
                        out[1][i] += d;
                    break;
                }
                case ChannelMode::Side:
                {
                    const float d = g * (chainPtr[0][i] - b[0][i]);
                    out[0][i] += d;
                    out[1][i] -= d;
                    break;
                }
            }
        }
    }

    if (modeChangePending && !nodeMix.isRamping() && nodeMix.value() == 0.0)
    {
        mode = pendingMode;
        modeChangePending = false;
        for (auto& slot : slots)
            if (slot.running)
            {
                const auto params = slot.params;
                stopSlot(slot);
                if (slot.wanted)
                    startSlot(slot, params);
            }
        nodeMix.setTarget(static_cast<double>(targetMix), mixSteps);
    }
    // Bypassed node: input closed and every send has rung out. Inserts give 0 for a 0 input; a delayed
    // chain needs its alignment delay flushed too, so wait one node latency before stopping.
    if (!wanted && nodeClosed && !sendRunning)
    {
        bool silent = true;
        for (int ch = 0; ch < workChannels && silent; ++ch)
            for (int i = 0; i < numSamples && silent; ++i)
                silent = chainPtr[ch][i] == 0.0f;
        if (silent)
        {
            for (auto& slot : slots)
                stopSlot(slot);
            running = false;
        }
    }
}

float FxNodeProcessor::getCompressorInputDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    return slot.running && slot.type == FxType::Compressor ? slot.compressor.getInputLevelDb() : -120.0f;
}

float FxNodeProcessor::getCompressorAutoMakeupDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    return slot.running && slot.type == FxType::Compressor ? slot.compressor.getAutoMakeupDb() : 0.0f;
}

float FxNodeProcessor::getLimiterOutputDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    return slot.running && slot.type == FxType::Limiter ? slot.limiter.lastOutputPeakDb() : -120.0f;
}

UdarnikCore::Meters FxNodeProcessor::getTransientMeters(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    if (slot.running && slot.type == FxType::TransientShaper && slot.transient.udarnikActive())
        return slot.transient.udarnik().meters();
    return {};
}

int FxNodeProcessor::getTransientLastHit(int slotIndex, std::array<float, UdarnikCore::kGraphPoints>& in,
                                          std::array<float, UdarnikCore::kGraphPoints>& out) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    if (slot.running && slot.type == FxType::TransientShaper && slot.transient.udarnikActive())
        return slot.transient.udarnik().lastHit(in, out);
    return 0;
}

float FxNodeProcessor::getGainReductionDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    if (!slot.running)
        return 0.0f;
    if (slot.type == FxType::Compressor)
        return slot.compressor.getGainReductionDb();
    if (slot.type == FxType::Limiter)
        return slot.limiter.lastGainReductionDb();
    return 0.0f;
}

float FxNodeProcessor::getReverbOutputDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    if (!slot.running || slot.type != FxType::Reverb || slot.pooled < 0 || pools == nullptr)
        return -120.0f;
    return pools->reverbs.get(slot.pooled).outputLevelDb();
}

float FxNodeProcessor::getReverbDuckDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    if (!slot.running || slot.type != FxType::Reverb || slot.pooled < 0 || pools == nullptr)
        return 0.0f;
    return pools->reverbs.get(slot.pooled).duckingDb();
}

float FxNodeProcessor::getDelayOutputDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    if (!slot.running || slot.type != FxType::Delay || slot.pooled < 0 || pools == nullptr)
        return -120.0f;
    return pools->delays.get(slot.pooled).outputLevelDb();
}

float FxNodeProcessor::getDelayDuckDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    if (!slot.running || slot.type != FxType::Delay || slot.pooled < 0 || pools == nullptr)
        return 0.0f;
    return pools->delays.get(slot.pooled).duckingDb();
}

float FxNodeProcessor::getSaturatorInputDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    return slot.running && slot.type == FxType::Saturator ? slot.saturator.inputLevelDb() : -120.0f;
}

float FxNodeProcessor::getSaturatorOutputDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    return slot.running && slot.type == FxType::Saturator ? slot.saturator.outputLevelDb() : -120.0f;
}
} // namespace hpeq::dsp
