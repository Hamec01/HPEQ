#pragma once

#include <array>
#include <vector>

#include "DSP/DspConfig.h"
#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
// Wet-only reverb for FX-node send slots (M4, written for HPEQ, DECISIONS D3).
//
// Structure: stereo pre-delay -> 8-line feedback delay network (Jot):
//  - delay lengths 31.3 ... 73.7 ms x size, read with linear interpolation (size glides);
//  - Householder feedback matrix A = I - (2/N) 1 1^T (lossless, orthogonal);
//  - per line a first-order absorption filter H_i(z) = g_i (1 - b_i) / (1 - b_i z^-1) with
//      g_i = 10^(-3 d_i / (T60 fs))          (decay at low/mid frequencies = T60)
//      b_i = (1 - r_i) / (1 + r_i),  r_i = 10^(-3 d_i / (T60 * hfRatio * fs)) / g_i
//    so the decay time at Nyquist is T60 * hfRatio;
//  - L input feeds the even lines, R the odd lines; outputs are two orthogonal sign patterns
//    over all lines (decorrelated L/R); width scales the wet side signal; a first-order high-pass
//    (low cut) on the wet keeps the low end of the dry clean.
// Output is the reverb only (no dry), so in a node chain it is used as s_k = s + mix * W(s).
// All memory is allocated in prepare(); process() never allocates.
class ReverbProcessor
{
public:
    static constexpr int kLines = 8;
    static constexpr double kMaxSize = 1.5, kMinSize = 0.3, kMaxPredelayMs = 250.0;

    void prepare(double sampleRate);
    void reset() noexcept;

    // wet[ch] = reverb of in[ch] (numChannels 1 or 2; mono uses the same input for both halves).
    void process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    float lastBlockPeak() const noexcept { return blockPeak; }

    // Nominal decay to -60 dB at low/mid frequencies plus pre-delay, for host tail reporting.
    static double tailSeconds(const FxSlotParams&) noexcept;

private:
    void updateTargets(const FxSlotParams&) noexcept;

    double sampleRate = 48000.0;
    std::array<std::vector<float>, kLines> lines;
    std::array<int, kLines> writePos {};
    std::array<double, kLines> length {}, lengthTarget {}, absorption {};
    std::array<double, kLines> gain {}, gainTarget {}, pole {}, poleTarget {};
    std::array<std::vector<float>, 2> predelay;
    int predelayPos = 0;
    double predelaySamples = 0.0, predelayTarget = 0.0;
    std::array<double, 2> lowCutState {};
    double lowCutCoefficient = 0.0;
    double width = 1.0;
    float blockPeak = 0.0f;
    bool primed = false;
};
} // namespace hpeq::dsp
