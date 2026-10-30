#include "Standalone/ModuleVariant.h"

#include <cmath>


namespace hpeq::standalone
{
namespace
{
using P = ParameterSpec;

P knob(const char* id, const char* name, float lo, float hi, float def, int target,
       float skew = 1.0f, const char* unit = "", ValueTransform transform = ValueTransform::Direct)
{
    P p;
    p.id = id;
    p.name = name;
    p.minimum = lo;
    p.maximum = hi;
    p.defaultValue = def;
    p.skew = skew;
    p.unit = unit;
    p.slotValueIndex = target;
    p.transform = transform;
    return p;
}

P choice(const char* id, const char* name, int def, int target, std::initializer_list<const char*> labels)
{
    P p;
    p.id = id;
    p.name = name;
    p.minimum = 0.0f;
    p.maximum = static_cast<float>(labels.size() - 1);
    p.defaultValue = static_cast<float>(def);
    p.interval = 1.0f;
    p.slotValueIndex = target;
    for (const auto* label : labels)
        p.choices.add(label);
    return p;
}

ModuleSpec makeEcho()
{
    namespace s = dsp::slot;
    return { ModuleKind::Echo, FxType::Delay, "HamloEcho", 1180, 760, 20,
             { knob("timeMs", "Time L", 1, 4000, 250, s::kDelayTimeMs, 0.32f, "ms"),
               knob("timeRMs", "Time R", 1, 4000, 375, s::kDlyTimeRMs, 0.32f, "ms"),
               knob("feedback", "Feedback", 0, 0.98f, 0.35f, s::kFeedback),
               knob("cross", "Cross", 0, 1, 0, s::kDlyCross),
               knob("driveDb", "Drive", 0, 24, 3, s::kDlyDriveDb, 1, "dB"),
               knob("width", "Width", 0, 1.5f, 1, s::kDelayWidth),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("modRate", "Mod Rate", 0.05f, 10, 0.5f, s::kDlyModRate, 0.36f, "Hz"),
               knob("modDepth", "Mod Depth", 0, 1, 0.3f, s::kDlyModDepth),
               knob("diffusion", "Diffusion", 0, 1, 0, s::kDlyDiffusion),
               knob("pitch", "Pitch", -12, 12, 0, s::kDlyPitch, 1, "st"),
               knob("lofi", "Lo-Fi", 0, 1, 0, s::kDlyLofi),
               knob("lowCutHz", "Low Cut", 10, 2000, 120, s::kDelayLowCutHz, 0.43f, "Hz"),
               knob("dampingHz", "High Cut", 500, 20000, 8000, s::kDampingHz, 0.5f, "Hz"),
               knob("bandHz", "Band Frequency", 100, 10000, 1000, s::kDlyBandHz, 0.5f, "Hz"),
               knob("bandGainDb", "Band Gain", -18, 12, 0, s::kDlyBandGainDb, 1, "dB"),
               knob("bandQ", "Band Q", 0.3f, 6, 1, s::kDlyBandQ, 0.5f),
               knob("duck", "Duck", 0, 1, 0, s::kDlyDuck),
               knob("lowRes", "Low Resonance", 0, 1, 0, s::kDlyLowRes),
               knob("highRes", "High Resonance", 0, 1, 0, s::kDlyHighRes),
               choice("character", "Character", 2, s::kDlyCharacter, { "Classic", "Clean", "Tape", "BBD", "Lo-Fi" }),
               choice("timeMode", "Time Mode", 0, s::kDlyTimeMode, { "Tape", "Fade" }),
               choice("sync", "Sync L", 8, s::kDelaySync,
                      { "Free", "1/32", "1/16", "1/16 D", "1/8 T", "1/8", "1/8 D", "1/4 T", "1/4", "1/4 D", "1/2", "1/1" }),
               choice("syncR", "Sync R", 6, s::kDlySyncR,
                      { "Free", "1/32", "1/16", "1/16 D", "1/8 T", "1/8", "1/8 D", "1/4 T", "1/4", "1/4 D", "1/2", "1/1" }),
               choice("link", "Link", 1, s::kDlyLink, { "Off", "On" }),
               choice("pingPong", "Ping-pong", 0, s::kPingPong, { "Off", "On" }),
               choice("freeze", "Freeze", 0, s::kDlyFreeze, { "Off", "On" }) } };
}

} // namespace

const ModuleSpec& moduleSpec(ModuleKind)
{
    static const ModuleSpec echo = makeEcho();
    return echo;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(ModuleKind kind)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& p : moduleSpec(kind).parameters)
    {
        const juce::ParameterID id { p.id, 1 };
        if (!p.choices.isEmpty())
        {
            layout.add(std::make_unique<juce::AudioParameterChoice>(id, p.name, p.choices,
                                                                    juce::roundToInt(p.defaultValue)));
            continue;
        }
        juce::NormalisableRange<float> range(p.minimum, p.maximum, p.interval, p.skew);
        auto attributes = juce::AudioParameterFloatAttributes {}.withLabel(p.unit);
        layout.add(std::make_unique<juce::AudioParameterFloat>(id, p.name, range, p.defaultValue, attributes));
    }
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}

dsp::FxSlotParams makeSlot(ModuleKind kind, const std::vector<std::atomic<float>*>& values, bool active) noexcept
{
    const auto& spec = moduleSpec(kind);
    dsp::FxSlotParams slot;
    slot.active = active;
    slot.type = spec.type;
    slot.mix = 1.0f;
    const auto count = juce::jmin(values.size(), spec.parameters.size());
    for (size_t i = 0; i < count; ++i)
    {
        const auto& p = spec.parameters[i];
        const float value = values[i]->load(std::memory_order_relaxed);
        if (p.slotValueIndex < 0)
            slot.mix = juce::jlimit(0.0f, 1.0f, value);
        else
            slot.values[static_cast<size_t>(p.slotValueIndex)] = value;
    }
    return slot;
}

int maximumLatencySamples(ModuleKind, double) noexcept { return 0; }

} // namespace hpeq::standalone
