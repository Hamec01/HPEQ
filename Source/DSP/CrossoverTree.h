#pragma once

#include <array>
#include <vector>

#include "Core/FxProjectState.h"
#include "DSP/Biquad.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// H3 frequency core (DECISIONS D4), phase-coherent, order-free, O(N) in the number of nodes.
//
// Every running FX node j owns two edges, lo_j and hi_j; AP_j = AP(lo_j) AP(hi_j) is their
// all-pass pair (2nd-order, Q = 1/sqrt 2: the sum of an LR4 low/high pair). Nodes are walked in
// index order (only allocated ones):
//
//     prefix_0 = x,   prefix_{j+1} = AP_j(prefix_j),   dry = prefix_N = prod AP_j (x)
//     band'_j  = LP4_{hi_j}( HP4_{lo_j}( prefix_j ) )               (node j processes this)
//     out      = dry + sum_j S_j(d_j),  S_j = prod_{k > j} AP_k,  d_j = node j's delta
//
// and the sum is evaluated by Horner's rule with one all-pass pair per node:
//     acc = d_first;  acc = AP_j(acc) + d_j  for every later node j.
//
// For linear processing this is exactly the M2 form (band_j = S_j band'_j is node j's LR4 band
// times the all-passes of every other edge): Linkwitz-Riley outputs are in phase with their
// all-pass sum, so band_j / dry = |HP4_lo| |LP4_hi| is real and a cut never adds energy. For a
// non-linear processor the delta is computed on band'_j and then all-passed by S_j, which keeps it
// phase-coherent with the dry; only the detector sees band'_j instead of S_j band'_j (the same band,
// different phase). Cost per channel: 2N + 4N + 2N sections instead of 2N + 4N + 2N(N-1).
//
// Edges are born and retired at kParkingHz, where an all-pass is numerically almost an identity
// (its zero state is its steady state), and glide to/from their targets: no crossfades. With no
// allocated edge the tree is not run and the layer is a bit-exact pass-through.
class CrossoverTree
{
public:
    static constexpr double kParkingHz = 1.0;
    static constexpr double kBirthSeconds = 0.060;

    void prepare(double sampleRate, int maxBlockSize);
    void reset() noexcept;

    // Called once per host block (control decisions only happen at block starts).
    void update(const std::array<bool, kMaxFxNodesPerLayer>& running,
                const std::array<double, kMaxFxNodesPerLayer>& lowHz,
                const std::array<double, kMaxFxNodesPerLayer>& highHz) noexcept;

    bool isActive() const noexcept { return anyAllocated; }
    bool hasBand(int node) const noexcept { return edges[static_cast<size_t>(node)].allocated; }

    // Split: dry[ch][i], band'[j][ch][i] and presence[j][i] for every allocated node j.
    void process(const float* const* in, int numChannels, int numSamples, int controlPhase,
                 float* const* dry, float* const* const* bands, float* const* presence) noexcept;

    // Recombine: out[ch][i] += sum_j S_j(deltas[j][ch][i]) for every allocated node (Horner).
    void combine(const float* const* const* deltas, float* const* out, int numChannels, int numSamples) noexcept;

private:
    struct NodeEdges
    {
        bool allocated = false, parking = false, birth = false;
        LinearRamp logLow, logHigh;
        LinearRamp presence; // 0 while the edges glide in, then 1 over kMixRampSeconds
        std::array<TptSvf, 2> prefixAllPass;     // AP(lo), AP(hi) in the split chain
        std::array<TptSvf, 2> highPass, lowPass; // own LR4 band
        std::array<TptSvf, 2> hornerAllPass;     // AP(lo), AP(hi) in the recombination chain
        // Coefficient updates made by process() at control ticks, replayed by combine() at the same
        // sample positions, so the recombination chain never runs ahead of the split chain.
        struct Retune { int sample; double lowHz, highHz; };
        std::vector<Retune> retunes;
        int retuneCount = 0;
    };

    void allocate(int node) noexcept;
    void updateCoefficients(int node, bool interpolate) noexcept;

    double sampleRate = 48000.0;
    int glideSteps = 1, birthSteps = 1, presenceSteps = 1;
    bool anyAllocated = false;
    std::array<NodeEdges, kMaxFxNodesPerLayer> edges;
};
} // namespace hpeq::dsp
