#pragma once

#include <array>
#include <vector>

#include "DSP/DspConfig.h"
#include "DSP/LayerSnapshot.h"
#include "DSP/Processors.h"
#include "DSP/Ramp.h"
#include "DSP/ReverbProcessor.h"

namespace hpeq::dsp
{
// Reverbs are large (delay lines); each layer owns a fixed pool allocated in prepare(). Send slots
// borrow an instance while they run. acquire()/release() are only called on the audio thread.
class ReverbPool
{
public:
    static constexpr int kSize = 8;

    void prepare(double sampleRate);
    int acquire() noexcept;              // -1 when every instance is in use
    void release(int index) noexcept;
    ReverbProcessor& get(int index) noexcept { return reverbs[static_cast<size_t>(index)]; }
    int inUse() const noexcept;

private:
    std::array<ReverbProcessor, kSize> reverbs;
    std::array<bool, kSize> used {};
};

// One FX node: a chain of up to 6 processors on the node's band (DECISIONS D2) and the node's delta
// added to the layer output. The band M comes from the layer's CrossoverTree (H3, DECISIONS D4):
//
//   b   = gate * M                       (node input gate, see below)
//   s_0 = b
//   insert slot:   s_k = s_{k-1} + mix_k * (P_k(s_{k-1}) - s_{k-1})   (mix_k == 1 -> s_k = P_k(s_{k-1}))
//   send slot:     s_k = s_{k-1} + mix_k * W_k(inGate_k * s_{k-1})      (W wet-only: Reverb)
//   out += presence * nodeMix * (s_K - b)
//
// All processors have zero latency, so every term is time-aligned (PDC = 0).
// Bypass keeps tails: disabling a node closes its input gate (20 ms) instead of fading its output,
// and disabling a send slot closes that slot's input; the reverb rings out and the slot/node stops
// once its output has stayed below -120 dBFS for 50 ms. Mix 0 is a mix setting, not a bypass: it
// removes the node's contribution immediately and exactly (MIX-0).
class FxNodeProcessor
{
public:
    void prepare(double sampleRate, int maxBlockSize, ReverbPool* pool);
    void reset() noexcept;
    void setTargets(const FxNodeParams&) noexcept;

    // `band` is this node's band from the tree.
    void process(const float* const* band, const float* presence, float* const* out, int numChannels, int numSamples) noexcept;

    bool isRunning() const noexcept { return running; }
    double targetLowHz() const noexcept { return lowHz; }
    double targetHighHz() const noexcept { return highHz; }
    float getGainReductionDb(int slotIndex) const noexcept;
    bool slotRunning(int slotIndex) const noexcept { return slots[static_cast<size_t>(slotIndex)].running; }
    // A send slot that wanted to start but found the reverb pool exhausted.
    bool slotStarved(int slotIndex) const noexcept { return slots[static_cast<size_t>(slotIndex)].starved; }

private:
    struct Slot
    {
        bool wanted = false, running = false, starved = false;
        FxType type = FxType::Gain;
        FxSlotParams params;
        LinearRamp mix, inGate;
        int reverb = -1;
        int silentSamples = 0;
        GainProcessor gain;
        CompressorProcessor compressor;
    };

    static bool isSend(FxType t) noexcept { return t == FxType::Reverb; }
    void startSlot(Slot&, const FxSlotParams&) noexcept;
    void stopSlot(Slot&) noexcept;
    void runInsert(Slot&, float* const* channels, int numChannels, int numSamples) noexcept;

    double sampleRate = 48000.0;
    int mixSteps = 1, silenceSamples = 1;
    ReverbPool* pool = nullptr;
    bool wanted = false, running = false;
    double lowHz = 250.0, highHz = 700.0; // clamped to [1 Hz, 0.45 fs], low <= high
    float targetMix = 1.0f;
    // Channel mode: Stereo / Linked Stereo (compressor detector linked), Dual Mono (unlinked),
    // Left / Right / Mid / Side (the chain runs on that component only; its delta is routed back).
    ChannelMode mode = ChannelMode::Stereo, pendingMode = ChannelMode::Stereo;
    bool modeChangePending = false;
    LinearRamp nodeMix, gate;
    std::array<Slot, kMaxFxPerNode> slots;

    std::array<std::vector<float>, kMaxChannels> chain, work, bandWork, gated, wet;
};
} // namespace hpeq::dsp
