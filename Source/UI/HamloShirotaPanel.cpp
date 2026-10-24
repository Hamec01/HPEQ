#include "UI/HamloShirotaPanel.h"

#include <cmath>

namespace hpeq::ui
{
namespace
{
constexpr float kW = 1180.0f, kH = 780.0f;
constexpr float kPi = juce::MathConstants<float>::pi;
constexpr int kBig = 6; // WIDTH .. MODE: the big row; BALANCE, MIX, OUTPUT: the small row
constexpr float kBigY = 245.0f, kBigLabelY = 160.0f, kBigReadoutY = 330.0f, kBigScale = 0.92f;
constexpr float kSmallY = 708.0f, kSmallLabelY = 664.0f, kSmallReadoutY = 752.0f, kSmallScale = 0.5f;
constexpr float kColumn = 1068.0f / 6.0f;
constexpr float kSwitchDialX = 66.0f, kSwitchDialW = 230.0f;
float switchTop(int i) { return 384.0f + static_cast<float>(i) * 110.0f; }
const juce::Rectangle<float> kCrtBezel { 331.0f, 382.0f, 404.0f, 238.0f };
const juce::Rectangle<float> kCrtScreen { 349.0f, 400.0f, 368.0f, 182.0f };
const juce::Point<float> kLamp { 1074.0f, 450.0f }, kToggle { 1074.0f, 556.0f };
const juce::Rectangle<float> kLcdLeft { 56.0f, 66.0f, 190.0f, 42.0f }, kLcdRight { 934.0f, 66.0f, 190.0f, 42.0f };

struct MapInfo
{
    double min, max;
    bool logarithmic;
};
// MONO BASS: under 20 Hz = off (LayerSnapshot); the knob's left end is 10 Hz.
constexpr std::array<MapInfo, HamloShirotaPanel::NumKnobs> kMaps { { { 0.0, 2.0, false }, { 0.0, 1.0, false }, { 0.0, 1.0, false },
                                                                    { 10.0, 500.0, true }, { -45.0, 45.0, false }, { 0.0, 2.0, false },
                                                                    { -1.0, 1.0, false }, { 0.0, 1.0, false }, { -24.0, 12.0, false } } };
constexpr std::array<const char*, HamloShirotaPanel::NumKnobs> kLabels { "Width", "Stereoize", "Space", "Mono bass", "Rotation", "Mode",
                                                                         "Balance", "Mix", "Output" };
const std::array<juce::StringArray, HamloShirotaPanel::NumSwitches> kSwitchOptions { juce::StringArray { "Off", "On" },
                                                                                     juce::StringArray { "Off", "On" } };
constexpr std::array<const char*, HamloShirotaPanel::NumSwitches> kSwitchLabels { "Safe (mono guard)", "Mono check" };

bool isBig(int knob) { return knob < kBig; }
float knobScaleFor(int knob) { return isBig(knob) ? kBigScale : kSmallScale; }
} // namespace

const juce::StringArray& HamloShirotaPanel::modeNames()
{
    static const juce::StringArray names { "EDINSTVO", "PROSTOR", "KOSMOS" };
    return names;
}

const juce::StringArray& HamloShirotaPanel::modeCharacters()
{
    static const juce::StringArray names { "M/S WIDTH", "STEREOIZE, MONO-SAFE", "SURROUND, BEHIND" };
    return names;
}

const char* HamloShirotaPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "width", "stereoize", "space", "monoBassHz", "rotation", "style", "balance", "mix", "outputDb" };
    return keys[p];
}

const char* HamloShirotaPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "safe", "monoListen" };
    return keys[s];
}

double HamloShirotaPanel::defaultFor(Param p)
{
    static constexpr double d[] { 1.0, 0.5, 0.5, 10.0, 0.0, 0.0, 0.0, 1.0, 0.0 }; // = defaultModule(StereoProcessor)
    return d[p];
}

