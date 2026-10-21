#pragma once

#include <array>

#include "DSP/Biquad.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// H1 frequency selector of an FX node: b = LR4_highpass(low) -> LR4_lowpass(high) applied to xE.
// Each LR4 side is two cascaded 2nd-order Butterworth sections (24 dB/oct, -6 dB at the edge).
// Edges glide in log-frequency on the control clock; latency is 0 (minimum-phase IIR).
class RegionSelector
{
public:
    void prepare(double sampleRate) noexcept;
    void reset(double lowHz, double highHz) noexcept;
    void setRange(double lowHz, double highHz) noexcept;

    // out[ch][i] = band-selected in[ch][i]. `controlPhase` as in PreEqProcessor.
    void process(const float* const* in, float* const* out, int numChannels, int numSamples, int controlPhase) noexcept;

    double currentLowHz() const noexcept;
    double currentHighHz() const noexcept;

private:
    void updateCoefficients() noexcept;

    double sampleRate = 48000.0;
    int rampSteps = 1;
    LinearRamp logLow, logHigh;
    std::array<Biquad, 4> sections; // HP, HP, LP, LP
};

} // namespace hpeq::dsp
