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
    double stepStartQ(long long k, double sq) const noexcept;
    Step stepAt(double q, double sq) const noexcept;
    double stepStartQ(long long k) const noexcept { return stepStartQ(k, stepQ); }
    Step stepAt(double q) const noexcept { return stepAt(q, stepQ); }
    int laneSteps(int engine) const noexcept { return laneLength[static_cast<size_t>(engine)] > 0 ? laneLength[static_cast<size_t>(engine)] : steps; }
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
    // R2: a lane on its band (FULL: the lane on x as before), SHAPE A / B
    void runLane(int lane, float* x, int numChannels, double q) noexcept;
    void shapeStage(int shape, float* x, int numChannels, double q) noexcept;
    double shapeLevel(int shape, double q) noexcept;
    void updateCrossovers() noexcept;
    // R3: THROW A / B
    void throwStage(int lane, float* x, int numChannels, double q) noexcept;
    double throwLevel(int lane, double q) noexcept;
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
    std::array<float, 228> lastValues {};
    // R4: the groove (offsets per 1/16 of the bar, in quarter notes, times GROOVE); all 0 = the straight grid
    std::array<double, 16> groove {};
    bool grooved = false;
    unsigned stamp = 0;
    int steps = 16, grid = 2, motionTarget = 0;
    bool triplet = false, freeMode = false;
    double swing = 0.5, humanize = 0.0, freeMs = 125.0, fadeSamples = 96.0, smooth = 0.0;
    std::uint32_t humanSeed = 1;
    std::array<bool, 3> on { true, true, true };
    std::array<double, 3> mixTarget { 1.0, 1.0, 1.0 }, depth { 1.0, 1.0, 0.6 };
    std::array<float, 4 * 3> patternQuads {};
    // R1 lanes (0 = as before): length, the lane's step in quarter notes, condition, steps 17 .. 32, probability codes
    std::array<int, 7> laneLength {}, laneCondition {};
    std::array<double, 7> laneQ { 0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25 };
    std::array<float, 4 * 7> extraQuads {};
    std::array<float, 8 * 7> probabilityQuads {};
    std::array<bool, 7> laneChance {}; // any probability code set

    // R2: the band split (Linkwitz-Riley 4th order = two Butterworth sections; the low band through the upper
    // crossover's all-pass so the bands sum to an all-pass) and SHAPE A / B
    struct Section
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        std::array<double, 2> z1 {}, z2 {};
        double run(int ch, double x) noexcept
        {
            const double y = b0 * x + z1[static_cast<size_t>(ch)];
            z1[static_cast<size_t>(ch)] = b1 * x - a1 * y + z2[static_cast<size_t>(ch)];
            z2[static_cast<size_t>(ch)] = b2 * x - a2 * y;
            return y;
        }
        void clear() noexcept { z1 = {}; z2 = {}; }
    };
    struct Splitter
    {
        // [0..1] low pass at f1, [2..3] high pass at f1, [4..5] low pass at f2, [6..7] high pass at f2,
        // [8..11] the low band's all-pass at f2 (low pass + high pass of f2 on it)
        std::array<Section, 12> s;
        int band = 0;
        void clear() noexcept { for (auto& x : s) x.clear(); }
    };
    struct ShapeLane
    {
        bool on = true, curve = false, empty = true;
        int target = 0;
        double mixTarget = 1.0, depth = 1.0, smooth = 0.0, res = 0.0, mix = 0.0;
        std::array<float, 4> quads {};
        Step step;
        bool stepValid = false;
        unsigned stamp = 0;
        double svfG = 0.0;
        std::array<double, 2> ic1 {}, ic2 {};
        std::array<float, 2> held {};
    };
    std::array<Splitter, 7> splitters;
    std::array<int, 7> band {};
    double crossoverLow = 200.0, crossoverHigh = 2500.0, appliedLow = -1.0, appliedHigh = -1.0;
    std::array<ShapeLane, 2> shapes;

    // R3: a THROW lane. One tape per lane serves DELAY / TONAL (the delay line), FLANGER and VINYL (a moving head);
    // REVERB is an 8-line feedback delay network behind four all-pass diffusers. Only the target's state is used;
    // it is cleared when the lane wakes up or changes its target.
    struct ThrowLane
    {
        bool on = true, empty = true, asleep = true;
        int target = 0, applied = -1, time = 7, note = 0;
        double mixTarget = 1.0, depth = 1.0, tail = 0.5, tone = 0.5, mix = 0.0;
        std::array<float, 4> quads {};
        Step step;
        bool stepValid = false;
        unsigned stamp = 0;
        long long quiet = 0;                   // samples the return stayed below -120 dBFS
        std::array<std::vector<float>, 2> line;
        int write = 0;                         // the next index written
        double delay = -1.0;                   // DELAY / TONAL: the delay in samples (smoothed)
        std::array<double, 2> damp {}, feedback {}, hp {}, lp1 {}, lp2 {};
        std::array<std::array<double, 6>, 2> allpass {};
        double coefA = 0.0, coefB = 0.0;       // coefficients refreshed every 16 samples
        bool coefValid = false;
        std::array<std::vector<float>, 8> fdn;
        std::array<int, 8> fdnPos {};
        std::array<double, 8> fdnLow {}, fdnGain {};
        std::array<std::vector<float>, 4> diffuser;
        std::array<int, 4> diffuserPos {};
        double brakeAge = 0.0, fromAge = 0.0;  // VINYL: the brake's extra age, the head being faded out
        double wowDepth = 0.0;                 // VINYL: the wow's depth follows the level over ~50 ms (no speed jumps)
        double eqLevel = -1.0;                 // VINYL: the level its record EQ was designed for
        int fade = 0;
        long long brakeStep = 0;
        bool brakeValid = false;
        double phase = 0.0;                    // DOWNSAMPLE
        std::array<float, 2> held {};
    };
    std::array<ThrowLane, 2> throws;
    void clearThrow(ThrowLane&) noexcept;
    float lineAt(const ThrowLane&, int ch, double age) const noexcept; // age 0 = the newest written sample
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
