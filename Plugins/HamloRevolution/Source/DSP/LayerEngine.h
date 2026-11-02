#pragma once

#include <array>
#include <memory>
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
    // `sharedPools`: pooled processors shared with other layers (ProjectEngine); null = own pools.
    void prepare(double sampleRate, int maxBlockSize, LayerPools* sharedPools = nullptr);
    void reset() noexcept;

    void publish(const LayerSnapshot&) noexcept;
    // Picks up a newer snapshot now (process() also does); lets a caller read the latency first.
    void update() noexcept
    {
        if (exchange.acquire())
            applySnapshot();
        updateTree();
    }
    // False when processing would return the input exactly: PRE-EQ neutral (nothing running or
    // fading), no allocated tree edge.
    bool needsProcessing() const noexcept { return hasEq || !preEq.isNeutral() || tree.isActive() || mixNode.isRunning(); }

    void process(float* const* channels, int numChannels, int numSamples, const Transport& transport = {}) noexcept;

    // Layer latency = the largest node latency of the snapshot (all nodes present, enabled or not, so
    // bypassing a node does not change it). The dry path and every node are aligned to it.
    int getLatencySamples() const noexcept { return layerLatency + mixNode.latencySamples(); }
    static int latencyFor(const LayerSnapshot&, double sampleRate);
    const FxNodeProcessor& getNode(int index) const noexcept { return nodes[static_cast<size_t>(index)]; }
    // D7: the whole-layer chain's node.
    const FxNodeProcessor& getMixNode() const noexcept { return mixNode; }
    // EQ pass: the PRE-EQ (dynamic gain offsets, solo state) for the page.
    const PreEqProcessor& getPreEq() const noexcept { return preEq; }
    bool isTreeActive() const noexcept { return tree.isActive(); }
    int reverbsInUse() const noexcept { return pools->reverbs.inUse(); }
    int delaysInUse() const noexcept { return pools->delays.inUse(); }
    int monstasInUse() const noexcept { return pools->monstas.inUse(); }

private:
    void applySnapshot() noexcept;
    void updateTree() noexcept;

    double sampleRate = 48000.0;
    int maxBlock = 0;
    int controlPhase = 0;

    SnapshotExchange<LayerSnapshot> exchange;
    PreEqProcessor preEq;
    std::array<FxNodeProcessor, kMaxFxNodesPerLayer> nodes;
    FxNodeProcessor mixNode; // D7: whole-layer chain, after the frequency nodes
    AlignmentDelay mixDryAlign;
    std::array<std::vector<float>, kMaxChannels> mixBand, mixDelta;
    std::vector<float> mixPresence;
    std::unique_ptr<LayerPools> ownPools; // allocated in prepare() only when no shared pools are given
    LayerPools* pools = nullptr;
    bool hasEq = false;
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

    // SOLO (HamloFader): while any node asks for it, the layer plays only those nodes' processed bands
    // (band aligned to the layer latency + its delta); 10 ms cross-fades in and out.
    std::array<bool, kMaxFxNodesPerLayer> solo {};
    bool anySolo = false;
    double soloMix = 0.0;
    std::array<AlignmentDelay, kMaxFxNodesPerLayer> soloBandAlign;
    std::array<std::vector<float>, kMaxChannels> soloOut, soloBand;
};
} // namespace hpeq::dsp
