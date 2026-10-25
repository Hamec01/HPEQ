#include "UI/HamloZastavaPanel.h"

#include <cmath>

namespace hpeq::ui
{
namespace
{
constexpr float kW = 1180.0f, kH = 780.0f;
constexpr float kPi = juce::MathConstants<float>::pi;
constexpr int kBig = 6; // THRESHOLD .. MODE: the big row; the rest: the small row
constexpr float kBigY = 245.0f, kBigLabelY = 160.0f, kBigReadoutY = 330.0f, kBigScale = 0.92f;
constexpr float kSmallY = 708.0f, kSmallLabelY = 664.0f, kSmallReadoutY = 752.0f, kSmallScale = 0.5f;
constexpr float kColumn = 1068.0f / 6.0f;
constexpr float kSwitchDialX = 66.0f, kSwitchDialW = 230.0f;
float switchTop(int i) { return 374.0f + static_cast<float>(i) * 88.0f; }
const juce::Rectangle<float> kCrtBezel { 331.0f, 382.0f, 404.0f, 238.0f };
const juce::Rectangle<float> kCrtScreen { 349.0f, 400.0f, 368.0f, 182.0f };
const juce::Point<float> kLamp { 1074.0f, 450.0f }, kToggle { 1074.0f, 556.0f };
const juce::Rectangle<float> kLcdLeft { 56.0f, 66.0f, 190.0f, 42.0f }, kLcdRight { 934.0f, 66.0f, 190.0f, 42.0f };

struct MapInfo
{
    double min, max;
    bool logarithmic;
};
constexpr std::array<MapInfo, HamloZastavaPanel::NumKnobs> kMaps { { { -90.0, 0.0, false }, { 0.0, 90.0, false }, { 0.05, 100.0, true },
                                                                    { 0.0, 500.0, false }, { 1.0, 2000.0, true }, { 0.0, 2.0, false },
                                                                    { 1.0, 100.0, true }, { 0.0, 24.0, false }, { 20.0, 10000.0, true },
                                                                    { 200.0, 20000.0, true }, { 0.0, 1.0, false }, { -24.0, 12.0, false } } };
constexpr std::array<const char*, HamloZastavaPanel::NumKnobs> kLabels { "Threshold", "Range", "Attack", "Hold", "Release", "Mode",
                                                                         "Ratio", "Hysteresis", "Key HP", "Key LP", "Mix", "Output" };
const std::array<juce::StringArray, HamloZastavaPanel::NumSwitches> kSwitchOptions { juce::StringArray { "Sputnik", "Vostok", "Soyuz" },
                                                                                     juce::StringArray { "Off", "On" },
                                                                                     juce::StringArray { "Off", "On" } };
constexpr std::array<const char*, HamloZastavaPanel::NumSwitches> kSwitchLabels { "Look-ahead", "Key listen", "Flip" };

bool isBig(int knob) { return knob < kBig; }
float knobScaleFor(int knob) { return isBig(knob) ? kBigScale : kSmallScale; }
} // namespace

const juce::StringArray& HamloZastavaPanel::modeNames()
{
    static const juce::StringArray names { "ZASTAVA", "PATRUL", "TREVOGA" };
    return names;
}

const char* HamloZastavaPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "thresholdDb", "rangeDb", "attackMs", "holdMs", "releaseMs", "style",
                                          "ratio", "hysteresisDb", "keyHpHz", "keyLpHz", "mix", "outputDb" };
    return keys[p];
}

const char* HamloZastavaPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "lookahead", "keyListen", "flip" };
    return keys[s];
}

double HamloZastavaPanel::defaultFor(Param p)
{
    static constexpr double d[] { -40.0, 40.0, 0.5, 20.0, 100.0, 0.0, 4.0, 4.0, 20.0, 20000.0, 1.0, 0.0 }; // = defaultModule(GateExpander)
    return d[p];
}

