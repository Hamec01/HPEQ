#pragma once

#include <array>
#include <memory>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "DSP/AlignmentDelay.h"
#include "DSP/MagSettings.h"
#include "DSP/Saturator.h"

namespace hamlo::dsp
{
// HamloMag v2 (docs/MAG_V2.md) around HPEQ's saturator. With SAMPLER, PHONE, SUB and DYNAMICS off it is the
// SaturatorProcessor exactly. Otherwise, per block (L = the saturator's latency for the settings):
//   SAMPLER  x -> 4x up -> [MPC60 / S950: anti-alias low-pass, a 257-tap Kaiser (beta 7.86, about -80 dB) FIR at
//            18 / 14 kHz, computed only at the device's ticks; SP-1200: none - it aliases - the same 128-sample delay]
//            -> sampled at the device rate (26.04 / 40 / 32 kHz, linear interpolation at the tick) -> 12-bit word
//            (MPC60 companded, mu = 7) -> held (the DAC's zero-order hold) -> output low-pass, 4th order (SP-1200
//            12 kHz, MPC60 18 kHz, S950 14 kHz) -> 4x down; latency S = the 4x oversampler's + 32 samples
//            (run 1 used an 8th-order Butterworth anti-alias filter: -20 dB at 21 kHz for the MPC60)
//   SUB      Linkwitz-Riley 4th order at SUB Hz: the high band is saturated, the low band delayed by L (CLASSIC:
//            through the same 10 Hz DC blocker) and added back - magnitude flat, the sub never saturated
//   DYNAMICS fast (1 ms / 30 ms) and slow (30 ms / 300 ms) peak envelopes of the input, d = 20 log10(fast / slow),
//            hit = clamp(d / 6, 0, 1); the saturated share w = 1 - |DYNAMICS| (1 - s), s = hit (+) or 1 - hit (-):
//            y = D_L(x) + w (sat(x) - D_L(x)) (w delayed by L) - at +100 % the hits saturate and the body stays clean,
//            at -100 % the other way round, 0 = as before. (Runs 1 .. 3 scaled the drive by +-12 dB and gave the level
//            back: HamloLaw's harmonics are bounded, so the extra drive on the hits then turned their harmonics down -
//            the lab read the opposite of the intent, docs/reports/v2/V2_REPORT.md)
//   PHONE    everything below 120 Hz (LR4), its peak envelope e (1 ms / 100 ms; 30 ms rippled on 50 Hz), u = l / e:
//            h = e (0.22 T2 + 0.22 T3 + 0.20 T4 + 0.20 T5 + 0.18 T6)(u) (Chebyshev: a tone gets exactly H2 .. H6, so
//            a 40 Hz sub reaches 200 .. 240 Hz, where a phone plays), band-limited to 70 .. 700 Hz (LR4 high-pass,
//            LR4 low-pass), x PHONE, delayed by L and added (run 1 stopped at H4: a deep sub gained 1 LU on a phone)
class MagV2Processor
{
public:
    void prepare(double sampleRate, int maxBlockSize); // allocates
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    static bool usesV2(const FxSlotParams&) noexcept; // SUB, PHONE or DYNAMICS on (the SAMPLER runs on its own)
    static bool usesSampler(const FxSlotParams&) noexcept;
    static constexpr int kFirTaps = 257, kFirDelay = 128; // at 4x: 32 base samples
    static int samplerLatency() { return SaturatorProcessor::latencySamples(2) + kFirDelay / 4; }
    static int latencySamples(const FxSlotParams&, double sampleRate);
    float inputLevelDb() const noexcept { return core.inputLevelDb(); }
    float outputLevelDb() const noexcept { return core.outputLevelDb(); }

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        double process(double x) noexcept
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
        void lowPass(double hz, double q, double fs) noexcept;
        void highPass(double hz, double q, double fs) noexcept;
        void reset() noexcept { z1 = z2 = 0.0; }
    };
    struct Channel
    {
        std::array<Biquad, 2> subLow, subHigh, phoneLow, phoneHigh, phoneTop;
        std::array<Biquad, 2> output;
        std::array<float, 512> ring {}; // the 4x input, for the FIR at the ticks (512 >= kFirTaps + 1)
        int write = 0;
        double phoneEnvelope = 0.0, dcIn = 0.0, dcOut = 0.0;
        double held = 0.0;
    };
    void sample(float* const* channels, int numChannels, int numSamples, int mode) noexcept;
    void design(const FxSlotParams&) noexcept;

    double fs = 48000.0;
    int maxBlock = 1;
    SaturatorProcessor core;
    std::unique_ptr<juce::dsp::Oversampling<float>> samplerOs;
    std::array<Channel, kMaxChannels> state;
    std::array<std::vector<float>, kMaxChannels> low, phone, source, dryPart;
    std::vector<float> modulation;
    AlignmentDelay lowDelay, phoneDelay, modDelay, dryDelay;
    double fastEnv = 0.0, slowEnv = 0.0, devicePhase = 0.0;
    std::array<std::array<float, kFirTaps>, 4> fir {}; // per device (0, 1 unused)
    double designedSub = -1.0;
    int designedSampler = -1;
};
} // namespace hamlo::dsp