juce::String HamloShirotaPanel::format(Param p, double v)
{
    switch (p)
    {
        case Width: case Stereoize: case Space: case Mix: return juce::String(juce::roundToInt(v * 100.0)) + " %";
        case MonoBass: return v < 20.0 ? juce::String("OFF") : juce::String(juce::roundToInt(v)) + " Hz";
        case Rotation: return (v > 0.0 ? "+" : "") + juce::String(v, 1) + juce::String::fromUTF8("\xc2\xb0");
        case Balance: return std::abs(v) < 0.005 ? juce::String("C") : (v < 0.0 ? "L " : "R ") + juce::String(juce::roundToInt(std::abs(v) * 100.0));
        case Output: return (v > 0.0 ? "+" : "") + juce::String(v, 1) + " dB";
        case Mode: return modeNames()[juce::jlimit(0, 2, juce::roundToInt(v))];
        case NumKnobs: break;
    }
    return {};
}

HamloShirotaPanel::HamloShirotaPanel()
{
    for (int i = 0; i < NumKnobs; ++i)
        centres[static_cast<size_t>(i)] = isBig(i) ? juce::Point<float>(56.0f + (static_cast<float>(i) + 0.5f) * kColumn, kBigY)
                                                   : juce::Point<float>(590.0f + static_cast<float>(i - kBig - 1) * kColumn, kSmallY);
    correlationMeter.bezel = { 751.0f, 382.0f, 248.0f, 238.0f };
    correlationMeter.min = -1.0f;
    correlationMeter.max = 1.0f;
    correlationMeter.redFrom = 1.0f;
    correlationMeter.title = "CORRELATION";
    correlationMeter.titleSize = 16.0f;
    correlationMeter.unit = "";
    for (float v : { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f })
        correlationMeter.ticks.push_back({ v, v == -1.0f || v == 0.0f || v == 1.0f }); // whole numbers labelled
    correlationMeter.seed = 1957;
    correlationMeter.shown = 1.0f;
    startTimerHz(30);
}

HamloShirotaPanel::~HamloShirotaPanel() = default;

double HamloShirotaPanel::normFor(Param p) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    const double v = juce::jlimit(m.min, m.max, values.knobs[static_cast<size_t>(p)]);
    return m.logarithmic ? std::log(v / m.min) / std::log(m.max / m.min) : (v - m.min) / (m.max - m.min);
}

double HamloShirotaPanel::valueFor(Param p, double n) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    const double v = m.logarithmic ? m.min * std::pow(m.max / m.min, n) : m.min + (m.max - m.min) * n;
    return p == Mode ? std::round(v) : v;
}

void HamloShirotaPanel::setNorm(Param p, double n, bool notify)
{
    const double v = valueFor(p, n);
    if (v == values.knobs[static_cast<size_t>(p)])
        return;
    values.knobs[static_cast<size_t>(p)] = v;
    if (notify && onKnobChange)
        onKnobChange(p, v);
    repaint();
}

void HamloShirotaPanel::setValues(const Values& v)
{
    if (dragKnob >= 0)
        return;
    values = v;
    repaint();
}

juce::AffineTransform HamloShirotaPanel::toScreen() const { return juce::AffineTransform::scale(scale).translated(offset); }
juce::Point<float> HamloShirotaPanel::toDesign(juce::Point<float> p) const { return (p - offset) / scale; }

void HamloShirotaPanel::resized()
{
    scale = juce::jmin(static_cast<float>(getWidth()) / kW, static_cast<float>(getHeight()) / kH);
    offset = { (static_cast<float>(getWidth()) - kW * scale) * 0.5f, (static_cast<float>(getHeight()) - kH * scale) * 0.5f };
    staticLayer = {};
    if (getWidth() > 0 && getHeight() > 0)
    {
        staticLayer = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g(staticLayer);
        g.addTransform(toScreen());
        renderStatic(g);
    }
    typing.reset();
}

juce::Rectangle<float> HamloShirotaPanel::readoutArea(int knob) const
{
    const auto c = centres[static_cast<size_t>(knob)];
    return { c.x - kColumn * 0.45f, (isBig(knob) ? kBigReadoutY : kSmallReadoutY) - 14.0f, kColumn * 0.9f, 28.0f };
}

