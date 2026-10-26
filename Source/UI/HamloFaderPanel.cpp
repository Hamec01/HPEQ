#include "UI/HamloFaderPanel.h"

#include <cmath>

namespace hpeq::ui
{
namespace
{
using namespace steel;

constexpr float kW = 1180.0f, kH = 700.0f;
constexpr float kPi = juce::MathConstants<float>::pi;

// Fader (design coordinates): track from +24 dB (top) to -60 dB (bottom); 0 dB at three quarters.
constexpr float kFaderX = 180.0f, kFaderTop = 178.0f, kFaderBottom = 600.0f;
const juce::Rectangle<float> kPlate { 405.0f, 41.0f, 370.0f, 92.0f };
const juce::Rectangle<float> kLcd { 940.0f, 52.0f, 184.0f, 64.0f };
const juce::Rectangle<float> kMeterIn { 330.0f, 150.0f, 330.0f, 236.0f }, kMeterOut { 690.0f, 150.0f, 330.0f, 236.0f };
const juce::Point<float> kLamp { 1074.0f, 236.0f };
constexpr std::array<float, 4> kLeverX { 395.0f, 505.0f, 650.0f, 770.0f }; // Ø L, Ø R, MUTE, SOLO
constexpr float kLeverY = 468.0f;
const juce::Point<float> kMatch { 905.0f, 466.0f };
constexpr std::array<float, 3> kKnobX { 470.0f, 680.0f, 890.0f }; // DELAY, TARGET, MIX
constexpr float kKnobY = 614.0f, kKnobLabelY = 538.0f, kKnobReadoutY = 672.0f, kKnobScale = 0.76f;

struct MapInfo
{
    double min, max;
};
constexpr std::array<MapInfo, HamloFaderPanel::NumParams> kMaps { { { -60.0, 24.0 }, { 0.0, 20.0 }, { -40.0, 0.0 }, { 0.0, 1.0 } } };
constexpr std::array<const char*, HamloFaderPanel::NumParams> kLabels { "Gain", "Delay", "Target", "Mix" };

juce::Point<float> knobCentre(int p) { return { kKnobX[static_cast<size_t>(p - 1)], kKnobY }; }
} // namespace

HamloFaderPanel::HamloFaderPanel()
{
    const auto ticks = [](std::initializer_list<float> all, std::initializer_list<float> labelled)
    {
        std::vector<std::pair<float, bool>> t;
        for (float v : all)
            t.push_back({ v, std::find(labelled.begin(), labelled.end(), v) != labelled.end() });
        return t;
    };
    for (int i = 0; i < 2; ++i)
    {
        auto& m = meters[static_cast<size_t>(i)];
        m.bezel = i == 0 ? kMeterIn : kMeterOut;
        m.min = -40.0f;
        m.max = 6.0f;
        m.redFrom = 0.0f;
        m.title = i == 0 ? "INPUT" : "OUTPUT";
        m.titleSize = 21.0f;
        m.ticks = ticks({ -40, -35, -30, -25, -20, -15, -10, -8, -6, -4, -2, 0, 2, 4, 6 }, { -40, -30, -20, -10, 0, 6 });
        m.seed = 425 + i;
        m.shown = -42.0f;
    }
    setOpaque(false);
    startTimerHz(30);
}

HamloFaderPanel::~HamloFaderPanel()
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

const char* HamloFaderPanel::keyFor(Param p)
{
    static constexpr const char* keys[] { "gainDb", "delayMs", "targetDb", "mix" };
    return keys[p];
}

const char* HamloFaderPanel::keyFor(Switch s)
{
    static constexpr const char* keys[] { "polarity", "mute", "solo" };
    return keys[s];
}

double HamloFaderPanel::defaultFor(Param p)
{
    return Values {}.knobs[static_cast<size_t>(p)];
}

juce::String HamloFaderPanel::format(Param p, double v)
{
    switch (p)
    {
        case Gain: return (v > 0.05 ? "+" : "") + juce::String(v, 1) + " dB";
        case Delay: return v < 0.005 ? juce::String("0 ms") : juce::String(v, 2) + " ms";
        case Target: return juce::String(v, 1) + " dB";
        case Mix: return juce::String(juce::roundToInt(v * 100.0)) + " %";
        case NumParams: break;
    }
    return {};
}

double HamloFaderPanel::matchGain(double targetDb, double inputLongDb) noexcept
{
    if (inputLongDb < -100.0)
        return 0.0; // nothing measured yet
    return juce::jlimit(-60.0, 24.0, targetDb - inputLongDb);
}

void HamloFaderPanel::pressMatch()
{
    const auto m = readMeters ? readMeters() : live;
    if (m.inputLongDb < -100.0f)
        return;
    if (onKnobGestureStart)
        onKnobGestureStart(Gain);
    setKnob(Gain, matchGain(values.knobs[Target], m.inputLongDb), true);
    if (onKnobGestureEnd)
        onKnobGestureEnd();
    matchFlash = 12;
}

double HamloFaderPanel::normFor(Param p) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    return (juce::jlimit(m.min, m.max, values.knobs[static_cast<size_t>(p)]) - m.min) / (m.max - m.min);
}

