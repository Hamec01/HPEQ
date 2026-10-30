#include "Standalone/ModuleVariant.h"

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

ModuleSpec makeVerb()
{
    namespace s = dsp::slot;
    return { ModuleKind::Verb, FxType::Reverb, "HamloVerb", 1180, 760, 18,
             { knob("predelayMs", "Pre-delay", 0, 500, 20, s::kPredelayMs, 2.3f, "ms"),
               knob("size", "Size", 0.3f, 1.5f, 1, s::kSize),
               knob("decaySeconds", "Decay", 0.1f, 30, 2.4f, s::kDecaySeconds, 0.35f, "s"),
               knob("attack", "Attack", 0, 1, 0.2f, s::kRevAttack),
               knob("distance", "Distance", 0, 1, 0.4f, s::kRevDistance),
               knob("width", "Width", 0, 1.5f, 1, s::kWidth),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("earlyDiffusion", "Early Diffusion", 0, 1, 0.7f, s::kRevEarlyDiff),
               knob("lateDiffusion", "Late Diffusion", 0, 1, 0.7f, s::kRevLateDiff),
               knob("modRate", "Mod Rate", 0.01f, 10, 0.6f, s::kRevModRate, 0.3f, "Hz"),
               knob("modDepth", "Mod Depth", 0, 1, 0.4f, s::kRevModDepth),
               knob("bassMult", "Bass Mult", 0.25f, 4, 1.2f, s::kRevBassMult, 0.5f, "x"),
               knob("bassXoverHz", "Bass Xover", 50, 2000, 300, s::kRevBassXover, 0.38f, "Hz"),
               knob("hfDecayRatio", "High Mult", 0.05f, 1, 0.5f, s::kHfRatio, 0.44f, "x"),
               knob("highXoverHz", "High Xover", 1000, 16000, 4000, s::kRevHighXover, 0.5f, "Hz"),
               knob("lowCutHz", "Low Cut", 10, 1000, 80, s::kLowCutHz, 0.5f, "Hz"),
               knob("highCutHz", "High Cut", 1000, 20000, 12000, s::kRevHighCut, 0.5f, "Hz"),
               knob("duck", "Duck", 0, 1, 0, s::kRevDuck),
               choice("space", "Space", 3, s::kRevSpace, { "Classic", "Room", "Chamber", "Hall", "Plate", "Cathedral", "Ambience" }),
               choice("color", "Color", 2, s::kRevColor, { "1970", "1980", "Now" }),
               choice("predelaySync", "Pre-delay Sync", 0, s::kRevSync,
                      { "Free", "1/64", "1/32", "1/16T", "1/16", "1/8T", "1/16D", "1/8", "1/4T", "1/8D", "1/4", "1/2" }),
               choice("freeze", "Freeze", 0, s::kRevFreeze, { "Off", "On" }) } };
}

} // namespace

const ModuleSpec& moduleSpec(ModuleKind kind)
{
    jassert(kind == ModuleKind::Verb);
    juce::ignoreUnused(kind);
    static const ModuleSpec verb = makeVerb();
    return verb;
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
