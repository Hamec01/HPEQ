#pragma once

#include <array>
#include <vector>

#include "DSP/CrossoverTree.h"
#include "DSP/FxNodeProcessor.h"
#include "DSP/LayerSnapshot.h"
#include "DSP/PreEqProcessor.h"

namespace hpeq::dsp
{
// One layer (decision B, frequency core H3 / DECISIONS D4):
//
//   xE = PRE-EQ(x)
//   no allocated edge:    y = xE                                       (tree not run: bit-exact)
//   otherwise:            y = D_L(AP(xE)) + sum_j S_j( D_{L-L_j}( presence_j * nodeMix_j * (chain_j(band'_j) - D_{L_j}(band'_j)) ) )
//   (L = layer latency, L_j = node latency; with L = 0 every D is an exact no-op)
//
// AP(xE) and band_j come from the layer's CrossoverTree (see there). Node ranges glide; nodes that
// start or stop have their edges born / parked at 1 Hz, so no structure change ever needs a
// crossfade between differently phased signals.
//
// Threading: publish() on the message thread, process() on the audio thread. process() never
// allocates, locks or waits; blocks longer than the prepared size are processed in chunks.
class LayerEngine
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset() noexcept;

    void publish(const LayerSnapshot&) noexcept;

    void process(float* const* channels, int numChannels, int numSamples, const Transport& transport = {}) noexcept;

    // Layer latency = the largest node latency of the snapshot (all nodes present, enabled or not, so
    // bypassing a node does not change it). The dry path and every node are aligned to it.
    int getLatencySamples() const noexcept { return layerLatency; }
    static int latencyFor(const LayerSnapshot&, double sampleRate);
    const FxNodeProcessor& getNode(int index) const noexcept { return nodes[static_cast<size_t>(index)]; }
    bool isTreeActive() const noexcept { return tree.isActive(); }
    int reverbsInUse() const noexcept { return pools.reverbs.inUse(); }
    int delaysInUse() const noexcept { return pools.delays.inUse(); }
    int monstasInUse() const noexcept { return pools.monstas.inUse(); }

private:
    void applySnapshot() noexcept;
    void updateTree() noexcept;

    double sampleRate = 48000.0;
    int maxBlock = 0;
    int controlPhase = 0;

    SnapshotExchange<LayerSnapshot> exchange;
    PreEqProcessor preEq;
    std::array<FxNodeProcessor, kMaxFxNodesPerLayer> nodes;
    LayerPools pools; // reverb/delay/monsta memory for pooled slots, allocated in prepare()
    int layerLatency = 0;
    AlignmentDelay dryAlign;
    std::array<AlignmentDelay, kMaxFxNodesPerLayer> deltaAlign;
    CrossoverTree tree;

    std::array<std::vector<float>, kMaxChannels> xeCopy, dry;
    std::array<std::array<std::vector<float>, kMaxChannels>, kMaxFxNodesPerLayer> bands;
    std::array<std::array<std::vector<float>, kMaxChannels>, kMaxFxNodesPerLayer> deltas;
    std::array<std::array<float*, kMaxChannels>, kMaxFxNodesPerLayer> deltaPtr {};
    std::array<float* const*, kMaxFxNodesPerLayer> deltaList {};
    std::array<std::vector<float>, kMaxFxNodesPerLayer> presence;
    std::array<float*, kMaxChannels> dryPtr {};
    std::array<std::array<float*, kMaxChannels>, kMaxFxNodesPerLayer> bandPtr {};
    std::array<float* const*, kMaxFxNodesPerLayer> bandList {};
    std::array<float*, kMaxFxNodesPerLayer> presencePtr {};
};
} // namespace hpeq::dsp
