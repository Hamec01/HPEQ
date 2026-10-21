#include "DSP/FxNodeProcessor.h"
#include "DSP/Biquad.h"

#include <algorithm>

namespace hpeq::dsp
{
void FxNodeProcessor::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    mixSteps = stepsFor(kMixRampSeconds, sampleRate);
    for (auto* group : { &chain, &work })
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
    for (auto& slot : slots)
    {
        slot.wanted = slot.running = false;
        slot.mix.reset(0.0);
        slot.gain.reset();
        slot.compressor.reset();
    }
}

void FxNodeProcessor::setTargets(const FxNodeParams& p) noexcept
{
    wanted = p.active;
    if (wanted && !running)
    {
        running = true;
        nodeMix.reset(0.0);
        for (auto& slot : slots)
        {
            slot.running = false;
            slot.mix.reset(0.0);
        }
    }
    if (!running)
        return;

    lowHz = design::clampFrequency(p.lowHz, sampleRate);
    highHz = design::clampFrequency(p.highHz, sampleRate);
    if (highHz < lowHz)
        std::swap(lowHz, highHz);
    nodeMix.setTarget(wanted ? static_cast<double>(p.mix) : 0.0, mixSteps);

    for (int k = 0; k < kMaxFxPerNode; ++k)
    {
        auto& slot = slots[static_cast<size_t>(k)];
        const bool inChain = k < p.numSlots;
        const auto& sp = p.slots[static_cast<size_t>(k)];
        slot.wanted = inChain && sp.active;

        if (slot.running && slot.type != sp.type && inChain)
            slot.running = false; // a different processor: restart it with a fade-in, never reuse state

        if (slot.wanted && !slot.running)
        {
            slot.running = true;
            slot.type = sp.type;
            slot.gain.reset();
            slot.compressor.reset();
            slot.mix.reset(0.0);
        }
        if (inChain)
            slot.params = sp;
        if (slot.running)
            slot.mix.setTarget(slot.wanted ? static_cast<double>(sp.mix) : 0.0, mixSteps);
    }
}

void FxNodeProcessor::runProcessor(Slot& slot, float* const* channels, int numChannels, int numSamples) noexcept
{
    switch (slot.type)
    {
        case FxType::Gain:       slot.gain.process(channels, numChannels, numSamples, slot.params); break;
        case FxType::Compressor: slot.compressor.process(channels, numChannels, numSamples, slot.params); break;
        case FxType::Limiter:
        case FxType::Reverb:
        case FxType::Delay:
        case FxType::TransientShaper:
        case FxType::Saturator:
        case FxType::MonstaFx:
        case FxType::StereoProcessor:
        case FxType::GateExpander:
            break; // not processed in M2; the snapshot never activates these
    }
}

void FxNodeProcessor::process(const float* const* bandPtr, const float* presence, float* const* out, int numChannels, int numSamples) noexcept
{
    if (!running)
        return;

    float* chainPtr[kMaxChannels] {};
    float* workPtr[kMaxChannels] {};
    for (int ch = 0; ch < numChannels; ++ch)
    {
        chainPtr[ch] = chain[static_cast<size_t>(ch)].data();
        workPtr[ch] = work[static_cast<size_t>(ch)].data();
    }

    for (int ch = 0; ch < numChannels; ++ch)
        std::copy(bandPtr[ch], bandPtr[ch] + numSamples, chainPtr[ch]);

    for (auto& slot : slots)
    {
        if (!slot.running)
            continue;
        for (int ch = 0; ch < numChannels; ++ch)
            std::copy(chainPtr[ch], chainPtr[ch] + numSamples, workPtr[ch]);
        runProcessor(slot, workPtr, numChannels, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const auto m = static_cast<float>(slot.mix.next());
            if (m == 1.0f)
                for (int ch = 0; ch < numChannels; ++ch)
                    chainPtr[ch][i] = workPtr[ch][i];
            else if (m != 0.0f)
                for (int ch = 0; ch < numChannels; ++ch)
                    chainPtr[ch][i] += m * (workPtr[ch][i] - chainPtr[ch][i]);
        }
        if (!slot.wanted && !slot.mix.isRamping() && slot.mix.value() == 0.0)
            slot.running = false;
    }

    if (nodeMix.isRamping() || nodeMix.value() != 0.0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto g = static_cast<float>(nodeMix.next()) * presence[i];
            for (int ch = 0; ch < numChannels; ++ch)
                out[ch][i] += g * (chainPtr[ch][i] - bandPtr[ch][i]);
        }
    }
    if (!wanted && !nodeMix.isRamping() && nodeMix.value() == 0.0)
        running = false;
}

float FxNodeProcessor::getGainReductionDb(int slotIndex) const noexcept
{
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    return slot.running && slot.type == FxType::Compressor ? slot.compressor.getGainReductionDb() : 0.0f;
}
} // namespace hpeq::dsp