juce::String HamloZastavaPanel::format(Param p, double v)
{
    switch (p)
    {
        case Threshold: case Range: case Hysteresis: return juce::String(v, 1) + " dB";
        case Output: return (v > 0.0 ? "+" : "") + juce::String(v, 1) + " dB";
        case Attack: return juce::String(v, v < 10.0 ? 2 : 1) + " ms";
        case Hold: case Release: return juce::String(juce::roundToInt(v)) + " ms";
        case Ratio: return juce::String(v, v < 10.0 ? 1 : 0) + ":1";
        case KeyHp: return v <= 20.0 ? juce::String("OFF") : (v >= 1000.0 ? juce::String(v / 1000.0, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz");
        case KeyLp: return v >= 20000.0 ? juce::String("OFF") : (v >= 1000.0 ? juce::String(v / 1000.0, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz");
        case Mix: return juce::String(juce::roundToInt(v * 100.0)) + " %";
        case Mode: return modeNames()[juce::jlimit(0, 2, juce::roundToInt(v))];
        case NumKnobs: break;
    }
    return {};
}

HamloZastavaPanel::HamloZastavaPanel()
{
    for (int i = 0; i < NumKnobs; ++i)
    {
        const int column = isBig(i) ? i : i - kBig;
        centres[static_cast<size_t>(i)] = { 56.0f + (static_cast<float>(column) + 0.5f) * kColumn, isBig(i) ? kBigY : kSmallY };
    }
    attenuationMeter.bezel = { 751.0f, 382.0f, 248.0f, 238.0f };
    attenuationMeter.min = -40.0f;
    attenuationMeter.max = 0.0f;
    attenuationMeter.redFrom = -6.0f;
    attenuationMeter.title = "ATTENUATION";
    attenuationMeter.titleSize = 16.0f;
    for (float v : { -40.0f, -30.0f, -20.0f, -12.0f, -6.0f, -3.0f, 0.0f })
        attenuationMeter.ticks.push_back({ v, true });
    attenuationMeter.seed = 1945;
    attenuationMeter.shown = 0.0f;
    keyHistory.assign(120, -90.0f);
    shutHistory.assign(120, 0.0f);
    startTimerHz(30);
}

HamloZastavaPanel::~HamloZastavaPanel()
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

double HamloZastavaPanel::normFor(Param p) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    const double v = juce::jlimit(m.min, m.max, values.knobs[static_cast<size_t>(p)]);
    return m.logarithmic ? std::log(v / m.min) / std::log(m.max / m.min) : (v - m.min) / (m.max - m.min);
}

double HamloZastavaPanel::valueFor(Param p, double n) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    const double v = m.logarithmic ? m.min * std::pow(m.max / m.min, n) : m.min + (m.max - m.min) * n;
    return p == Mode ? std::round(v) : v;
}

void HamloZastavaPanel::setNorm(Param p, double n, bool notify)
{
    const double v = valueFor(p, n);
    if (v == values.knobs[static_cast<size_t>(p)])
        return;
    values.knobs[static_cast<size_t>(p)] = v;
    if (notify && onKnobChange)
        onKnobChange(p, v);
    repaint();
}

void HamloZastavaPanel::setValues(const Values& v)
{
    if (dragKnob >= 0)
        return;
    values = v;
    repaint();
}

juce::AffineTransform HamloZastavaPanel::toScreen() const { return juce::AffineTransform::scale(scale).translated(offset); }
juce::Point<float> HamloZastavaPanel::toDesign(juce::Point<float> p) const { return (p - offset) / scale; }

void HamloZastavaPanel::resized()
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

juce::Rectangle<float> HamloZastavaPanel::readoutArea(int knob) const
{
    const auto c = centres[static_cast<size_t>(knob)];
    return { c.x - kColumn * 0.45f, (isBig(knob) ? kBigReadoutY : kSmallReadoutY) - 14.0f, kColumn * 0.9f, 28.0f };
}

void HamloZastavaPanel::renderStatic(juce::Graphics& g) const
{
    steel::panel(g, kW, kH, 1945);
    steel::scratches(g, { { 14, { 256.0f, 14.0f, 82.0f, 122.0f }, -4.0f }, { 4, { 834.0f, 14.0f, 82.0f, 122.0f }, 4.0f } });
    steel::nameplate(g, { 590.0f - 185.0f, 41.0f, 370.0f, 92.0f }, "HamloZastava", "MODEL KPP-1", juce::String::fromUTF8("\xe2\x84\x96 1945"));
    steel::lcdWindow(g, kLcdLeft);
    steel::lcdWindow(g, kLcdRight);
    steel::engraved(g, "KEY", kLcdLeft.translated(0.0f, 44.0f).withHeight(18.0f), 12.0f, steel::stencilDim);
    steel::engraved(g, "STATE", kLcdRight.translated(0.0f, 44.0f).withHeight(18.0f), 12.0f, steel::stencilDim);
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
    // the small row sits on its own strip
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
    steel::crtBezel(g, kCrtBezel, kCrtScreen, "KEY  -  LAST 4 S", kCrtBezel.getBottom() - 16.0f);
    steel::meterFace(g, attenuationMeter);
    steel::engraved(g, "ON", { kLamp.x - 50.0f, 488.0f, 100.0f, 30.0f }, 20.0f);
}

void HamloZastavaPanel::drawCrt(juce::Graphics& g) const
{
    const auto s = kCrtScreen;
    steel::crtBegin(g, s);
    const float top = s.getY() + 12.0f, height = s.getHeight() - 24.0f;
    const auto yFor = [&](float db) { return top + height * juce::jlimit(0.0f, 1.0f, -db / 72.0f); };
    const float x0 = s.getX() + 30.0f, w = s.getWidth() - 36.0f;
    const auto xFor = [&](size_t i) { return x0 + w * static_cast<float>(i) / static_cast<float>(keyHistory.size() - 1); };
    // shut stretches: shaded columns, darker the deeper the attenuation
    for (size_t i = 0; i < shutHistory.size(); ++i)
        if (shutHistory[i] > 1.0f)
        {
            g.setColour(steel::phosphorDim.withAlpha(0.06f + 0.22f * juce::jlimit(0.0f, 1.0f, shutHistory[i] / 40.0f)));
            g.fillRect(xFor(i), top, w / static_cast<float>(keyHistory.size() - 1) + 0.5f, height);
        }
    for (float db : { 0.0f, -12.0f, -24.0f, -36.0f, -48.0f, -60.0f, -72.0f })
    {
        g.setColour(steel::phosphorDim.withAlpha(0.3f));
        g.drawHorizontalLine(juce::roundToInt(yFor(db)), x0, s.getRight() - 6.0f);
        steel::crtText(g, juce::String(static_cast<int>(db)), { s.getX() + 3.0f, yFor(db) - 7.0f, 26.0f, 14.0f }, 10.5f, juce::Justification::centredLeft, 0.6f);
    }
    // threshold (and the shut level, threshold - hysteresis)
    const float t = static_cast<float>(values.knobs[Threshold]);
    g.setColour(steel::phosphor.withAlpha(0.75f));
    g.drawLine(x0, yFor(t), s.getRight() - 6.0f, yFor(t), 1.4f);
    g.setColour(steel::phosphor.withAlpha(0.3f));
    const float shut = t - static_cast<float>(values.knobs[Hysteresis]);
    for (float x = x0; x < s.getRight() - 6.0f; x += 8.0f)
        g.drawLine(x, yFor(shut), x + 4.0f, yFor(shut), 1.0f);
    juce::Path line;
    for (size_t i = 0; i < keyHistory.size(); ++i)
    {
        const float y = yFor(keyHistory[i]);
        if (i == 0)
            line.startNewSubPath(xFor(i), y);
        else
            line.lineTo(xFor(i), y);
    }
    steel::glow(g, line, steel::phosphor, 2.0f, 1.0f);
    steel::crtText(g, "THRESHOLD " + juce::String(t, 1) + " dB", { s.getRight() - 170.0f, s.getY() + 4.0f, 162.0f, 14.0f }, 11.0f,
                   juce::Justification::centredRight, 0.85f);
    steel::crtEnd(g, s);
}

void HamloZastavaPanel::paint(juce::Graphics& g)
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
            steel::engraved(g, text, { c.x - kColumn * 0.5f, kBigLabelY - 17.0f, kColumn, 34.0f }, 22.0f, steel::stencil, juce::Justification::centred, 0.1f);
            static const juce::StringArray characters { "GATE", "SOFT EXPANDER", "DUCKER" };
            steel::engraved(g, characters[juce::jlimit(0, 2, juce::roundToInt(values.knobs[Mode]))], readoutArea(i), 12.0f, steel::stencilDim,
                            juce::Justification::centred, 0.05f);
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
        const float dialTop = switchTop(sw) + 22.0f;
        for (int i = 0; i < n; ++i)
        {
            const float x = kSwitchDialX + kSwitchDialW * (0.12f + 0.76f * static_cast<float>(i) / static_cast<float>(n - 1));
            g.setFont(steel::stencilFont(13.5f).withKerningFactor(0.1f));
            g.setColour(i == selected ? juce::Colour(0xfffff6d0) : steel::stencilDim);
            g.drawText(options[i].toUpperCase(), juce::Rectangle<float>(x - 44.0f, dialTop, 88.0f, 18.0f), juce::Justification::centred);
        }
        const float spread = n == 2 ? 34.0f : 38.0f;
        const float a = (-spread + 2.0f * spread / static_cast<float>(n - 1) * static_cast<float>(selected)) * kPi / 180.0f;
        steel::lever(g, { kSwitchDialX + kSwitchDialW * 0.5f, dialTop + 50.0f }, a, 0.85f); // clear of the option names
    }
    drawCrt(g);
    steel::meterNeedle(g, attenuationMeter);
    steel::lamp(g, kLamp, values.on, 1.0f);
    steel::lever(g, kToggle, values.on ? 0.0f : kPi, 1.15f, values.on ? steel::LeverPosition::Up : steel::LeverPosition::Down);
    steel::lcdText(g, kLcdLeft, reading.keyDb <= -90.0f ? juce::String("-inf dB") : juce::String(reading.keyDb, 1) + " dB", 21.0f);
    const auto state = values.switches[Flip] == 1 ? juce::String("FLIP") : (reading.attenuationDb < 1.0f ? juce::String("OPEN") : "SHUT -" + juce::String(juce::roundToInt(reading.attenuationDb)) + " dB");
    steel::lcdText(g, kLcdRight, values.on ? state : juce::String("OFF"), 20.0f);
}

