#include "UI/HamloRedWallPanel.h"

#include <cmath>

namespace hamlo::ui
{
namespace
{
constexpr float kW = 1180.0f, kH = 820.0f; // design stage (700 + the ENGINE / DRIVE / TUBE strip)
constexpr float kPi = juce::MathConstants<float>::pi;
constexpr float kKnobY = 253.0f, kLabelY = 160.0f, kReadoutY = 337.0f;
constexpr float kSwitchDialX = 66.0f, kSwitchDialW = 230.0f;
constexpr float kStripTop = 688.0f, kStripKnobY = 740.0f, kStripKnobScale = 0.6f;
constexpr int kRowKnobs = 8, kRowSwitches = 3; // the top row and the left column; the rest sit in the strip
float switchTop(int i) { return i < kRowSwitches ? 372.0f + static_cast<float>(i) * 88.0f : kStripTop; }
bool inStrip(int knob) { return knob >= kRowKnobs; }
const juce::Rectangle<float> kCrtBezel { 331.0f, 385.0f, 404.0f, 238.0f };
const juce::Rectangle<float> kCrtScreen { 349.0f, 403.0f, 368.0f, 182.0f };
const juce::Point<float> kLamp { 1074.0f, 453.0f }, kToggle { 1074.0f, 559.0f };
const juce::Rectangle<float> kLcdLeft { 56.0f, 66.0f, 190.0f, 42.0f }, kLcdRight { 934.0f, 66.0f, 190.0f, 42.0f };

struct MapInfo
{
    double min, max;
    bool logarithmic;
};
// THRESHOLD turns right to push harder (0 -> -30 dB); STYLE has three detents.
constexpr std::array<MapInfo, HamloRedWallPanel::NumKnobs> kMaps { { { 0.0, -30.0, false }, { -20.0, 0.0, false }, { 1.0, 1000.0, true },
                                                                      { 0.1, 10.0, true }, { 0.0, 1.0, false }, { -24.0, 0.0, false },
                                                                      { 0.0, 1.0, false }, { 0.0, 2.0, false }, { 0.0, 1.0, false },
                                                                      { 0.0, 1.0, false } } };
constexpr float kKnobScale = 0.84f; // eight knobs share the 1068 px row
float knobScaleFor(int knob) { return inStrip(knob) ? kStripKnobScale : kKnobScale; }
// the strip knobs: the title left of the knob, the readout right of it
juce::Rectangle<float> stripLabel(juce::Point<float> c) { return { c.x - 172.0f, c.y - 16.0f, 120.0f, 32.0f }; }
juce::Rectangle<float> stripReadout(juce::Point<float> c) { return { c.x + 54.0f, c.y - 16.0f, 96.0f, 32.0f }; }
constexpr std::array<const char*, HamloRedWallPanel::NumKnobs> kLabels { "Loudness", "Limit", "Release", "Lookahead", "Link", "Output", "Mix", "Style", "Drive", "Tube" };
const std::array<juce::StringArray, HamloRedWallPanel::NumSwitches> kSwitchOptions { juce::StringArray { "Off", "On" },
                                                                                     juce::StringArray { "Manual", "Auto" },
                                                                                     juce::StringArray { "Normal", "Unity", "Delta" },
                                                                                     juce::StringArray { "Classic", "V2" } };
constexpr std::array<const char*, HamloRedWallPanel::NumSwitches> kSwitchLabels { "True Peak", "Release", "Monitor", "Engine" };
} // namespace

const juce::StringArray& HamloRedWallPanel::styleNames()
{
    static const juce::StringArray names { "GLASNOST", "STAKHANOV", "OCTOBER" };
    return names;
}

const char* HamloRedWallPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "thresholdDb", "ceilingDb", "releaseMs", "lookaheadMs", "link", "outputDb", "mix", "style", "drive", "tube" };
    return keys[p];
}

const char* HamloRedWallPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "truePeak", "autoRelease", "monitor", "engine" };
    return keys[s];
}

double HamloRedWallPanel::defaultFor(Param p)
{
    static constexpr double d[] { 0.0, 0.0, 60.0, 2.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0 }; // = defaultModule(Limiter), mix 100 %; no colour
    return d[p];
}

juce::String HamloRedWallPanel::format(Param p, double v)
{
    switch (p)
    {
        case Threshold: return (v < 0.0 ? "+" + juce::String(-v, 1) : juce::String(0.0, 1)) + " dB"; // the push
        case Ceiling: case Output: return juce::String(v, 1) + " dB";
        case Release: return juce::String(juce::roundToInt(v)) + " ms";
        case Lookahead: return juce::String(v, 2) + " ms";
        case Link: case Mix: case Drive: case Tube: return juce::String(juce::roundToInt(v * 100.0)) + " %";
        case Style: return styleNames()[juce::jlimit(0, 2, juce::roundToInt(v))];
        case NumKnobs: break;
    }
    return {};
}

