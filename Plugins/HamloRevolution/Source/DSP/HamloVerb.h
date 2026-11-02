#pragma once

#include <array>
#include <atomic>
#include <vector>

#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
// HamloVerb (module pass 2, docs/fx-addon/REVERB_DESIGN.md): the space reverb behind the Reverb
// module's spaces 1..6. Wet-only, like the M4 reverb it extends.
//
//   in -> pre-delay (free or tempo sync) -+-> early field: image-source taps of the space's room
//                                         |   (two microphones), early-diffusion allpasses
//                                         +-> attack taps (build-up) -> input diffusion -> 16-line FDN
//   early * earlyGain(distance) + late * lateGain(distance) -> colour bandwidth -> width -> low/high
//   cut (12 dB/oct) -> ducking under the input -> wet
//
// FDN: 16 lines, Hadamard feedback (orthogonal, lossless), per line a sine-modulated read (cubic
// interpolation; an exact integer read when the depth is 0 and the size has settled), a Schroeder
// allpass in the loop (late diffusion) and a three-band decay: gain g_M (mid T60) times a low shelf
// (DC gain g_L / g_M at Bass Xover) times a high shelf (Nyquist gain g_H / g_M at High Xover), both RBJ
// with slope 1 (monotonic), g_X = 10^(-3 d / (T60_X fs)); the loop gain never exceeds max(g_L, g_M) < 1.
// Freeze sets every gain to exactly 1, fades the modulation out (an interpolated read is a low-pass)
// and closes the input: a lossless loop.
// All memory is allocated in prepare(); process() never allocates; every smoother and LFO runs per
// sample, so a render does not depend on the host block size.
namespace verb
{
enum Space { Classic = 0, Room, Chamber, Hall, Plate, Cathedral, Ambience, NumSpaces };
enum Color { Color1970 = 0, Color1980, ColorNow };

constexpr int kLines = 16;
constexpr int kMaxEarlyTaps = 24;
constexpr double kMaxPredelayMs = 500.0;
constexpr double kMaxAttackSeconds = 0.25;   // x size
constexpr double kMinSize = 0.3, kMaxSize = 1.5;
constexpr double kMaxModMs = 1.6;            // depth 1 at the deepest space and colour
constexpr int kSyncDivisions = 12;           // 0 = free (ms), then the note values below

const char* spaceName(int space) noexcept;   // "Classic", "Room", ...
const char* colorName(int color) noexcept;   // "1970", "1980", "NOW"
const char* syncName(int division) noexcept; // "Free", "1/64", ...
double syncBeats(int division) noexcept;     // length of a division in quarter notes (0 for free)

struct SpaceDef
{
    double roomX, roomY, roomZ;              // metres; 0 = no early field (plate)
    double reflection;                       // wall reflection gain per bounce
    double minDelayMs, maxDelayMs;           // FDN line lengths at size 1
    double allpassShare;                     // in-loop allpass length / line length
    double inputDiffusion;                   // allpass coefficient at late diffusion 1
    double earlyLevel, lateLevel;            // output balance at distance 0.5
    double earlyToLate;                      // how much of the early field also feeds the tank
    double modMs;                            // modulation depth at depth 1 (ms)
    double outputGain;                       // level match between spaces
};
const SpaceDef& spaceDef(int space) noexcept;

// Early taps of a space at size 1, for one microphone (0 = left, 1 = right): time after the direct
// sound (s) and signed gain, strongest first. Computed by the image-source method (orders 1..3).
struct EarlyTaps
{
    int count = 0;
    std::array<double, kMaxEarlyTaps> seconds {}, gain {};
};
EarlyTaps earlyTaps(int space, int mic);

// Decay time (s) at a frequency for the current settings (the three-band law), for the display.
double decayAt(double hz, const FxSlotParams&) noexcept;
} // namespace verb

class HamloVerb
{
public:
    void prepare(double sampleRate);
    void reset() noexcept;
    void process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams&,
                 const Transport&) noexcept;

    float lastBlockPeak() const noexcept { return blockPeak; }
    float outputLevelDb() const noexcept { return meterOutDb.load(std::memory_order_relaxed); }
    float duckingDb() const noexcept { return meterDuckDb.load(std::memory_order_relaxed); }

    static double tailSeconds(const FxSlotParams&, double bpm) noexcept;
    static double predelaySeconds(const FxSlotParams&, double bpm) noexcept;