void HamloZastavaPanel::take(const Reading& r)
{
    reading = r;
    const float target = values.on ? -std::max(0.0f, r.maxAttenuationDb) : 0.0f;
    attenuationMeter.shown += (target - attenuationMeter.shown) * (target < attenuationMeter.shown ? 0.7f : 0.25f);
    std::rotate(keyHistory.begin(), keyHistory.begin() + 1, keyHistory.end());
    keyHistory.back() = juce::jmax(-90.0f, r.keyDb);
    std::rotate(shutHistory.begin(), shutHistory.begin() + 1, shutHistory.end());
    shutHistory.back() = values.on ? r.maxAttenuationDb : 0.0f;
}

void HamloZastavaPanel::timerCallback()
{
    if (!isShowing())
        return;
    take(readMeters ? readMeters() : Reading {});
    repaint();
}

void HamloZastavaPanel::settleMeters(const Reading& r)
{
    take(r);
    attenuationMeter.shown = juce::jlimit(-41.0f, 0.0f, -r.maxAttenuationDb);
    repaint();
}

int HamloZastavaPanel::knobAt(juce::Point<float> d) const
{
    for (int i = 0; i < NumKnobs; ++i)
        if (d.getDistanceFrom(centres[static_cast<size_t>(i)]) <= (isBig(i) ? 56.0f : 32.0f))
            return i;
    return -1;
}

