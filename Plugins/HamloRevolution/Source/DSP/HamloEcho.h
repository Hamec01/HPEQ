#pragma once

#include <array>
#include <atomic>
#include <vector>

#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
// HamloEcho (module pass 3, docs/fx-addon/DELAY_DESIGN.md): the delay behind the Delay module's
// characters 1..4. Wet-only, like the M6 delay it extends.
//
//   in -> (freeze gate) -> L / R lines (4 s) -> read: time + wobble; TAPE glides the time (the pitch
//   bends like tape), FADE cross-fades two heads (no pitch change)
//   -> loop: low cut (12 dB/oct, resonance) -> band (bell) -> high cut (12 dB/oct, resonance) ->
//      diffusion (4 allpasses) -> pitch (two-voice grain shifter; a new grain starts where it best
//      matches the playing one, so the cross-fades stay in phase) -> character (drive; Tape / BBD colour;
//      Lo-Fi rate and bits) -> safety clip 4 tanh(x / 4)
//   -> echo e: width -> ducking -> wet
//   feedback: L <- fb ((1 - cross) eL + cross eR), R likewise; PING-PONG feeds the input to the left line.
// Freeze cross-fades the feedback to the raw echo with gain 1 (lossless; the wobble fades out) and
// closes the input. Filter coefficients follow at control rate on the running sample clock; LFOs and
// smoothers run per sample, so a render does not depend on the host block size. All memory is
// allocated in prepare().
namespace echo
{
enum Character { Classic = 0, Clean, Tape, Bbd, LoFi, NumCharacters };
enum TimeMode { TapeTime = 0, FadeTime };

constexpr double kMaxSeconds = 4.0;
constexpr int kSyncDivisions = 12; // 0 = free, then the M6 delay's divisions

const char* characterName(int) noexcept; // "Classic", "Clean", "Tape", "BBD", "Lo-Fi"
const char* syncName(int) noexcept;      // "Free", "1/32", ...
double syncBeats(int) noexcept;          // quarter notes (0 for free)

// Echo time of the left (right = false) or right line in seconds (link: the right follows the left).
double timeSeconds(const FxSlotParams&, bool right, double bpm) noexcept;
// Magnitude (dB) of one pass through the loop filters (low cut, band, high cut, the character's own
// bandwidth) at a frequency, for the display and the tests.
double loopResponseDb(double hz, const FxSlotParams&, double sampleRate = 48000.0) noexcept;
} // namespace echo

class EchoProcessor
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

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1 = 0, z2 = 0;
        double process(double x) noexcept
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };
    struct Allpass
    {
        std::vector<float> buffer;
        int write = 0, length = 1;
        float process(float x, float g) noexcept;
    };
    struct Channel
    {
        std::vector<float> line;
        double delay = -1.0, target = 0.0;           // TAPE: the gliding read
        double fadeFrom = 0.0, fadeTo = 0.0, fade = 1.0; // FADE: two heads, 0 -> 1
        Biquad lowCut, band, highCut;
        std::array<Allpass, 4> diffusers;
        std::vector<float> pitchRing;
        int pitchWrite = 0;
        std::array<double, 2> grainDelay {}, grainPos { 0.0, 0.5 }; // two voices, half a grain apart
        double tone1 = 0.0, tone2 = 0.0;             // character bandwidth (one-poles)
        double held = 0.0;                           // lo-fi sample-and-hold value
    };

    void updateTargets(const FxSlotParams&, const Transport&) noexcept;
    void designFilters() noexcept;
    double readLine(const Channel&, double delay) const noexcept;
    double loop(Channel&, double y) noexcept;
    double pitchShift(Channel&, double x) noexcept;
    double readPitch(const Channel&, double delay) const noexcept;

    double sampleRate = 48000.0;
    std::array<Channel, 2> channels;
    int writePos = 0, lineSize = 0, pitchSize = 0;
    bool primed = false;

    // targets (per block) and their smoothed values
    int character = echo::Tape, timeMode = echo::TapeTime;
    bool pingPong = false, freeze = false;
    double feedback = 0.0, feedbackTarget = 0.0, cross = 0.0, crossTarget = 0.0, width = 1.0, widthTarget = 1.0;
    double driveGain = 1.0, driveTarget = 1.0;
    double modDepthSamples = 0.0, modDepthTarget = 0.0, modRate = 0.5;
    double diffusion = 0.0, diffusionTarget = 0.0;
    double pitchRatio = 1.0;
    double lofi = 0.0;
    double lowHz = 120.0, lowHzTarget = 120.0, highHz = 8000.0, highHzTarget = 8000.0, bandHz = 1000.0, bandHzTarget = 1000.0;
    double lowQ = 0.707, lowQTarget = 0.707, highQ = 0.707, highQTarget = 0.707, bandGainDb = 0.0, bandGainTarget = 0.0;
    double bandQ = 1.0, bandQTarget = 1.0;
    bool highOpen = false;
    double freezeMix = 0.0, inputGate = 1.0;
    double duckAmount = 0.0, duckEnvelope = 0.0, duckGain = 1.0;
    double toneCoefficient = 1.0; // the character's own bandwidth
    int controlCounter = 0, holdCounter = 0;

    // LFOs: wobble (quadrature: L sine, R cosine) and tape flutter
    double wowCos = 1.0, wowSin = 0.0, wowRotCos = 1.0, wowRotSin = 0.0;
    double flutterCos = 1.0, flutterSin = 0.0, flutterRotCos = 1.0, flutterRotSin = 0.0;

    float blockPeak = 0.0f;
    std::atomic<float> meterOutDb { -120.0f }, meterDuckDb { 0.0f };
};
} // namespace hpeq::dsp
