#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "DSP/DspConfig.h"
#include "DSP/HamloEcho.h"
#include "DSP/HamloMag.h"
#include "DSP/HamloRedWall.h"
#include "DSP/HamloUdarnik.h"
#include "DSP/HamloZastava.h"
#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
// M6 processors (written for HPEQ, DECISIONS D3). All are allocation-free in process(); memory is
// sized in prepare(). Every law is documented here so tests can check it.

// Saturator (insert). y = (f(g x + b) - f(b)) / n, then a 10 Hz DC blocker and the output trim, with
//   g = 10^(drive/20), b = bias, f = tanh | tube (tanh above 0, tanh(1.4u)/1.4 below) | hard clip,
//   n = g f'(b) when auto gain is on (small signals keep unity gain), else 1.
// Oversampling 2x / 4x: JUCE half-band FIR (equiripple, max quality) with integer latency, so the
// node can align its dry path exactly. Latency per setting: latencySamples().
// Model 0 is that law ("Classic", bit-exact for projects saved before the HamloMag pass); models 1..5
// run HamloMag (HamloMag.h) around the same oversamplers.
class SaturatorProcessor
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    // Integer latency at the base rate for oversampling index 0 (1x) / 1 (2x) / 2 (4x). The first call
    // builds the table (allocates): it is made from prepare() / the message thread.
    static int latencySamples(int oversamplingIndex);
    // The whole slot: oversampling + the HamloMag wow delay (TAPE, VINYL).
    static int latencySamples(const FxSlotParams&, double sampleRate);
    float inputLevelDb() const noexcept { return mag.inputLevelDb(); }
    float outputLevelDb() const noexcept { return mag.outputLevelDb(); }
    // Oversampled processing runs in sub-blocks of this size, so the oversampler buffers stay small
    // (M8: 49 KB -> about 1/8 per slot); the FIR stages stream, so the output does not change.
    static constexpr int kOversamplingChunk = 64;

private:
    void shape(float* data, int n, float g, float bias, float norm, int curve) noexcept;

    double sampleRate = 48000.0;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 2> oversamplers; // 2x, 4x
    int activeOversampling = 0;
    std::array<double, kMaxChannels> dcIn {}, dcOut {};
    double dcCoefficient = 0.999;
    MagProcessor mag;
    int magFactor = 0; // oversampling factor the HamloMag cores are prepared for
};

// Transient shaper (insert). Linked peak envelopes: fast (attack 0.5 ms, release 20 ms * s) and slow
// (attack 20 ms * s, release 250 ms * s), s = 0.5 + speed. d = 20 log10(fast / slow) in dB (> 0 on
// attacks, < 0 in decays). gain dB = attack * clamp(d / 6, 0, 1) + sustain * clamp(-d / 6, 0, 1).
// Attack = sustain = 0 dB is an exact identity.
// With any HamloUdarnik option set (HamloUdarnik.h) that core runs instead; latency = its look-ahead.
class TransientProcessor
{
public:
    void prepare(double sampleRate) { sampleRate_ = sampleRate; core.prepare(sampleRate); reset(); } // allocates (core rings)
    void reset() noexcept { fast = slow = 0.0; core.reset(); }
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    static bool usesUdarnik(const FxSlotParams&) noexcept;
    static int lookaheadSamples(const FxSlotParams&, double sampleRate) noexcept;
    const UdarnikCore& udarnik() const noexcept { return core; }
    bool udarnikActive() const noexcept { return coreActive; }

private:
    double sampleRate_ = 48000.0, fast = 0.0, slow = 0.0;
    UdarnikCore core;
    bool coreActive = false;
};

// Stereo (insert). Needs two work channels (Stereo / Linked / Dual Mono node modes); otherwise it
// passes the signal. M = (L+R)/2, S = width * (L-R)/2, L = M+S, R = M-S, then linear balance
// (balance > 0 attenuates L by (1 - balance), < 0 attenuates R). Width 1, balance 0 = exact identity.
class StereoProcessor
{
public:
    void prepare(double) noexcept {}
    void reset() noexcept {}
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;
};

// Gate / downward expander (insert). Level x (dB) = linked peak detector (instant attack, 10 ms release). The gate opens above T and
// closes below T - hysteresis; after a close condition it stays open for `hold`. While closed the
// target attenuation is min(range, (T - x)(ratio - 1)); open: 0. Attenuation is smoothed in dB with
// a branching one-pole (attack while opening, release while closing). 0 dB attenuation = identity.
class GateProcessor
{
public:
    static constexpr double kDetectorReleaseMs = 10.0;

    void prepare(double sampleRate) { sampleRate_ = sampleRate; core.prepare(sampleRate); reset(); } // allocates (core rings)
    void reset() noexcept { open = false; holdLeft = 0; attenuationDb = 0.0; detector = 0.0; transitions = 0; core.reset(); }
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;
    long long stateChanges() const noexcept { return transitions; }