bool HamloZastavaPanel::hitSwitch(juce::Point<float> d, int& which, int& option) const
{
    for (int s = 0; s < NumSwitches; ++s)
    {
        const float dialTop = switchTop(s) + 22.0f;
        if (d.y < dialTop || d.y > dialTop + 62.0f || d.x < kSwitchDialX || d.x > kSwitchDialX + kSwitchDialW)
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

bool HamloZastavaPanel::hitPower(juce::Point<float> d) const
{
    return d.getDistanceFrom(kLamp) <= 34.0f || juce::Rectangle<float>(kToggle.x - 22, kToggle.y - 30, 44, 60).contains(d);
}

void HamloZastavaPanel::mouseDown(const juce::MouseEvent& e)
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

void HamloZastavaPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob < 0)
        return;
    const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 200.0f) * scale;
    setNorm(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange, true);
}

void HamloZastavaPanel::mouseUp(const juce::MouseEvent&)
{
    if (dragKnob >= 0 && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
}

void HamloZastavaPanel::mouseDoubleClick(const juce::MouseEvent& e)
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

void HamloZastavaPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
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

void HamloZastavaPanel::editReadout(Param p)
{
    const auto area = readoutArea(p).transformedBy(toScreen()).toNearestInt();
    typing = std::make_unique<juce::TextEditor>();
    typing->setBounds(area);
    typing->setFont(steel::stencilFont((isBig(p) ? 22.0f : 15.0f) * scale * 1.1f));
    typing->setJustification(juce::Justification::centred);
    typing->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff120b04));
    typing->setColour(juce::TextEditor::textColourId, juce::Colour(0xffffb347));
    const double shown = p == Mix ? values.knobs[Mix] * 100.0 : values.knobs[static_cast<size_t>(p)];
    typing->setText(juce::String(shown, p == Attack ? 2 : 1), false);
    typing->selectAll();
    juce::Component::SafePointer<HamloZastavaPanel> self(this);
    const auto closeEditor = [self] { juce::MessageManager::callAsync([self] { if (self != nullptr) self->typing.reset(); }); };
    const auto commit = [this, p, closeEditor]
    {
        if (typing == nullptr)
            return;
        double v = typing->getText().retainCharacters("-0123456789.").getDoubleValue();
        if (p == Mix)
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