void HamloShirotaPanel::renderStatic(juce::Graphics& g) const
{
    steel::panel(g, kW, kH, 1957);
    steel::scratches(g, { { 23, { 256.0f, 14.0f, 82.0f, 122.0f }, 4.0f }, { 8, { 834.0f, 14.0f, 82.0f, 122.0f }, -4.0f },
                          { 16, { 70.0f, 636.0f, 70.0f, 98.0f }, -3.0f } });
    steel::nameplate(g, { 590.0f - 185.0f, 41.0f, 370.0f, 92.0f }, "HamloShirota", "MODEL SHR-2", juce::String::fromUTF8("\xe2\x84\x96 1957"));
    steel::lcdWindow(g, kLcdLeft);
    steel::lcdWindow(g, kLcdRight);
    steel::engraved(g, "CORRELATION", kLcdLeft.translated(0.0f, 44.0f).withHeight(18.0f), 12.0f, steel::stencilDim);
    steel::engraved(g, "WIDTH", kLcdRight.translated(0.0f, 44.0f).withHeight(18.0f), 12.0f, steel::stencilDim);
    for (int i = 0; i < NumKnobs; ++i)
    {
        const auto c = centres[static_cast<size_t>(i)];
        const bool big = isBig(i);
        if (i != Mode) // the MODE knob is titled with the chosen mode itself (paint)
            steel::engraved(g, juce::String(kLabels[static_cast<size_t>(i)]).toUpperCase(),
                            { c.x - kColumn * 0.5f, (big ? kBigLabelY : kSmallLabelY) - 14.0f, kColumn, 28.0f }, big ? 22.0f : 15.0f, steel::stencil,
                            juce::Justification::centred, big ? 0.1f : 0.06f);
        steel::knobScale(g, c, knobScaleFor(i));
    }
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawHorizontalLine(juce::roundToInt(kSmallLabelY - 26.0f), 56.0f, 1124.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawHorizontalLine(juce::roundToInt(kSmallLabelY - 25.0f), 56.0f, 1124.0f);
    for (int i = 0; i < NumSwitches; ++i)
    {
        const float top = switchTop(i);
        if (i > 0)
        {
            g.setColour(juce::Colours::black);
            g.drawHorizontalLine(juce::roundToInt(top - 3.0f), 56.0f, 306.0f);
            g.setColour(juce::Colours::white.withAlpha(0.07f));
            g.drawHorizontalLine(juce::roundToInt(top - 4.0f), 56.0f, 306.0f);
        }
        steel::engraved(g, juce::String(kSwitchLabels[static_cast<size_t>(i)]).toUpperCase(), { 56.0f, top, 250.0f, 22.0f }, 16.0f);
    }
    steel::crtBezel(g, kCrtBezel, kCrtScreen, "VECTORSCOPE", kCrtBezel.getBottom() - 16.0f);
    steel::meterFace(g, correlationMeter);
    steel::engraved(g, "ON", { kLamp.x - 50.0f, 488.0f, 100.0f, 30.0f }, 20.0f);
}

void HamloShirotaPanel::drawCrt(juce::Graphics& g) const
{
    const auto s = kCrtScreen;
    steel::crtBegin(g, s);
    const auto c = s.getCentre();
    const float radius = s.getHeight() * 0.5f - 12.0f;
    // axes: M vertical, S across, L and R diagonals
    g.setColour(steel::phosphorDim.withAlpha(0.35f));
    g.drawLine(c.x, c.y - radius, c.x, c.y + radius, 1.0f);
    g.drawLine(c.x - radius * 1.6f, c.y, c.x + radius * 1.6f, c.y, 1.0f);
    g.drawLine(c.x - radius * 0.72f, c.y - radius * 0.72f, c.x + radius * 0.72f, c.y + radius * 0.72f, 1.0f);
    g.drawLine(c.x + radius * 0.72f, c.y - radius * 0.72f, c.x - radius * 0.72f, c.y + radius * 0.72f, 1.0f);
    steel::crtText(g, "M", { c.x - 8.0f, s.getY() + 2.0f, 16.0f, 12.0f }, 10.5f, juce::Justification::centred, 0.7f);
    steel::crtText(g, "L", { c.x - radius * 0.72f - 14.0f, c.y - radius * 0.72f - 12.0f, 12.0f, 12.0f }, 10.5f, juce::Justification::centred, 0.7f);
    steel::crtText(g, "R", { c.x + radius * 0.72f + 2.0f, c.y - radius * 0.72f - 12.0f, 12.0f, 12.0f }, 10.5f, juce::Justification::centred, 0.7f);
    steel::crtText(g, "+S", { s.getRight() - 30.0f, c.y - 14.0f, 24.0f, 12.0f }, 10.5f, juce::Justification::centredRight, 0.6f);
    steel::crtText(g, "-S", { s.getX() + 6.0f, c.y - 14.0f, 24.0f, 12.0f }, 10.5f, juce::Justification::centredLeft, 0.6f);

    if (showsError())
    {
        steel::crtText(g, "OSHIBKA", { s.getX(), c.y - 34.0f, s.getWidth(), 36.0f }, 30.0f, juce::Justification::centred, 1.0f);
        steel::crtText(g, "THE NODE WORKS ON ONE CHANNEL (MID / SIDE / LEFT / RIGHT).", { s.getX(), c.y + 8.0f, s.getWidth(), 14.0f }, 10.0f,
                       juce::Justification::centred, 0.85f);
        steel::crtText(g, "SET ITS CHANNEL MODE TO STEREO TO WIDEN.", { s.getX(), c.y + 24.0f, s.getWidth(), 14.0f }, 10.0f,
                       juce::Justification::centred, 0.85f);
        steel::crtEnd(g, s);
        return;
    }
    // points: mid up, side across (left channel upper-left), auto-scaled
    for (int i = 0; i < kScopePoints; ++i)
    {
        const float l = scopeL[static_cast<size_t>(i)] * scopeScale, r = scopeR[static_cast<size_t>(i)] * scopeScale;
        const float x = c.x + (r - l) * 0.7071f * radius, y = c.y - (l + r) * 0.7071f * radius;
        const float age = static_cast<float>(i) / static_cast<float>(kScopePoints);
        g.setColour(steel::phosphor.withAlpha(0.12f + 0.55f * age));
        g.fillRect(x - 0.9f, y - 0.9f, 1.8f, 1.8f);
    }
    steel::crtEnd(g, s);
}

void HamloShirotaPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    for (int i = 0; i < NumKnobs; ++i)
    {
        const auto c = centres[static_cast<size_t>(i)];
        const auto p = static_cast<Param>(i);
        const float angle = (-135.0f + static_cast<float>(normFor(p)) * 270.0f) * kPi / 180.0f;
        steel::brassKnob(g, c, angle, knobScaleFor(i), 1.0f, steel::KnobAsset::AluminumBrassInsert, steel::PointerAsset::WhiteEnamel);
        const auto text = format(p, values.knobs[static_cast<size_t>(i)]);
        if (p == Mode)
        {
            // the mode's name on top, its real character small below (owner)
            steel::engraved(g, text, { c.x - kColumn * 0.5f, kBigLabelY - 17.0f, kColumn, 34.0f }, 22.0f, steel::stencil, juce::Justification::centred, 0.1f);
            steel::engraved(g, modeCharacters()[juce::jlimit(0, 2, juce::roundToInt(values.knobs[Mode]))], readoutArea(i).expanded(10.0f, 0.0f), 12.0f,
                            steel::stencilDim, juce::Justification::centred, 0.05f);
            continue;
        }
        const auto r = readoutArea(i);
        g.setFont(steel::stencilFont(isBig(i) ? 23.0f : 16.0f));
        g.setColour(juce::Colours::black);
        g.drawText(text, r.translated(0.0f, 1.0f), juce::Justification::centred);
        g.setColour(steel::stencil);
        g.drawText(text, r, juce::Justification::centred);
    }
    for (int sw = 0; sw < NumSwitches; ++sw)
    {
        const auto& options = kSwitchOptions[static_cast<size_t>(sw)];
        const int n = options.size(), selected = juce::jlimit(0, n - 1, values.switches[static_cast<size_t>(sw)]);
        const float dialTop = switchTop(sw) + 24.0f;
        for (int i = 0; i < n; ++i)
        {
            const float x = kSwitchDialX + kSwitchDialW * (0.12f + 0.76f * static_cast<float>(i) / static_cast<float>(n - 1));
            g.setFont(steel::stencilFont(13.5f).withKerningFactor(0.1f));
            g.setColour(i == selected ? juce::Colour(0xfffff6d0) : steel::stencilDim);
            g.drawText(options[i].toUpperCase(), juce::Rectangle<float>(x - 44.0f, dialTop, 88.0f, 18.0f), juce::Justification::centred);
        }
        const float a = (-34.0f + 68.0f * static_cast<float>(selected)) * kPi / 180.0f;
        steel::lever(g, { kSwitchDialX + kSwitchDialW * 0.5f, dialTop + 52.0f }, a, 0.85f);
    }
    drawCrt(g);
    steel::meterNeedle(g, correlationMeter);
    steel::lamp(g, kLamp, values.on, 1.0f);
    steel::lever(g, kToggle, values.on ? 0.0f : kPi, 1.15f, values.on ? steel::LeverPosition::Up : steel::LeverPosition::Down);
    steel::lcdText(g, kLcdLeft, showsError() ? juce::String("----") : (reading.correlation >= 0.0f ? "+" : "") + juce::String(reading.correlation, 2), 21.0f);
    const auto right = reading.guardDb < -0.1f && values.on ? "SAFE " + juce::String(reading.guardDb, 1) + " dB"
                                                            : juce::String(juce::roundToInt(values.knobs[Width] * 100.0)) + " %";
    steel::lcdText(g, kLcdRight, right, 20.0f);
}

