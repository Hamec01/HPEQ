#include "UI/HamloUdarnikPanel.h"

#include <cmath>

namespace hamlo::ui
{
namespace
{
constexpr float kW = 1180.0f, kH = 700.0f;
constexpr float kPi = juce::MathConstants<float>::pi;
constexpr float kKnobY = 250.0f, kLabelY = 168.0f, kReadoutY = 318.0f, kKnobScale = 0.66f;
constexpr float kSwitchDialX = 66.0f, kSwitchDialW = 230.0f;
float switchTop(int i) { return 372.0f + static_cast<float>(i) * 104.0f; }
const juce::Rectangle<float> kCrtBezel { 331.0f, 385.0f, 404.0f, 238.0f };
const juce::Rectangle<float> kCrtScreen { 349.0f, 403.0f, 368.0f, 182.0f };
const juce::Point<float> kLamp { 1074.0f, 453.0f }, kToggle { 1074.0f, 559.0f };
const juce::Rectangle<float> kLcdLeft { 56.0f, 66.0f, 190.0f, 42.0f }, kLcdRight { 934.0f, 66.0f, 190.0f, 42.0f };
constexpr float kColumn = 1068.0f / HamloUdarnikPanel::NumKnobs;

struct MapInfo
{
    double min, max;
    bool logarithmic;
};
constexpr std::array<MapInfo, HamloUdarnikPanel::NumKnobs> kMaps { { { -24.0, 24.0, false }, { -24.0, 24.0, false }, { 0.0, 1.0, false },
                                                                    { 0.5, 2.0, true }, { 0.5, 2.0, true }, { 0.0, 1.0, false },
                                                                    { 0.0, 1.0, false }, { 0.0, 1.0, false }, { -24.0, 12.0, false },
                                                                    { 0.0, 2.0, false } } };
constexpr std::array<const char*, HamloUdarnikPanel::NumKnobs> kLabels { "Snap", "Body", "Speed", "Rise", "Decay", "Sens",
                                                                         "Preserve", "Mix", "Output", "Mode" };
const std::array<juce::StringArray, HamloUdarnikPanel::NumSwitches> kSwitchOptions { juce::StringArray { "Sputnik", "Vostok", "Soyuz" },
                                                                                     juce::StringArray { "Full", "Attack", "Body" } };
constexpr std::array<const char*, HamloUdarnikPanel::NumSwitches> kSwitchLabels { "Look-ahead", "Listen" };
} // namespace

const juce::StringArray& HamloUdarnikPanel::modeNames()
{
    static const juce::StringArray names { "PRAVDA", "UDARNIK", "PERESTROIKA" };
    return names;
}

const char* HamloUdarnikPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "attackDb", "sustainDb", "speed", "rise", "decay", "sensitivity", "preserve", "mix", "outputDb", "style" };
    return keys[p];
}

const char* HamloUdarnikPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "lookahead", "listen" };
    return keys[s];
}

double HamloUdarnikPanel::defaultFor(Param p)
{
    static constexpr double d[] { 0.0, 0.0, 0.5, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0 }; // = defaultModule(TransientShaper)
    return d[p];
}

juce::String HamloUdarnikPanel::format(Param p, double v)
{
    switch (p)
    {
        case Snap: case Body: case Output: return (v > 0.0 ? "+" : "") + juce::String(v, 1) + " dB";
        case Speed: return juce::String(v, 2);
        case Rise: case Decay: return juce::String::fromUTF8("\xc3\x97") + juce::String(v, 2);
        case Sensitivity: case Preserve: case Mix: return juce::String(juce::roundToInt(v * 100.0)) + " %";
        case Mode: return modeNames()[juce::jlimit(0, 2, juce::roundToInt(v))];
        case NumKnobs: break;
    }
    return {};
}