double HamloFaderPanel::valueFor(Param p, double n) const
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    return m.min + (m.max - m.min) * juce::jlimit(0.0, 1.0, n);
}

void HamloFaderPanel::setKnob(Param p, double v, bool notify)
{
    const auto& m = kMaps[static_cast<size_t>(p)];
    values.knobs[static_cast<size_t>(p)] = juce::jlimit(m.min, m.max, v);
    if (notify && onKnobChange)
        onKnobChange(p, values.knobs[static_cast<size_t>(p)]);
    repaint();
}

void HamloFaderPanel::setSwitch(Switch s, int v)
{
    values.switches[static_cast<size_t>(s)] = v;
    if (onSwitchChange)
        onSwitchChange(s, v);
    repaint();
}

void HamloFaderPanel::setValues(const Values& v)
{
    if (dragKnob >= 0 || dragFader)
        return;
    values = v;
    repaint();
}

juce::AffineTransform HamloFaderPanel::toScreen() const { return juce::AffineTransform::scale(scale).translated(offset); }
juce::Point<float> HamloFaderPanel::toDesign(juce::Point<float> p) const { return (p - offset) / scale; }

// fader law: 0 dB at three quarters of the travel; below it the dB scale opens up (square law), above it linear
float HamloFaderPanel::faderY(double db) const
{
    db = juce::jlimit(-60.0, 24.0, db);
    const double n = db >= 0.0 ? 0.75 + 0.25 * db / 24.0 : 0.75 * std::pow((db + 60.0) / 60.0, 2.0);
    return kFaderBottom - static_cast<float>(n) * (kFaderBottom - kFaderTop);
}

double HamloFaderPanel::faderDb(float y) const
{
    const double n = juce::jlimit(0.0, 1.0, static_cast<double>((kFaderBottom - y) / (kFaderBottom - kFaderTop)));
    return n >= 0.75 ? 24.0 * (n - 0.75) / 0.25 : 60.0 * std::sqrt(n / 0.75) - 60.0;
}

void HamloFaderPanel::resized()
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