HamloRedWallPanel::HamloRedWallPanel()
{
    for (int i = 0; i < kRowKnobs; ++i) // eight columns over the content width 56 .. 1124
        centres[static_cast<size_t>(i)] = { 56.0f + (static_cast<float>(i) + 0.5f) * (1068.0f / kRowKnobs), kKnobY };
    centres[Drive] = { 560.0f, kStripKnobY };
    centres[Tube] = { 930.0f, kStripKnobY };
    grMeter.bezel = { 751.0f, 385.0f, 248.0f, 238.0f };
    grMeter.min = -20.0f;
    grMeter.max = 0.0f;
    grMeter.redFrom = -6.0f;
    grMeter.title = "GAIN REDUCTION";
    grMeter.titleSize = 17.0f;
    for (float v : { -20.0f, -18.0f, -16.0f, -14.0f, -12.0f, -10.0f, -8.0f, -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f })
        grMeter.ticks.push_back({ v, v == -20.0f || v == -10.0f || v == -5.0f || v == -2.0f || v == 0.0f });
    grMeter.seed = 1917;
    grMeter.shown = 0.0f;
    history.assign(120, 0.0f);
    startTimerHz(30);
}

HamloRedWallPanel::~HamloRedWallPanel()
{
    stopTimer();
    if (typing != nullptr)
    {
        typing->onReturnKey = {};
        typing->onFocusLost = {};
        typing->onEscapeKey = {};
        typing.reset();
    }
}

double HamloRedWallPanel::normFor(Param p) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    const double lo = std::min(m.min, m.max), hi = std::max(m.min, m.max);
    const double v = juce::jlimit(lo, hi, values.knobs[static_cast<size_t>(p)]);
    return m.logarithmic ? std::log(v / m.min) / std::log(m.max / m.min) : (v - m.min) / (m.max - m.min);
}

double HamloRedWallPanel::valueFor(Param p, double n) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    const double v = m.logarithmic ? m.min * std::pow(m.max / m.min, n) : m.min + (m.max - m.min) * n;
    return p == Style ? std::round(v) : v;
}

void HamloRedWallPanel::setNorm(Param p, double n, bool notify)
{
    const double v = valueFor(p, n);
    if (v == values.knobs[static_cast<size_t>(p)])
        return;
    values.knobs[static_cast<size_t>(p)] = v;
    if (notify && onKnobChange)
        onKnobChange(p, v);
    repaint();
}

void HamloRedWallPanel::setValues(const Values& v)
{
    if (dragKnob >= 0)
        return;
    values = v;
    repaint();
}

juce::AffineTransform HamloRedWallPanel::toScreen() const { return juce::AffineTransform::scale(scale).translated(offset); }
juce::Point<float> HamloRedWallPanel::toDesign(juce::Point<float> p) const { return (p - offset) / scale; }

void HamloRedWallPanel::resized()
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

