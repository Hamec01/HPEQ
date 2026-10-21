#include "DSP/FxNodeProcessor.h"
#include "DSP/Biquad.h"

#include <algorithm>

namespace hpeq::dsp
{
void ReverbPool::prepare(double sampleRate)
{
    for (auto& r : reverbs)
        r.prepare(sampleRate);
    used = {};
}

int ReverbPool::acquire() noexcept
{
    for (int i = 0; i < kSize; ++i)
        if (!used[static_cast<size_t>(i)])
        {
            used[static_cast<size_t>(i)] = true;
            reverbs[static_cast<size_t>(i)].reset();
            return i;
        }
    return -1;
}

void ReverbPool::release(int index) noexcept
{
    if (index >= 0)
        used[static_cast<size_t>(index)] = false;
}

int ReverbPool::inUse() const noexcept
{
    return static_cast<int>(std::count(used.begin(), used.end(), true));
}

void FxNodeProcessor::prepare(double newSampleRate, int maxBlockSize, ReverbPool* reverbPool)
{
    sampleRate = newSampleRate;
    pool = reverbPool;
    mixSteps = stepsFor(kMixRampSeconds, sampleRate);
    silenceSamples = stepsFor(0.05, sampleRate);
    for (auto* group : { &chain, &work, &bandWork, &gated, &wet })
        for (auto& channel : *group)
            channel.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    for (auto& slot : slots)
    {
        slot.gain.prepare(sampleRate);
        slot.compressor.prepare(sampleRate);
    }
    reset();
}

void FxNodeProcessor::reset() noexcept
{
    wanted = running = false;
    nodeMix.reset(0.0);
    gate.reset(1.0);
    for (auto& slot : slots)
        stopSlot(slot);
}

void FxNodeProcessor::stopSlot(Slot& slot) noexcept
{
    if (pool != nullptr)
        pool->release(slot.reverb);
    slot.reverb = -1;
    slot.running = false;
    slot.silentSamples = 0;
    slot.mix.reset(0.0);
    slot.inGate.reset(0.0);
    slot.gain.reset();
    slot.compressor.reset();
}

void FxNodeProcessor::startSlot(Slot& slot, const FxSlotParams& sp) noexcept
{
    slot.type = sp.type;
    slot.gain.reset();
    slot.compressor.reset();
    slot.silentSamples = 0;
    if (isSend(sp.type))
    {
        slot.reverb = pool != nullptr ? pool->acquire() : -1;
        slot.starved = slot.reverb < 0;
        if (slot.starved)
            return; // stays off; reported through slotStarved()
        // A send starts with silence in its delay lines: open the input, output weight at once.
        slot.mix.reset(sp.mix);
        slot.inGate.reset(0.0);
        slot.inGate.setTarget(1.0, mixSteps);
    }
    else
    {
        slot.starved = false;
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
        for (auto& slot : slots)
            stopSlot(slot);
    }
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
        const bool inChain = k < p.numSlots;
        const auto& sp = p.slots[static_cast<size_t>(k)];
        slot.wanted = inChain && sp.active;

        if (slot.running && inChain && slot.type != sp.type)
            stopSlot(slot); // a different processor: never reuse its state
        if (slot.wanted && !slot.running)
            startSlot(slot, sp);
        if (!slot.wanted)
            slot.starved = false;
        if (inChain)
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

void FxNodeProcessor::runInsert(Slot& slot, float* const* channels, int numChannels, int numSamples) noexcept
{
    switch (slot.type)
    {
        case FxType::Gain:       slot.gain.process(channels, numChannels, numSamples, slot.params); break;
        case FxType::Compressor: slot.compressor.process(channels, numChannels, numSamples, slot.params, mode != ChannelMode::DualMono); break;
        case FxType::Reverb:
        case FxType::Limiter:
        case FxType::Delay:
        case FxType::TransientShaper:
        case FxType::Saturator:
        case FxType::MonstaFx:
        case FxType::StereoProcessor:
        case FxType::GateExpander:
            break; // sends are handled separately; the rest arrive in M6
    }
}

void FxNodeProcessor::process(const float* const* bandPtr, const float* presence, float* const* out, int numChannels, int numSamples) noexcept
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
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        bandWorkPtr[ch] = bandWork[static_cast<size_t>(ch)].data();
        gatedPtr[ch] = gated[static_cast<size_t>(ch)].data();
        chainPtr[ch] = chain[static_cast<size_t>(ch)].data();
        workPtr[ch] = work[static_cast<size_t>(ch)].data();
        wetPtr[ch] = wet[static_cast<size_t>(ch)].data();
    }

    const float* const* b = bandPtr; // Stereo-like modes and Left use the band as it is
    if (workChannels > 0)
    {
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
    }

    bool sendRunning = false;
    for (auto& slot : slots)
    {
        if (!slot.running)
            continue;
        if (isSend(slot.type))
        {
            if (workChannels > 0)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    const auto g = static_cast<float>(slot.inGate.next());
                    for (int ch = 0; ch < workChannels; ++ch)
                        workPtr[ch][i] = g * chainPtr[ch][i];
                }
                auto& reverb = pool->get(slot.reverb);
                reverb.process(workPtr, wetPtr, workChannels, numSamples, slot.params);
                for (int i = 0; i < numSamples; ++i)
                {
                    const auto m = static_cast<float>(slot.mix.next());
                    for (int ch = 0; ch < workChannels; ++ch)
                        chainPtr[ch][i] += m * wetPtr[ch][i];
                }
                const bool nodeClosed = !gate.isRamping() && gate.value() == 0.0;
                const bool inputClosed = nodeClosed || (!slot.inGate.isRamping() && slot.inGate.value() == 0.0);
                slot.silentSamples = inputClosed && reverb.lastBlockPeak() < 1.0e-6f ? slot.silentSamples + numSamples : 0;
                if (slot.silentSamples >= silenceSamples)
                {
                    stopSlot(slot); // tail has rung out
                    continue;
                }
            }
            sendRunning = true;
            continue;
        }
        if (workChannels > 0)
        {
            for (int ch = 0; ch < workChannels; ++ch)
                std::copy(chainPtr[ch], chainPtr[ch] + numSamples, workPtr[ch]);
            runInsert(slot, workPtr, workChannels, numSamples);
        }
        for (int i = 0; i < numSamples; ++i)
        {
            const auto m = static_cast<float>(slot.mix.next());
            if (m == 1.0f)
                for (int ch = 0; ch < workChannels; ++ch)
                    chainPtr[ch][i] = workPtr[ch][i];
            else if (m != 0.0f)
                for (int ch = 0; ch < workChannels; ++ch)
                    chainPtr[ch][i] += m * (workPtr[ch][i] - chainPtr[ch][i]);
        }
        if (!slot.wanted && !slot.mix.isRamping() && slot.mix.value() == 0.0)
            stopSlot(slot);
    }

    if (nodeMix.isRamping() || nodeMix.value() != 0.0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto g = static_cast<float>(nodeMix.next()) * presence[i];
            if (workChannels == 0)
                continue;
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
    // Bypassed node: input closed and every send has rung out. Inserts give exactly 0 for a 0 input.
    if (!wanted && !gate.isRamping() && gate.value() == 0.0 && !sendRunning)
    {
        for (auto& slot : slots)
            stopSlot(slot);
        running = false;
    }
}

float FxNodeProcessor::getGainReductionDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    return slot.running && slot.type == FxType::Compressor ? slot.compressor.getGainReductionDb() : 0.0f;
}
} // namespace hpeq::dsp