void HamloFaderPanel::renderStatic(juce::Graphics& g) const
{
    panel(g, kW, kH, 425);
    scratches(g, { { 5, { 250.0f, 16.0f, 80.0f, 118.0f }, -3.0f }, { 9, { 836.0f, 18.0f, 80.0f, 118.0f }, 5.0f } });
    nameplate(g, kPlate, "HamloFader", juce::String::fromUTF8("MODEL \xd0\xa0\xd0\x93-1"), juce::String::fromUTF8("\xe2\x84\x96 0425"));
    lcdWindow(g, kLcd);

    // fader: engraved scale, the slot
    engraved(g, "GAIN", { kFaderX - 80.0f, 140.0f, 160.0f, 28.0f }, 22.0f);
    for (float db : { 24.0f, 18.0f, 12.0f, 6.0f, 3.0f, 0.0f, -3.0f, -6.0f, -10.0f, -15.0f, -20.0f, -30.0f, -40.0f, -60.0f })
    {
        const float y = faderY(db);
        const bool major = db == 0.0f || std::fmod(std::abs(db), 6.0f) == 0.0f || db == -10.0f || db <= -20.0f;
        g.setColour(juce::Colour(0xffefe3bd).withAlpha(db == 0.0f ? 1.0f : 0.8f));
        g.drawLine(kFaderX - 64.0f, y, kFaderX - (major ? 40.0f : 48.0f), y, db == 0.0f ? 2.6f : 1.5f);
        g.drawLine(kFaderX + (major ? 40.0f : 48.0f), y, kFaderX + 64.0f, y, db == 0.0f ? 2.6f : 1.5f);
        if (db == 3.0f || db == -3.0f || db == 18.0f || db == -15.0f)
            continue; // minor ticks: no numbers, so the scale around 0 dB stays readable
        engraved(g, (db > 0 ? "+" : "") + juce::String(static_cast<int>(db)), { kFaderX - 124.0f, y - 9.0f, 54.0f, 18.0f }, 13.0f, stencil,
                 juce::Justification::centredRight, 0.05f);
    }
    const auto slot = juce::Rectangle<float>(kFaderX - 6.0f, kFaderTop - 14.0f, 12.0f, kFaderBottom - kFaderTop + 28.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.fillRoundedRectangle(slot.translated(0.0f, 1.5f), 6.0f);
    g.setColour(juce::Colour(0xff050505));
    g.fillRoundedRectangle(slot, 6.0f);

    meterFace(g, meters[0]);
    meterFace(g, meters[1]);

    const std::array<const char*, 4> leverLabels { "PHASE L", "PHASE R", "MUTE", "SOLO" };
    for (int i = 0; i < 4; ++i)
        engraved(g, leverLabels[static_cast<size_t>(i)], { kLeverX[static_cast<size_t>(i)] - 55.0f, 412.0f, 110.0f, 22.0f }, 16.0f);
    engraved(g, "MATCH", { kMatch.x - 60.0f, 412.0f, 120.0f, 22.0f }, 17.0f);
    engraved(g, "ON", { kLamp.x - 40.0f, kLamp.y + 34.0f, 80.0f, 24.0f }, 18.0f);
    g.setColour(juce::Colours::black);
    g.drawHorizontalLine(516, 330.0f, 1124.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawHorizontalLine(515, 330.0f, 1124.0f);
    for (int p = Delay; p < NumParams; ++p)
    {
        const auto c = knobCentre(p);
        engraved(g, juce::String(kLabels[static_cast<size_t>(p)]).toUpperCase(), { c.x - 80.0f, kKnobLabelY - 14.0f, 160.0f, 28.0f }, 20.0f);
        knobScale(g, c, kKnobScale);
    }
}

void HamloFaderPanel::drawFader(juce::Graphics& g) const
{
    const float y = faderY(values.knobs[Gain]);
    const auto cap = juce::Rectangle<float>(kFaderX - 46.0f, y - 20.0f, 92.0f, 40.0f);
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(cap.translated(3.0f, 7.0f), 5.0f);
    juce::ColourGradient body(juce::Colour(0xffd9d9d2), cap.getX(), cap.getY(), juce::Colour(0xff4b4b47), cap.getX(), cap.getBottom(), false);
    body.addColour(0.45, juce::Colour(0xff9c9c95));
    body.addColour(0.55, juce::Colour(0xff6a6a64));
    g.setGradientFill(body);
    g.fillRoundedRectangle(cap, 5.0f);
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    for (float x = cap.getX() + 2.0f; x < cap.getRight() - 2.0f; x += 2.0f)
        g.drawVerticalLine(juce::roundToInt(x), cap.getY() + 2.0f, cap.getBottom() - 2.0f);
    g.setColour(juce::Colour(0xff151514));
    g.fillRect(cap.getX() + 8.0f, y - 1.5f, cap.getWidth() - 16.0f, 3.0f); // the index line
    g.setColour(juce::Colour(0xffb3261e));
    g.fillRect(cap.getX() + 8.0f, y - 0.6f, 10.0f, 1.2f);
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.drawRoundedRectangle(cap, 5.0f, 1.2f);

    const auto r = juce::Rectangle<float>(kFaderX - 80.0f, kFaderBottom + 26.0f, 160.0f, 30.0f);
    const auto text = values.switches[Mute] > 0 ? juce::String("MUTED") : format(Gain, values.knobs[Gain]);
    g.setFont(stencilFont(24.0f));
    g.setColour(juce::Colours::black);
    g.drawText(text, r.translated(0.0f, 1.0f), juce::Justification::centred);
    g.setColour(values.switches[Mute] > 0 ? juce::Colour(0xffff6a50) : stencil);
    g.drawText(text, r, juce::Justification::centred);
}

void HamloFaderPanel::drawToggles(juce::Graphics& g) const
{
    const int pol = values.switches[Polarity];
    const std::array<bool, 4> on { (pol & 1) != 0, (pol & 2) != 0, values.switches[Mute] > 0, values.switches[Solo] > 0 };
    for (int i = 0; i < 4; ++i)
    {
        const juce::Point<float> c(kLeverX[static_cast<size_t>(i)], kLeverY);
        lever(g, c, on[static_cast<size_t>(i)] ? 0.0f : kPi, 0.8f);
        const auto lampColour = i == 2 ? juce::Colour(0xffff3b2d) : i == 3 ? juce::Colour(0xffffb020) : juce::Colour(0xff57d8ff);
        lamp(g, { c.x + 40.0f, c.y - 18.0f }, on[static_cast<size_t>(i)], 0.3f, lampColour);
    }
    // MATCH: a round push button; it glows for a moment when it sets the gain
    const float r = 28.0f;
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillEllipse(kMatch.x - r, kMatch.y - r + 5.0f, 2.0f * r, 2.0f * r);
    g.setColour(juce::Colour(0xff111111));
    g.fillEllipse(kMatch.x - r, kMatch.y - r, 2.0f * r, 2.0f * r);
    const bool lit = matchFlash > 0;
    juce::ColourGradient cap(lit ? juce::Colour(0xfffff0a0) : juce::Colour(0xffe2c06c), kMatch.x - 8.0f, kMatch.y - 10.0f,
                             lit ? juce::Colour(0xffb07a10) : juce::Colour(0xff5d4413), kMatch.x + 18.0f, kMatch.y + 20.0f, true);
    g.setGradientFill(cap);
    g.fillEllipse(kMatch.x - r + 5.0f, kMatch.y - r + 5.0f, 2.0f * r - 10.0f, 2.0f * r - 10.0f);
    g.setFont(stencilFont(11.0f).withKerningFactor(0.1f));
    g.setColour(juce::Colour(0xff2a2010));
    g.drawText(juce::String::fromUTF8("\xe2\x86\x92 ") + format(Target, values.knobs[Target]), juce::Rectangle<float>(kMatch.x - 60.0f, kMatch.y + 34.0f, 120.0f, 16.0f),
               juce::Justification::centred);
    g.setColour(stencilDim);
    g.drawText(juce::String::fromUTF8("\xe2\x86\x92 ") + format(Target, values.knobs[Target]), juce::Rectangle<float>(kMatch.x - 60.0f, kMatch.y + 33.0f, 120.0f, 16.0f),
               juce::Justification::centred);
}

void HamloFaderPanel::drawLcd(juce::Graphics& g) const
{
    const auto db = [](float v) { return v < -100.0f ? juce::String("--") : juce::String(v, 1); };
    const juce::String line1 = "IN " + db(live.inputDb) + " OUT " + db(live.outputDb);
    juce::String line2;
    if (values.switches[Mute] > 0)
        line2 = "MUTE";
    else
        line2 = "GAIN " + format(Gain, values.knobs[Gain]);
    if (values.switches[Solo] > 0)
        line2 << " SOLO";
    if (values.switches[Polarity] != 0)
        line2 << juce::String::fromUTF8(" \xc3\x98");
    lcdText(g, kLcd.withHeight(kLcd.getHeight() * 0.5f).translated(0.0f, 3.0f), line1, line1.length() > 17 ? 14.0f : 16.0f);
    lcdText(g, kLcd.withTrimmedTop(kLcd.getHeight() * 0.5f).translated(0.0f, -3.0f), line2, line2.length() > 15 ? 15.0f : 17.0f);
}

void HamloFaderPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    drawFader(g);
    meterNeedle(g, meters[0]);
    meterNeedle(g, meters[1]);
    drawToggles(g);
    lamp(g, kLamp, values.on, 0.75f);
    drawLcd(g);
    for (int p = Delay; p < NumParams; ++p)
    {
        const auto c = knobCentre(p);
        const float angle = (-135.0f + static_cast<float>(normFor(static_cast<Param>(p))) * 270.0f) * kPi / 180.0f;
        brassKnob(g, c, angle, kKnobScale, 1.0f, KnobAsset::AluminumBrassInsert, PointerAsset::WhiteEnamel);
        const auto r = juce::Rectangle<float>(c.x - 80.0f, kKnobReadoutY - 13.0f, 160.0f, 26.0f);
        const auto text = format(static_cast<Param>(p), values.knobs[static_cast<size_t>(p)]);
        g.setFont(stencilFont(21.0f));
        g.setColour(juce::Colours::black);
        g.drawText(text, r.translated(0.0f, 1.0f), juce::Justification::centred);
        g.setColour(stencil);
        g.drawText(text, r, juce::Justification::centred);
    }
}

void HamloFaderPanel::timerCallback()
{
    if (!isShowing())
        return;
    Meters m;
    if (readMeters)
        m = readMeters();
    if (!values.on)
        m = {};
    live = m;
    const std::array<float, 2> targets { juce::jlimit(-60.0f, 9.0f, m.inputDb), juce::jlimit(-60.0f, 9.0f, m.outputDb) };
    for (size_t i = 0; i < meters.size(); ++i)
        meters[i].shown += (targets[i] - meters[i].shown) * (targets[i] > meters[i].shown ? 0.5f : 0.15f);
    if (matchFlash > 0)
        --matchFlash;
    repaint();
}

void HamloFaderPanel::settleMeters(const Meters& m)
{
    live = m;
    meters[0].shown = juce::jlimit(-42.0f, 7.0f, m.inputDb);
    meters[1].shown = juce::jlimit(-42.0f, 7.0f, m.outputDb);
    repaint();
}

int HamloFaderPanel::knobAt(juce::Point<float> d) const
{
    for (int p = Delay; p < NumParams; ++p)
        if (d.getDistanceFrom(knobCentre(p)) <= 60.0f * kKnobScale)
            return p;
    return -1;
}

void HamloFaderPanel::mouseDown(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    if (std::abs(d.x - kFaderX) <= 50.0f && d.y >= kFaderTop - 24.0f && d.y <= kFaderBottom + 24.0f)
    {
        dragFader = true;
        if (onKnobGestureStart)
            onKnobGestureStart(Gain);
        const float capY = faderY(values.knobs[Gain]);
        if (std::abs(d.y - capY) > 22.0f) // a click on the track jumps the cap there
            setKnob(Gain, faderDb(d.y), true);
        dragStartY = e.position.y;
        dragStartNorm = faderY(values.knobs[Gain]);
        return;
    }
    if (const int k = knobAt(d); k >= 0)
    {
        dragKnob = k;
        dragStartNorm = normFor(static_cast<Param>(k));
        dragStartY = e.position.y;
        if (onKnobGestureStart)
            onKnobGestureStart(static_cast<Param>(k));
        return;
    }
    for (int i = 0; i < 4; ++i)
        if (juce::Rectangle<float>(kLeverX[static_cast<size_t>(i)] - 30.0f, kLeverY - 40.0f, 80.0f, 70.0f).contains(d))
        {
            if (i < 2)
                setSwitch(Polarity, values.switches[Polarity] ^ (1 << i));
            else
                setSwitch(i == 2 ? Mute : Solo, values.switches[i == 2 ? Mute : Solo] > 0 ? 0 : 1);
            return;
        }
    if (d.getDistanceFrom(kMatch) <= 30.0f)
    {
        pressMatch();
        return;
    }
    if (d.getDistanceFrom(kLamp) <= 28.0f)
    {
        values.on = !values.on;
        if (onPowerChange)
            onPowerChange(values.on);
        repaint();
        return;
    }
    if (juce::Rectangle<float>(kFaderX - 80.0f, kFaderBottom + 26.0f, 160.0f, 30.0f).contains(d))
    {
        editReadout(Gain);
        return;
    }
    for (int p = Delay; p < NumParams; ++p)
        if (juce::Rectangle<float>(knobCentre(p).x - 80.0f, kKnobReadoutY - 13.0f, 160.0f, 26.0f).contains(d))
        {
            editReadout(static_cast<Param>(p));
            return;
        }
}

void HamloFaderPanel::mouseDrag(const juce::MouseEvent& e)
{
    const float fine = e.mods.isShiftDown() ? 0.25f : 1.0f;
    if (dragFader)
    {
        setKnob(Gain, faderDb(static_cast<float>(dragStartNorm) + (e.position.y - dragStartY) / scale * fine), true);
        return;
    }
    if (dragKnob >= 0)
    {
        const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 200.0f) * scale;
        setKnob(static_cast<Param>(dragKnob), valueFor(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange), true);
    }
}

