#pragma once

#include <array>

#include "DSP/Biquad.h"
#include "DSP/LayerSnapshot.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// PRE-EQ of one layer (decision B: always before the FX nodes). M2 scope: Bell bands.
//
// Each band glides frequency (log), gain (dB) and Q at control rate; coefficients are recomputed on
// the control clock only while something moves. Switching a band on fades its gain in from 0 dB;
// switching it off fades to 0 dB and then the band is skipped. A band at exactly 0 dB is not run,
// so an EQ with no audible band is a bit-exact pass-through (T01).
class PreEqProcessor
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    // Applies new targets; called once per block before process().
    void setTargets(const std::array<EqBandParams, kMaxEqNodesPerLayer>&) noexcept;

    // In-place. `controlPhase` is the engine's running position within the control interval.
    void process(float* const* channels, int numChannels, int numSamples, int controlPhase) noexcept;

    bool isNeutral() const noexcept;

private:
    struct Band
    {
        bool wanted = false;   // target says the band should sound
        bool running = false;  // band is in the signal path
        LinearRamp logFrequency, gainDb, q;
        Biquad filter;
    };

    void updateCoefficients(Band&) noexcept;
    void advanceControl(Band&) noexcept;

    double sampleRate = 48000.0;
    int rampSteps = 1;
    std::array<Band, kMaxEqNodesPerLayer> bands;
};
} // namespace hpeq::dsp