void HamloRedWallPanel::renderStatic(juce::Graphics& g) const
{
    steel::panel(g, kW, kH, 1917);
    steel::scratches(g, { { 2, { 256.0f, 14.0f, 82.0f, 122.0f }, -4.0f }, { 15, { 834.0f, 14.0f, 82.0f, 122.0f }, 4.0f },
                          { 12, { 236.0f, 598.0f, 64.0f, 92.0f }, 3.0f } });
    steel::nameplate(g, { 590.0f - 185.0f, 41.0f, 370.0f, 92.0f }, "HamloRedWall", "MODEL OGR-1", juce::String::fromUTF8("\xe2\x84\x96 1917"));
    steel::lcdWindow(g, kLcdLeft);
    steel::lcdWindow(g, kLcdRight);
    steel::engraved(g, "OUTPUT", kLcdLeft.translated(0.0f, 44.0f).withHeight(18.0f), 12.0f, steel::stencilDim);
    steel::engraved(g, "REDUCTION", kLcdRight.translated(0.0f, 44.0f).withHeight(18.0f), 12.0f, steel::stencilDim);
    for (int i = 0; i < NumKnobs; ++i)
    {
        const auto c = centres[static_cast<size_t>(i)];
        if (inStrip(i))
            steel::engraved(g, juce::String(kLabels[static_cast<size_t>(i)]).toUpperCase(), stripLabel(c), 19.0f, steel::stencil,
                            juce::Justification::centredRight, 0.1f);
        else if (i != Style) // the STYLE knob is titled with the chosen style itself (paint)
            steel::engraved(g, juce::String(kLabels[static_cast<size_t>(i)]).toUpperCase(), { c.x - 66.0f, kLabelY - 17.0f, 132.0f, 34.0f }, 19.0f,
                            steel::stencil, juce::Justification::centred, 0.1f);
        steel::knobScale(g, c, knobScaleFor(i));
    }
    // STYLE detents: the two end names beside the scale (the selected one is the red readout)
    for (int k : { 0, 2 })
    {
        const float a = (-135.0f + 135.0f * static_cast<float>(k)) * kPi / 180.0f;
        const auto c = centres[Style];
        const juce::Point<float> p(c.x + std::sin(a) * 78.0f, c.y - std::cos(a) * 78.0f);
        steel::engraved(g, styleNames()[k], { p.x - 46.0f, p.y + 2.0f, 92.0f, 16.0f }, 10.5f, steel::stencilDim);
    }
    // the strip: an engraved groove across the panel
    g.setColour(juce::Colours::black);
    g.drawHorizontalLine(juce::roundToInt(kStripTop - 14.0f), 56.0f, kW - 56.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawHorizontalLine(juce::roundToInt(kStripTop - 15.0f), 56.0f, kW - 56.0f);
    for (int i = 0; i < NumSwitches; ++i)
    {
        const float top = switchTop(i);
        if (i > 0 && i < kRowSwitches)
        {
            g.setColour(juce::Colours::black);
            g.drawHorizontalLine(juce::roundToInt(top - 3.0f), 56.0f, 306.0f);
            g.setColour(juce::Colours::white.withAlpha(0.07f));
            g.drawHorizontalLine(juce::roundToInt(top - 4.0f), 56.0f, 306.0f);
        }
        steel::engraved(g, juce::String(kSwitchLabels[static_cast<size_t>(i)]).toUpperCase(), { 56.0f, top, 250.0f, 22.0f }, 16.0f);
    }
    steel::crtBezel(g, kCrtBezel, kCrtScreen, "GAIN REDUCTION  -  LAST 4 S", kCrtBezel.getBottom() - 16.0f);
    steel::meterFace(g, grMeter);
    steel::engraved(g, "ON", { kLamp.x - 50.0f, 491.0f, 100.0f, 30.0f }, 20.0f);
}

void HamloRedWallPanel::drawCrt(juce::Graphics& g) const
{
    const auto s = kCrtScreen;
    steel::crtBegin(g, s);
    const auto yFor = [&s](float grDb) { return s.getY() + 12.0f + (s.getHeight() - 24.0f) * juce::jlimit(0.0f, 1.0f, grDb / 20.0f); };
    for (float db : { 0.0f, 3.0f, 6.0f, 12.0f, 20.0f })
    {
        g.setColour(steel::phosphorDim.withAlpha(0.35f));
        g.drawHorizontalLine(juce::roundToInt(yFor(db)), s.getX() + 30.0f, s.getRight() - 6.0f);
        steel::crtText(g, juce::String(-static_cast<int>(db)), { s.getX() + 4.0f, yFor(db) - 7.0f, 26.0f, 14.0f }, 11.0f, juce::Justification::centredLeft, 0.6f);
    }
    juce::Path line;
    const float x0 = s.getX() + 30.0f, w = s.getWidth() - 36.0f;
    for (size_t i = 0; i < history.size(); ++i)
    {
        const float x = x0 + w * static_cast<float>(i) / static_cast<float>(history.size() - 1);
        const float y = yFor(history[i]);
        if (i == 0)
            line.startNewSubPath(x, y);
        else
            line.lineTo(x, y);
    }
    steel::glow(g, line, steel::phosphor, 2.0f, 1.0f);
    steel::crtText(g, "GR " + juce::String(std::abs(grText), 1) + " dB", { s.getRight() - 120.0f, s.getY() + 6.0f, 112.0f, 16.0f }, 13.0f,
                   juce::Justification::centredRight, 0.9f);
    steel::crtEnd(g, s);
}

void HamloRedWallPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    for (int i = 0; i < NumKnobs; ++i)
    {
        const auto c = centres[static_cast<size_t>(i)];
        const auto p = static_cast<Param>(i);
        float angle = (-135.0f + static_cast<float>(normFor(p)) * 270.0f) * kPi / 180.0f;
        steel::brassKnob(g, c, angle, knobScaleFor(i), 1.0f, steel::KnobAsset::AluminumBrassInsert, steel::PointerAsset::WhiteEnamel);
        const auto text = format(p, values.knobs[static_cast<size_t>(i)]);
        if (inStrip(i))
        {
            const auto r = stripReadout(c);
            g.setFont(steel::stencilFont(20.0f));
            g.setColour(juce::Colours::black);
            g.drawText(text, r.translated(0.0f, 1.0f), juce::Justification::centredLeft);
            g.setColour(steel::stencil);
            g.drawText(text, r, juce::Justification::centredLeft);
            continue;
        }
        if (p == Style)
        {
            // owner: the chosen style is the knob's title, no separate readout
            steel::engraved(g, text, { c.x - 66.0f, kLabelY - 17.0f, 132.0f, 34.0f }, 19.0f, steel::stencil, juce::Justification::centred, 0.1f);
            static const juce::StringArray characters { "TRANSPARENT", "FAST, LESS DUCKING", "SOFT CLIP, LOUDER" };
            steel::engraved(g, characters[juce::jlimit(0, 2, juce::roundToInt(values.knobs[Style]))], { c.x - 66.0f, kReadoutY - 14.0f, 132.0f, 28.0f },
                            12.0f, steel::stencilDim, juce::Justification::centred, 0.05f);
            continue;
        }
        const auto r = juce::Rectangle<float>(c.x - 64.0f, kReadoutY - 16.0f, 128.0f, 32.0f);
        g.setFont(steel::stencilFont(20.0f));
        g.setColour(juce::Colours::black);
        g.drawText(text, r.translated(0.0f, 1.0f), juce::Justification::centred);
        g.setColour(steel::stencil);
        g.drawText(text, r, juce::Justification::centred);
    }
    for (int sw = 0; sw < NumSwitches; ++sw)
    {
        const auto& options = kSwitchOptions[static_cast<size_t>(sw)];
        const int n = options.size(), selected = juce::jlimit(0, n - 1, values.switches[static_cast<size_t>(sw)]);
        const float dialTop = switchTop(sw) + 22.0f;
        for (int i = 0; i < n; ++i)
        {
            const float x = kSwitchDialX + kSwitchDialW * (0.12f + 0.76f * static_cast<float>(i) / static_cast<float>(n - 1));
            g.setFont(steel::stencilFont(13.5f).withKerningFactor(0.1f));
            g.setColour(i == selected ? juce::Colour(0xfffff6d0) : steel::stencilDim);
            g.drawText(options[i].toUpperCase(), juce::Rectangle<float>(x - 40.0f, dialTop, 80.0f, 18.0f), juce::Justification::centred);
        }
        const float spread = n == 2 ? 34.0f : 38.0f;
        const float a = (-spread + 2.0f * spread / static_cast<float>(n - 1) * static_cast<float>(selected)) * kPi / 180.0f;
        steel::lever(g, { kSwitchDialX + kSwitchDialW * 0.5f, dialTop + 46.0f }, a, 1.0f);
    }
    drawCrt(g);
    steel::meterNeedle(g, grMeter);
    steel::lamp(g, kLamp, values.on, 1.0f);
    steel::lever(g, kToggle, values.on ? 0.0f : kPi, 1.15f, values.on ? steel::LeverPosition::Up : steel::LeverPosition::Down);
    steel::lcdText(g, kLcdLeft, (outputDb <= -90.0f ? juce::String("-inf") : juce::String(outputDb, 1)) + (values.switches[TruePeak] == 1 ? " dBTP" : " dB"), 21.0f);
    steel::lcdText(g, kLcdRight, "GR " + juce::String(std::abs(grText), 1) + " dB", 21.0f);
}

