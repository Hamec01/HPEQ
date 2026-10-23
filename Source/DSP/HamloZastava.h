#pragma once

#include <array>
#include <atomic>
#include <vector>

#include "DSP/DspConfig.h"
#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
// HamloZastava (module pass 7, docs/fx-addon/GATE_DESIGN.md §2): the gate's options around the M6 law. The
// GateProcessor runs this core only when an option is set; with every option at its default the M6 code runs
// (bit-exact for older projects).
//
// Detector time n, audio time m = n - L (L = look-ahead 0 / 2 / 5 ms).
//   key k_c = LP(HP(x_c)) (2nd-order Butterworth, each off when 0); level = max_c |k_c|
//   peak detector (10 ms release), x = dB; open above T, shut below T - hysteresis after the hold (M6)
//   target: ZASTAVA open ? 0 : min(range, (T - x)(ratio - 1)); PATRUL clamp((T - x)(ratio - 1), 0, range);
//           TREVOGA (duck) open ? range : 0
//   one-pole in dB: ATTACK while opening (TREVOGA: while ducking), RELEASE otherwise
//   applied A(m) = min (TREVOGA: max) of the attenuation over [m, m + L]: opens L early, closes on time
//   y = x_d g | FLIP x_d - x_d g | KEY LISTEN k_d; OUTPUT last.
// Latency = L.
class ZastavaCore
{
public:
    enum Style { Zastava = 0, Patrul = 1, Trevoga = 2 };
    static int lookaheadSamples(int index, double sampleRate) noexcept; // 0 / 2 / 5 ms

    void prepare(double sampleRate); // allocates the rings
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    struct Meters
    {
        float attenuationDb = 0.0f;    // now (end of the last block)
        float maxAttenuationDb = 0.0f; // largest in the last block
        float keyDb = -120.0f;         // key peak of the last block
        bool open = false;
        int openings = 0;              // since reset
    };
    Meters meters() const noexcept;

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        std::array<std::array<double, 2>, kMaxChannels> z {};
        double process(int c, double x) noexcept
        {
            auto& s = z[static_cast<size_t>(c)];
            const double y = b0 * x + s[0];
            s[0] = b1 * x - a1 * y + s[1];
            s[1] = b2 * x - a2 * y;
            return y;
        }
    };
    void design(double hpHz, double lpHz) noexcept;
    void clearState() noexcept;

    double fs = 48000.0;
    int capacity = 1, lookahead = -1;
    double designedHp = -1.0, designedLp = -1.0;
    Biquad hp, lp;
    bool hpOn = false, lpOn = false;
    long long counter = 0;
    double detector = 0.0, attenuationDb = 0.0;
    bool open = false;
    int holdLeft = 0, openings = 0;
    std::array<std::vector<float>, kMaxChannels> audio, key;
    std::vector<float> dequeValue;
    std::vector<long long> dequeIndex;
    int dequeHead = 0, dequeTail = 0;
    std::atomic<float> meterNow { 0.0f }, meterMax { 0.0f }, meterKey { -120.0f };
    std::atomic<bool> meterOpen { false };
    std::atomic<int> meterOpenings { 0 };
};
} // namespace hpeq::dsp
