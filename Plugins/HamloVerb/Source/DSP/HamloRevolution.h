#pragma once

#include <array>
#include <atomic>
#include <complex>
#include <memory>
#include <vector>

#include "DSP/LayerSnapshot.h"
#include "DSP/RevolutionFft.h"

namespace hpeq::dsp
{
// HamloRevolution (module pass 10): TIME, GLITCH and MOTION in one of six serial orders, each with its
// own on / mix / depth and a 16-step pattern (Core/Revolution.h). Zero algorithmic latency: every head
// reads already written input (TIME and GLITCH play earlier material by design), so nothing is added to
// the plug-in delay compensation.
//
// Clock: a virtual sample counter n with an anchor (q0, n0) and dq = quarter notes (or free steps) per
// sample, q(n) = q0 + (n - n0) dq. The anchor moves only on a tempo change (continuously) or on a
// transport jump (new epoch), so the musical position is the integral of the tempo, not ppq / bpm.
// Step boundaries come from q with swing and humanize; a run is a set of equal steps inside one pattern
// cycle. Decisions depend only on n, the pattern and the parameters: the result does not depend on the
// host block size.
//
// Histories: TIME and GLITCH each record their own input (an engine after another one hears the processed
// signal). A head that would need a sample older than what was written since the epoch, or beyond the
// capacity, plays the stage's input instead. Reset and a transport jump are O(1): the written count is
// zeroed, nothing is cleared.
//
// Pass 10b: TIME's HALF and 2X can keep the pitch (slot::kTimeStretch): RHYTHM = WSOLA with a bounded
// coarse-to-fine search (stereo-linked), CLEAN = phase vocoder with identity phase locking that keeps the
// channels' phase differences. Both read their grains a fixed 1.5 frames (+ search) behind the tape head
// (a source age, not latency). Splices estimate the two heads' energies and correlation and correct the
// crossfade's level (C-ATD, cap +3 dB). The input's attack strength per pattern step is measured (cheap
// energy novelty) for the page and for Generate with LISTEN.
class RevolutionEngine
{
public:
    static constexpr double kTimeHistorySeconds = 4.5;
    static constexpr int kFeelSteps = 16;

    RevolutionEngine();
    ~RevolutionEngine();

    // `glitchStorage` (owned by MonstaProcessor, shared with the M6 Monsta history) is GLITCH's tape.
    void prepare(double sampleRate, std::array<std::vector<float>, 2>& glitchStorage);
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&, const Transport&, int blockOffset) noexcept;

    // UI: the pattern step under the play head (-1 before the first block), warming up after an epoch.
    int currentStep() const noexcept { return shownStep.load(std::memory_order_relaxed); }
    bool warmingUp() const noexcept { return warming.load(std::memory_order_relaxed); }
    int epochCount() const noexcept { return epochs.load(std::memory_order_relaxed); }
    int timeCapacity() const noexcept { return tapes[0].size; }
    int glitchCapacity() const noexcept { return tapes[1].size; }
    // Attack strength 0..1 per pattern step (averaged over the pattern cycles heard so far).
    void getFeel(std::array<float, kFeelSteps>&) const noexcept;
    // The wave screen (owner, 2026-10-26): input and output peak per 1/8 step over the pattern cycle, the
    // latest cycle written over the previous one.
    static constexpr int kWaveBins = 8 * kFeelSteps;
    void getWave(std::array<float, kWaveBins>& in, std::array<float, kWaveBins>& out) const noexcept;
    // Frame sizes of the stretch renderers at this sample rate (RHYTHM, CLEAN).
    int rhythmFrame() const noexcept { return frameW; }
    int cleanFrame() const noexcept { return frameP; }

private:
    struct Tape
    {
        float* data[2] { nullptr, nullptr };
        int size = 0, head = -1; // head = index of the newest sample
        long long written = 0;   // since the epoch (saturates at size)
        void push(const float* x, int numChannels) noexcept;
        float at(int ch, long long age) const noexcept;
        // `count` samples from `age` (oldest) towards the present into dst (at most two copies)
        void copy(int ch, long long age, int count, float* dst) const noexcept;
        bool holds(double oldestAge) const noexcept { return oldestAge <= static_cast<double>(std::min<long long>(written, size) - 1); }
    };
    struct Step
    {
        long long k = 0;
        double start = 0.0, end = 0.0; // in q
    };
    struct Segment
    {
        bool valid = false;
        int action = 0;
        long long runK = 0;
        double b = 0.0, w = 0.0, wStep = 0.0; // run start (virtual samples), run and first-step length
        long long loop = 0;                   // FREEZE / STUTTER / RATCHET loop length in samples
        unsigned stamp = 0;
        double endN = 0.0, startN = 0.0;      // the samples this segment is current for
        bool same(const Segment& o) const noexcept;
    };
    struct Stage
    {
        Segment cur, prev;
        int fadePos = 0, fadeLength = 0;
        double mix = 0.0;
        double joinA = 0.0, joinB = 0.0, joinC = 0.0; // C-ATD: energies of the two heads and their correlation
        bool join = false;
    };
    struct StretchVoice
    {
        bool active = false, fallback = false;
        Segment segment;
        int mode = 0, size = 0, hop = 0;
        long long base = 0, nextFrame = 0, outPos = 0, prevCentre = 0, prevGrain = 0;
        double start = 0.0, rate = 1.0, delay = 0.0; // source position of synthesis time t: start + rate t - delay
        bool havePrev = false;
        std::array<std::vector<float>, 2> ola;
        std::vector<std::complex<float>> prevRef, rot; // CLEAN: last reference spectrum, rotation per bin
    };

