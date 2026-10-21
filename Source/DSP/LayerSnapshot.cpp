#include "DSP/LayerSnapshot.h"

namespace hpeq::dsp
{
namespace
{
float param(const juce::NamedValueSet& set, const char* key, float fallback)
{
    const auto* value = set.getVarPointer(juce::Identifier(key));
    return value != nullptr ? static_cast<float>(static_cast<double>(*value)) : fallback;
}

FxSlotParams makeSlot(const FxSlotState& state)
{
    FxSlotParams slotParams;
    slotParams.type = state.type;
    slotParams.mix = state.mix;
    slotParams.active = state.enabled && isFxTypeProcessed(state.type);
    switch (state.type)
    {
        case FxType::Gain:
            slotParams.values[slot::kGainDb] = juce::jlimit(-60.0f, 24.0f, param(state.parameters, "gainDb", 0.0f));
            break;
        case FxType::Compressor:
            slotParams.values[slot::kThresholdDb] = juce::jlimit(-60.0f, 0.0f, param(state.parameters, "thresholdDb", -18.0f));
            slotParams.values[slot::kRatio] = juce::jlimit(1.0f, 100.0f, param(state.parameters, "ratio", 4.0f));
            slotParams.values[slot::kKneeDb] = juce::jlimit(0.0f, 24.0f, param(state.parameters, "kneeDb", 6.0f));
            slotParams.values[slot::kAttackMs] = juce::jlimit(0.05f, 500.0f, param(state.parameters, "attackMs", 10.0f));
            slotParams.values[slot::kReleaseMs] = juce::jlimit(1.0f, 5000.0f, param(state.parameters, "releaseMs", 120.0f));
            slotParams.values[slot::kMakeupDb] = juce::jlimit(-24.0f, 24.0f, param(state.parameters, "makeupDb", 0.0f));
            break;
        case FxType::Reverb:
            slotParams.values[slot::kPredelayMs] = juce::jlimit(0.0f, 250.0f, param(state.parameters, "predelayMs", 10.0f));
            slotParams.values[slot::kDecaySeconds] = juce::jlimit(0.1f, 20.0f, param(state.parameters, "decaySeconds", 1.8f));
            slotParams.values[slot::kHfRatio] = juce::jlimit(0.05f, 1.0f, param(state.parameters, "hfDecayRatio", 0.5f));
            slotParams.values[slot::kSize] = juce::jlimit(0.3f, 1.5f, param(state.parameters, "size", 1.0f));
            slotParams.values[slot::kWidth] = juce::jlimit(0.0f, 1.5f, param(state.parameters, "width", 1.0f));
            slotParams.values[slot::kLowCutHz] = juce::jlimit(10.0f, 1000.0f, param(state.parameters, "lowCutHz", 80.0f));
            break;
        case FxType::Limiter:
        case FxType::Delay:
        case FxType::TransientShaper:
        case FxType::Saturator:
        case FxType::MonstaFx:
        case FxType::StereoProcessor:
        case FxType::GateExpander:
            break;
    }
    return slotParams;
}
} // namespace

bool isEqShapeProcessed(EqShape) noexcept
{
    return true; // M3: every shape and slope of the model is implemented
}

bool isFxTypeProcessed(FxType type) noexcept
{
    return type == FxType::Gain || type == FxType::Compressor || type == FxType::Reverb; // M6 adds the rest
}

LayerSnapshot makeSnapshot(const LayerState& layer)
{
    LayerSnapshot snapshot;
    for (size_t i = 0; i < layer.eqNodes.size() && i < snapshot.eq.size(); ++i)
    {
        const auto& node = layer.eqNodes[i];
        auto& band = snapshot.eq[i];
        band.active = node.enabled && isEqShapeProcessed(node.shape);
        band.shape = node.shape;
        band.frequencyHz = static_cast<float>(node.frequencyHz);
        band.gainDb = static_cast<float>(node.gainDb);
        band.q = static_cast<float>(node.q);
        band.slopeDbPerOct = node.slopeDbPerOct;
        band.channel = node.channel;
    }
    for (size_t i = 0; i < layer.fxNodes.size() && i < snapshot.fx.size(); ++i)
    {
        const auto& node = layer.fxNodes[i];
        auto& fx = snapshot.fx[i];
        fx.active = node.enabled;
        fx.lowHz = static_cast<float>(node.lowHz);
        fx.highHz = static_cast<float>(node.highHz);
        fx.mix = node.mix;
        fx.channel = node.channel;
        fx.numSlots = static_cast<int>(std::min(node.chain.size(), fx.slots.size()));
        for (int s = 0; s < fx.numSlots; ++s)
            fx.slots[static_cast<size_t>(s)] = makeSlot(node.chain[static_cast<size_t>(s)]);
    }
    return snapshot;
}
} // namespace hpeq::dsp
