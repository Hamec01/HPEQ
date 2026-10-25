#pragma once

#include <cmath>
#include <cstdint>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/FxProjectState.h"

namespace hpeq::ui
{
// EQ pass: the Black Steel palette for the whole window (steel, stencil cream, amber lamps, the green
// phosphor of the scopes); the module pages draw their own steel.
namespace colours
{
const juce::Colour background { 0xff1a1a17 };  // steel (under the worn plate the editor paints)
const juce::Colour panel { 0xff0a0d0b };       // black glass of the scopes, lists and LCDs
const juce::Colour panelRaised { 0xff2a2a25 }; // a steel key
const juce::Colour outline { 0xff3c3d35 };
const juce::Colour grid { 0xff15231a };        // phosphor graticule
const juce::Colour text { 0xffe6d8a6 };        // stencil cream
const juce::Colour dim { 0xff9a9174 };
const juce::Colour accent { 0xffffb347 };      // amber
const juce::Colour phosphor { 0xff6dff9a };    // green CRT trace
const juce::Colour input { 0xff6e7a6a };
const juce::Colour eqCurve { 0xfff2ead0 };
const juce::Colour eqNode { 0xffff5a45 };
const juce::Colour warning { 0xffffc94a };
const juce::Colour danger { 0xffff4a3a };

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
        case FxType::MonstaFx: return "HamloRevolution";
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
        case FxType::MonstaFx: return "Time / Glitch / Motion";
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
