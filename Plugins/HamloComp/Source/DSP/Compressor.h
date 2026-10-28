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
    // HamloComp only (docs/COMPRESSOR_MODELS.md); HPEQ has neither: 0 = CLASSIC / ALL, the law below.
    float model = 0.0f;  // 0 CLASSIC, 1 VCA, 2 FET, 3 OPTO, 4 TUBE
    float source = 0.0f; // 0 ALL, 1 DRUMS, 2 SAMPLES, 3 VOC
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
//
// HamloComp models and sources (docs/COMPRESSOR_MODELS.md; CLASSIC + ALL = everything above, bit-identical):
//  - SOURCE shapes only what the detector hears (2nd-order RBJ biquads, per channel): DRUMS high-pass
//    100 Hz; SAMPLES high-pass 50 Hz and a 30 ms RMS averaged with the level; VOC two high-passes at 120 Hz
//    and a +4 dB bell at 3 kHz (Q 1). It works with every model, CLASSIC included.
//  - VCA: feed-forward, the law as set; auto release: a fast envelope (ATTACK / RELEASE) and a slow one
//    (charges with 300 ms, releases with 5 x RELEASE), the larger wins - short peaks recover fast,
//    sustained compression slowly. No colour.
//  - FET: feedback - the detector listens to the previous output sample, with the gain computer written
//    on the output level (c = (y - T)(R - 1) above the knee), so the static curve is still the law; the
//    release runs faster from deep reduction (time RELEASE / (1 + c / 10 dB)). The attack step per sample is
//    at most 0.5 / (R - 1) of the distance (and the target at most 60 dB), which keeps the loop from ringing.
//    Colour: even + odd.
//  - OPTO: RMS (5 ms) detector, knee >= 12 dB; a fast stage f (ATTACK / RELEASE) and a slow stage s that
//    follows f (charges with 0.5 s, releases with 2 s (1 + 2 m), m = one-pole (5 s) of [f > 1 dB]);
//    c = max(f, s / 2): half the reduction comes back with RELEASE, the rest slowly, slower after long
//    compression. Colour: a little even.
//  - TUBE: RMS (10 ms) detector; vari-mu curve: the slope of the output curve is 1 / r(u) with
//    r(u) = 1 + (R - 1) min(1, u / W), u = level over the threshold, W = max(18 dB, knee) - the ratio rises
//    from 1 : 1 at the threshold to RATIO W over it; release program-dependent: RELEASE (1 + 7 h), h = one-pole
//    (1 s) of [c > 1 dB]. Colour: mostly even.
//  - colour, after the gain, per channel: y += sT m (k2 (e - <e>) + k3 (tanh u - u)), u = y / sT,
//    sT = threshold (linear), e = u^2 / (1 + |u|), <e> its 20 Hz mean, m = min(1, c / 12 dB); (k2, k3) =
//    FET (0.03, 0.08), OPTO (0.01, 0), TUBE (0.04, 0.02). Without gain reduction m = 0: an exact identity.
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
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float run(float x) noexcept
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return static_cast<float>(y);
        }
    };
    static constexpr int kScStages = 3;
    void setSource(int source) noexcept;
    float sidechain(int channel, float x) noexcept;
    void processModel(float* const* channels, int numChannels, int numSamples, const CompressorSettings&, bool linked, int model) noexcept;

    static constexpr int kChunk = 256; // scratch size; longer calls are processed in chunks
    double sampleRate = 48000.0;
    std::array<double, kMaxChannels> smoothedReductionDb {}, meanSquare {}, hold {}, autoMakeupDb {};
    std::array<float, kChunk> level {}, target {}, gainDb {}, gain {}, reductionDb {};
    std::atomic<float> meterGainReductionDb { 0.0f }, meterInputDb { -120.0f }, meterAutoMakeupDb { 0.0f };

    // sources and models
    int scSource = -1;
    std::array<std::array<Biquad, kScStages>, kMaxChannels> sc {};
    std::array<std::array<float, kChunk>, kMaxChannels> scBuffer {};
    std::array<double, kMaxChannels> fastDb {}, slowDb {}, memory {}, meanSquare30 {}, evenMean {}, lastOut {};
};
} // namespace hamlo::dsp