    // HamloZastava (HamloZastava.h): runs instead of the M6 law when any of its options is set.
    static bool usesZastava(const FxSlotParams&) noexcept;
    static int lookaheadSamples(const FxSlotParams&, double sampleRate) noexcept;
    const ZastavaCore& zastava() const noexcept { return core; }
    bool zastavaActive() const noexcept { return coreActive; }

private:
    double sampleRate_ = 48000.0, attenuationDb = 0.0, detector = 0.0;
    bool open = false;
    int holdLeft = 0;
    long long transitions = 0;
    ZastavaCore core;
    bool coreActive = false;
};

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
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    // Latency: the look-ahead, + 8 samples with TRUE PEAK (HamloRedWall).
    static int lookaheadSamples(const FxSlotParams&, double sampleRate) noexcept;
    static int baseLookaheadSamples(const FxSlotParams&, double sampleRate) noexcept;
    // True when any HamloRedWall option is set (otherwise the M6 law runs, bit-exact).
    static bool usesRedWall(const FxSlotParams&) noexcept;
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

// The M6 delay, kept as the "Classic" character (bit-exact for projects saved before the HamloEcho pass).
// Delay (wet-only send, pooled: 2 s of stereo memory per instance). Time in ms, or synced to the
// host tempo (note divisions). Feedback path: low cut and high damping (one-pole each); ping-pong
// crosses the feedback between channels and feeds the mono sum into the left line. The read time
// glides (100 ms) when it changes. Output = echoes only, with width applied to the wet M/S.
class ClassicDelay
{
public:
    static constexpr double kMaxSeconds = 2.0;

    void prepare(double sampleRate);
    void reset() noexcept;
    void process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams&, const Transport&) noexcept;

    static double delaySeconds(const FxSlotParams&, double bpm) noexcept;
    static double tailSeconds(const FxSlotParams&, double bpm) noexcept;
    float lastBlockPeak() const noexcept { return blockPeak; }

private:
    double sampleRate = 48000.0;
    std::array<std::vector<float>, 2> lines;
    int writePos = 0;
    double delay = -1.0;
    std::array<double, 2> lowState {}, highState {};
    float blockPeak = 0.0f;
};

// The Delay module: Classic (the M6 delay above) or a HamloEcho character (DELAY_DESIGN.md). A change
// between the two engines fades the old one out (10 ms), clears the new one and fades it in.
class DelayProcessor
{
public:
    static constexpr double kMaxSeconds = ClassicDelay::kMaxSeconds;

    void prepare(double sampleRate);
    void reset() noexcept;
    void process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams&, const Transport&) noexcept;

    // The Classic delay's time (also used by the M6 tests).
    static double delaySeconds(const FxSlotParams& p, double bpm) noexcept { return ClassicDelay::delaySeconds(p, bpm); }
    static double tailSeconds(const FxSlotParams&, double bpm) noexcept;
    float lastBlockPeak() const noexcept { return blockPeak; }
    float outputLevelDb() const noexcept { return meterOutDb.load(std::memory_order_relaxed); }
    float duckingDb() const noexcept { return engine > 0 ? echo.duckingDb() : 0.0f; }

private:
    ClassicDelay classic;
    EchoProcessor echo;
    int engine = -1; // -1 none yet, 0 Classic, 1 HamloEcho
    double fade = 1.0;
    bool fadingOut = false;
    int fadeSamples = 1;
    float blockPeak = 0.0f;
    std::atomic<float> meterOutDb { -120.0f };
};

// Monsta FX (insert, pooled: ~3.1 s of input history). Tempo-synced slicer: the timeline is cut into
// slices of one note division (host ppq when the transport plays, otherwise a free-running sample
// counter at the host/fallback tempo). Each slice's action comes from a hash of (seed, slice index):
// with probability `chance` the slice is Repeat (replays the previous slice), Reverse (plays the
// previous slice backwards) or Stutter (loops the first quarter of the previous slice); otherwise it
// passes. Acted-on slices can be bit-crushed and gated in 4 steps. 64-sample raised-cosine fades at
// slice edges. Decisions depend only on (seed, slice index, position), so the output does not depend
// on the host block size (T10), unlike HPDG's Monsta (M0: -33 dB between block sizes).
class MonstaProcessor
{
public:
    static constexpr double kHistorySeconds = 3.1;

    void prepare(double sampleRate);
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&, const Transport&, int blockOffset) noexcept;

    enum class Action { Pass, Repeat, Reverse, Stutter };
    static Action actionFor(const FxSlotParams&, long long slice) noexcept;
    static std::uint32_t hash(std::uint32_t seed, std::uint32_t value) noexcept;

private:
    double sampleRate = 48000.0;
    std::array<std::vector<float>, 2> history;
    int historySize = 0, writePos = 0;
    long long freeRunning = 0;
    bool havePosition = false;
};
} // namespace hpeq::dsp