void HamloShirotaPanel::take(const Reading& r)
{
    reading = r;
    const float target = values.on && !showsError() ? juce::jlimit(-1.0f, 1.0f, r.correlation) : 1.0f;
    correlationMeter.shown += (target - correlationMeter.shown) * 0.3f;
    if (readScope && readScope(scopeL, scopeR))
    {
        double energy = 1.0e-10;
        for (int i = 0; i < kScopePoints; ++i)
            energy += scopeL[static_cast<size_t>(i)] * scopeL[static_cast<size_t>(i)] + scopeR[static_cast<size_t>(i)] * scopeR[static_cast<size_t>(i)];
        const float wanted = juce::jlimit(0.5f, 2000.0f, 0.35f / static_cast<float>(std::sqrt(energy / (2.0 * kScopePoints)))); // the cloud fills the screen
        scopeScale += (wanted - scopeScale) * (wanted < scopeScale ? 1.0f : 0.1f);
    }
}

void HamloShirotaPanel::timerCallback()
{
    if (!isShowing())
        return;
    take(readMeters ? readMeters() : Reading {});
    repaint();
}

void HamloShirotaPanel::settleMeters(const Reading& r)
{
    take(r);
    correlationMeter.shown = values.on && !showsError() ? juce::jlimit(-1.0f, 1.0f, r.correlation) : 1.0f;
    repaint();
}

