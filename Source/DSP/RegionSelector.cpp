#include "DSP/RegionSelector.h"

#include <cmath>

namespace hpeq::dsp
{
void RegionSelector::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    rampSteps = stepsFor(kParameterRampSeconds, sampleRate / kControlInterval);
    reset(250.0, 700.0);
}

void RegionSelector::reset(double lowHz, double highHz) noexcept
{
    logLow.reset(std::log2(design::clampFrequency(lowHz, sampleRate)));
    logHigh.reset(std::log2(design::clampFrequency(highHz, sampleRate)));
    for (auto& s : sections)
        s.reset();
    updateCoefficients();
}

void RegionSelector::setRange(double lowHz, double highHz) noexcept
{
    const double low = std::log2(design::clampFrequency(lowHz, sampleRate));
    const double high = std::log2(design::clampFrequency(highHz, sampleRate));
    if (low == logLow.targetValue() && high == logHigh.targetValue())
        return;
    logLow.setTarget(low, rampSteps);
    logHigh.setTarget(high, rampSteps);
}

void RegionSelector::updateCoefficients() noexcept
{
    const auto hp = design::highPass(sampleRate, std::exp2(logLow.value()), design::kButterworthQ);
    const auto lp = design::lowPass(sampleRate, std::exp2(logHigh.value()), design::kButterworthQ);
    sections[0].setCoefficients(hp);
    sections[1].setCoefficients(hp);
    sections[2].setCoefficients(lp);
    sections[3].setCoefficients(lp);
}

void RegionSelector::process(const float* const* in, float* const* out, int numChannels, int numSamples, int controlPhase) noexcept
{
    int phase = controlPhase;
    for (int i = 0; i < numSamples; ++i)
    {
        if (phase == 0 && (logLow.isRamping() || logHigh.isRamping()))
        {
            logLow.next();
            logHigh.next();
            updateCoefficients();
        }
        for (int ch = 0; ch < numChannels; ++ch)
        {
            double v = in[ch][i];
            for (auto& s : sections)
                v = s.processSample(ch, v);
            out[ch][i] = static_cast<float>(v);
        }
        phase = (phase + 1) % kControlInterval;
    }
}

double RegionSelector::currentLowHz() const noexcept { return std::exp2(logLow.value()); }
double RegionSelector::currentHighHz() const noexcept { return std::exp2(logHigh.value()); }

} // namespace hpeq::dsp
