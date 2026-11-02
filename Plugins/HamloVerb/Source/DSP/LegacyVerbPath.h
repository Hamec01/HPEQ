#pragma once
#include <array>
#include <vector>
#include "DSP/ReverbProcessor.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// The single stereo send node used by the M0 standalone Verb, with no other
// modules or layer graph. Keep every float rounding boundary: algebraically
// reducing this to dry + mix * wet breaks existing sessions' null tests.
class LegacyVerbPath
{
public:
    void prepare(double fs, int block)
    {
        reverb.prepare(fs);
        mixSteps = stepsFor(kMixRampSeconds, fs);
        silenceLimit = stepsFor(0.05, fs);
        for (auto* group : { &input, &wet })
            for (auto& channel : *group) channel.assign(static_cast<size_t>(block), 0.0f);
        reset();
    }
    void reset() noexcept
    {
        reverb.reset();
        running = false;
        silent = 0;
        nodeMix.reset(0.0);
        nodeMix.setTarget(1.0, mixSteps);
        mix.reset(0.0);
        inputGate.reset(0.0);
    }
    void setTargets(const FxSlotParams& params) noexcept
    {
        if (params.active && !running)
        {
            reverb.reset();
            running = true;
            silent = 0;
            mix.reset(params.mix);
            inputGate.reset(0.0);
            inputGate.setTarget(1.0, mixSteps);
        }
        if (running)
        {
            mix.setTarget(params.mix, mixSteps);
            inputGate.setTarget(params.active ? 1.0 : 0.0, mixSteps);
        }
    }
    void process(float* const* channels, int countChannels, int count,
                 const FxSlotParams& params, const Transport& transport) noexcept
    {
        setTargets(params);
        if (running)
        {
            for (int i = 0; i < count; ++i)
            {
                const auto g = static_cast<float>(inputGate.next());
                for (int ch = 0; ch < countChannels; ++ch)
                    input[static_cast<size_t>(ch)][static_cast<size_t>(i)] = g * channels[ch][i];
            }
            const float* inputs[] { input[0].data(), input[1].data() };
            float* outputs[] { wet[0].data(), wet[1].data() };
            reverb.process(inputs, outputs, countChannels, count, params, transport);
            for (int i = 0; i < count; ++i)
            {
                const auto m = static_cast<float>(mix.next());
                const auto g = static_cast<float>(nodeMix.next());
                for (int ch = 0; ch < countChannels; ++ch)
                {
                    const float d = channels[ch][i];
                    float chain = d;
                    chain += m * outputs[ch][i];
                    float delta = 0.0f;
                    delta += g * (chain - d);
                    channels[ch][i] += delta;
                }
            }
            const bool closed = !inputGate.isRamping() && inputGate.value() == 0.0;
            silent = closed && reverb.lastBlockPeak() < 1.0e-6f ? silent + count : 0;
            if (silent >= silenceLimit)
            {
                running = false;
                silent = 0;
                mix.reset(0.0);
                inputGate.reset(0.0);
            }
        }
        else
        {
            for (int i = 0; i < count; ++i) nodeMix.next();
        }
    }
    float outputLevelDb() const noexcept { return running ? reverb.outputLevelDb() : -120.0f; }
    float duckingDb() const noexcept { return running ? reverb.duckingDb() : 0.0f; }
private:
    ReverbProcessor reverb;
    std::array<std::vector<float>, 2> input, wet;
    LinearRamp nodeMix, mix, inputGate;
    bool running = false;
    int mixSteps = 1, silenceLimit = 1, silent = 0;
};
} // namespace hpeq::dsp
