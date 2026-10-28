#include "DSP/RedWallColour.h"

#include <algorithm>
#include <cmath>

namespace hamlo::dsp
{
void RedWallColour::prepare(double sampleRate, int maxBlock)
{
    fs = sampleRate;
    maxChunk = std::max(1, maxBlock);
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(static_cast<size_t>(kMaxChannels), 2,
                                                                   juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampler->initProcessing(static_cast<size_t>(maxChunk));
    delay = static_cast<int>(std::lround(oversampler->getLatencyInSamples()));
    dryDelay.prepare(delay);
    for (auto& d : dry)
        d.assign(static_cast<size_t>(maxChunk), 0.0f);
    levels.assign(static_cast<size_t>(maxChunk), 1.0f);
    reset();
}

void RedWallColour::reset() noexcept
{
    if (oversampler != nullptr)
        oversampler->reset();
    dryDelay.reset();
    power = fade = 0.0;
    dcIn = {};
    dcOut = {};
    running = false;
}

void RedWallColour::process(float* const* channels, int numChannels, int numSamples, const LimiterSettings& p) noexcept
{
    for (int start = 0; start < numSamples; start += maxChunk)
    {
        float* chunk[kMaxChannels] {};
        const int n = std::min(maxChunk, numSamples - start);
        for (int c = 0; c < numChannels; ++c)
            chunk[c] = channels[c] + start;
        processChunk(chunk, numChannels, n, p);
    }
}

void RedWallColour::processChunk(float* const* channels, int numChannels, int numSamples, const LimiterSettings& p) noexcept
{
    numChannels = std::min(numChannels, kMaxChannels);
    const bool on = active(p);
    // the delayed input: the stage's output when off, and what it crossfades against
    float* dryPtr[kMaxChannels] {};
    for (int c = 0; c < numChannels; ++c)
    {
        dryPtr[c] = dry[static_cast<size_t>(c)].data();
        std::copy(channels[c], channels[c] + numSamples, dryPtr[c]);
    }
    dryDelay.process(dryPtr, dryPtr, numChannels, numSamples, delay);
    if (!on && fade <= 0.0)
    {
        running = false;
        for (int c = 0; c < numChannels; ++c)
            std::copy(dryPtr[c], dryPtr[c] + numSamples, channels[c]);
        return;
    }
    if (on && !running)
    {
        oversampler->reset(); // starts clean; the 10 ms fade hides the filter's start
        power = 0.0;
        dcIn = {};
        dcOut = {};
        running = true;
    }

    const double drive = std::clamp(static_cast<double>(p.values[slot::kLimDrive]), 0.0, 1.0);
    const double tube = std::clamp(static_cast<double>(p.values[slot::kLimTube]), 0.0, 1.0);
    const double ceiling = std::pow(10.0, std::clamp(static_cast<double>(p.values[slot::kCeilingDb]), -60.0, 0.0) / 20.0);
    const double push = std::pow(10.0, -std::clamp(static_cast<double>(p.values[slot::kLimThresholdDb]), -30.0, 0.0) / 20.0);
    const double scale = push / ceiling;
    const double h = 0.15 * tube, knee = std::pow(10.0, (12.0 - 9.0 * tube) / 20.0), width = 0.3 * knee;
    const double k = 1.0 + 2.5 * drive;
    const double levelC = std::exp(-1.0 / (0.050 * fs));
    const double fadeStep = 1.0 / (0.010 * fs);

    // the program's level at the base rate, linked
    for (int i = 0; i < numSamples; ++i)
    {
        double x2 = 0.0;
        for (int c = 0; c < numChannels; ++c)
            x2 = std::max(x2, static_cast<double>(channels[c][i]) * channels[c][i]);
        power = x2 + levelC * (power - x2);
        levels[static_cast<size_t>(i)] = static_cast<float>(std::max(1.0e-4, std::sqrt(power)));
    }

    juce::dsp::AudioBlock<float> block(channels, static_cast<size_t>(numChannels), static_cast<size_t>(numSamples));
    auto up = oversampler->processSamplesUp(block);
    const size_t factor = up.getNumSamples() / static_cast<size_t>(numSamples);
    for (size_t c = 0; c < up.getNumChannels(); ++c)
    {
        float* d = up.getChannelPointer(c);
        for (size_t i = 0; i < up.getNumSamples(); ++i)
        {
            double x = d[i];
            if (tube > 0.0)
            {
                const double level = levels[i / factor];
                double v = x / level;
                if (v > 0.0)
                    v -= h * v * v / (1.0 + v);
                const double m = std::abs(v);
                if (m > knee)
                    v = std::copysign(knee + width * std::tanh((m - knee) / width), v);
                x = v * level;
            }
            if (drive > 0.0)
                x = std::tanh(k * x * scale) / (k * scale);
            d[i] = static_cast<float>(x);
        }
    }
    oversampler->processSamplesDown(block);

    const double dcC = std::exp(-2.0 * 3.14159265358979323846 * 5.0 / fs);
    for (int i = 0; i < numSamples; ++i)
    {
        fade = std::clamp(fade + (on ? fadeStep : -fadeStep), 0.0, 1.0);
        for (int c = 0; c < numChannels; ++c)
        {
            const auto ci = static_cast<size_t>(c);
            const double y = channels[c][i];
            const double out = y - dcIn[ci] + dcC * dcOut[ci];
            dcIn[ci] = y;
            dcOut[ci] = out;
            channels[c][i] = static_cast<float>(dryPtr[c][i] + fade * (out - dryPtr[c][i]));
        }
    }
}
} // namespace hamlo::dsp
