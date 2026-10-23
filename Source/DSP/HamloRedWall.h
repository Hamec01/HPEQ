#pragma once

#include <array>
#include <atomic>
#include <vector>

#include "DSP/DspConfig.h"
#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
// HamloRedWall (module pass 5, LIMITER_DESIGN.md): the limiter's options around the M6 law. The
// LimiterProcessor runs this core only when an option is set; with every option at its default the M6
// code runs (bit-exact for older projects).
//
// Per sample and channel c:
//   u_c = x_c * 10^(-T/20)                                     THRESHOLD T (push)
//   OCTOBER: u_c = s tanh(u_c / s), s = 1.25 ceiling            soft clip before the limiter
//   TRUE PEAK: v_c[n] = u_c[n - 8], level = max(|v|, 4x interpolated points on both sides of it)
//              (16-tap polyphase windowed sinc); otherwise v = u, level = |u|
//   pre-delay P = L - Le (Le = L, STAKHANOV: max(1, L / 4))
//   t_c = min(1, ceiling / level_c);  LINK l: t'_c = l min(t_L, t_R) + (1 - l) t_c
//   M6 core with look-ahead Le per channel: sliding minimum over Le + 1, release (AUTO: coefficient
//   between 0.25 x and 6 x the release by a 0.3 s hold follower of [reduction > 1 dB]; STAKHANOV: release
//   halved), moving average over Le, audio delayed by Le  ->  y_c (sample peak <= ceiling)
//   MONITOR: UNITY y * 10^(T/20); DELTA (delayed core input) - y
// Latency = L (+ 8 with TRUE PEAK).
class RedWallCore
{
public:
    static constexpr int kTruePeakDelay = 8;

    void prepare(double sampleRate, int maxLookahead);
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&, int lookahead) noexcept;

    float lastGainReductionDb() const noexcept { return meterReduction.load(std::memory_order_relaxed); }
    float lastOutputPeakDb() const noexcept { return meterOutput.load(std::memory_order_relaxed); }

private:
    struct Channel
    {
        std::array<float, 16> history {};
        float previousInterSample = 0.0f;
        std::vector<float> preAudio, preLevel; // pre-delay ring
        std::vector<float> audio;              // core delay ring
        std::vector<double> averaged;
        std::vector<float> dequeValue;
        std::vector<long long> dequeIndex;
        int dequeHead = 0, dequeTail = 0;
        double released = 1.0, runningSum = 0.0, hold = 0.0;
    };

    double sampleRate = 48000.0;
    int capacity = 0;        // ring sizes
    int coreLookahead = -1;  // Le of the current state
    int preDelay = -1;       // P of the current state
    bool truePeak = false;
    long long counter = 0;
    int prePos = 0, corePos = 0;
    std::array<Channel, kMaxChannels> ch;
    std::array<std::array<float, 16>, 3> phaseTaps {}; // fractions 1/4, 2/4, 3/4
    std::atomic<float> meterReduction { 0.0f }, meterOutput { -120.0f };
};
} // namespace hpeq::dsp
