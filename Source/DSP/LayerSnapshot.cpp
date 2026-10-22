#include "DSP/LayerSnapshot.h"

#include <cmath>

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
            slotParams.values[slot::kCompDetector] = juce::jlimit(0.0f, 1.0f, std::round(param(state.parameters, "detector", 0.0f)));
            slotParams.values[slot::kCompCharacter] = juce::jlimit(0.0f, 2.0f, std::round(param(state.parameters, "character", 0.0f)));
            slotParams.values[slot::kCompAutoMakeup] = juce::jlimit(0.0f, 1.0f, std::round(param(state.parameters, "autoMakeup", 0.0f)));
            break;
        case FxType::Reverb:
            slotParams.values[slot::kPredelayMs] = juce::jlimit(0.0f, 500.0f, param(state.parameters, "predelayMs", 10.0f));
            slotParams.values[slot::kDecaySeconds] = juce::jlimit(0.1f, 30.0f, param(state.parameters, "decaySeconds", 1.8f));
            slotParams.values[slot::kHfRatio] = juce::jlimit(0.05f, 1.0f, param(state.parameters, "hfDecayRatio", 0.5f));
            slotParams.values[slot::kSize] = juce::jlimit(0.3f, 1.5f, param(state.parameters, "size", 1.0f));
            slotParams.values[slot::kWidth] = juce::jlimit(0.0f, 1.5f, param(state.parameters, "width", 1.0f));
            slotParams.values[slot::kLowCutHz] = juce::jlimit(10.0f, 1000.0f, param(state.parameters, "lowCutHz", 80.0f));
            // Projects saved before the HamloVerb pass have no "space": they keep the M4 reverb (Classic).
            slotParams.values[slot::kRevSpace] = juce::jlimit(0.0f, 6.0f, std::round(param(state.parameters, "space", 0.0f)));
            slotParams.values[slot::kRevColor] = juce::jlimit(0.0f, 2.0f, std::round(param(state.parameters, "color", 2.0f)));
            slotParams.values[slot::kRevSync] = juce::jlimit(0.0f, 11.0f, std::round(param(state.parameters, "predelaySync", 0.0f)));
            slotParams.values[slot::kRevAttack] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "attack", 0.2f));
            slotParams.values[slot::kRevEarlyDiff] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "earlyDiffusion", 0.7f));
            slotParams.values[slot::kRevLateDiff] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "lateDiffusion", 0.7f));
            slotParams.values[slot::kRevModRate] = juce::jlimit(0.01f, 10.0f, param(state.parameters, "modRate", 0.6f));
            slotParams.values[slot::kRevModDepth] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "modDepth", 0.4f));
            slotParams.values[slot::kRevBassMult] = juce::jlimit(0.25f, 4.0f, param(state.parameters, "bassMult", 1.2f));
            slotParams.values[slot::kRevBassXover] = juce::jlimit(50.0f, 2000.0f, param(state.parameters, "bassXoverHz", 300.0f));
            slotParams.values[slot::kRevHighXover] = juce::jlimit(1000.0f, 16000.0f, param(state.parameters, "highXoverHz", 4000.0f));
            slotParams.values[slot::kRevHighCut] = juce::jlimit(1000.0f, 20000.0f, param(state.parameters, "highCutHz", 12000.0f));
            slotParams.values[slot::kRevDistance] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "distance", 0.4f));
            slotParams.values[slot::kRevDuck] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "duck", 0.0f));
            slotParams.values[slot::kRevFreeze] = juce::jlimit(0.0f, 1.0f, std::round(param(state.parameters, "freeze", 0.0f)));
            break;
        case FxType::Saturator:
            slotParams.values[slot::kDriveDb] = juce::jlimit(0.0f, 48.0f, param(state.parameters, "driveDb", 12.0f));
            slotParams.values[slot::kCurve] = juce::jlimit(0.0f, 2.0f, param(state.parameters, "curve", 0.0f));
            slotParams.values[slot::kBias] = juce::jlimit(-0.9f, 0.9f, param(state.parameters, "bias", 0.0f));
            slotParams.values[slot::kTrimDb] = juce::jlimit(-24.0f, 24.0f, param(state.parameters, "trimDb", 0.0f));
            slotParams.values[slot::kOversampling] = juce::jlimit(0.0f, 2.0f, param(state.parameters, "oversampling", 1.0f));
            slotParams.values[slot::kAutoGain] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "autoGain", 1.0f));
            break;
        case FxType::TransientShaper:
            slotParams.values[slot::kAttackGainDb] = juce::jlimit(-24.0f, 24.0f, param(state.parameters, "attackDb", 0.0f));
            slotParams.values[slot::kSustainGainDb] = juce::jlimit(-24.0f, 24.0f, param(state.parameters, "sustainDb", 0.0f));
            slotParams.values[slot::kTransientSpeed] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "speed", 0.5f));
            break;
        case FxType::Delay:
            slotParams.values[slot::kDelayTimeMs] = juce::jlimit(1.0f, 4000.0f, param(state.parameters, "timeMs", 250.0f));
            slotParams.values[slot::kDelaySync] = juce::jlimit(0.0f, 11.0f, param(state.parameters, "sync", 8.0f));
            slotParams.values[slot::kFeedback] = juce::jlimit(0.0f, 0.98f, param(state.parameters, "feedback", 0.35f));
            slotParams.values[slot::kPingPong] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "pingPong", 0.0f));
            slotParams.values[slot::kDampingHz] = juce::jlimit(500.0f, 20000.0f, param(state.parameters, "dampingHz", 8000.0f));
            slotParams.values[slot::kDelayLowCutHz] = juce::jlimit(10.0f, 2000.0f, param(state.parameters, "lowCutHz", 120.0f));
            slotParams.values[slot::kDelayWidth] = juce::jlimit(0.0f, 1.5f, param(state.parameters, "width", 1.0f));
            // Projects saved before the HamloEcho pass have no "character": they keep the M6 delay (Classic).
            slotParams.values[slot::kDlyCharacter] = juce::jlimit(0.0f, 4.0f, std::round(param(state.parameters, "character", 0.0f)));
            slotParams.values[slot::kDlyTimeRMs] = juce::jlimit(1.0f, 4000.0f, param(state.parameters, "timeRMs", 375.0f));
            slotParams.values[slot::kDlySyncR] = juce::jlimit(0.0f, 11.0f, std::round(param(state.parameters, "syncR", 6.0f)));
            slotParams.values[slot::kDlyLink] = juce::jlimit(0.0f, 1.0f, std::round(param(state.parameters, "link", 1.0f)));
            slotParams.values[slot::kDlyCross] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "cross", 0.0f));
            slotParams.values[slot::kDlyDriveDb] = juce::jlimit(0.0f, 24.0f, param(state.parameters, "driveDb", 3.0f));
            slotParams.values[slot::kDlyModRate] = juce::jlimit(0.05f, 10.0f, param(state.parameters, "modRate", 0.5f));
            slotParams.values[slot::kDlyModDepth] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "modDepth", 0.3f));
            slotParams.values[slot::kDlyDiffusion] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "diffusion", 0.0f));
            slotParams.values[slot::kDlyPitch] = juce::jlimit(-12.0f, 12.0f, param(state.parameters, "pitch", 0.0f));
            slotParams.values[slot::kDlyLowRes] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "lowRes", 0.0f));
            slotParams.values[slot::kDlyHighRes] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "highRes", 0.0f));
            slotParams.values[slot::kDlyBandHz] = juce::jlimit(100.0f, 10000.0f, param(state.parameters, "bandHz", 1000.0f));
            slotParams.values[slot::kDlyBandGainDb] = juce::jlimit(-18.0f, 12.0f, param(state.parameters, "bandGainDb", 0.0f));
            slotParams.values[slot::kDlyBandQ] = juce::jlimit(0.3f, 6.0f, param(state.parameters, "bandQ", 1.0f));
            slotParams.values[slot::kDlyDuck] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "duck", 0.0f));
            slotParams.values[slot::kDlyFreeze] = juce::jlimit(0.0f, 1.0f, std::round(param(state.parameters, "freeze", 0.0f)));
            slotParams.values[slot::kDlyLofi] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "lofi", 0.0f));
            slotParams.values[slot::kDlyTimeMode] = juce::jlimit(0.0f, 1.0f, std::round(param(state.parameters, "timeMode", 0.0f)));
            break;
        case FxType::MonstaFx:
            slotParams.values[slot::kDivision] = juce::jlimit(0.0f, 3.0f, param(state.parameters, "division", 2.0f));
            slotParams.values[slot::kChance] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "chance", 0.35f));
            slotParams.values[slot::kSeed] = juce::jlimit(0.0f, 9999.0f, std::floor(param(state.parameters, "seed", 1.0f)));
            slotParams.values[slot::kCrushBits] = juce::jlimit(0.0f, 16.0f, param(state.parameters, "crushBits", 0.0f));
            slotParams.values[slot::kGateDepth] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "gateDepth", 0.0f));
            slotParams.values[slot::kReverseShare] = juce::jlimit(0.0f, 1.0f, param(state.parameters, "reverseShare", 0.3f));
            break;
        case FxType::StereoProcessor:
            slotParams.values[slot::kStereoWidth] = juce::jlimit(0.0f, 2.0f, param(state.parameters, "width", 1.0f));
            slotParams.values[slot::kStereoBalance] = juce::jlimit(-1.0f, 1.0f, param(state.parameters, "balance", 0.0f));
            break;
        case FxType::GateExpander:
            slotParams.values[slot::kGateThresholdDb] = juce::jlimit(-90.0f, 0.0f, param(state.parameters, "thresholdDb", -40.0f));
            slotParams.values[slot::kGateRatio] = juce::jlimit(1.0f, 100.0f, param(state.parameters, "ratio", 4.0f));
            slotParams.values[slot::kGateRangeDb] = juce::jlimit(0.0f, 90.0f, param(state.parameters, "rangeDb", 40.0f));
            slotParams.values[slot::kGateAttackMs] = juce::jlimit(0.05f, 100.0f, param(state.parameters, "attackMs", 0.5f));
            slotParams.values[slot::kGateHoldMs] = juce::jlimit(0.0f, 500.0f, param(state.parameters, "holdMs", 20.0f));
            slotParams.values[slot::kGateReleaseMs] = juce::jlimit(1.0f, 2000.0f, param(state.parameters, "releaseMs", 100.0f));
            slotParams.values[slot::kGateHysteresisDb] = juce::jlimit(0.0f, 24.0f, param(state.parameters, "hysteresisDb", 4.0f));
            break;
        case FxType::Limiter:
            slotParams.values[slot::kCeilingDb] = juce::jlimit(-60.0f, 0.0f, param(state.parameters, "ceilingDb", -1.0f));
            slotParams.values[slot::kLimiterReleaseMs] = juce::jlimit(1.0f, 2000.0f, param(state.parameters, "releaseMs", 80.0f));
            slotParams.values[slot::kLookaheadMs] = juce::jlimit(0.1f, 10.0f, param(state.parameters, "lookaheadMs", 2.0f));
            break;
    }
    return slotParams;
}
} // namespace

