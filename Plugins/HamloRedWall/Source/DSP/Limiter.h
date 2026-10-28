#pragma once

#include <array>
#include <atomic>
#include <vector>

#include "DSP/HamloRedWall.h"
#include "DSP/LimiterSettings.h"

namespace hamlo::dsp
{
// Band limiter (insert) with look-ahead L samples (= its latency). Per sample the target gain
// t = min(1, ceiling / max|x|) is computed on the undelayed input; h = sliding minimum of t over the
// last L+1 samples; r = h with a release one-pole on rising values; the applied gain is the moving
// average of r over L samples, on audio delayed by L. Every value in that average window is <= the
// target of the sample being output, so the output sample peak never exceeds the ceiling.
// "Band limiter" limits this node's band only; the summed layer can still exceed it (01 §7).
class LimiterProcessor
{
public:
    static constexpr int kMaxLookahead = 2048;
    static constexpr double kMaxLookaheadMs = 10.0;
    // Buffer size for this rate: 10 ms (+1), at most kMaxLookahead (M8: memory follows the rate).
    static int maxLookaheadSamples(double sampleRate) noexcept;

    void prepare(double sampleRate); // allocates the look-ahead buffers
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const LimiterSettings&) noexcept;

    // Latency: the look-ahead, + 8 samples with TRUE PEAK (HamloRedWall).
    static int lookaheadSamples(const LimiterSettings&, double sampleRate) noexcept;
    static int baseLookaheadSamples(const LimiterSettings&, double sampleRate) noexcept;
    // True when any HamloRedWall option is set (otherwise the M6 law runs, bit-exact).
    static bool usesRedWall(const LimiterSettings&) noexcept;
    float lastGainReductionDb() const noexcept { return redWallActive ? core.lastGainReductionDb() : meter.load(std::memory_order_relaxed); }
    float lastOutputPeakDb() const noexcept { return redWallActive ? core.lastOutputPeakDb() : meterOutput.load(std::memory_order_relaxed); }

private:
    double sampleRate_ = 48000.0;
    int lookahead = -1;
    // Heap memory sized in prepare(): every slot owns a limiter, kept off the engine object itself.
    std::array<std::vector<float>, kMaxChannels> audio;
    std::vector<double> averaged; // the running sum adds and removes exactly these values (no drift)
    std::vector<float> dequeValue;
    std::vector<long long> dequeIndex;
    int dequeHead = 0, dequeTail = 0, capacity = 0;
    long long counter = 0;
    int writePos = 0;
    double released = 1.0, runningSum = 0.0;
    std::atomic<float> meter { 0.0f }, meterOutput { -120.0f };
    RedWallCore core;
    bool redWallActive = false;
};
} // namespace hamlo::dsp
