#include "DSP/PreEqProcessor.h"

#include <cmath>

namespace hpeq::dsp
{
void PreEqProcessor::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    rampSteps = stepsFor(kParameterRampSeconds, sampleRate / kControlInterval);
    reset();
}

void PreEqProcessor::reset() noexcept
{
    for (auto& band : bands)
    {
        band = Band {};
        band.logFrequency.reset(std::log2(1000.0));
        band.q.reset(0.707);
    }
}

void PreEqProcessor::setTargets(const std::array<EqBandParams, kMaxEqNodesPerLayer>& targets) noexcept
{
    for (size_t i = 0; i < bands.size(); ++i)
    {
        auto& band = bands[i];
        const auto& t = targets[i];
        const double logF = std::log2(design::clampFrequency(t.frequencyHz, sampleRate));
        const bool wanted = t.active && t.gainDb != 0.0f;

        if (wanted && !band.running)
        {
            // Enter at 0 dB with the new frequency/Q, then glide the gain in.
            band.filter.reset();
            band.logFrequency.reset(logF);
            band.q.reset(t.q);
            band.gainDb.reset(0.0);
            band.running = true;
        }
        band.wanted = wanted;
        if (band.running)
        {
            band.logFrequency.setTarget(logF, rampSteps);
            band.q.setTarget(t.q, rampSteps);
            band.gainDb.setTarget(wanted ? static_cast<double>(t.gainDb) : 0.0, rampSteps);
            updateCoefficients(band);
        }
    }
}

void PreEqProcessor::updateCoefficients(Band& band) noexcept
{
    band.filter.setCoefficients(design::peak(sampleRate, std::exp2(band.logFrequency.value()), band.q.value(), band.gainDb.value()));
}

void PreEqProcessor::advanceControl(Band& band) noexcept
{
    if (!band.logFrequency.isRamping() && !band.gainDb.isRamping() && !band.q.isRamping())
        return;
    band.logFrequency.next();
    band.gainDb.next();
    band.q.next();
    updateCoefficients(band);
    if (!band.wanted && !band.gainDb.isRamping() && band.gainDb.value() == 0.0)
        band.running = false; // faded out to an exact 0 dB: leave the path
}

void PreEqProcessor::process(float* const* channels, int numChannels, int numSamples, int controlPhase) noexcept
{
    for (auto& band : bands)
    {
        if (!band.running)
            continue;
        int phase = controlPhase;
        for (int i = 0; i < numSamples; ++i)
        {
            if (phase == 0)
            {
                advanceControl(band);
                if (!band.running)
                    break;
            }
            for (int ch = 0; ch < numChannels; ++ch)
                channels[ch][i] = static_cast<float>(band.filter.processSample(ch, channels[ch][i]));
            phase = (phase + 1) % kControlInterval;
        }
    }
}

bool PreEqProcessor::isNeutral() const noexcept
{
    for (const auto& band : bands)
        if (band.running)
            return false;
    return true;
}
} // namespace hpeq::dsp
