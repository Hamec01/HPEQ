#include "UI/HamloTochkaPanel.h"

#include <cmath>

#include "UI/EqShapeIcons.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
namespace
{
using namespace steel;

constexpr float kW = 1100.0f, kH = 340.0f;
constexpr float kPi = juce::MathConstants<float>::pi;

const juce::Rectangle<float> kPlate { 18.0f, 14.0f, 272.0f, 82.0f };
const juce::Rectangle<float> kShapeWindow { 18.0f, 108.0f, 272.0f, 66.0f };
const juce::Rectangle<float> kSlopeWindow { 18.0f, 196.0f, 130.0f, 40.0f };
const juce::Rectangle<float> kChannelWindow { 160.0f, 196.0f, 130.0f, 40.0f };
const juce::Point<float> kOnLamp { 52.0f, 296.0f }, kSolo { 146.0f, 296.0f };
const juce::Rectangle<float> kDelete { 208.0f, 278.0f, 82.0f, 36.0f };
const juce::Point<float> kDynLever { 742.0f, 64.0f }, kDynLamp { 786.0f, 50.0f };
const juce::Rectangle<float> kGainLcd { 928.0f, 24.0f, 152.0f, 52.0f };
const juce::Rectangle<float> kChooser { 18.0f, 14.0f, 1064.0f, 312.0f };

struct KnobInfo
{
    float x, y, s;
    double min, max;
    bool log;
    const char* label;
};
const std::array<KnobInfo, HamloTochkaPanel::NumParams> kKnobs { {
    { 380.0f, 182.0f, 0.62f, 10.0, 22000.0, true, "FREQ" },
    { 520.0f, 182.0f, 0.72f, -24.0, 24.0, false, "GAIN" },
    { 660.0f, 182.0f, 0.62f, 0.1, 18.0, true, "Q" },
    { 790.0f, 160.0f, 0.42f, -60.0, 0.0, false, "THRESH" },
    { 895.0f, 160.0f, 0.42f, 1.0, 20.0, true, "RATIO" },
    { 1000.0f, 160.0f, 0.42f, -24.0, 24.0, false, "RANGE" },
    { 842.0f, 270.0f, 0.42f, 0.1, 200.0, true, "ATTACK" },
    { 947.0f, 270.0f, 0.42f, 5.0, 2000.0, true, "RELEASE" },
} };

const std::array<EqShape, 9> kShapes { EqShape::Bell, EqShape::LowShelf, EqShape::HighShelf, EqShape::LowCut, EqShape::HighCut,
                                       EqShape::Notch, EqShape::BandPass, EqShape::Tilt, EqShape::AllPass };
const std::array<int, 8> kSlopes { 6, 12, 18, 24, 36, 48, 72, 96 };
const std::array<ChannelMode, 5> kChannels { ChannelMode::Stereo, ChannelMode::Left, ChannelMode::Right, ChannelMode::Mid, ChannelMode::Side };
const juce::Colour kInk { 0xffffb347 }, kPhosphor { 0xff6dff9a };

juce::Rectangle<float> chooserTile(int i)
{
    const float w = (kChooser.getWidth() - 40.0f) / 3.0f, h = (kChooser.getHeight() - 40.0f) / 3.0f;
    return { kChooser.getX() + 10.0f + static_cast<float>(i % 3) * (w + 10.0f), kChooser.getY() + 10.0f + static_cast<float>(i / 3) * (h + 10.0f), w, h };
}

bool isGain(EqShape s) { return s == EqShape::Bell || s == EqShape::LowShelf || s == EqShape::HighShelf || s == EqShape::Tilt; }
bool isCut(EqShape s) { return s == EqShape::LowCut || s == EqShape::HighCut; }

void drawShape(juce::Graphics& g, EqShape shape, juce::Rectangle<float> r, juce::Colour ink, float width)
{
    juce::Path twist;
    const auto p = eqShapePath(shape, r, &twist);
    g.setColour(ink.withAlpha(0.18f));
    g.strokePath(p, juce::PathStrokeType(width * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(ink);
    g.strokePath(p, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (!twist.isEmpty())
    {
        g.setColour(ink.withAlpha(0.55f));
        g.strokePath(twist, juce::PathStrokeType(width * 0.6f));
    }
}

void shadowText(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::FontOptions font, juce::Colour colour)
{
    g.setFont(font);
    g.setColour(juce::Colours::black);
    g.drawText(text, r.translated(0.0f, 1.0f), juce::Justification::centred);
    g.setColour(colour);
    g.drawText(text, r, juce::Justification::centred);
}

// Headphones, the listen symbol.
void headphones(juce::Graphics& g, juce::Point<float> c, float s, juce::Colour colour)
{
    juce::Path arc;
    arc.addCentredArc(c.x, c.y + 2.0f * s, 9.0f * s, 9.0f * s, 0.0f, -kPi * 0.5f, kPi * 0.5f, true);
    g.setColour(colour);
    g.strokePath(arc, juce::PathStrokeType(2.2f * s));
    g.fillRoundedRectangle(c.x - 11.0f * s, c.y + 1.0f * s, 5.0f * s, 9.0f * s, 1.5f * s);
    g.fillRoundedRectangle(c.x + 6.0f * s, c.y + 1.0f * s, 5.0f * s, 9.0f * s, 1.5f * s);
}
} // namespace

HamloTochkaPanel::HamloTochkaPanel()
{
    setOpaque(false);
    startTimerHz(30);
}

HamloTochkaPanel::~HamloTochkaPanel()
{
    stopTimer();
}

double HamloTochkaPanel::valueOf(const EqNodeState& n, Param p)
{
    switch (p)
    {
        case Freq: return n.frequencyHz;
        case Gain: return n.gainDb;
        case Q: return n.q;
        case Thresh: return n.thresholdDb;
        case Ratio: return n.ratio;
        case Range: return n.rangeDb;
        case Attack: return n.attackMs;
        case Release: return n.releaseMs;
        case NumParams: break;
    }
    return 0.0;
}

void HamloTochkaPanel::setValueOf(EqNodeState& n, Param p, double v)
{
    switch (p)
    {
        case Freq: n.frequencyHz = v; break;
        case Gain: n.gainDb = v; break;
        case Q: n.q = v; break;
        case Thresh: n.thresholdDb = v; break;
        case Ratio: n.ratio = v; break;
        case Range: n.rangeDb = v; break;
        case Attack: n.attackMs = v; break;
        case Release: n.releaseMs = v; break;
        case NumParams: break;
    }
}

juce::String HamloTochkaPanel::format(Param p, double v)
{
    switch (p)
    {
        case Freq: return formatHz(v);
        case Gain: case Thresh: case Range: return (v > 0.05 ? "+" : "") + juce::String(v, 1) + " dB";
        case Q: return juce::String(v, 2);
        case Ratio: return juce::String(v, v < 10.0 ? 1 : 0) + ":1";
        case Attack: case Release: return v < 10.0 ? juce::String(v, 1) + " ms" : juce::String(juce::roundToInt(v)) + " ms";
        case NumParams: break;
    }
    return {};
}

bool HamloTochkaPanel::enabledFor(const EqNodeState& n, Param p)
{
    switch (p)
    {
        case Gain: return isGain(n.shape);
        case Q: return n.shape != EqShape::LowCut && n.shape != EqShape::HighCut && n.shape != EqShape::Tilt;
        case Thresh: case Ratio: case Range: case Attack: case Release: return n.dynamic && isGain(n.shape);
        default: return true;
    }
}

double HamloTochkaPanel::normFor(Param p) const
{
    const auto& k = kKnobs[static_cast<size_t>(p)];
    const double v = juce::jlimit(k.min, k.max, valueOf(values.node, p));
    return k.log ? std::log(v / k.min) / std::log(k.max / k.min) : (v - k.min) / (k.max - k.min);
}

double HamloTochkaPanel::valueFor(Param p, double n) const
{
    const auto& k = kKnobs[static_cast<size_t>(p)];
    n = juce::jlimit(0.0, 1.0, n);
    return k.log ? k.min * std::pow(k.max / k.min, n) : k.min + (k.max - k.min) * n;
}

void HamloTochkaPanel::change(const juce::String& what)
{
    if (onChange)
        onChange(values.node, what);
    repaint();
}

void HamloTochkaPanel::setParam(Param p, double v)
{
    const auto& k = kKnobs[static_cast<size_t>(p)];
    setValueOf(values.node, p, juce::jlimit(k.min, k.max, v));
    change(juce::String(k.label).toLowerCase());
}

void HamloTochkaPanel::setValues(const Values& v)
{
    if (dragKnob >= 0)
        return;
    values = v;
    repaint();
}

void HamloTochkaPanel::chooseShape(EqShape shape)
{
    if (onGestureStart)
        onGestureStart("Shape");
    values.node.shape = shape;
    if (!isGain(shape))
        values.node.gainDb = 0.0;
    shapesOpen = false;
    change("Shape");
    if (onGestureEnd)
        onGestureEnd();
}

void HamloTochkaPanel::pressSolo()
{
    values.node.solo = !values.node.solo;
    if (onSolo)
        onSolo(values.node.solo);
    repaint();
}

void HamloTochkaPanel::setDynamic(bool on)
{
    if (onGestureStart)
        onGestureStart("Dynamic");
    values.node.dynamic = on;
    change("Dynamic");
    if (onGestureEnd)
        onGestureEnd();
}

void HamloTochkaPanel::openShapes(bool open)
{
    shapesOpen = open;
    repaint();
}

void HamloTochkaPanel::settleDynamicGain(float db)
{
    dynamicGain = db;
    repaint();
}

juce::AffineTransform HamloTochkaPanel::toScreen() const { return juce::AffineTransform::scale(scale).translated(offset); }
juce::Point<float> HamloTochkaPanel::toDesign(juce::Point<float> p) const { return (p - offset) / scale; }

void HamloTochkaPanel::resized()
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
}

void HamloTochkaPanel::renderStatic(juce::Graphics& g) const
{
    panel(g, kW, kH, 777);
    scratches(g, { { 11, { 300.0f, 250.0f, 70.0f, 70.0f }, -6.0f }, { 19, { 700.0f, 270.0f, 60.0f, 56.0f }, 4.0f } }, 0.7f);
    lcdWindow(g, kShapeWindow);
    lcdWindow(g, kSlopeWindow);
    lcdWindow(g, kChannelWindow);
    lcdWindow(g, kGainLcd);
    engraved(g, "SHAPE", { kShapeWindow.getX(), kShapeWindow.getBottom() + 2.0f, 80.0f, 14.0f }, 10.5f, stencil, juce::Justification::centredLeft);
    engraved(g, "SLOPE", { kSlopeWindow.getX(), kSlopeWindow.getBottom() + 2.0f, kSlopeWindow.getWidth(), 14.0f }, 10.5f);
    engraved(g, "CHANNEL", { kChannelWindow.getX(), kChannelWindow.getBottom() + 2.0f, kChannelWindow.getWidth(), 14.0f }, 10.5f);
    engraved(g, "ON", { kOnLamp.x - 30.0f, kOnLamp.y + 22.0f, 60.0f, 14.0f }, 11.0f);
    engraved(g, "SOLO", { kSolo.x - 30.0f, kSolo.y + 22.0f, 60.0f, 14.0f }, 11.0f);
    engraved(g, "DYNAMIC", { 700.0f, 10.0f, 150.0f, 18.0f }, 14.0f);
    // a groove between the point and the dynamics
    g.setColour(juce::Colours::black);
    g.drawVerticalLine(712, 20.0f, kH - 20.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawVerticalLine(713, 20.0f, kH - 20.0f);
    for (int p = 0; p < NumParams; ++p)
    {
        const auto& k = kKnobs[static_cast<size_t>(p)];
        knobScale(g, { k.x, k.y }, k.s);
        engraved(g, k.label, { k.x - 50.0f, k.y - k.s * 60.0f - 22.0f, 100.0f, 16.0f }, p <= Q ? 14.0f : 11.0f);
    }
}

void HamloTochkaPanel::drawShapeChooser(juce::Graphics& g) const
{
    g.setColour(juce::Colour(0xf0070807));
    g.fillRoundedRectangle(kChooser, 8.0f);
    g.setColour(juce::Colours::black);
    g.drawRoundedRectangle(kChooser, 8.0f, 2.0f);
    for (size_t i = 0; i < kShapes.size(); ++i)
    {
        const auto r = chooserTile(static_cast<int>(i));
        const bool current = kShapes[i] == values.node.shape;
        lcdWindow(g, r);
        drawShape(g, kShapes[i], r.reduced(14.0f, 12.0f).withTrimmedRight(r.getWidth() * 0.42f), current ? kPhosphor : kInk, 2.0f);
        const auto label = r.withTrimmedLeft(r.getWidth() * 0.58f);
        g.setFont(monoFont(17.0f).withStyle("Bold"));
        g.setColour((current ? kPhosphor : kInk).withAlpha(0.95f));
        g.drawText(shapeName(kShapes[i]).toUpperCase(), label, juce::Justification::centredLeft);
    }
}

void HamloTochkaPanel::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(toScreen());
    const auto& n = values.node;

    nameplate(g, kPlate, "HamloTochka", juce::String::fromUTF8("EQ ") + juce::String(values.number) + juce::String::fromUTF8(" \xc2\xb7 \xd0\xa2\xd0\xa7-1"),
              juce::String::fromUTF8("\xe2\x84\x96 ") + juce::String(values.number).paddedLeft('0', 4));

    // SHAPE: the picture and the name
    drawShape(g, n.shape, kShapeWindow.reduced(12.0f, 10.0f).withWidth(110.0f), kInk, 2.0f);
    lcdText(g, kShapeWindow.withTrimmedLeft(130.0f), shapeName(n.shape).toUpperCase() + juce::String::fromUTF8(" \xe2\x96\xbe"), 17.0f,
            juce::Justification::centredLeft);
    lcdText(g, kSlopeWindow, isCut(n.shape) ? juce::String(n.slopeDbPerOct) + " dB/oct" : juce::String("--"), 15.0f);
    lcdText(g, kChannelWindow, channelName(n.channel).toUpperCase(), 15.0f);

    lamp(g, kOnLamp, n.enabled, 0.42f);
    // SOLO: a round push button with headphones; amber when this point is heard alone
    {
        const float r = 18.0f;
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillEllipse(kSolo.x - r, kSolo.y - r + 4.0f, 2.0f * r, 2.0f * r);
        juce::ColourGradient cap(n.solo ? juce::Colour(0xffffd27a) : juce::Colour(0xff8a8a83), kSolo.x - 6.0f, kSolo.y - 8.0f,
                                 n.solo ? juce::Colour(0xff8a5a10) : juce::Colour(0xff2c2c2a), kSolo.x + 12.0f, kSolo.y + 14.0f, true);
        g.setGradientFill(cap);
        g.fillEllipse(kSolo.x - r, kSolo.y - r, 2.0f * r, 2.0f * r);
        headphones(g, kSolo, 0.9f, n.solo ? juce::Colour(0xff2a1a05) : juce::Colour(0xffe8e2cc));
    }
    crtButton(g, kDelete, "DELETE", false);

    // the knobs; GAIN carries the dynamic range (dim) and what the dynamics add now (bright)
    for (int p = 0; p < NumParams; ++p)
    {
        const auto param = static_cast<Param>(p);
        const auto& k = kKnobs[static_cast<size_t>(p)];
        const bool on = enabledFor(n, param);
        const float angle = (-135.0f + static_cast<float>(normFor(param)) * 270.0f) * kPi / 180.0f;
        brassKnob(g, { k.x, k.y }, angle, k.s, on ? 1.0f : 0.45f, p <= Q ? KnobAsset::AluminumBrassInsert : KnobAsset::LatheSteelRust,
                  PointerAsset::WhiteEnamel);
        const auto r = juce::Rectangle<float>(k.x - 60.0f, k.y + k.s * 60.0f + 4.0f, 120.0f, 18.0f);
        shadowText(g, on || param <= Q ? format(param, valueOf(n, param)) : juce::String("--"), r, stencilFont(p <= Q ? 16.0f : 13.0f),
                   on ? stencil : stencilDim);
    }
    if (n.dynamic && isGain(n.shape))
    {
        const auto& k = kKnobs[Gain];
        const float radius = k.s * 60.0f + 9.0f;
        const auto angleOf = [&](double db) { return (-135.0f + static_cast<float>((juce::jlimit(-24.0, 24.0, db) + 24.0) / 48.0) * 270.0f) * kPi / 180.0f; };
        const auto arc = [&](double from, double to, juce::Colour c, float width)
        {
            juce::Path p;
            p.addCentredArc(k.x, k.y, radius, radius, 0.0f, angleOf(std::min(from, to)), angleOf(std::max(from, to)), true);
            g.setColour(c);
            g.strokePath(p, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        arc(n.gainDb, n.gainDb + n.rangeDb, kInk.withAlpha(0.35f), 4.0f);
        if (std::abs(dynamicGain) > 0.05f)
            arc(n.gainDb, n.gainDb + dynamicGain, kInk, 4.0f);
    }

    // DYNAMIC lever, lamp and the gain it adds (steel::lamp leaves a gradient fill: an opaque colour first,
    // or the lever image draws nothing)
    g.setColour(juce::Colours::white);
    lever(g, kDynLever, 0.0f, 1.0f, n.dynamic ? LeverPosition::Up : LeverPosition::Down);
    lamp(g, kDynLamp, n.dynamic && isGain(n.shape), 0.3f, kInk);
    juce::String line1 = "DYN", line2;
    if (!isGain(n.shape))
        line2 = "GAIN SHAPES";
    else if (!n.dynamic)
        line2 = "STATIC";
    else
        line2 = (dynamicGain > 0.05f ? "+" : "") + juce::String(dynamicGain, 1) + " dB";
    lcdText(g, kGainLcd.withHeight(22.0f).translated(0.0f, 4.0f), line1, 12.0f);
    lcdText(g, kGainLcd.withTrimmedTop(22.0f).translated(0.0f, -2.0f), line2, 18.0f);

    if (shapesOpen)
        drawShapeChooser(g);
}

void HamloTochkaPanel::timerCallback()
{
    if (!isShowing())
        return;
    const float db = values.node.dynamic && readDynamicGain ? readDynamicGain() : 0.0f;
    if (std::abs(db - dynamicGain) > 0.01f)
    {
        dynamicGain = db;
        repaint();
    }
}

int HamloTochkaPanel::knobAt(juce::Point<float> d) const
{
    for (int p = 0; p < NumParams; ++p)
    {
        const auto& k = kKnobs[static_cast<size_t>(p)];
        if (d.getDistanceFrom({ k.x, k.y }) <= 60.0f * k.s + 4.0f)
            return p;
    }
    return -1;
}

void HamloTochkaPanel::mouseDown(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    if (shapesOpen)
    {
        for (size_t i = 0; i < kShapes.size(); ++i)
            if (chooserTile(static_cast<int>(i)).contains(d))
                return chooseShape(kShapes[i]);
        return openShapes(false);
    }
    auto& n = values.node;
    if (kShapeWindow.contains(d))
        return openShapes(true);
    if (kSlopeWindow.contains(d) && isCut(n.shape))
    {
        auto it = std::find(kSlopes.begin(), kSlopes.end(), n.slopeDbPerOct);
        int i = it == kSlopes.end() ? 1 : static_cast<int>(it - kSlopes.begin());
        i = (i + (e.mods.isPopupMenu() ? static_cast<int>(kSlopes.size()) - 1 : 1)) % static_cast<int>(kSlopes.size());
        if (onGestureStart) onGestureStart("Slope");
        n.slopeDbPerOct = kSlopes[static_cast<size_t>(i)];
        change("Slope");
        if (onGestureEnd) onGestureEnd();
        return;
    }
    if (kChannelWindow.contains(d))
    {
        auto it = std::find(kChannels.begin(), kChannels.end(), n.channel);
        int i = it == kChannels.end() ? 0 : static_cast<int>(it - kChannels.begin());
        i = (i + (e.mods.isPopupMenu() ? static_cast<int>(kChannels.size()) - 1 : 1)) % static_cast<int>(kChannels.size());
        if (onGestureStart) onGestureStart("Channel");
        n.channel = kChannels[static_cast<size_t>(i)];
        change("Channel");
        if (onGestureEnd) onGestureEnd();
        return;
    }
    if (d.getDistanceFrom(kOnLamp) <= 20.0f)
    {
        if (onGestureStart) onGestureStart("Enable");
        n.enabled = !n.enabled;
        change("Enable");
        if (onGestureEnd) onGestureEnd();
        return;
    }
    if (d.getDistanceFrom(kSolo) <= 20.0f)
        return pressSolo();
    if (kDelete.contains(d))
    {
        if (onDelete)
            onDelete();
        return;
    }
    if (juce::Rectangle<float>(kDynLever.x - 24.0f, kDynLever.y - 34.0f, 72.0f, 60.0f).contains(d))
        return setDynamic(!n.dynamic);
    if (const int k = knobAt(d); k >= 0 && enabledFor(n, static_cast<Param>(k)))
    {
        dragKnob = k;
        dragStartNorm = normFor(static_cast<Param>(k));
        dragStartY = e.position.y;
        if (onGestureStart)
            onGestureStart(juce::String(kKnobs[static_cast<size_t>(k)].label).toLowerCase());
    }
}

void HamloTochkaPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob < 0)
        return;
    const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 220.0f) * scale;
    setParam(static_cast<Param>(dragKnob), valueFor(static_cast<Param>(dragKnob), dragStartNorm + (dragStartY - e.position.y) / pixelsPerRange));
}

void HamloTochkaPanel::mouseUp(const juce::MouseEvent&)
{
    if (dragKnob >= 0 && onGestureEnd)
        onGestureEnd();
    dragKnob = -1;
}

void HamloTochkaPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int p = knobAt(toDesign(e.position));
    if (p < 0 || !enabledFor(values.node, static_cast<Param>(p)))
        return;
    static constexpr double defaults[NumParams] { 1000.0, 0.0, 0.707, -30.0, 3.0, -6.0, 10.0, 120.0 };
    if (onGestureStart)
        onGestureStart(juce::String(kKnobs[static_cast<size_t>(p)].label).toLowerCase());
    setParam(static_cast<Param>(p), defaults[p]);
    if (onGestureEnd)
        onGestureEnd();
}

void HamloTochkaPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const int p = knobAt(toDesign(e.position));
    if (p < 0 || shapesOpen || !enabledFor(values.node, static_cast<Param>(p)))
        return;
    const double dir = wheel.deltaY > 0 ? 1.0 : -1.0;
    if (onGestureStart)
        onGestureStart(juce::String(kKnobs[static_cast<size_t>(p)].label).toLowerCase());
    setParam(static_cast<Param>(p), valueFor(static_cast<Param>(p), normFor(static_cast<Param>(p)) + (e.mods.isShiftDown() ? 0.004 : 0.02) * dir));
    if (onGestureEnd)
        onGestureEnd();
}
} // namespace hpeq::ui