int HamloShirotaPanel::knobAt(juce::Point<float> d) const
{
    for (int i = 0; i < NumKnobs; ++i)
        if (d.getDistanceFrom(centres[static_cast<size_t>(i)]) <= (isBig(i) ? 56.0f : 32.0f))
            return i;
    return -1;
}

bool HamloShirotaPanel::hitSwitch(juce::Point<float> d, int& which, int& option) const
{
    for (int s = 0; s < NumSwitches; ++s)
    {
        const float dialTop = switchTop(s) + 24.0f;
        if (d.y < dialTop || d.y > dialTop + 70.0f || d.x < kSwitchDialX || d.x > kSwitchDialX + kSwitchDialW)
            continue;
        which = s;
        const int n = kSwitchOptions[static_cast<size_t>(s)].size();
        option = -1;
        if (d.y <= dialTop + 20.0f)
            for (int i = 0; i < n; ++i)
            {
                const float x = kSwitchDialX + kSwitchDialW * (0.12f + 0.76f * static_cast<float>(i) / static_cast<float>(n - 1));
                if (std::abs(d.x - x) <= 40.0f)
                    option = i;
            }
        if (option < 0)
            option = (values.switches[static_cast<size_t>(s)] + 1) % n;
        return true;
    }
    return false;
}