HamloUdarnikPanel::HamloUdarnikPanel()
{
    for (int i = 0; i < NumKnobs; ++i) // ten columns over the content width 56 .. 1124
        centres[static_cast<size_t>(i)] = { 56.0f + (static_cast<float>(i) + 0.5f) * kColumn, kKnobY };
    impactMeter.bezel = { 751.0f, 385.0f, 248.0f, 238.0f };
    impactMeter.min = -6.0f;
    impactMeter.max = 12.0f;
    impactMeter.redFrom = 9.0f;
    impactMeter.title = "IMPACT";
    impactMeter.titleSize = 19.0f;
    for (float v : { -6.0f, -3.0f, 0.0f, 3.0f, 6.0f, 9.0f, 12.0f })
        impactMeter.ticks.push_back({ v, true });
    impactMeter.seed = 1961;
    impactMeter.shown = 0.0f;
    gainHistory.assign(120, 0.0f);
    markHistory.assign(120, -1.0f);
    startTimerHz(30);
}

HamloUdarnikPanel::~HamloUdarnikPanel()
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

double HamloUdarnikPanel::normFor(Param p) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    const double v = juce::jlimit(m.min, m.max, values.knobs[static_cast<size_t>(p)]);
    return m.logarithmic ? std::log(v / m.min) / std::log(m.max / m.min) : (v - m.min) / (m.max - m.min);
}

double HamloUdarnikPanel::valueFor(Param p, double n) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    const double v = m.logarithmic ? m.min * std::pow(m.max / m.min, n) : m.min + (m.max - m.min) * n;
    return p == Mode ? std::round(v) : v;
}

void HamloUdarnikPanel::setNorm(Param p, double n, bool notify)
{
    const double v = valueFor(p, n);
    if (v == values.knobs[static_cast<size_t>(p)])
        return;
    values.knobs[static_cast<size_t>(p)] = v;
    if (notify && onKnobChange)
        onKnobChange(p, v);
    repaint();
}

void HamloUdarnikPanel::setValues(const Values& v)
{
    if (dragKnob >= 0)
        return;
    values = v;
    repaint();
}

juce::AffineTransform HamloUdarnikPanel::toScreen() const { return juce::AffineTransform::scale(scale).translated(offset); }
juce::Point<float> HamloUdarnikPanel::toDesign(juce::Point<float> p) const { return (p - offset) / scale; }

void HamloUdarnikPanel::resized()
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

void HamloUdarnikPanel::renderStatic(juce::Graphics& g) const
{
    steel::panel(g, kW, kH, 1961);
    steel::scratches(g, { { 19, { 256.0f, 14.0f, 82.0f, 122.0f }, 4.0f }, { 11, { 834.0f, 14.0f, 82.0f, 122.0f }, -4.0f },
                          { 5, { 236.0f, 590.0f, 66.0f, 96.0f }, -3.0f } });
    steel::nameplate(g, { 590.0f - 185.0f, 41.0f, 370.0f, 92.0f }, "HamloUdarnik", "MODEL UD-1", juce::String::fromUTF8("\xe2\x84\x96 1961"));
    steel::lcdWindow(g, kLcdLeft);
    steel::lcdWindow(g, kLcdRight);
    steel::engraved(g, "IMPACT", kLcdLeft.translated(0.0f, 44.0f).withHeight(18.0f), 12.0f, steel::stencilDim);
    steel::engraved(g, "LAST HIT", kLcdRight.translated(0.0f, 44.0f).withHeight(18.0f), 12.0f, steel::stencilDim);
    for (int i = 0; i < NumKnobs; ++i)
    {
        const auto c = centres[static_cast<size_t>(i)];
        if (i != Mode) // the MODE knob is titled with the chosen mode itself (paint)
            steel::engraved(g, juce::String(kLabels[static_cast<size_t>(i)]).toUpperCase(), { c.x - kColumn * 0.5f, kLabelY - 14.0f, kColumn, 28.0f },
                            17.0f, steel::stencil, juce::Justification::centred, 0.08f);
        steel::knobScale(g, c, kKnobScale);
    }
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
    steel::crtBezel(g, kCrtBezel, kCrtScreen, "GAIN  -  LAST 4 S", kCrtBezel.getBottom() - 16.0f);
    steel::meterFace(g, impactMeter);
    steel::engraved(g, "ON", { kLamp.x - 50.0f, 491.0f, 100.0f, 30.0f }, 20.0f);
}

