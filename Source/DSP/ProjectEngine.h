#pragma once

#include <array>
#include <memory>
#include <vector>

#include "Core/FxProjectState.h"
#include "DSP/DelayLine.h"
#include "DSP/LayerEngine.h"
#include "DSP/Ramp.h"
#include "Rack/NativeRegistry.h"

namespace hpeq::dsp
{
// Layer controls as the audio thread sees them (made on the message thread from LayerState).
struct LayerControlParams
{
    bool enabled = true;          // false: the layer is not processed (after its weight has faded out)
    float weight = 1.0f;          // mix * heard; heard = !mute && (no solo anywhere || solo)
    float gain = 1.0f;            // linear
    float pan = 0.0f;             // -1..1, linear balance
    float width = 1.0f;           // 0..2, M/S
    ChannelMode channel = ChannelMode::Stereo; // Stereo / Left / Right / Mid / Side (Linked/Dual = Stereo)
    bool identityControls = true; // gain 1, pan 0, width 1, Stereo
};

struct ProjectSnapshot
{
    std::uint64_t nativeGeneration=0;
    bool wetOnly=false;
    double outputTrim=1,inputTrim=1;
    LayerRouting routing = LayerRouting::Parallel;
    std::array<LayerControlParams, kNumLayers> layers {};
};
static_assert(std::is_trivially_copyable_v<ProjectSnapshot>);

ProjectSnapshot makeProjectSnapshot(const FxProjectState&);

// Ten layers (01 §8, DECISIONS D5). L_j{.} = a full layer (PRE-EQ -> FX nodes, LayerEngine) followed
// by its controls (channel component, gain, width, pan). w_j = the smoothed layer weight.
//
// Let A_j be layer j's crossover-only all-pass and C the product of live A_j.
//   Parallel: y = D_L(C{x}) + sum_j w_j (D_{L-L_j}(C_except_j{L_j{x}}) - D_L(C{x})), L = max L_j
//   Serial: x_{j+1} = D_{L_j}(A_j{x_j}) + w_j (L_j{x_j} - D_{L_j}(A_j{x_j})); y = x_10,
//                             L = sum_j L_j
//
// Latency: L_j counts whether the layer is enabled, muted or not (like node latency), so toggling a
// layer never changes the reported latency; a disabled layer in Serial is a pure delay D_{L_j}.
// A layer is skipped (no DSP) when it is disabled and faded out, or when it is the identity
// (neutral PRE-EQ, no running node, identity controls): then its delta is exactly 0, so ten empty or
// bypassed layers give the input bit-exactly. Mute / Solo only change w_j: the layer keeps running,
// including its phase in the shared reference, so a neutral wet branch cannot comb with dry.
// so un-muting is seamless. Re-enabling a disabled layer resets it first (its tails were cut when it
// was disabled). Channel-mode changes fade the layer out, switch, and fade it back in.
// Pooled processors (Reverb / Delay / Monsta) are shared by all layers of the project.
class ProjectEngine
{
public:
    static int maxLayerLatency(double sampleRate) { return 2*FxNodeProcessor::maxNodeLatency(sampleRate); } // frequency nodes, then whole-layer chain
    static int maxLatency(double sampleRate) { return maxLayerLatency(sampleRate) * kNumLayers; }

    ProjectEngine();
    void prepare(double sampleRate, int maxBlockSize,int channels=2);
    void reset() noexcept;

    // Message thread. Publishes every layer and the routing/controls.
    bool publish(const FxProjectState&) noexcept;
    const juce::String& publicationError() const noexcept { return lastError; }
    double nativeTailSeconds() const noexcept { return nativeRegistry->tailSeconds(); }
    double liveNativeTailSeconds() const noexcept { return liveNativeTail.load(std::memory_order_relaxed); }
    native::Runtime* nativeRuntime(int layer,int node,int slot) const noexcept {return nativeRegistry->find(layer,node,slot);}
    std::shared_ptr<native::Runtime> shareNativeRuntime(int layer,int node,int slot) const noexcept {return nativeRegistry->share(layer,node,slot);}
    native::Runtime* nativeRightRuntime(int layer,int node,int slot) const noexcept {return nativeRegistry->findRight(layer,node,slot);}

    void process(float* const* channels, int numChannels, int numSamples, const Transport& = {}) noexcept;

    int getLatencySamples() const noexcept { return latency; }
    static int latencyFor(const FxProjectState&, double sampleRate);

    const LayerEngine& getLayer(int index) const noexcept { return *layers[static_cast<size_t>(index)].engine; }
    bool isLayerProcessing(int index) const noexcept { return layers[static_cast<size_t>(index)].processedLastBlock; }
    int layersProcessedLastBlock() const noexcept;
    const LayerPools& getPools() const noexcept { return *pools; }

private:
    struct Layer
    {
        std::unique_ptr<LayerEngine> engine;
        LayerControlParams params;
        ChannelMode mode = ChannelMode::Stereo;
        bool modePending = false;
        bool live = false;      // engine has been running (false after a disable: reset before reuse)
        bool processedLastBlock = false;
        bool phaseRetiring=false;
        LinearRamp weight, gain, pan, width;
        AlignmentDelay outAlign; // Parallel: D_{L-L_j}; Serial: D_{L_j} of the layer input
        AlignmentDelay restAlign; // the component the channel mode leaves untouched
        CrossoverTree::PhasePath restPhase,referencePhase;
        std::array<CrossoverTree::PhasePath,kNumLayers> branchPhase;
        std::array<std::vector<float>,kMaxChannels> rendered;
    };

    bool layerNeedsDsp(Layer&) const noexcept;
    // Runs layer `layer` on `io` in place (L_j{.}): channel component, engine, controls.
    void runLayer(Layer&, float* const* io, int numChannels, int numSamples, const Transport&, int blockOffset) noexcept;
    void applySnapshot() noexcept;

    double sampleRate = 48000.0;
    int maxBlock = 0, rampSteps = 1;
    int streamChannels=2;
    std::atomic<double> liveNativeTail{0};
    int latency = 0, latencyCap = 1;
    bool freshStart = true; // first block after reset(): controls land on their targets at once
    SnapshotExchange<ProjectSnapshot> exchange;
    LayerRouting routing = LayerRouting::Parallel;
    std::unique_ptr<LayerPools> pools;
    std::unique_ptr<native::Registry> nativeRegistry; // outlives the layer engines and their runtime references
    std::array<Layer, kNumLayers> layers;
    FxProjectState lastPublished;
    juce::String lastError;
    AlignmentDelay inputAlign; // Parallel: D_L(x)
    AlignmentDelay wetReference;
    LinearRamp wetRemoval, outputTrim, inputTrim;
    std::array<std::vector<float>, kMaxChannels> wetDry;

    std::array<std::vector<float>, kMaxChannels> dryAligned, work, current, component, rest;
    std::array<CrossoverTree::PhasePath,kNumLayers> commonPhase;
};
} // namespace hpeq::dsp