bool HamloShirotaPanel::hitPower(juce::Point<float> d) const
{
    return d.getDistanceFrom(kLamp) <= 34.0f || juce::Rectangle<float>(kToggle.x - 22, kToggle.y - 30, 44, 60).contains(d);
}

void HamloShirotaPanel::mouseDown(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    if (const int k = knobAt(d); k >= 0)
    {
        dragKnob = k;
        dragStartNorm = normFor(static_cast<Param>(k));
        dragStartY = e.position.y;
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        return;
    }
    int which = -1, option = -1;
    if (hitSwitch(d, which, option))
    {
        values.switches[static_cast<size_t>(which)] = option;
        if (onSwitchChange)
            onSwitchChange(static_cast<Switch>(which), option);
        repaint();
        return;
    }
    if (hitPower(d))
    {
        values.on = !values.on;
        if (onPowerChange)
            onPowerChange(values.on);
        repaint();
        return;
    }
    for (int i = 0; i < NumKnobs; ++i)
        if (i != Mode && readoutArea(i).contains(d))
        {
            editReadout(static_cast<Param>(i));
            return;
        }
}

void HamloShirotaPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob < 0)
        return;
    const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 200.0f) * scale;
    setNorm(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange, true);
}

void HamloShirotaPanel::mouseUp(const juce::MouseEvent&)
{
    if (dragKnob >= 0 && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
}

void HamloShirotaPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (const int k = knobAt(toDesign(e.position)); k >= 0)
    {
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        values.knobs[static_cast<size_t>(k)] = defaultFor(static_cast<Param>(k));
        if (onKnobChange)
            onKnobChange(static_cast<Param>(k), values.knobs[static_cast<size_t>(k)]);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        repaint();
    }
}

void HamloShirotaPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (const int k = knobAt(toDesign(e.position)); k >= 0)
    {
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        const double step = (k == Mode ? 0.5 : (e.mods.isShiftDown() ? 0.004 : 0.02)) * (wheel.deltaY > 0 ? 1.0 : -1.0);
        setNorm(static_cast<Param>(k), normFor(static_cast<Param>(k)) + step, true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
    }
}

void HamloShirotaPanel::editReadout(Param p)
{
    const auto area = readoutArea(p).transformedBy(toScreen()).toNearestInt();
    typing = std::make_unique<juce::TextEditor>();
    typing->setBounds(area);
    typing->setFont(steel::stencilFont((isBig(p) ? 22.0f : 15.0f) * scale * 1.1f));
    typing->setJustification(juce::Justification::centred);
    typing->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff120b04));
    typing->setColour(juce::TextEditor::textColourId, juce::Colour(0xffffb347));
    const bool percent = p == Width || p == Stereoize || p == Space || p == Mix || p == Balance;
    const double shown = percent ? values.knobs[static_cast<size_t>(p)] * 100.0 : values.knobs[static_cast<size_t>(p)];
    typing->setText(juce::String(shown, 1), false);
    typing->selectAll();
    juce::Component::SafePointer<HamloShirotaPanel> self(this);
    const auto closeEditor = [self] { juce::MessageManager::callAsync([self] { if (self != nullptr) self->typing.reset(); }); };
    const auto commit = [this, p, percent, closeEditor]
    {
        if (typing == nullptr)
            return;
        double v = typing->getText().retainCharacters("-0123456789.").getDoubleValue();
        if (percent)
            v /= 100.0;
        const auto& m = kMaps[static_cast<size_t>(p)];
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        values.knobs[static_cast<size_t>(p)] = juce::jlimit(m.min, m.max, v);
        if (onKnobChange)
            onKnobChange(p, values.knobs[static_cast<size_t>(p)]);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        closeEditor();
        repaint();
    };
    typing->onReturnKey = commit;
    typing->onFocusLost = commit;
    typing->onEscapeKey = closeEditor;
    addAndMakeVisible(*typing);
    typing->grabKeyboardFocus();
}
} // namespace hpeq::ui
