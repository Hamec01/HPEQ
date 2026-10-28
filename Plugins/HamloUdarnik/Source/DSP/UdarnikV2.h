#pragma once

#include <array>
#include <atomic>
#include <vector>

#include "DSP/HamloUdarnik.h"
#include "DSP/TransientSettings.h"

namespace hamlo::dsp
{
// HamloUdarnik v2 engine (docs/UDARNIK_V2.md). A differential-envelope shaper (the SPL principle): everything is
// a difference of two envelopes in dB, so the effect does not depend on the level, and the weights reach 1 so
// the knobs deliver what they say. Detector time n, audio time m = n - L (L = look-ahead 0 / 2 / 5 ms).
//
//   l   = sqrt(one-pole 3 ms of max_c x_c^2)                       (RMS level, linked)
//   f   = follower(l, attack 0.1 ms, release 8 ms s), sl = follower(l, attack 15 ms s r, release 150 ms s)
//         s = 0.5 + SPEED, r = RISE in PERESTROIKA (x0.5 .. x2: how long the attack lasts), else 1
//   A   = 20 log10(f / sl) (> 0 on attacks)
//   a hit starts where the CLASSIC core's onset detector fires: n_s = 20 log10(e1 / e10) >= theta, e1 / e10 = peak
//   followers (0.05 / 1 ms, 1 / 10 ms), theta = 6 + 18 (1 - SENS) + 6 (1 - PRESERVE) dB (PRESERVE 0: only hits that
//   stand out by more count); re-armed 3 dB under theta and 10 ms on. Its phases, not the envelopes, set the weights
//   (so they reach 1 and the knobs deliver what they say; a steady tone never starts a hit):
//   w_a = 1 while A still rises, then for 0.7 T_a, a cosine to 0 at T_a, T_a = 15 ms x s (x RISE in PERESTROIKA:
//         how long the attack lasts); a slow attack keeps the window open until its peak
//   w_b = (1 - w_a) from 0.7 T_a (3 ms ramp) for 500 ms x s, then a 50 ms fade; the next hit starts over
//   G   = SNAP w_a + BODY w_b
//         PERESTROIKA adds w_b F (1 - 1 / DECAY), F = the fall from the hit's peak in dB: the decay's slope is
//         divided by DECAY (x2 = twice as long)
//   UDARNIK is peak-neutral: where g = 10^(G/20) > 1 the boost a (g - 1) of a sample |x| = a is soft-limited to
//         the room under the sample peak the detector has seen (P, 50 ms release, look-ahead included):
//         y = a + R tanh(a (g - 1) / R), R = max(0, P - a) - the attack gets denser, its peak does not rise; what
//         the attack could not get under its peak (SNAP - its achieved energy gain, measured over the window) the
//         body gives back (x 10^(-that w_b / 20)), so the attack / body contrast still follows SNAP
//   G is smoothed (0.5 ms) and applied to the audio L samples late; it is read min(L, 2 ms) early, which
//   compensates the RMS detector's lag, so the attack gain starts with the hit, not before it
//   LISTEN: ATTACK y w, BODY y (1 - w), w = the attack's share; OUTPUT last.
// Meters and the last-hit graph as UdarnikCore's (the page shows them the same way).
class UdarnikV2
{
public:
    static constexpr double leadMs = 2.0; // the gain is read this much early (<= L): the detector's lag (measured best)
    static constexpr int kGraphPoints = UdarnikCore::kGraphPoints;

    void prepare(double sampleRate); // allocates the rings
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const TransientSettings&) noexcept;

    UdarnikCore::Meters meters() const noexcept;
    int lastHit(std::array<float, kGraphPoints>& in, std::array<float, kGraphPoints>& out) const noexcept;

private:
    double fs = 48000.0;
    int capacity = 1;
    int lookahead = -1;
    long long counter = 0;
    double meanSquare = 0.0, fast = 0.0, slow = 0.0, hold = 0.0, gainSmooth = 0.0, peakHold = 0.0;
    bool inHit = false, armed = true;
    double e1 = 0.0, e10 = 0.0;
    long long hitStart = -1, riseEnd = -1;
    double riseMax = 0.0;
    double hitFloor = 0.0, hitPeak = 0.0, bodyFade = 0.0;
    std::array<std::vector<float>, kMaxChannels> audio;
    std::vector<float> gainDb, attackShare, bodyShare;
    bool attackOpen = false;
    double attackIn = 0.0, attackOut = 0.0, giveBack = 0.0;

    // last hit graph
    long long capT0 = -1;
    int capBins = 0;
    std::array<float, kGraphPoints> capIn {}, capOut {};
    std::atomic<int> hitSequence { 0 };
    std::array<std::atomic<float>, kGraphPoints> hitIn {}, hitOut {};
    double impact = 0.0;
    std::atomic<float> meterImpact { 0.0f }, meterGainMin { 0.0f }, meterGainMax { 0.0f }, meterInput { -120.0f }, meterStrength { -120.0f };
    std::atomic<int> meterEvents { 0 };
    int events = 0;
};
} // namespace hamlo::dsp