void HamloUdarnikPanel::drawCrt(juce::Graphics& g) const
{
    const auto s = kCrtScreen;
    steel::crtBegin(g, s);
    // left: applied gain (dB, +-12) of the last 4 s, hit marks at the top (brightness = confidence)
    const auto left = juce::Rectangle<float>(s.getX(), s.getY(), s.getWidth() * 0.62f, s.getHeight());
    const auto yFor = [&left](float db) { return left.getCentreY() - (left.getHeight() * 0.5f - 18.0f) * juce::jlimit(-1.0f, 1.0f, db / 12.0f); };
    for (float db : { 12.0f, 6.0f, 0.0f, -6.0f, -12.0f })
    {
        g.setColour(steel::phosphorDim.withAlpha(db == 0.0f ? 0.55f : 0.3f));
        g.drawHorizontalLine(juce::roundToInt(yFor(db)), left.getX() + 30.0f, left.getRight() - 4.0f);
        steel::crtText(g, (db > 0.0f ? "+" : "") + juce::String(static_cast<int>(db)), { left.getX() + 3.0f, yFor(db) - 7.0f, 28.0f, 14.0f }, 10.5f,
                       juce::Justification::centredLeft, 0.6f);
    }
    const float x0 = left.getX() + 30.0f, w = left.getWidth() - 36.0f;
    juce::Path line;
    for (size_t i = 0; i < gainHistory.size(); ++i)
    {
        const float x = x0 + w * static_cast<float>(i) / static_cast<float>(gainHistory.size() - 1);
        const float y = yFor(gainHistory[i]);
        if (i == 0)
            line.startNewSubPath(x, y);
        else
            line.lineTo(x, y);
        if (markHistory[i] >= 0.0f)
            steel::glowDot(g, { x, left.getY() + 9.0f }, 3.0f, steel::phosphor, 0.25f + 0.75f * markHistory[i]);
    }
    steel::glow(g, line, steel::phosphor, 2.0f, 1.0f);

    // right: the last hit, input (dim) and output (bright) envelopes, 0 .. -40 dB over 160 ms
    const auto right = juce::Rectangle<float>(left.getRight() + 6.0f, s.getY() + 22.0f, s.getRight() - left.getRight() - 14.0f, s.getHeight() - 40.0f);
    g.setColour(steel::phosphorDim.withAlpha(0.4f));
    g.drawVerticalLine(juce::roundToInt(left.getRight() + 2.0f), s.getY() + 8.0f, s.getBottom() - 8.0f);
    float peak = 1.0e-9f;
    for (int i = 0; i < kHitPoints; ++i)
        peak = std::max({ peak, hitIn[static_cast<size_t>(i)], hitOut[static_cast<size_t>(i)] });
    const auto curve = [&](const std::array<float, kHitPoints>& e)
    {
        juce::Path p;
        for (int i = 0; i < kHitPoints; ++i)
        {
            const float db = juce::jlimit(-40.0f, 0.0f, 20.0f * std::log10(std::max(1.0e-9f, e[static_cast<size_t>(i)]) / peak));
            const float x = right.getX() + right.getWidth() * static_cast<float>(i) / static_cast<float>(kHitPoints - 1);
            const float y = right.getY() + right.getHeight() * (-db / 40.0f);
            if (i == 0)
                p.startNewSubPath(x, y);
            else
                p.lineTo(x, y);
        }
        return p;
    };
    if (hitSequence > 0)
    {
        steel::glow(g, curve(hitIn), steel::phosphorDim, 1.5f, 0.7f);
        steel::glow(g, curve(hitOut), steel::phosphor, 2.0f, 1.0f);
    }
    steel::crtText(g, "LAST HIT  IN / OUT", { right.getX(), s.getY() + 4.0f, right.getWidth(), 14.0f }, 10.5f, juce::Justification::centredRight, 0.7f);
    steel::crtEnd(g, s);
}

void HamloUdarnikPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    for (int i = 0; i < NumKnobs; ++i)
    {
        const auto c = centres[static_cast<size_t>(i)];
        const auto p = static_cast<Param>(i);
        const float angle = (-135.0f + static_cast<float>(normFor(p)) * 270.0f) * kPi / 180.0f;
        steel::brassKnob(g, c, angle, kKnobScale, 1.0f, steel::KnobAsset::AluminumBrassInsert, steel::PointerAsset::WhiteEnamel);
        const auto text = format(p, values.knobs[static_cast<size_t>(i)]);
        if (p == Mode)
        {
            // owner (pass 5): the chosen mode is the knob's title, no separate readout
            steel::engraved(g, text, { c.x - kColumn * 0.75f, kLabelY - 14.0f, kColumn * 1.5f, 28.0f }, 15.0f, steel::stencil,
                            juce::Justification::centred, 0.05f);
            static const juce::StringArray characters { "CLASSIC SHAPER", "PEAK-NEUTRAL PUNCH", "TRANSIENT MORPH" };
            steel::engraved(g, characters[juce::jlimit(0, 2, juce::roundToInt(values.knobs[Mode]))],
                            { c.x - kColumn * 0.75f, kReadoutY - 14.0f, kColumn * 1.5f, 28.0f }, 10.5f, steel::stencilDim, juce::Justification::centred, 0.03f);
            continue;
        }
        const auto r = juce::Rectangle<float>(c.x - kColumn * 0.5f, kReadoutY - 14.0f, kColumn, 28.0f);
        g.setFont(steel::stencilFont(18.0f));
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
        const float a = (-38.0f + 38.0f * static_cast<float>(selected)) * kPi / 180.0f;
        steel::lever(g, { kSwitchDialX + kSwitchDialW * 0.5f, dialTop + 54.0f }, a, 0.85f); // clear of the option names
    }
    drawCrt(g);
    steel::meterNeedle(g, impactMeter);
    steel::lamp(g, kLamp, values.on, 1.0f);
    steel::lever(g, kToggle, values.on ? 0.0f : kPi, 1.15f, values.on ? steel::LeverPosition::Up : steel::LeverPosition::Down);
    steel::lcdText(g, kLcdLeft, (impactText >= 0.0f ? "+" : "") + juce::String(impactText, 1) + " dB", 21.0f);
    steel::lcdText(g, kLcdRight, reading.events > 0 ? juce::String(juce::roundToInt(reading.confidence * 100.0f)) + "%  " + juce::String(juce::roundToInt(reading.strengthDb)) + " dB"
                                                    : juce::String("NO HIT"), 19.0f);
}

void HamloUdarnikPanel::take(const Reading& r)
{
    reading = r;
    const float target = values.on ? r.impactDb : 0.0f;
    impactMeter.shown += (target - impactMeter.shown) * 0.3f;
    impactText += (target - impactText) * 0.35f;
    // the history shows the larger excursion of the block's gain
    const float gain = values.on ? (std::abs(r.gainMaxDb) >= std::abs(r.gainMinDb) ? r.gainMaxDb : r.gainMinDb) : 0.0f;
    std::rotate(gainHistory.begin(), gainHistory.begin() + 1, gainHistory.end());
    gainHistory.back() = gain;
    std::rotate(markHistory.begin(), markHistory.begin() + 1, markHistory.end());
    markHistory.back() = r.events > lastEvents ? r.confidence : -1.0f;
    lastEvents = r.events;
    if (readLastHit)
    {
        std::array<float, kHitPoints> in {}, out {};
        if (const int seq = readLastHit(in, out); seq > 0 && seq != hitSequence)
        {
            hitSequence = seq;
            hitIn = in;
            hitOut = out;
        }
    }
}

