#pragma once

#include <array>

#include "Core/FxProjectState.h"
#include "DSP/Biquad.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// H3 frequency core (DECISIONS D4), phase-coherent and order-free.
//
// Every running FX node j owns two edges, lo_j and hi_j. With AP_e the 2nd-order all-pass of edge e
// (Q = 1/sqrt 2, the sum of an LR4 low/high pair) and E the set of allocated edges:
//
//     dry       = prod_{e in E} AP_e (x)
//     band_j    = prod_{e in E, e not lo_j/hi_j} AP_e ( LP4_{hi_j}( HP4_{lo_j}(x) ) )
//
// Linkwitz-Riley outputs are in phase with their all-pass sum, so HP4 = AP*|HP4| and LP4 = AP*|LP4|:
// band_j / dry = |HP4_lo(f)| * |LP4_hi(f)| is real at every frequency. A node's delta therefore never
// rotates against the dry signal (the H1 failure), touching nodes add up like a crossover, overlapping
// nodes simply add, and nothing depends on the order of the edges, so edges may glide past each other.
//
// Edges are born and retired at kParkingHz, where an all-pass is numerically almost an identity
// (its zero state is its steady state), and glide to/from their targets: no tree rebuild and no
// crossfade between differently phased signals is ever needed. With no allocated edge the tree is
// not run and the layer is a bit-exact pass-through.
class CrossoverTree
{
public:
    static constexpr double kParkingHz = 1.0;
    static constexpr double kBirthSeconds = 0.060;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    // Called once per host block (control decisions only happen at block starts).
    // running[j]: node j is processing; low/high: its clamped target edges.
    void update(const std::array<bool, kMaxFxNodesPerLayer>& running,
                const std::array<double, kMaxFxNodesPerLayer>& lowHz,
                const std::array<double, kMaxFxNodesPerLayer>& highHz) noexcept;

    bool isActive() const noexcept { return anyAllocated; }
    bool hasBand(int node) const noexcept { return edges[static_cast<size_t>(node)].allocated; }

    // dry[ch][i], bands[j][ch][i] and presence[j][i] for every allocated node j. presence fades a
    // node's delta in once its edges have arrived (decided on the sample clock, not per host block).
    void process(const float* const* in, int numChannels, int numSamples, int controlPhase,
                 float* const* dry, float* const* const* bands, float* const* presence) noexcept;

private:
    struct NodeEdges
    {
        bool allocated = false, parking = false, birth = false;
        LinearRamp logLow, logHigh;
        LinearRamp presence; // 0 while the edges glide in, then 1 over kMixRampSeconds
        std::array<TptSvf, 2> dryAllPass;        // AP_lo, AP_hi in the dry chain
        std::array<TptSvf, 2> highPass, lowPass; // own LR4 band
        // this node's band chain: AP of every other node's edges, [otherNode][side]
        std::array<std::array<TptSvf, 2>, kMaxFxNodesPerLayer> foreignAllPass;
    };

    void allocate(int node) noexcept;
    void updateCoefficients(int node) noexcept;

    double sampleRate = 48000.0;
    int glideSteps = 1, birthSteps = 1, presenceSteps = 1;
    bool anyAllocated = false;
    std::array<NodeEdges, kMaxFxNodesPerLayer> edges;
};
} // namespace hpeq::dsp
