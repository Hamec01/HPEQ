#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <juce_data_structures/juce_data_structures.h>

namespace hpeq
{
// Project model of one HPEQ instance (03_AGENT_TASK.md "Два типа объектов", DECISIONS D2).
// Pure data: no DSP, no UI, no HPDG types. Edited on the message thread; the audio thread will get
// immutable snapshots of it from M2 on, never this object.

constexpr int kStateSchemaVersion = 1;
constexpr int kNumLayers = 10;
constexpr int kMaxEqNodesPerLayer = 24;  // 01 §3 starting budget, to be refined by measurement
constexpr int kMaxFxNodesPerLayer = 8;
constexpr int kMaxFxPerNode = 6;         // D2: chain of up to 6 processors per FX node

constexpr double kMinFrequencyHz = 10.0;
constexpr double kMaxFrequencyHz = 40000.0; // clamped further against Nyquist when DSP is prepared

enum class ChannelMode { Stereo, Left, Right, Mid, Side, LinkedStereo, DualMono };
enum class EqShape { Bell, LowShelf, HighShelf, LowCut, HighCut, Notch, BandPass, Tilt, AllPass };
// Gain is a utility/reference processor (03 M2.1 "mock processor Gain -6 dB").
enum class FxType { Compressor, Limiter, Reverb, Delay, TransientShaper, Saturator, MonstaFx, StereoProcessor, GateExpander, Gain };
enum class LayerRouting { Parallel, Serial };

struct FxSlotState
{
    FxType type = FxType::Compressor;
    bool enabled = true;
    float mix = 1.0f;
    juce::NamedValueSet parameters; // processor-specific, defined with each processor
};

struct EqNodeState
{
    std::uint32_t id = 0;
    bool enabled = true;
    EqShape shape = EqShape::Bell;
    double frequencyHz = 1000.0;
    double gainDb = 0.0;
    double q = 0.707;
    int slopeDbPerOct = 12; // cuts only; the DSP decides which slopes it really supports
    ChannelMode channel = ChannelMode::Stereo;
    // D6: optional modules on the point's band (after PRE-EQ), 0..kMaxFxPerNode, processed in order.
    std::vector<FxSlotState> chain;
    int engineSlot = -1; // stable engine node 0..7 while the chain is not empty (assignEngineSlots)
};


struct FxNodeState
{
    std::uint32_t id = 0;
    bool enabled = true;
    bool solo = false;
    double lowHz = 250.0;
    double highHz = 700.0;
    float mix = 1.0f;
    ChannelMode channel = ChannelMode::Stereo;
    std::vector<FxSlotState> chain { FxSlotState{} }; // 1..kMaxFxPerNode, processed in order
    int engineSlot = -1; // stable engine node 0..7 (assignEngineSlots)
};

struct LayerState
{
    juce::String name;
    bool enabled = true;
    bool mute = false;
    bool solo = false;
    float gainDb = 0.0f;
    float mix = 1.0f;
    float pan = 0.0f;   // -1..1
    float width = 1.0f; // 0..2
    ChannelMode channel = ChannelMode::Stereo;
    std::vector<EqNodeState> eqNodes;
    std::vector<FxNodeState> fxNodes;
    // D7: modules on the whole layer, after the frequency nodes and before the layer controls (0..6).
    std::vector<FxSlotState> mixChain;
};

struct FxProjectState
{
    LayerRouting routing = LayerRouting::Parallel;
    int selectedLayer = 0;
    std::uint32_t nextNodeId = 1;
    std::array<LayerState, kNumLayers> layers = makeDefaultLayers();

    static std::array<LayerState, kNumLayers> makeDefaultLayers();
};

// The layer the UI shows and edits.
inline LayerState& selectedLayer(FxProjectState& p) { return p.layers[static_cast<size_t>(juce::jlimit(0, kNumLayers - 1, p.selectedLayer))]; }
inline const LayerState& selectedLayer(const FxProjectState& p) { return p.layers[static_cast<size_t>(juce::jlimit(0, kNumLayers - 1, p.selectedLayer))]; }

// Copies `source` (nodes, EQ, controls, name) into layer `targetIndex`, giving every node a fresh id
// from nextNodeId so ids stay unique in the project.
void copyLayerInto(FxProjectState&, const LayerState& source, int targetIndex);
// A module as the UI creates it. Compressor (module pass): threshold -30 dB, ratio 4, attack 1 ms,
// release 80 ms, Punch, auto makeup on; the DSP's own defaults (used when a key is absent, e.g. in
// older projects) are unchanged. Other types: no explicit parameters.
FxSlotState defaultModule(FxType);

// D6: band of an EQ point's modules, the edges of a band-pass with the point's frequency and Q.
std::pair<double, double> pointModuleBandHz(const EqNodeState&);
// D6: gives every FX region and every EQ point with modules a stable engine node (0..7): valid unique
// slots are kept, the rest get the lowest free slot (regions first, then points, in list order); points
// without modules get -1. Points that do not fit lose their modules (the UI prevents that case).
void assignEngineSlots(LayerState&);
// Free engine nodes of a layer (after assignEngineSlots semantics).
int freeEngineSlots(const LayerState&);

// True when the layer has anything that changes the sound (nodes or non-neutral controls).
bool layerHasContent(const LayerState&);

// Clamps every value into its documented range and enforces the count limits. Idempotent.
void sanitize(FxProjectState&);

juce::ValueTree toValueTree(const FxProjectState&);

// Rejects trees without schemaVersion or with a version this build does not know; `error` says why.
std::optional<FxProjectState> fromValueTree(const juce::ValueTree&, juce::String* error = nullptr);

juce::String toString(ChannelMode);
juce::String toString(EqShape);
juce::String toString(FxType);
juce::String toString(LayerRouting);
} // namespace hpeq