void HamloFaderPanel::mouseUp(const juce::MouseEvent&)
{
    if ((dragKnob >= 0 || dragFader) && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
    dragFader = false;
}

void HamloFaderPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    int p = knobAt(d);
    if (p < 0 && std::abs(d.x - kFaderX) <= 50.0f && d.y >= kFaderTop - 24.0f && d.y <= kFaderBottom + 24.0f)
        p = Gain;
    if (p < 0)
        return;
    if (onKnobGestureStart)
        onKnobGestureStart(static_cast<Param>(p));
    setKnob(static_cast<Param>(p), defaultFor(static_cast<Param>(p)), true);
    if (onKnobGestureEnd)
        onKnobGestureEnd();
}

void HamloFaderPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto d = toDesign(e.position);
    const double dir = wheel.deltaY > 0 ? 1.0 : -1.0;
    int p = knobAt(d);
    if (p < 0 && std::abs(d.x - kFaderX) <= 70.0f && d.y >= kFaderTop - 24.0f && d.y <= kFaderBottom + 24.0f)
        p = Gain;
    if (p < 0)
        return;
    if (onKnobGestureStart)
        onKnobGestureStart(static_cast<Param>(p));
    if (p == Gain)
        setKnob(Gain, values.knobs[Gain] + (e.mods.isShiftDown() ? 0.1 : 0.5) * dir, true);
    else
        setKnob(static_cast<Param>(p), valueFor(static_cast<Param>(p), normFor(static_cast<Param>(p)) + (e.mods.isShiftDown() ? 0.004 : 0.02) * dir), true);
    if (onKnobGestureEnd)
        onKnobGestureEnd();
}

