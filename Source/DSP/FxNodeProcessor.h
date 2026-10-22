#pragma once

#include <array>
#include <vector>

#include "DSP/DelayLine.h"
#include "DSP/DspConfig.h"
#include "DSP/LayerSnapshot.h"
#include "DSP/Pool.h"
#include "DSP/Processors.h"
#include "DSP/ProcessorsM6.h"
#include "DSP/Ramp.h"
#include "DSP/ReverbProcessor.h"

namespace hpeq::dsp
{
// Large processors shared by the slots of all layers of a project (memory allocated once, in
// prepare(); M7: one set per project instead of one per layer, DECISIONS D5).
using ReverbPool = SharedPool<ReverbProcessor, 16>;
using DelayPool = SharedPool<DelayProcessor, 8>;
using MonstaPool = SharedPool<MonstaProcessor, 8>;

struct LayerPools
{
    ReverbPool reverbs;
    DelayPool delays;
    MonstaPool monstas;

    void prepare(double sampleRate)
    {
        reverbs.prepare(sampleRate);
        delays.prepare(sampleRate);
        monstas.prepare(sampleRate);
    }
};

// One FX node: a chain of up to 6 processors on the node's band (DECISIONS D2) and the node's delta
// added to the layer output. The band M comes from the layer's CrossoverTree (H3, DECISIONS D4):
//
//   b   = gate * M                       (node input gate, see below)
//   s_0 = b
//   insert slot:  s_k = D_k(s_{k-1}) + mix_k * (P_k(s_{k-1}) - D_k(s_{k-1}))   (D_k: the slot's latency)
//   send slot:    s_k = s_{k-1} + mix_k * W_k(inGate_k * s_{k-1})               (W wet-only: Reverb, Delay)
//   out += presence * nodeMix * (s_K - D_node(b))
//
// Latency (PDC): Saturator oversampling and the Limiter look-ahead delay their slot; every slot
// present in the chain keeps its delay even while bypassed, so the node latency (the sum) only changes
// when the chain or those settings change. The layer aligns all nodes and the dry signal to the
// largest node latency and reports it to the host.
// Bypass keeps tails: disabling a node closes its input gate (20 ms) instead of fading its output,
// and disabling a send slot closes that slot's input; the tail rings out and the slot/node stops
// once its output has stayed below -120 dBFS for 50 ms. Mix 0 removes the contribution at once.
class FxNodeProcessor
{
public:
    // Alignment capacities for a sample rate (largest slot: 10 ms look-ahead or 4x oversampling).
    static int maxSlotLatency(double sampleRate);
    static int maxNodeLatency(double sampleRate) { return maxSlotLatency(sampleRate) * kMaxFxPerNode; }

    void prepare(double sampleRate, int maxBlockSize, LayerPools* pools);
    void reset() noexcept;
    void setTargets(const FxNodeParams&) noexcept;

    // `band` is this node's band from the tree; `blockOffset` = position of this chunk in the host block.
    void process(const float* const* band, const float* presence, float* const* out, int numChannels, int numSamples,
                 const Transport&, int blockOffset) noexcept;

    bool isRunning() const noexcept { return running; }
    double targetLowHz() const noexcept { return lowHz; }
    double targetHighHz() const noexcept { return highHz; }
    int latencySamples() const noexcept { return nodeLatency; }
    float getGainReductionDb(int slotIndex) const noexcept;
    // Compressor slot: detector level of the last block (dB) and auto makeup (dB); others: -120 / 0.
    float getCompressorInputDb(int slotIndex) const noexcept;
    float getCompressorAutoMakeupDb(int slotIndex) const noexcept;
    // Reverb slot: wet output level of the last block (dB) and ducking (dB >= 0); others: -120 / 0.
    float getReverbOutputDb(int slotIndex) const noexcept;
    float getReverbDuckDb(int slotIndex) const noexcept;
    bool slotRunning(int slotIndex) const noexcept { return slots[static_cast<size_t>(slotIndex)].running; }
    // A pooled slot (Reverb, Delay, Monsta) that wanted to start but found its pool exhausted.
    bool slotStarved(int slotIndex) const noexcept { return slots[static_cast<size_t>(slotIndex)].starved; }

    static int slotLatency(const FxSlotParams&, double sampleRate);
    static int nodeLatencyFor(const FxNodeParams&, double sampleRate);

private:
    struct Slot
    {
        bool wanted = false, running = false, starved = false, inChain = false;
        FxType type = FxType::Gain;
        FxSlotParams params;
        LinearRamp mix, inGate;
        int pooled = -1;
        int silentSamples = 0;
        int latency = 0;
        AlignmentDelay align;
        GainProcessor gain;
        CompressorProcessor compressor;
        SaturatorProcessor saturator;
        TransientProcessor transient;
        StereoProcessor stereo;
        GateProcessor gateFx;
        LimiterProcessor limiter;
    };

    static bool isSend(FxType t) noexcept { return t == FxType::Reverb || t == FxType::Delay; }
    static bool isPooled(FxType t) noexcept { return isSend(t) || t == FxType::MonstaFx; }
    void startSlot(Slot&, const FxSlotParams&) noexcept;
    void stopSlot(Slot&) noexcept;
    void releasePooled(Slot&) noexcept;
    void runInsert(Slot&, float* const* channels, int numChannels, int numSamples, const Transport&, int blockOffset) noexcept;
    float sendPeak(const Slot&) noexcept;

    double sampleRate = 48000.0;
    int mixSteps = 1, silenceSamples = 1;
    LayerPools* pools = nullptr;
    bool wanted = false, running = false;
    double lowHz = 250.0, highHz = 700.0; // clamped to [1 Hz, 0.45 fs], low <= high
    float targetMix = 1.0f;
    int nodeLatency = 0;
    int slotLatencyCap = 1, nodeLatencyCap = 1;
    // Channel mode: Stereo / Linked Stereo (compressor detector linked), Dual Mono (unlinked),
    // Left / Right / Mid / Side (the chain runs on that component only; its delta is routed back).
    ChannelMode mode = ChannelMode::Stereo, pendingMode = ChannelMode::Stereo;
    bool modeChangePending = false;
    LinearRamp nodeMix, gate;
    std::array<Slot, kMaxFxPerNode> slots;
    AlignmentDelay bandAlign;

    std::array<std::vector<float>, kMaxChannels> chain, work, bandWork, gated, wet, aligned, bandDelayed;
};
} // namespace hpeq::dsp
