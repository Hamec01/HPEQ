#pragma once

#include <cmath>
#include <cstdint>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/FxProjectState.h"

namespace hpeq::ui
{
namespace colours
{
const juce::Colour background { 0xff0d1117 };
const juce::Colour panel { 0xff141a23 };
const juce::Colour panelRaised { 0xff1b2230 };
const juce::Colour outline { 0xff2a3442 };
const juce::Colour grid { 0xff1f2732 };
const juce::Colour text { 0xffdce3ec };
const juce::Colour dim { 0xff7d8a9b };
const juce::Colour accent { 0xff3fb8e8 };
const juce::Colour input { 0xff6b7686 };
const juce::Colour eqCurve { 0xffeef2f6 };
const juce::Colour eqNode { 0xffff6b6b };
const juce::Colour warning { 0xfff0b43c };
const juce::Colour danger { 0xffe0505a };

inline juce::Colour forNode(int index)
{
    static const juce::Colour palette[] { juce::Colour(0xfff0a23c), juce::Colour(0xff3ccf7a), juce::Colour(0xff3c8cf0),
                                          juce::Colour(0xffb05cf0), juce::Colour(0xfff05ca0), juce::Colour(0xffe8d24a),
                                          juce::Colour(0xff3ce0d0), juce::Colour(0xfff0605c) };
    return palette[static_cast<size_t>(index) % 8];
}
} // namespace colours

// What the workspace, list and inspector have selected (by stable node id).
struct Selection
{
    enum class Kind { None, Eq, Fx, Mix }; // Mix: the whole-layer chain (D7)
    Kind kind = Kind::None;
    std::uint32_t id = 0;
    int slot = -1; // selected module in the inspector; -1 = the module grid (D6)

    bool isEq(std::uint32_t nodeId) const noexcept { return kind == Kind::Eq && id == nodeId; }
    bool isFx(std::uint32_t nodeId) const noexcept { return kind == Kind::Fx && id == nodeId; }
};

// Logarithmic frequency axis 20 Hz ... 20 kHz (01 §9: f(u) = f_min (f_max / f_min)^u).
struct FrequencyAxis
{
    static constexpr double kMin = 20.0, kMax = 20000.0;

    static float xFor(double hz, juce::Rectangle<float> area)
    {
        const double u = std::log(juce::jlimit(kMin, kMax, hz) / kMin) / std::log(kMax / kMin);
        return area.getX() + static_cast<float>(u) * area.getWidth();
    }

    static double hzFor(float x, juce::Rectangle<float> area)
    {
        const double u = juce::jlimit(0.0, 1.0, static_cast<double>((x - area.getX()) / area.getWidth()));
        return kMin * std::pow(kMax / kMin, u);
    }
};

inline juce::String formatHz(double hz)
{
    return hz >= 1000.0 ? juce::String(hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String(juce::roundToInt(hz)) + " Hz";
}

inline juce::String shapeName(EqShape s)
{
    switch (s)
    {
        case EqShape::Bell: return "Bell";
        case EqShape::LowShelf: return "Low Shelf";
        case EqShape::HighShelf: return "High Shelf";
        case EqShape::LowCut: return "Low Cut";
        case EqShape::HighCut: return "High Cut";
        case EqShape::Notch: return "Notch";
        case EqShape::BandPass: return "Band Pass";
        case EqShape::Tilt: return "Tilt";
        case EqShape::AllPass: return "All Pass";
    }
    return {};
}

// Module names: the owner's own names (HamloVerb, HamloShirota, ...) are what the user sees; the plain
// function ("Reverb", "Stereo", ...) goes next to them where there is room, so nobody has to guess.
inline juce::String fxName(FxType t)
{
    switch (t)
    {
        case FxType::Compressor: return "HamloComp";
        case FxType::Limiter: return "HamloRedWall";
        case FxType::Reverb: return "HamloVerb";
        case FxType::Delay: return "HamloEcho";
        case FxType::TransientShaper: return "HamloUdarnik";
        case FxType::Saturator: return "HamloMag";
        case FxType::MonstaFx: return "Monsta FX";
        case FxType::StereoProcessor: return "HamloShirota";
        case FxType::GateExpander: return "HamloZastava";
        case FxType::Gain: return "HamloFader";
    }
    return {};
}

inline juce::String fxFunction(FxType t)
{
    switch (t)
    {
        case FxType::Compressor: return "Compressor";
        case FxType::Limiter: return "Limiter";
        case FxType::Reverb: return "Reverb";
        case FxType::Delay: return "Delay";
        case FxType::TransientShaper: return "Transient";
        case FxType::Saturator: return "Saturator";
        case FxType::MonstaFx: return "Slicer";
        case FxType::StereoProcessor: return "Stereo";
        case FxType::GateExpander: return "Gate / Expander";
        case FxType::Gain: return "Band level";
    }
    return {};
}

// "HamloShirota · Stereo": menus, the module list, the grid tiles, tooltips.
inline juce::String fxFullName(FxType t)
{
    return fxName(t) + juce::String::fromUTF8(" \xc2\xb7 ") + fxFunction(t); // " · "
}

inline juce::String channelName(ChannelMode m)
{
    switch (m)
    {
        case ChannelMode::Stereo: return "Stereo";
        case ChannelMode::Left: return "Left";
        case ChannelMode::Right: return "Right";
        case ChannelMode::Mid: return "Mid";
        case ChannelMode::Side: return "Side";
        case ChannelMode::LinkedStereo: return "Linked";
        case ChannelMode::DualMono: return "Dual Mono";
    }
    return {};
}

// Processors that really process audio in this build; the rest are listed but not selectable.
inline bool isFxAvailable(FxType) { return true; } // M6: every type is processed

inline juce::String nodeTitle(const FxNodeState& n)
{
    juce::StringArray names;
    for (const auto& s : n.chain)
        names.add(fxName(s.type));
    return names.joinIntoString(" > ");
}

inline bool isGainShapeUi(EqShape s)
{
    return s == EqShape::Bell || s == EqShape::LowShelf || s == EqShape::HighShelf || s == EqShape::Tilt;
}
} // namespace hpeq::ui
