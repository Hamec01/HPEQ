#pragma once

#include <array>
#include <atomic>
#include <vector>

#include "DSP/TransientSettings.h"

namespace hamlo::dsp
{
// HamloUdarnik (module pass 6, docs/fx-addon/TRANSIENT_DESIGN.md §3): the transient shaper's options
// around the M6 law. The TransientProcessor runs this core only when an option is set; with every option
// at its default the M6 code runs (bit-exact for older projects).
//
// Detector time n, audio time m = n - L (L = look-ahead 0 / 2 / 5 ms), l = max_c |x_c|.
//   M6 envelopes fast / slow -> d = 20 log10(fast / slow), w_a = clamp(d / 6), w_s = clamp(-d / 6)
//   scales e1 (0.05 / 1 ms) and e10 (1 / 10 ms): n_s = 20 log10(e1 / e10)
//   onset: armed, n_s >= 6 + 18 (1 - sensitivity); event: confidence clamp((rise - 9) / 6), rise = how far e10
//   climbs over its value at the onset within 5 ms (dB); peak over 10 ms, strength = peak / accent reference
//   (2 s), gate strength >= 12 - 72 sensitivity dB
//   k = 1 (PRAVDA, sensitivity 1) or confidence * gate
//   attack window a (ramp before t0 with look-ahead, 1 until 0.7 T_a, cosine to 0 at T_a), body b = (1 - a) w_s
//   PRAVDA       G = k (SNAP max(w_a, pre-ramp) + BODY w_s); PRESERVE pulls `slow` to the level before a hit
//   UDARNIK      G = k (SNAP a + (BODY + beta) b), beta = energy-neutral body (K-weighted energies, so the
//                loudness stays); where g = 10^(G/20) > 1:
//                y = s tanh(g artanh(x / s)), s = peak reference (hold / the hit's own peak by PRESERVE)
//                -> output sample peak < s <= the input's own recent peak
//   PERESTROIKA  G = k (SNAP a + BODY b + G_rise + G_decay): rise time x RISE, decay time x DECAY
//   G smoothed 0.1 ms; LISTEN attack y w / body y - y w; OUTPUT last.
// Latency = L.
class UdarnikCore
{
public:
    static constexpr int kGraphPoints = 64; // last hit: 64 x 2.5 ms
    enum Style { Pravda = 0, Udarnik = 1, Perestroika = 2 };

    // Look-ahead index 0 SPUTNIK (0 ms) / 1 VOSTOK (2 ms) / 2 SOYUZ (5 ms), in samples.
    static int lookaheadSamples(int index, double sampleRate) noexcept;

    void prepare(double sampleRate); // allocates the rings
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const TransientSettings&) noexcept;

    struct Meters
    {
        float impactDb = 0.0f;   // attack/body contrast change, 300 ms
        float gainMinDb = 0.0f, gainMaxDb = 0.0f; // applied gain range of the last block
        float inputDb = -120.0f; // input peak of the last block
        int events = 0;          // hits detected since reset
        float confidence = 0.0f, strengthDb = -120.0f; // of the last hit
    };
    Meters meters() const noexcept;
    // The last hit's input and output envelopes (peak per 2.5 ms, linear). Returns the hit's sequence
    // number (0 = none yet).
    int lastHit(std::array<float, kGraphPoints>& in, std::array<float, kGraphPoints>& out) const noexcept;

private:
    struct Event
    {
        long long t0 = 0;
        float confidence = 0.0f, peak = 0.0f, wMax = 0.0f, strengthDb = -120.0f;
        float before = 0.0f, riseMax = 0.0f; // e10 at the onset and its maximum over the first 5 ms
        bool used = false;
    };

    // BS.1770 K-weighting (high shelf + high pass) for the UDARNIK energy balance, per channel, input and output.
    struct KWeight
    {
        double s0 = 1, s1 = 0, s2 = 0, sa1 = 0, sa2 = 0, ha1 = 0, ha2 = 0;
        std::array<std::array<double, 4>, kMaxChannels * 2> z {};
        double process(int stream, double x) noexcept
        {
            auto& s = z[static_cast<size_t>(stream)];
            const double u = s0 * x + s[0];
            s[0] = s1 * x - sa1 * u + s[1];
            s[1] = s2 * x - sa2 * u;
            const double v = u + s[2];
            s[2] = -2.0 * u - ha1 * v + s[3];
            s[3] = u - ha2 * v;
            return v;
        }
    };

    void clearState() noexcept;
    float envelopeAt(double sampleTime) const noexcept;
    void publishHit() noexcept;

    double fs = 48000.0;
    int capacity = 1;       // ring size (>= max look-ahead + 1)
    int lookahead = -1;     // L of the current state
    int decimation = 1, historySize = 1, windowBuckets = 1;
    long long counter = 0;

    // detector
    double fast = 0.0, slow = 0.0, e1 = 0.0, e10 = 0.0, accent = 0.0;
    double riseSum = 0.0;   // rise envelope: rectified mean of the level over 1 ms (running sum)
    int riseWindow = 1;
    double hold = 0.0;      // peak hold (60 ms, release 200 ms)
    int holdLeft = 0;
    bool armed = true;
    long long lastOnset = -1000000;
    std::array<Event, 4> events {};
    int newest = -1, eventCount = 0;
    float bucketMax = 0.0f;
    std::vector<float> dequeValue;
    std::vector<long long> dequeIndex;
    int dequeHead = 0, dequeTail = 0;
    std::vector<float> history; // rise envelope every `decimation` samples
    long long historyLast = -1; // sample time of the newest stored value

    // rings (detector -> audio)
    std::array<std::vector<float>, kMaxChannels> audio;
    std::vector<float> level, wa, ws;

    // audio-time state
    long long govT0 = -1;
    double peakDecay = 0.0, risePeak = 0.0, riseFadeFrom = 0.0;
    bool pastPeak = false, riseActive = false;
    int riseFadeLeft = 0;
    double gainSmooth = 0.0, beta = 0.0, betaRaw = 0.0;
    double aIn = 0.0, aOut = 0.0, bIn = 0.0;
    KWeight kWeight;
    double mAin = 0.0, mAout = 0.0, mBin = 0.0, mBout = 0.0;

    // last hit graph
    long long capT0 = -1;
    int capBins = 0;
    std::array<float, kGraphPoints> capIn {}, capOut {};
    std::atomic<int> hitSequence { 0 }, hitVersion { 0 };
    std::array<std::atomic<float>, kGraphPoints> hitIn {}, hitOut {};

    std::atomic<float> meterImpact { 0.0f }, meterGainMin { 0.0f }, meterGainMax { 0.0f }, meterInput { -120.0f },
        meterConfidence { 0.0f }, meterStrength { -120.0f };
    std::atomic<int> meterEvents { 0 };
};
} // namespace hamlo::dsp