    void readParams(const FxSlotParams&) noexcept;
    void syncClock(const FxSlotParams&, const Transport&, int blockOffset) noexcept;
    void newEpoch() noexcept;
    double stepStartQ(long long k) const noexcept;
    Step stepAt(double q) const noexcept;
    double sampleOf(double q) const noexcept { return static_cast<double>(n0) + (q - q0) / dq; }
    int value(int engine, long long k) const noexcept;
    Segment segmentFor(int engine, double q) const noexcept;
    long long maxAge(int engine, int action, double w, double wStep, long long loop) const noexcept;
    // The head at sample nn while the tape's newest sample is n (ahead = nn - n >= 0). False when it can
    // only give the stage's input and that is not known yet (ahead > 0).
    bool headAt(int engine, const Segment&, long long nn, long long ahead, const float* in, float* out, int numChannels) const noexcept;
    bool isStretch(int engine, const Segment&) const noexcept;
    void startVoice(int index, const Segment&) noexcept;
    void advanceVoice(StretchVoice&) noexcept;
    void lookAhead(const Stage&) noexcept;
    void voiceFrame(StretchVoice&, long long t) noexcept;
    void voiceOut(StretchVoice&, const float* in, float* out, int numChannels) noexcept;
    void estimateJoin(int engine, Stage&, bool prevStretch, bool curStretch, int numChannels) noexcept;
    void analyse(const float* in, int numChannels, int step) noexcept;
    float readFrac(const Tape&, int ch, double age) const noexcept;
    float readHalfband(const Tape&, int ch, long long age) const noexcept;
    void tapeStage(int engine, float* x, int numChannels, double q) noexcept;
    void motionStage(float* x, int numChannels, double q) noexcept;
    double motionLevel(double q) noexcept;

    double sampleRate = 48000.0;
    std::vector<float> timeStorage[2];
    std::array<Tape, 2> tapes; // TIME, GLITCH
    std::array<Stage, 2> stages;
    std::array<float, 15> halfband {};

    // stretch renderers (TIME only): the current head's voice, the one fading out, and the next run's,
    // started one frame early so its frames come one per hop instead of all at the run start (P6)
    std::array<StretchVoice, 3> voices;
    int voiceCur = -1, voicePrev = -1, voiceNext = -1, stretchMode = 0, frameW = 1024, frameP = 2048;
    bool smartJoin = true;
    Segment upcoming;              // the segment after the current TIME one (looked up once per segment)
    double upcomingFor = -1.0;     // ... for the current segment ending here
    RevolutionFft fft;
    std::vector<float> windowW, windowP, peakMag, spanL, spanR, naturalL, naturalR; // contiguous copies of the tape
    std::vector<std::complex<float>> specIn, specOut, specL, specR, specRef, peakRot;
    std::vector<int> nearestPeak;

    // attack analysis per pattern step
    int feelFrame = 64, feelFill = 0, feelLastStep = -1;
    double feelEnergy = 0.0;
    std::array<double, 4> feelHistory {};
    int feelHistoryCount = 0;
    std::array<double, kFeelSteps> stepPeak {};
    std::array<std::atomic<float>, kFeelSteps> feel {};

    // parameters of the current block
    std::array<float, 48> lastValues {};
    unsigned stamp = 0;
    int steps = 16, grid = 2, motionTarget = 0;
    bool triplet = false, freeMode = false;
    double swing = 0.5, humanize = 0.0, freeMs = 125.0, fadeSamples = 96.0, smooth = 0.0;
    std::uint32_t humanSeed = 1;
    std::array<bool, 3> on { true, true, true };
    std::array<double, 3> mixTarget { 1.0, 1.0, 1.0 }, depth { 1.0, 1.0, 0.6 };
    std::array<float, 4 * 3> patternQuads {};
    std::array<int, 3> order { 0, 1, 2 };
    int activeOrder = 0, wantedOrder = 0;
    double orderGain = 1.0;
    bool fresh = true;

    // clock
    long long n = 0, n0 = 0;
    double q0 = 0.0, dq = 1.0 / 24000.0, stepQ = 0.25;
    bool anchored = false, lastPlaying = false, lastFree = false;

    // MOTION
    Step motionStep;
    bool motionStepValid = false;
    unsigned motionStamp = 0;
    double motionMix = 0.0;
    double svfG = 0.0;
    std::array<double, 2> ic1 {}, ic2 {};
    std::array<float, 2> held {};

    std::atomic<int> shownStep { -1 }, epochs { 0 };
    std::array<std::atomic<float>, kWaveBins> waveIn {}, waveOut {};
    int waveBin = -1, waveStepIndex = 0;
    Step waveStep {};
    bool waveStepValid = false;
    unsigned waveStamp = 0;
    double waveStepScale = 1.0;
    float waveInPeak = 0.0f, waveOutPeak = 0.0f;
    std::atomic<bool> warming { false };
};
} // namespace hpeq::dsp
