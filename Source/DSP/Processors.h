#pragma once

#include <atomic>

#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
// Insert ("replace") processors that run on the band signal inside an FX node chain. Each works
// in place on numChannels x numSamples, is allocation-free and keeps per-channel state only.

// Utility gain: y = g x. 0 dB is an exact identity.
class GainProcessor
{
public:
    void prepare(double) noexcept {}
    void reset() noexcept {}
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;
};

// Feed-forward compressor, written for HPEQ (DECISIONS D3), every behaviour documented:
//  - detector: stereo-linked sample peak, level x = 20 log10 max(|L|, |R|)
//  - gain computer: soft knee of width W (dB) around threshold T, ratio R
//        x <  T - W/2 : y = x
//        |x - T| <= W/2: y = x + (1/R - 1) (x - T + W/2)^2 / (2W)
//        x >  T + W/2 : y = T + (x - T) / R
//  - ballistics: gain reduction c = x - y smoothed in dB with a branching one-pole
//        (attack when c rises, release when it falls); coefficient exp(-1 / (tau * fs)),
//        so 63 % of a step is reached after tau = attackMs / releaseMs exactly
//  - output: g = 10^((makeup - c_smoothed) / 20). Below threshold with 0 dB makeup g == 1 exactly.
// Reference: Giannoulis, Massberg, Reiss, "Digital Dynamic Range Compressor Design", JAES 2012.
class CompressorProcessor
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    static double staticGainReductionDb(double levelDb, double thresholdDb, double ratio, double kneeDb) noexcept;
    float getGainReductionDb() const noexcept { return meterGainReductionDb.load(std::memory_order_relaxed); }

private:
    double sampleRate = 48000.0;
    double smoothedReductionDb = 0.0;
    std::atomic<float> meterGainReductionDb { 0.0f };
};
} // namespace hpeq::dsp
