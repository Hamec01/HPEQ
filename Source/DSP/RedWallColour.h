#pragma once

#include <array>
#include <memory>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "DSP/AlignmentDelay.h"
#include "DSP/LimiterSettings.h"

namespace hamlo::dsp
{
// HamloRedWall's colour stage before the limiter (docs/REDWALL_V2.md): TUBE then DRIVE, 4x oversampled. Its latency
// is the oversampler's and constant, so the plug-in's latency does not move when the knobs do. With DRIVE = TUBE = 0
// the stage is the input delayed by that latency, bit-exact. Per sample:
//   TUBE   relative to the program's own level L = RMS 50 ms of the linked input (base rate, floor -80 dBFS); v = x / L
//          warmth  v > 0: v - h v^2 / (1 + v), h = 0.15 TUBE - the positive half-wave bent: H2, no expansion
//          weight  |v| above K = 12 - 9 TUBE dB: K + w tanh((|v| - K) / w), w = 0.3 K - the peaks rounded against
//                  the body (a lower crest factor, the peak never raised)
//          x = v L
//   DRIVE  relative to the ceiling, with c = LIMIT (linear) and P = 10^(-LOUDNESS / 20): u = x P / c,
//          tanh(k u) / k, k = 1 + 2.5 DRIVE - odd harmonics; x = c u / P
//   then a 5 Hz DC blocker (the half-wave bend leaves an offset)
// Switching the stage on or off crossfades (10 ms) against the delayed input.
class RedWallColour
{
public:
    void prepare(double sampleRate, int maxBlock); // allocates
    void reset() noexcept;
    int latency() const noexcept { return delay; }
    static bool active(const LimiterSettings& p) noexcept { return p.values[slot::kLimDrive] > 0.0f || p.values[slot::kLimTube] > 0.0f; }
    void process(float* const* channels, int numChannels, int numSamples, const LimiterSettings&) noexcept;

private:
    void processChunk(float* const* channels, int numChannels, int numSamples, const LimiterSettings&) noexcept;

    double fs = 48000.0;
    int maxChunk = 1, delay = 0;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    AlignmentDelay dryDelay;
    std::array<std::vector<float>, kMaxChannels> dry;
    std::vector<float> levels;
    double power = 0.0, fade = 0.0;
    std::array<double, kMaxChannels> dcIn {}, dcOut {};
    bool running = false;
};
} // namespace hamlo::dsp
