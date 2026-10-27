#pragma once

#include <array>
#include <atomic>

namespace hamlo::dsp
{
constexpr int kMaxChannels = 2;

// The compressor's settings, in the units of its page (HPEQ passes the same values in its FxSlotParams:
// slot::kThresholdDb, kRatio, kKneeDb, kAttackMs, kReleaseMs, kMakeupDb, kCompDetector, kCompCharacter,
// kCompAutoMakeup).
struct CompressorSettings
{
    float thresholdDb = -30.0f;
    float ratio = 4.0f;
    float kneeDb = 6.0f;
    float attackMs = 1.0f;
    float releaseMs = 80.0f;
    float makeupDb = 0.0f;
    float detector = 0.0f;   // 0 Peak, 1 RMS
    float character = 1.0f;  // 0 Clean, 1 Punch, 2 Glue
    float autoMakeup = 1.0f; // 0 off, 1 on
};

// Feed-forward compressor (from HPEQ, DECISIONS D3), every behaviour documented:
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
    void process(float* const* channels, int numChannels, int numSamples, const CompressorSettings&, bool linked = true) noexcept;

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
} // namespace hamlo::dsp