void HamloUdarnikPanel::timerCallback()
{
    if (!isShowing())
        return;
    take(readMeters ? readMeters() : Reading {});
    repaint();
}

void HamloUdarnikPanel::settleMeters(const Reading& r)
{
    take(r);
    impactMeter.shown = juce::jlimit(-7.0f, 13.0f, r.impactDb);
    impactText = r.impactDb;
    repaint();
}

int HamloUdarnikPanel::knobAt(juce::Point<float> d) const
{
    for (int i = 0; i < NumKnobs; ++i)
        if (d.getDistanceFrom(centres[static_cast<size_t>(i)]) <= 44.0f)
            return i;
    return -1;
}

bool HamloUdarnikPanel::hitSwitch(juce::Point<float> d, int& which, int& option) const
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

bool HamloUdarnikPanel::hitPower(juce::Point<float> d) const
{
    return d.getDistanceFrom(kLamp) <= 34.0f || juce::Rectangle<float>(kToggle.x - 22, kToggle.y - 30, 44, 60).contains(d);
}

void HamloUdarnikPanel::mouseDown(const juce::MouseEvent& e)
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
        if (i != Mode && juce::Rectangle<float>(centres[static_cast<size_t>(i)].x - kColumn * 0.5f, kReadoutY - 14.0f, kColumn, 28.0f).contains(d))
        {
            editReadout(static_cast<Param>(i));
            return;
        }
}

void HamloUdarnikPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob < 0)
        return;
    const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 200.0f) * scale;
    setNorm(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange, true);
}

void HamloUdarnikPanel::mouseUp(const juce::MouseEvent&)
{
    if (dragKnob >= 0 && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
}

void HamloUdarnikPanel::mouseDoubleClick(const juce::MouseEvent& e)
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

void HamloUdarnikPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
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

void HamloUdarnikPanel::editReadout(Param p)
{
    const auto c = centres[static_cast<size_t>(p)];
    const auto area = juce::Rectangle<float>(c.x - kColumn * 0.5f, kReadoutY - 14.0f, kColumn, 28.0f).transformedBy(toScreen()).toNearestInt();
    typing = std::make_unique<juce::TextEditor>();
    typing->setBounds(area);
    typing->setFont(steel::stencilFont(18.0f * scale * 1.1f));
    typing->setJustification(juce::Justification::centred);
    typing->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff120b04));
    typing->setColour(juce::TextEditor::textColourId, juce::Colour(0xffffb347));
    const bool percent = p == Sensitivity || p == Preserve || p == Mix;
    const double shown = percent ? values.knobs[static_cast<size_t>(p)] * 100.0 : values.knobs[static_cast<size_t>(p)];
    typing->setText(juce::String(shown, p == Rise || p == Decay || p == Speed ? 2 : 1), false);
    typing->selectAll();
    juce::Component::SafePointer<HamloUdarnikPanel> self(this);
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
juce::String HamloUdarnikPanel::helpKeyAt(juce::Point<float> local) const
{
    const auto d = toDesign(local);
    if (const int k = knobAt(d); k >= 0)
        return keyFor(static_cast<Param>(k));
    int which = -1, option = -1;
    if (hitSwitch(d, which, option))
        return keyFor(static_cast<Switch>(which));
    return {};
}

juce::Rectangle<float> HamloUdarnikPanel::contentBounds() const
{
    return juce::Rectangle<float>(0.0f, 0.0f, kW, kH).transformedBy(toScreen());
}

} // namespace hamlo::ui
