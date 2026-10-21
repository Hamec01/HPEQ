#pragma once

#include <array>
#include <vector>

#include "DSP/FxNodeProcessor.h"
#include "DSP/LayerSnapshot.h"
#include "DSP/PreEqProcessor.h"

namespace hpeq::dsp
{
// One layer (decision B):  x -> PRE-EQ -> xE ;  y = xE + sum_i nodeMix_i * delta_i(xE)
//
// Threading: publish() on the message thread, process() on the audio thread. process() never
// allocates, locks or waits; blocks longer than the prepared size are processed in chunks.
// Latency is 0 in M2 (IIR only); getLatencySamples() is the single place that will change.
class LayerEngine
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset() noexcept;

    void publish(const LayerSnapshot&) noexcept;

    void process(float* const* channels, int numChannels, int numSamples) noexcept;

    int getLatencySamples() const noexcept { return 0; }
    const FxNodeProcessor& getNode(int index) const noexcept { return nodes[static_cast<size_t>(index)]; }

private:
    void applySnapshot() noexcept;

    double sampleRate = 48000.0;
    int maxBlock = 0;
    int controlPhase = 0;

    SnapshotExchange<LayerSnapshot> exchange;
    PreEqProcessor preEq;
    std::array<FxNodeProcessor, kMaxFxNodesPerLayer> nodes;
    std::array<std::vector<float>, kMaxChannels> xeCopy;
};
} // namespace hpeq::dsp