void HamloFaderPanel::editReadout(Param p)
{
    const auto area = (p == Gain ? juce::Rectangle<float>(kFaderX - 70.0f, kFaderBottom + 26.0f, 140.0f, 30.0f)
                                 : juce::Rectangle<float>(knobCentre(p).x - 70.0f, kKnobReadoutY - 14.0f, 140.0f, 28.0f))
                          .transformedBy(toScreen())
                          .toNearestInt();
    typing = std::make_unique<juce::TextEditor>();
    typing->setBounds(area);
    typing->setFont(stencilFont(20.0f * scale * 1.1f));
    typing->setJustification(juce::Justification::centred);
    typing->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff120b04));
    typing->setColour(juce::TextEditor::textColourId, juce::Colour(0xffffb347));
    const double v = values.knobs[static_cast<size_t>(p)];
    typing->setText(p == Mix ? juce::String(juce::roundToInt(v * 100.0)) : juce::String(v, 2), false);
    typing->selectAll();
    juce::Component::SafePointer<HamloFaderPanel> self(this);
    const auto closeEditor = [self] { juce::MessageManager::callAsync([self] { if (self != nullptr) self->typing.reset(); }); };
    const auto commit = [this, p, closeEditor]
    {
        if (typing == nullptr)
            return;
        double entered = typing->getText().retainCharacters("-0123456789.").getDoubleValue();
        if (p == Mix)
            entered /= 100.0;
        if (onKnobGestureStart)
            onKnobGestureStart(p);
        setKnob(p, entered, true);
        if (onKnobGestureEnd)
            onKnobGestureEnd();
        closeEditor();
    };
    typing->onReturnKey = commit;
    typing->onFocusLost = commit;
    typing->onEscapeKey = closeEditor;
    addAndMakeVisible(*typing);
    typing->grabKeyboardFocus();
}
juce::String HamloFaderPanel::helpKeyAt(juce::Point<float> local) const
{
    const auto d = toDesign(local);
    if (const int k = knobAt(d); k >= 0)
        return keyFor(static_cast<Param>(k));
    return {};
}

juce::Rectangle<float> HamloFaderPanel::contentBounds() const
{
    return juce::Rectangle<float>(0.0f, 0.0f, kW, kH).transformedBy(toScreen());
}

} // namespace hpeq::ui
