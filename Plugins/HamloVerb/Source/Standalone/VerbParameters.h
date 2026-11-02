#pragma once
#include "Standalone/ModuleVariant.h"
#include "DSP/VerbV2Settings.h"

namespace hpeq::standalone
{
// APVTS parameter order: the original 22, the original bypass, then additions.
// The spec/raw-value array excludes bypass and follows these stable indices.
enum VerbParameterIndex
{
    LegacyParameterCount = 22,
    Engine = LegacyParameterCount, Routing, Output, Early, Late, TimeShape,
    ShapeSync, GateThreshold, ShapeTime, GateRelease, ShimmerAmount, ShimmerPitch,
    ShimmerFeedback, DuckAttack, DuckRelease, EqStart,
    VerbParameterCount = EqStart + 60
};

inline void appendVerbParameters(ModuleSpec& spec)
{
    auto number = [&](const juce::String& id, const juce::String& name, float lo, float hi,
                      float def, float skew = 1.0f, const char* unit = "")
    {
        ParameterSpec p;
        p.id = id; p.name = name; p.minimum = lo; p.maximum = hi;
        p.defaultValue = def; p.skew = skew; p.unit = unit; p.slotValueIndex = -2;
        spec.parameters.push_back(std::move(p));
    };
    auto choice = [&](const juce::String& id, const juce::String& name, juce::StringArray labels, int def = 0)
    {
        number(id, name, 0.0f, static_cast<float>(labels.size() - 1), static_cast<float>(def));
        spec.parameters.back().choices = std::move(labels);
        spec.parameters.back().interval = 1.0f;
    };
    choice("engine", "Character", { "Legacy", "Clean", "Warm", "Velvet", "Crisp", "Deep", "Bloom" });
    choice("routing", "Routing", { "Insert", "Send" });
    number("outputDb", "Output", -24, 12, 0, 1, "dB");
    number("earlyDb", "Early Level", -60, 12, 0, 1, "dB");
    number("lateDb", "Tail Level", -60, 12, 0, 1, "dB");
    choice("timeShape", "Time Shape", { "Off", "Gated", "Reverse Swell", "Reverse Audio" });
    choice("shapeSync", "Shape Sync", { "Free", "1/64", "1/32", "1/16T", "1/16", "1/8T", "1/16D", "1/8", "1/4T", "1/8D", "1/4", "1/2" });
    number("gateThresholdDb", "Shape Threshold", -60, 0, -30, 1, "dB");
    number("shapeTimeMs", "Shape Time", 50, 2000, 250, 0.4f, "ms");
    number("gateReleaseMs", "Gate Release", 1, 500, 30, 0.4f, "ms");
    number("shimmerAmount", "Shimmer Amount", 0, 1, 0);
    number("shimmerPitch", "Shimmer Pitch", -12, 24, 12, 1, "st");
    number("shimmerFeedback", "Shimmer Feedback", 0, 0.85f, 0.35f);
    number("duckAttackMs", "Duck Attack", 0.1f, 100, 5, 0.4f, "ms");
    number("duckReleaseMs", "Duck Release", 10, 2000, 250, 0.4f, "ms");
    constexpr float frequencies[] { 120, 350, 1000, 3000, 7000, 12000 };
    for (int kind = 0; kind < 2; ++kind)
        for (int band = 0; band < 6; ++band)
        {
            const auto id = juce::String(kind == 0 ? "decayEq" : "postEq") + juce::String(band + 1);
            const auto label = juce::String(kind == 0 ? "Decay " : "Post ") + juce::String(band + 1) + " ";
            choice(id + "On", label + "Enabled", { "Off", "On" });
            choice(id + "Type", label + "Type", kind == 0
                ? juce::StringArray { "Bell", "Low Shelf", "High Shelf" }
                : juce::StringArray { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut" });
            number(id + "Hz", label + "Frequency", 20, 20000, frequencies[band], 0.25f, "Hz");
            number(id + "Value", label + (kind == 0 ? "Decay" : "Gain"), kind == 0 ? -2.0f : -18.0f,
                   kind == 0 ? 2.0f : 18.0f, 0, 1, kind == 0 ? "oct" : "dB");
            number(id + "Q", label + "Q", 0.2f, 8, 0.707f, 0.5f);
        }
}

inline dsp::VerbV2Settings readVerbSettings(const std::vector<std::atomic<float>*>& values,
                                           const dsp::FxSlotParams& base) noexcept
{
    const auto read = [&](int index) { return values[static_cast<size_t>(index)]->load(std::memory_order_relaxed); };
    dsp::VerbV2Settings s;
    s.base = base;
    s.engine = juce::roundToInt(read(Engine));
    s.earlyDb = read(Early); s.lateDb = read(Late);
    s.timeShape = juce::roundToInt(read(TimeShape)); s.shapeSync = juce::roundToInt(read(ShapeSync));
    s.gateThresholdDb = read(GateThreshold); s.shapeTimeMs = read(ShapeTime); s.gateReleaseMs = read(GateRelease);
    s.shimmerAmount = read(ShimmerAmount); s.shimmerPitch = read(ShimmerPitch); s.shimmerFeedback = read(ShimmerFeedback);
    s.duckAttackMs = read(DuckAttack); s.duckReleaseMs = read(DuckRelease);
    for (int kind = 0; kind < 2; ++kind)
        for (int band = 0; band < 6; ++band)
        {
            auto& b = kind == 0 ? s.decayEq[static_cast<size_t>(band)] : s.postEq[static_cast<size_t>(band)];
            const int offset = EqStart + kind * 30 + band * 5;
            b.enabled = read(offset) >= 0.5f; b.type = juce::roundToInt(read(offset + 1));
            b.hz = read(offset + 2); b.value = read(offset + 3); b.q = read(offset + 4);
        }
    return s;
}
} // namespace hpeq::standalone