void HamloRedWallPanel::timerCallback()
{
    if (!isShowing())
        return;
    float out = -120.0f, gr = 0.0f;
    if (readMeters)
        std::tie(out, gr) = readMeters();
    if (!values.on)
        gr = 0.0f;
    const float target = -std::max(0.0f, gr);
    grMeter.shown += (target - grMeter.shown) * (target < grMeter.shown ? 0.70f : 0.19f);
    grText += (target - grText) * 0.35f;
    outputDb = out;
    std::rotate(history.begin(), history.begin() + 1, history.end());
    history.back() = std::max(0.0f, gr);
    repaint();
}

void HamloRedWallPanel::settleMeters(float out, float reductionDb)
{
    outputDb = out;
    grMeter.shown = juce::jlimit(-21.0f, 0.0f, -reductionDb);
    grText = -reductionDb;
    std::rotate(history.begin(), history.begin() + 1, history.end()); // the real reading joins the history
    history.back() = std::max(0.0f, reductionDb);
    repaint();
}

juce::Rectangle<float> HamloRedWallPanel::readoutArea(Param p) const
{
    const auto c = centres[static_cast<size_t>(p)];
    return inStrip(p) ? stripReadout(c) : juce::Rectangle<float>(c.x - 64.0f, kReadoutY - 16.0f, 128.0f, 32.0f);
}

