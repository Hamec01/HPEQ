#pragma once

#include <array>
#include <atomic>
#include <vector>

#include "DSP/DspConfig.h"

#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
// Insert ("replace") processors that run on the band signal inside an FX node chain. Each works
// in place on numChannels x numSamples, is allocation-free and keeps per-channel state only.

// HamloFader, the band level utility (module pass 9): y_c = p_c * g * x_c(t - d), with
//   g = 10^(gainDb / 20) (0 when muted), p_c = -1 for an inverted channel, d = delay (0..20 ms).
// Unchanged settings with no polarity, delay or mute: exactly the old law y = g x (0 dB = identity),
// so older projects render bit-exactly. Changes glide: gain 20 ms, mute a linear 10 ms ramp to exactly 0,
// polarity 10 ms (through 0),
// delay 50 ms (a short pitch bend, like moving a microphone). Meters: band input RMS (300 ms and 3 s,
// the long one is what MATCH reads) and output RMS (300 ms). Solo is applied by the layer (LayerEngine).
class GainProcessor
{
public:
    static constexpr double kMaxDelayMs = 20.0;

    void prepare(double sampleRate); // allocates the delay lines
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    float inputDb() const noexcept { return meterIn.load(std::memory_order_relaxed); }
    float inputLongDb() const noexcept { return meterInLong.load(std::memory_order_relaxed); }
    float outputDb() const noexcept { return meterOut.load(std::memory_order_relaxed); }

private:
    double sampleRate = 48000.0;
    std::array<std::vector<float>, kMaxChannels> lines;
    int writePos = 0;
    bool primed = false;
    double gain = 1.0, delay = 0.0, muteGain = 1.0;
    std::array<double, kMaxChannels> polarity { 1.0, 1.0 };
    double msIn = 0.0, msInLong = 0.0, msOut = 0.0;
    std::atomic<float> meterIn { -120.0f }, meterInLong { -120.0f }, meterOut { -120.0f };
};

// Feed-forward compressor, written for HPEQ (DECISIONS D3), every behaviour documented:
//  - detector: sample peak; linked: x = 20 log10 max(|L|, |R|) and one gain for both channels,
//    unlinked (Dual Mono): one detector and gain per channel
//  - gain computer: soft knee of width W (dB) around threshold T, ratio R
//        x <  T - W/2 : y = x
//        |x - T| <= W/2: y = x + (1/R - 1) (x - T + W/2)^2 / (2W)
//        x >  T + W/2 : y = T + (x - T) / R
//  - ballistics: gain reduction c = x - y smoothed in dB with a branching one-pole
//        (attack when c rises, release when it falls); coefficient exp(-1 / (tau * fs)),
//        so 63 % of a step is reached after tau = attackMs / releaseMs exactly
//  - output: g = 10^((makeup - c_smoothed) / 20). Below threshold with 0 dB makeup g == 1 exactly.
// Options (M9 module pass; all off = the law above, bit-identical):
//  - detector RMS: x = 10 log10(m), m = one-pole mean square (time constant 10 ms) of the linked
//    channels' largest square (a sine reads 3.01 dB below its peak, as an RMS meter does)
//  - auto makeup: a = one-pole (2 s) of c_smoothed; g = 10^((makeup + a - c_smoothed) / 20), so the
//    long-term level stays about equal while the short-term dynamics are compressed
//  - character Punch: before the gain, x' = x + m (s tanh(x / s) - x), s = threshold (linear),
//    m = 0.35 min(1, c_smoothed / 12 dB): saturation relative to the threshold (independent of the
//    absolute level) that grows with the gain reduction; no reduction = exact identity
//  - character Glue: program-dependent release: the release coefficient is interpolated between the
//    ones of 0.25 x and 2 x the release time by h, a one-pole (1 s) of [c_smoothed > 1 dB]: short peaks
//    recover fast, long compression recovers slowly
// Reference: Giannoulis, Massberg, Reiss, "Digital Dynamic Range Compressor Design", JAES 2012.
class CompressorProcessor
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&, bool linked = true) noexcept;

    static double staticGainReductionDb(double levelDb, double thresholdDb, double ratio, double kneeDb) noexcept;
    float getGainReductionDb() const noexcept { return meterGainReductionDb.load(std::memory_order_relaxed); }
    // Detector level of the last block (dB, max over the block), for the input needle meter.
    float getInputLevelDb() const noexcept { return meterInputDb.load(std::memory_order_relaxed); }
    // Current auto makeup gain (dB).
    float getAutoMakeupDb() const noexcept { return meterAutoMakeupDb.load(std::memory_order_relaxed); }

private:
    static constexpr int kChunk = 256; // scratch size; longer calls are processed in chunks
    double sampleRate = 48000.0;
    std::array<double, kMaxChannels> smoothedReductionDb {}, meanSquare {}, hold {}, autoMakeupDb {};
    std::array<float, kChunk> level {}, target {}, gainDb {}, gain {}, reductionDb {};
    std::atomic<float> meterGainReductionDb { 0.0f }, meterInputDb { -120.0f }, meterAutoMakeupDb { 0.0f };
};
} // namespace hpeq::dsp
