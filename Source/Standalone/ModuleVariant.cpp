#include "Standalone/ModuleVariant.h"
#include "DSP/LegacyGateProcessor.h"
#include <cmath>
namespace hpeq::standalone
{
namespace
{
using P=ParameterSpec;
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

ModuleSpec makeZastava()
{
    namespace s = dsp::slot;
    return { ModuleKind::Zastava, FxType::GateExpander, "HamloZastava", 1180, 780, 12,
             { knob("thresholdDb", "Threshold", -90, 0, -40, s::kGateThresholdDb, 1, "dB"),
               knob("rangeDb", "Range", 0, 90, 40, s::kGateRangeDb, 1, "dB"),
               knob("attackMs", "Attack", 0.05f, 100, 0.5f, s::kGateAttackMs, 0.35f, "ms"),
               knob("holdMs", "Hold", 0, 500, 20, s::kGateHoldMs, 1, "ms"),
               knob("releaseMs", "Release", 1, 2000, 100, s::kGateReleaseMs, 0.4f, "ms"),
               choice("style", "Mode", 0, s::kGateStyle, { "ZASTAVA", "PATRUL", "TREVOGA" }),
               knob("ratio", "Ratio", 1, 100, 4, s::kGateRatio, 0.5f, ":1"),
               knob("hysteresisDb", "Hysteresis", 0, 24, 4, s::kGateHysteresisDb, 1, "dB"),
               knob("keyHpHz", "Key HP", 20, 10000, 20, s::kGateKeyHpHz, 0.5f, "Hz", ValueTransform::HighPassOff),
               knob("keyLpHz", "Key LP", 200, 20000, 20000, s::kGateKeyLpHz, 0.5f, "Hz", ValueTransform::LowPassOff),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("outputDb", "Output", -24, 12, 0, s::kGateOutputDb, 1, "dB"),
               choice("lookahead", "Look-ahead", 1, s::kGateLookahead, { "Sputnik", "Vostok", "Soyuz" }),
               choice("keyListen", "Key Listen", 0, s::kGateKeyListen, { "Off", "On" }),
               choice("flip", "Flip", 0, s::kGateFlip, { "Off", "On" }) } };
}

float transformed(float value, ValueTransform transform) noexcept
{
    switch (transform)
    {
        case ValueTransform::Direct: return value;
        case ValueTransform::Log2: return std::log2(juce::jlimit(0.5f, 2.0f, value));
        case ValueTransform::OneMinus: return 1.0f - juce::jlimit(0.0f, 1.0f, value);
        case ValueTransform::HighPassOff: return value <= 20.0f ? 0.0f : value;
        case ValueTransform::LowPassOff: return value >= 20000.0f ? 0.0f : value;
        case ValueTransform::MonoBassOff: return value < 20.0f ? 0.0f : value;
    }
    return value;
}
}
const ModuleSpec& moduleSpec(ModuleKind) {static const ModuleSpec value=makeZastava();return value;}

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
            slot.values[static_cast<size_t>(p.slotValueIndex)] = transformed(value, p.transform);
    }
    return slot;
}

int maximumLatencySamples(ModuleKind,double fs) noexcept {return dsp::ZastavaCore::lookaheadSamples(2,fs);}
}
