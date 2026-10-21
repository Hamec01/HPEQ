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
};

// Processor parameter block. The meaning of `values` depends on `type` (see the k* indices).
struct FxSlotParams
{
    bool active = false;
    FxType type = FxType::Gain;
    float mix = 1.0f;
    std::array<float, 8> values {};
};

namespace slot
{
// Gain
constexpr int kGainDb = 0;
// Compressor
constexpr int kThresholdDb = 0, kRatio = 1, kKneeDb = 2, kAttackMs = 3, kReleaseMs = 4, kMakeupDb = 5;
} // namespace slot

struct FxNodeParams
{
    bool active = false;
    float lowHz = 250.0f;
    float highHz = 700.0f;
    float mix = 1.0f;
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