private:
    struct Line
    {
        std::vector<float> buffer;
        double length = 0.0, lengthTarget = 0.0; // samples
        std::vector<float> allpass;
        int allpassLength = 1;
        double lfoCos = 1.0, lfoSin = 0.0, lfoRotCos = 1.0, lfoRotSin = 0.0; // phase at the next tick, rotation per tick
        double gainMid = 0.0, lowRatio = 1.0, highRatio = 1.0;
        double gainMidTarget = 0.0, lowRatioTarget = 1.0, highRatioTarget = 1.0;
    };
    struct Allpass
    {
        std::vector<float> buffer;
        int write = 0, length = 1;
        float process(float x, float g) noexcept;
    };
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        std::array<double, 2> z1 {}, z2 {};
        double process(double x, int ch) noexcept;
    };

    void configureSpace(int space) noexcept;
    void updateTargets(const FxSlotParams&, const Transport&) noexcept;
    void updateFilters() noexcept;
    void updateShelves() noexcept;
    void controlTick() noexcept;

    double sampleRate = 48000.0;
    int space = verb::Hall;
    bool primed = false;

    std::array<Line, verb::kLines> lines;
    std::array<std::vector<float>, 2> predelay;
    int predelayWrite = 0;
    double predelaySamples = 0.0, predelayTarget = 0.0;
    std::array<std::vector<float>, 2> early; // the pre-delayed input, read by the early taps
    int earlyWrite = 0;
    std::array<verb::EarlyTaps, 2> taps;     // of the current space (size 1)
    std::array<std::array<Allpass, 4>, 2> earlyDiffusers, inputDiffusers;

    // smoothed controls
    double size = 1.0, sizeTarget = 1.0;
    double attack = 0.0, attackTarget = 0.0;
    double earlyDiff = 0.0, earlyDiffTarget = 0.0, lateDiff = 0.0, lateDiffTarget = 0.0;
    double modDepthSamples = 0.0, modDepthTarget = 0.0;
    double earlyGain = 0.0, earlyGainTarget = 0.0, lateGain = 0.0, lateGainTarget = 0.0;
    double width = 1.0, widthTarget = 1.0;
    double inputGate = 1.0, inputGateTarget = 1.0; // freeze closes it
    double lowCutHz = 80.0, lowCutTarget = 80.0, highCutHz = 20000.0, highCutTarget = 20000.0;
    double bassCos = 1.0, bassSin = 0.0, highCos = 1.0, highSin = 0.0; // shelf corners
    double colorCoefficient = 1.0;                // tank bandwidth one-pole (1 = open)
    std::array<double, 2> colorState {};
    double saturation = 0.0;                      // 1970: soft saturation in the loop
    double duckAmount = 0.0, duckEnvelope = 0.0, duckGain = 1.0;
    double earlyDamp = 1.0;
    std::array<double, 2> earlyDampState {};
    int controlCounter = 0;
    int lineWrite = 0, allpassWrite = 0; // shared by all lines (equal buffer sizes)
    // Per-line loop state as arrays (the arithmetic of the 16 lines vectorises): read delay and its
    // per-sample step to the next control tick, decay shelves (coefficients, states) and gain.
    using LineArray = std::array<double, verb::kLines>;
    LineArray delayNow {}, delayStep {}, delayNext {};
    LineArray lb0 {}, lb1 {}, lb2 {}, la1 {}, la2 {}, hb0 {}, hb1 {}, hb2 {}, ha1 {}, ha2 {};
    LineArray lz1 {}, lz2 {}, hz1 {}, hz2 {}, gMid {};
    double tickGainK = 0.0, tickSizeK = 0.0, tickFilterK = 0.0;
    bool shelvesDirty = true, filtersDirty = true, tapsDirty = true;
    std::array<std::array<int, verb::kMaxEarlyTaps>, 2> tapWhole {};
    std::array<std::array<float, verb::kMaxEarlyTaps>, 2> tapFrac {}, tapGain {};
    double attackSpread = 0.0;
    Biquad lowCut, highCut;
    bool highCutOpen = true;

    float blockPeak = 0.0f;
    std::atomic<float> meterOutDb { -120.0f }, meterDuckDb { 0.0f };
};
} // namespace hpeq::dsp
