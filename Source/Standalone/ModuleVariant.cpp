#include "Standalone/ModuleVariant.h"
#include <cmath>
#include "Standalone/FaderHostControls.h"
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

ModuleSpec makeFader()
{
    namespace s = dsp::slot;
    return { ModuleKind::Fader, FxType::Gain, "HamloFader", 1180, 700, 4,
             { knob("gainDb", "Gain", -60, 24, 0, s::kGainDb, 1, "dB"),
               knob("delayMs", "Delay", 0, 20, 0, s::kFadDelayMs, 1, "ms"),
               knob("targetDb", "Target", -40, 0, -18, s::kFadTargetDb, 1, "dB"),
               knob("mix", "Mix", 0, 1, 1, -1),
               choice("polarity", "Polarity", 0, s::kFadPolarity, { "Off", "Left", "Right", "Both" }),
               choice("mute", "Mute", 0, s::kFadMute, { "Off", "On" }),
               choice("solo", "Solo", 0, s::kFadSolo, { "Off", "On" }) } };
}

}
const ModuleSpec& moduleSpec(ModuleKind){static const auto fader=makeFader();return fader;}
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
    appendFaderControls(layout);
    return layout;
}

dsp::FxSlotParams makeSlot(ModuleKind kind,const std::vector<std::atomic<float>*>& values,bool active) noexcept
{
    const auto& spec=moduleSpec(kind);dsp::FxSlotParams slot;slot.active=active;slot.type=spec.type;slot.mix=1;
    const auto count=juce::jmin(values.size(),spec.parameters.size());
    for(size_t i=0;i<count;++i){const auto& p=spec.parameters[i];const float value=values[i]->load(std::memory_order_relaxed);if(p.slotValueIndex<0)slot.mix=juce::jlimit(0.f,1.f,value);else slot.values[static_cast<size_t>(p.slotValueIndex)]=value;}
    return slot;
}
int maximumLatencySamples(ModuleKind,double) noexcept{return 0;}
}
