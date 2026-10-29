#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <cmath>
#include <vector>

#include "DSP/DspConfig.h"
#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
// HamloShirota (module pass 8, docs/fx-addon/STEREO_DESIGN.md §2): the stereo module's options around the M6
// law. The StereoProcessor runs this core only when an option is set; with every option at its default the
// M6 code runs (bit-exact for older projects).
//
//   M = (L + R) / 2, S = (L - R) / 2
//   MONO BASS f: S <- HP^8(S) (four 2nd-order Butterworth high-passes at f)
//   WIDTH w: S <- w S
//   STEREOIZE a (PROSTOR, KOSMOS): S <- S + a D(M), D = HP 150 Hz + eight all-pass sections: only S changes,
//   so L + R = 2 M exactly (the mono sum is untouched)
//   KOSMOS space k: Blauert's "behind" bands on M and S (+6k dB at 1 kHz and 11 kHz, -6k dB at 4.5 kHz),
//   S <- (1 + k) S
//   SAFE (look-ahead D = 2.5 ms, latency D): M and S wait D samples; side gain min(1, sqrt(sum M^2 / sum S^2))
//   over the centred 5 ms window, at once, recovering over 100 ms => corr(L, R) >= 0
//   L = M + S, R = M - S; ROTATION; BALANCE (M6); MONO: L = R = (L + R) / 2; OUTPUT last.
// One channel: passes untouched (the page shows OSHIBKA). Latency 0, or D with SAFE.
class ShirotaCore
{
public:
    enum Style { Edinstvo = 0, Prostor = 1, Kosmos = 2 };
    static constexpr int kScopePoints = 512;

    void prepare(double sampleRate); // allocates the SAFE rings
    static int safeLookaheadSamples(double sampleRate) noexcept { return static_cast<int>(std::lround(0.0025 * sampleRate)); }
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    struct Meters
    {
        float correlation = 1.0f;  // output, 300 ms
        float guardDb = 0.0f;      // SAFE side reduction (dB <= 0), end of the last block
        bool oneChannel = false;   // the last block had fewer than two channels
    };
    Meters meters() const noexcept;
    // The newest output points (L, R), oldest first.
    void scope(std::array<float, kScopePoints>& left, std::array<float, kScopePoints>& right) const noexcept;

private:
    struct Section
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        double process(double x) noexcept
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };
    void design(double monoBassHz, double space) noexcept;

    double fs = 48000.0;
    double designedBass = -1.0, designedSpace = -1.0;
    std::array<Section, 4> bass {};
    Section stereoizeHp;
    std::array<Section, 8> allPass {};
    std::array<Section, 3> eqMid {}, eqSide {};
    double guard = 1.0;
    // SAFE look-ahead: m, s and their squares over the last 2 D + 1 samples
    std::vector<double> ringM, ringS, ringM2, ringS2;
    int ringSize = 1, ringPos = 0, safeDelay = 0;
    double sumM2 = 0.0, sumS2 = 0.0;
    bool safeRunning = false;
    double sumLR = 0.0, sumLL = 0.0, sumRR = 0.0;
    int scopeDecimation = 2, scopeCount = 0;
    // on the heap (prepare): 540 stereo slots would otherwise carry 4 KB each inside the engine objects
    std::unique_ptr<std::atomic<float>[]> scopeL, scopeR;
    std::atomic<int> scopeWrite { 0 };
    std::atomic<float> meterCorrelation { 1.0f }, meterGuard { 0.0f };
    std::atomic<bool> meterOneChannel { false };
};
} // namespace hpeq::dsp