int HamloRedWallPanel::knobAt(juce::Point<float> d) const
{
    for (int i = 0; i < NumKnobs; ++i)
        if (d.getDistanceFrom(centres[static_cast<size_t>(i)]) <= (inStrip(i) ? 40.0f : 52.0f))
            return i;
    return -1;
}

bool HamloRedWallPanel::hitSwitch(juce::Point<float> d, int& which, int& option) const
{
    for (int s = 0; s < NumSwitches; ++s)
    {
        const float dialTop = switchTop(s) + 22.0f;
        if (d.y < dialTop || d.y > dialTop + 66.0f || d.x < kSwitchDialX || d.x > kSwitchDialX + kSwitchDialW)
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

bool HamloRedWallPanel::hitPower(juce::Point<float> d) const
{
    return d.getDistanceFrom(kLamp) <= 34.0f || juce::Rectangle<float>(kToggle.x - 22, kToggle.y - 30, 44, 60).contains(d);
}

void HamloRedWallPanel::mouseDown(const juce::MouseEvent& e)
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
        if (i != Style && readoutArea(static_cast<Param>(i)).contains(d))
        {
            editReadout(static_cast<Param>(i));
            return;
        }
}

void HamloRedWallPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob < 0)
        return;
    const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 200.0f) * scale;
    setNorm(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange, true);
}

void HamloRedWallPanel::mouseUp(const juce::MouseEvent&)
{
    if (dragKnob >= 0 && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
}

void HamloRedWallPanel::mouseDoubleClick(const juce::MouseEvent& e)
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

void HamloRedWallPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (const int k = knobAt(toDesign(e.position)); k >= 0)
    {
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        const double step = (k == Style ? 0.5 : (e.mods.isShiftDown() ? 0.004 : 0.02)) * (wheel.deltaY > 0 ? 1.0 : -1.0);
        setNorm(static_cast<Param>(k), normFor(static_cast<Param>(k)) + step, true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
    }
}

void HamloRedWallPanel::editReadout(Param p)
{
    const auto area = readoutArea(p).expanded(6.0f, 0.0f).transformedBy(toScreen()).toNearestInt();
    typing = std::make_unique<juce::TextEditor>();
    typing->setBounds(area);
    typing->setFont(steel::stencilFont(22.0f * scale * 1.1f));
    typing->setJustification(juce::Justification::centred);
    typing->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff120b04));
    typing->setColour(juce::TextEditor::textColourId, juce::Colour(0xffffb347));
    const bool percent = p == Link || p == Mix || p == Drive || p == Tube;
    const double shown = percent ? values.knobs[static_cast<size_t>(p)] * 100.0 : p == Threshold ? -values.knobs[Threshold] : values.knobs[static_cast<size_t>(p)];
    typing->setText(juce::String(shown, p == Lookahead ? 2 : 1), false);
    typing->selectAll();
    juce::Component::SafePointer<HamloRedWallPanel> self(this);
    const auto closeEditor = [self] { juce::MessageManager::callAsync([self] { if (self != nullptr) self->typing.reset(); }); };
    const auto commit = [this, p, percent, closeEditor]
    {
        if (typing == nullptr)
            return;
        double v = typing->getText().retainCharacters("-0123456789.").getDoubleValue();
        if (percent)
            v /= 100.0;
        if (p == Threshold)
            v = -std::abs(v); // typed as the push: "6" = +6 dB louder
        const auto& m = kMaps[static_cast<size_t>(p)];
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        values.knobs[static_cast<size_t>(p)] = juce::jlimit(std::min(m.min, m.max), std::max(m.min, m.max), v);
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
juce::String HamloRedWallPanel::helpKeyAt(juce::Point<float> local) const
{
    const auto d = toDesign(local);
    if (const int k = knobAt(d); k >= 0)
        return keyFor(static_cast<Param>(k));
    int which = -1, option = -1;
    if (hitSwitch(d, which, option))
        return keyFor(static_cast<Switch>(which));
    return {};
}

juce::Rectangle<float> HamloRedWallPanel::contentBounds() const
{
    return juce::Rectangle<float>(0.0f, 0.0f, kW, kH).transformedBy(toScreen());
}

} // namespace hamlo::ui
