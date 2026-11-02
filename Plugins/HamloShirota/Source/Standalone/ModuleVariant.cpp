#include "Standalone/ModuleVariant.h"
#include "Standalone/V2Parameters.h"
#include "DSP/HamloShirota.h"
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

ModuleSpec makeShirota()
{
    namespace s = dsp::slot;
    return { ModuleKind::Shirota, FxType::StereoProcessor, "HamloShirota", 1180, 900, 9,
             { knob("width", "Width", 0, 2, 1, s::kStereoWidth),
               knob("stereoize", "Stereoize", 0, 1, 0.5f, s::kStStereoize),
               knob("space", "Space", 0, 1, 0.5f, s::kStSpace),
               knob("monoBassHz", "Mono Bass", 10, 500, 10, s::kStMonoBassHz, 0.5f, "Hz", ValueTransform::MonoBassOff),
               knob("rotation", "Rotation", -45, 45, 0, s::kStRotation, 1, "deg"),
               choice("style", "Mode", 0, s::kStStyle, { "EDINSTVO", "PROSTOR", "KOSMOS" }),
               knob("balance", "Balance", -1, 1, 0, s::kStereoBalance),
               knob("mix", "Mix", 0, 1, 1, -1),
               knob("outputDb", "Output", -24, 12, 0, s::kStOutputDb, 1, "dB"),
               choice("safe", "Safe", 1, s::kStSafe, { "Off", "On" }),
               choice("monoListen", "Mono Check", 0, s::kStMonoListen, { "Off", "On" }) } };
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
const ModuleSpec& moduleSpec(ModuleKind)
{static const ModuleSpec shirota=makeShirota();return shirota;}

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
    addV2Parameters(layout);
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

int maximumLatencySamples(ModuleKind,double rate) noexcept
{return dsp::ShirotaCore::safeLookaheadSamples(rate);}
}
