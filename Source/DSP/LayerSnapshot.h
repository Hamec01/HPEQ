#pragma once

#include <array>
#include <atomic>
#include <type_traits>

#include "Core/FxProjectState.h"

namespace hpeq::dsp
{
// Plain-data image of one layer that the audio thread reads. Fixed capacity, trivially copyable:
// built on the message thread from LayerState, handed over through SnapshotExchange.

struct EqBandParams
{
    bool active = false;
    EqShape shape = EqShape::Bell;
    float frequencyHz = 1000.0f;
    float gainDb = 0.0f;
    float q = 0.707f;
    int slopeDbPerOct = 12;
    ChannelMode channel = ChannelMode::Stereo;
};

// Processor parameter block. The meaning of `values` depends on `type` (see the k* indices).
struct FxSlotParams
{
    bool active = false;
    FxType type = FxType::Gain;
    float mix = 1.0f;
    std::array<float, 32> values {};
};

namespace slot
{
// Gain
// Gain = HamloFader (band level utility). gainDb is the M-era law; the rest: polarity 0 none / 1 left /
// 2 right / 3 both, delay in ms (0..20, not compensated: it is the point), mute, solo (the layer plays
// only its soloed bands), targetDb (the level MATCH aims at; read by the page, not the DSP).
constexpr int kGainDb = 0, kFadPolarity = 1, kFadDelayMs = 2, kFadMute = 3, kFadSolo = 4, kFadTargetDb = 5;
// Compressor
constexpr int kThresholdDb = 0, kRatio = 1, kKneeDb = 2, kAttackMs = 3, kReleaseMs = 4, kMakeupDb = 5,
              kCompDetector = 6, kCompCharacter = 7, kCompAutoMakeup = 8; // detector 0 Peak / 1 RMS; character 0 Clean / 1 Punch / 2 Glue
// Reverb (wet-only send). 0..5 are the M4 reverb ("Classic"); the rest drive the HamloVerb spaces
// (space 0 Classic / 1 Room / 2 Chamber / 3 Hall / 4 Plate / 5 Cathedral / 6 Ambience; color 0 1970 /
// 1 1980 / 2 Now; sync 0 = free pre-delay, else a note division, see verb::syncBeats).
constexpr int kPredelayMs = 0, kDecaySeconds = 1, kHfRatio = 2, kSize = 3, kWidth = 4, kLowCutHz = 5,
              kRevSpace = 6, kRevColor = 7, kRevSync = 8, kRevAttack = 9, kRevEarlyDiff = 10, kRevLateDiff = 11,
              kRevModRate = 12, kRevModDepth = 13, kRevBassMult = 14, kRevBassXover = 15, kRevHighXover = 16,
              kRevHighCut = 17, kRevDistance = 18, kRevDuck = 19, kRevFreeze = 20;
// Saturator (insert; curve 0 tanh / 1 tube / 2 hard; oversampling 0 = 1x, 1 = 2x, 2 = 4x). 0..5 are the M6
// saturator ("Classic"); the rest drive the HamloMag models (model 0 Classic / 1 Tape / 2 Tube / 3 Iron /
// 4 Germanium / 5 Vinyl; speed 0..2 = 19 / 9.5 / 4.76 cm/s or 33 / 45 / 78 rpm; tone and even/odd -1..1).
constexpr int kDriveDb = 0, kCurve = 1, kBias = 2, kTrimDb = 3, kOversampling = 4, kAutoGain = 5,
              kSatModel = 6, kSatPunish = 7, kSatTone = 8, kSatLowCutHz = 9, kSatHighCutHz = 10, kSatThump = 11, kSatSteep = 12,
              kSatHarmonics = 13, kSatEvenOdd = 14, kSatTapeBias = 15, kSatSpeed = 16, kSatWow = 17, kSatHiss = 18, kSatHum = 19,
              kSatCrackle = 20, kSatNoiseFollow = 21;
// Transient shaper (insert). 0..2 are the M6 shaper; the rest drive HamloUdarnik (module pass 6):
// style 0 PRAVDA / 1 UDARNIK / 2 PERESTROIKA; look-ahead 0 SPUTNIK 0 ms / 1 VOSTOK 2 ms / 2 SOYUZ 5 ms;
// rise and decay as log2 of the factor (-1..1, 0 = x1); strictness = 1 - sensitivity (0 = every front counts);
// preserve 0..1; listen 0 full / 1 attack / 2 body; output dB (-24..+12). Every value 0 = the M6 shaper.
constexpr int kAttackGainDb = 0, kSustainGainDb = 1, kTransientSpeed = 2,
              kTrStyle = 3, kTrLookahead = 4, kTrRiseLog2 = 5, kTrDecayLog2 = 6, kTrStrictness = 7, kTrPreserve = 8, kTrListen = 9,
              kTrOutputDb = 10;
// Delay (wet-only send; sync 0 = free time, else a note division index). 0..6 are the M6 delay
// ("Classic"; damping = the high cut); the rest drive the HamloEcho characters (character 0 Classic /
// 1 Clean / 2 Tape / 3 BBD / 4 Lo-Fi; time mode 0 Tape / 1 Fade; resonance 0..1; band gain in dB).
constexpr int kDelayTimeMs = 0, kDelaySync = 1, kFeedback = 2, kPingPong = 3, kDampingHz = 4, kDelayLowCutHz = 5, kDelayWidth = 6,
              kDlyCharacter = 7, kDlyTimeRMs = 8, kDlySyncR = 9, kDlyLink = 10, kDlyCross = 11, kDlyDriveDb = 12,
              kDlyModRate = 13, kDlyModDepth = 14, kDlyDiffusion = 15, kDlyPitch = 16, kDlyLowRes = 17, kDlyHighRes = 18,
              kDlyBandHz = 19, kDlyBandGainDb = 20, kDlyBandQ = 21, kDlyDuck = 22, kDlyFreeze = 23, kDlyLofi = 24, kDlyTimeMode = 25;
// Monsta FX (insert; division 0 = 1/4, 1 = 1/8, 2 = 1/16, 3 = 1/32)
constexpr int kDivision = 0, kChance = 1, kSeed = 2, kCrushBits = 3, kGateDepth = 4, kReverseShare = 5;
// Stereo (insert; needs a stereo work domain)
constexpr int kStereoWidth = 0, kStereoBalance = 1,
              // HamloShirota (module pass 8): style 0 EDINSTVO / 1 PROSTOR / 2 KOSMOS; stereoize 0..1; space 0..1;
              // mono bass Hz (0 = off); rotation degrees; safe 0/1; mono listen 0/1; output dB. Every value 0 = the M6 law.
              kStStyle = 2, kStStereoize = 3, kStSpace = 4, kStMonoBassHz = 5, kStRotation = 6, kStSafe = 7, kStMonoListen = 8, kStOutputDb = 9;
// Gate / expander (insert)
constexpr int kGateThresholdDb = 0, kGateRatio = 1, kGateRangeDb = 2, kGateAttackMs = 3, kGateHoldMs = 4, kGateReleaseMs = 5, kGateHysteresisDb = 6,
              // HamloZastava (module pass 7): style 0 ZASTAVA / 1 PATRUL / 2 TREVOGA; look-ahead 0 / 1 / 2 (0 / 2 / 5 ms);
              // key HP / LP Hz (0 = off); key listen 0/1; flip 0/1; output dB. Every value 0 = the M6 gate.
              kGateStyle = 7, kGateLookahead = 8, kGateKeyHpHz = 9, kGateKeyLpHz = 10, kGateKeyListen = 11, kGateFlip = 12, kGateOutputDb = 13;
// Band limiter (insert, look-ahead = latency)
constexpr int kCeilingDb = 0, kLimiterReleaseMs = 1, kLookaheadMs = 2,
              // HamloRedWall (module pass 5): threshold dB; true peak 0/1; auto release 0/1;
              // style 0 GLASNOST / 1 STAKHANOV / 2 OCTOBER; link 0..1 (absent = 1); monitor 0 normal / 1 unity / 2 delta;
              // output dB (-24..0, after the limiter)
              kLimThresholdDb = 3, kLimTruePeak = 4, kLimAutoRelease = 5, kLimStyle = 6, kLimLink = 7, kLimMonitor = 8, kLimOutputDb = 9;
} // namespace slot

// Host transport as seen by the layer for one host block (tempo-synced FX: Delay, Monsta).
// Without a playing transport the tempo is still the host's (or 120 BPM) and positions come from
// a free-running sample counter.
struct Transport
{
    double bpm = 120.0;
    bool playing = false;
    bool hasPosition = false;
    double ppqAtBlockStart = 0.0;
};

struct FxNodeParams
{
    bool active = false;
    float lowHz = 250.0f;
    float highHz = 700.0f;
    float mix = 1.0f;
    ChannelMode channel = ChannelMode::Stereo;
    int numSlots = 0;
    std::array<FxSlotParams, kMaxFxPerNode> slots {};
};

struct LayerSnapshot
{
    std::array<EqBandParams, kMaxEqNodesPerLayer> eq {};
    std::array<FxNodeParams, kMaxFxNodesPerLayer> fx {};
    FxNodeParams mix {}; // D7: the whole-layer chain (band unused)
};

static_assert(std::is_trivially_copyable_v<LayerSnapshot>);

// What the M2 engine really processes. Anything else in the model is carried in state but inactive
// in the snapshot, so the DSP never pretends to run an unimplemented processor or shape.
bool isEqShapeProcessed(EqShape) noexcept;
bool isFxTypeProcessed(FxType) noexcept;

LayerSnapshot makeSnapshot(const LayerState&);

// Single-writer / single-reader triple buffer. The writer never blocks the reader and the reader
// never allocates or waits; it always sees the most recent complete snapshot.
template <typename T>
class SnapshotExchange
{
    static_assert(std::is_trivially_copyable_v<T>);

public:
    // Writer side (message thread). Not re-entrant: callers serialise writes.
    void publish(const T& value) noexcept
    {
        buffers[static_cast<size_t>(back)] = value;
        back = middle.exchange(back | kFresh, std::memory_order_acq_rel) & kIndexMask;
    }

    // Reader side (audio thread). Returns true when a newer snapshot was picked up.
    bool acquire() noexcept
    {
        if ((middle.load(std::memory_order_acquire) & kFresh) == 0)
            return false;
        front = middle.exchange(front, std::memory_order_acq_rel) & kIndexMask;
        return true;
    }

    const T& current() const noexcept { return buffers[static_cast<size_t>(front)]; }

private:
    static constexpr int kIndexMask = 3, kFresh = 4;
    std::array<T, 3> buffers {};
    std::atomic<int> middle { 1 };
    int front = 0, back = 2;
};
} // namespace hpeq::dsp
