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
    std::array<float, 24> values {};
};

namespace slot
{
// Gain
constexpr int kGainDb = 0;
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
// Saturator (insert; curve 0 tanh / 1 tube / 2 hard; oversampling 0 = 1x, 1 = 2x, 2 = 4x)
constexpr int kDriveDb = 0, kCurve = 1, kBias = 2, kTrimDb = 3, kOversampling = 4, kAutoGain = 5;
// Transient shaper (insert)
constexpr int kAttackGainDb = 0, kSustainGainDb = 1, kTransientSpeed = 2;
// Delay (wet-only send; sync 0 = free time, else a note division index)
constexpr int kDelayTimeMs = 0, kDelaySync = 1, kFeedback = 2, kPingPong = 3, kDampingHz = 4, kDelayLowCutHz = 5, kDelayWidth = 6;
// Monsta FX (insert; division 0 = 1/4, 1 = 1/8, 2 = 1/16, 3 = 1/32)
constexpr int kDivision = 0, kChance = 1, kSeed = 2, kCrushBits = 3, kGateDepth = 4, kReverseShare = 5;
// Stereo (insert; needs a stereo work domain)
constexpr int kStereoWidth = 0, kStereoBalance = 1;
// Gate / expander (insert)
constexpr int kGateThresholdDb = 0, kGateRatio = 1, kGateRangeDb = 2, kGateAttackMs = 3, kGateHoldMs = 4, kGateReleaseMs = 5, kGateHysteresisDb = 6;
// Band limiter (insert, look-ahead = latency)
constexpr int kCeilingDb = 0, kLimiterReleaseMs = 1, kLookaheadMs = 2;
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