bool isEqShapeProcessed(EqShape) noexcept
{
    return true; // M3: every shape and slope of the model is implemented
}

bool isFxTypeProcessed(FxType) noexcept
{
    return true; // M6: every FX type of the model is implemented
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
    // D6: FX regions and EQ points with modules go to their stable engine nodes. The slots of a
    // sanitized project are kept; an unsanitized layer gets the same assignment on a copy.
    auto slotted = layer;
    assignEngineSlots(slotted);
    const auto fill = [](FxNodeParams& fx, bool active, double lowHz, double highHz, float mix, ChannelMode channel,
                         const std::vector<FxSlotState>& chain)
    {
        fx.active = active;
        fx.lowHz = static_cast<float>(lowHz);
        fx.highHz = static_cast<float>(highHz);
        fx.mix = mix;
        fx.channel = channel;
        fx.numSlots = static_cast<int>(std::min(chain.size(), fx.slots.size()));
        for (int s = 0; s < fx.numSlots; ++s)
            fx.slots[static_cast<size_t>(s)] = makeSlot(chain[static_cast<size_t>(s)]);
    };
    for (const auto& node : slotted.fxNodes)
        if (node.engineSlot >= 0)
            fill(snapshot.fx[static_cast<size_t>(node.engineSlot)], node.enabled, node.lowHz, node.highHz, node.mix, node.channel, node.chain);
    for (const auto& node : slotted.eqNodes)
        if (node.engineSlot >= 0)
        {
            const auto [low, high] = pointModuleBandHz(node);
            fill(snapshot.fx[static_cast<size_t>(node.engineSlot)], node.enabled, low, high, 1.0f, node.channel, node.chain);
        }
    return snapshot;
}
} // namespace hpeq::dsp
