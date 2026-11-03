#include "UI/HamloRevolutionSimplePage.h"

#include <cmath>

#include "Core/RevolutionSimple.h"
#include "UI/BlackSteel.h"
#include "UI/RevolutionHelp.h"

namespace hpeq::ui
{
namespace
{
namespace rev = hpeq::revolution;
namespace simple = hpeq::revolution::simple;
using namespace hpeq::ui::steel;

constexpr float kW = 1180.0f, kH = 1080.0f; // the ADVANCED page's frame (R8)
constexpr float kPi = juce::MathConstants<float>::pi;

const juce::Rectangle<float> kConnect { 800.0f, 28.0f, 270.0f, 48.0f };
const juce::Point<float> kLinkLamp { 1110.0f, 52.0f };
const juce::Rectangle<float> kLinkLcd { 800.0f, 88.0f, 340.0f, 44.0f };
const juce::Rectangle<float> kBold { 960.0f, 250.0f, 180.0f, 38.0f };
const juce::Rectangle<float> kAdvanced { 960.0f, 196.0f, 180.0f, 44.0f };
juce::Rectangle<float> genreRect(int g) { return { 60.0f + 166.0f * static_cast<float>(g), 196.0f, 150.0f, 44.0f }; }
juce::Rectangle<float> fillRect(int f) { return { 60.0f + 96.0f * static_cast<float>(f), 556.0f, 86.0f, 40.0f }; }
juce::Rectangle<float> targetRect(int t) { return { 640.0f + 126.0f * static_cast<float>(t), 556.0f, 114.0f, 40.0f }; }
const juce::Point<float> kGenerate { 150.0f, 672.0f };
const juce::Rectangle<float> kLock { 250.0f, 650.0f, 140.0f, 44.0f };
const juce::Rectangle<float> kInfoLcd { 700.0f, 634.0f, 440.0f, 76.0f };
const juce::Rectangle<float> kRise { 430.0f, 650.0f, 120.0f, 44.0f }, kFill { 562.0f, 650.0f, 120.0f, 44.0f }; // the transition page
constexpr float kPreviewY = 752.0f, kPreviewRow = 44.0f, kCellX = 230.0f, kCellPitch = 56.0f;

struct KnobInfo
{
    float x, y, s;
    const char* label;
};
const std::array<KnobInfo, HamloRevolutionSimplePage::NumKnobs> kKnobs { {
    { 140.0f, 392.0f, 0.85f, "INTENT" },
    { 360.0f, 392.0f, 0.85f, "AMOUNT" },
    { 590.0f, 392.0f, 0.85f, "VARIATION" },
    { 820.0f, 392.0f, 0.85f, "GROOVE" },
    { 1040.0f, 392.0f, 0.72f, "MIX" },
} };

const juce::Colour kLaneInk[rev::kLanes] { juce::Colour(0xff6dff9a), juce::Colour(0xffffb347), juce::Colour(0xff8fd8ff), juce::Colour(0xffff7ad9),
                                          juce::Colour(0xffb58cff), juce::Colour(0xffffe066), juce::Colour(0xff6fe3c1) };
} // namespace

HamloRevolutionSimplePage::HamloRevolutionSimplePage()
{
    setOpaque(true);
}

const char* HamloRevolutionSimplePage::keyFor(Knob k)
{
    static constexpr const char* keys[NumKnobs] { "simpleIntent", "simpleAmount", "simpleVariation", "simpleGroove", "mix" };
    return keys[k];
}

double HamloRevolutionSimplePage::defaultFor(Knob k) { return Values {}.knobs[static_cast<size_t>(k)]; }

void HamloRevolutionSimplePage::setValues(const Values& v)
{
    values = v;
    repaint();
}

juce::Point<float> HamloRevolutionSimplePage::toDesign(juce::Point<float> p) const { return (p - offset) / scale; }

void HamloRevolutionSimplePage::resized()
{
    scale = juce::jmin(static_cast<float>(getWidth()) / kW, static_cast<float>(getHeight()) / kH);
    offset = { (static_cast<float>(getWidth()) - kW * scale) * 0.5f, (static_cast<float>(getHeight()) - kH * scale) * 0.5f };
    staticLayer = {};
    if (getWidth() > 0 && getHeight() > 0)
    {
        staticLayer = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g(staticLayer);
        g.fillAll(juce::Colours::black);
        g.addTransform(juce::AffineTransform::scale(scale).translated(offset));
        drawStatic(g);
    }
}

void HamloRevolutionSimplePage::drawStatic(juce::Graphics& g) const
{
    panel(g, kW, kH, 11);
    engraved(g, "REVOLUTION", { 420.0f, 30.0f, 360.0f, 44.0f }, 30.0f);
    engraved(g, "SIMPLE", { 420.0f, 78.0f, 360.0f, 24.0f }, 15.0f, stencilDim);
    engraved(g, "GENRE", { 60.0f, 166.0f, 200.0f, 22.0f }, 13.0f, stencil, juce::Justification::centredLeft);
    for (const auto& k : kKnobs)
    {
        knobScale(g, { k.x, k.y }, k.s);
        engraved(g, k.label, { k.x - 90.0f, k.y - k.s * 60.0f - 34.0f, 180.0f, 20.0f }, 15.0f);
    }
    engraved(g, "FILL EVERY", { 60.0f, 530.0f, 300.0f, 22.0f }, 13.0f, stencil, juce::Justification::centredLeft);
    engraved(g, "TARGET", { 640.0f, 530.0f, 300.0f, 22.0f }, 13.0f, stencil, juce::Justification::centredLeft);
    engraved(g, "GENERATE", { kGenerate.x - 70.0f, kGenerate.y + 42.0f, 140.0f, 20.0f }, 12.0f);
    lcdWindow(g, kLinkLcd);
    lcdWindow(g, kInfoLcd);
    for (const float y : { 160.0f, 262.0f, 520.0f, 614.0f, 730.0f })
    {
        g.setColour(juce::Colours::black);
        g.drawHorizontalLine(juce::roundToInt(y), 20.0f, kW - 20.0f);
        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.drawHorizontalLine(juce::roundToInt(y) - 1, 20.0f, kW - 20.0f);
    }
    for (int l = 0; l < rev::kLanes; ++l)
    {
        const float y = kPreviewY + kPreviewRow * static_cast<float>(l);
        engraved(g, rev::engineName(l), { 50.0f, y, 150.0f, 34.0f }, 12.0f, stencil, juce::Justification::centredLeft);
        for (int s = 0; s < rev::kSteps; ++s)
        {
            const juce::Rectangle<float> r { kCellX + kCellPitch * static_cast<float>(s), y, kCellPitch - 4.0f, 34.0f };
            g.setColour(juce::Colour(0xff030403));
            g.fillRoundedRectangle(r, 4.0f);
        }
    }
}

void HamloRevolutionSimplePage::paint(juce::Graphics& g)
{
    if (staticLayer.isValid())
        g.drawImageAt(staticLayer, 0, 0);
    g.addTransform(juce::AffineTransform::scale(scale).translated(offset));
    crtButton(g,kBold,"BOLD",false);

    // CONNECT TO HPDG, its lamp and status
    const bool linked = values.link == Link::Linked;
    crtButton(g, kConnect, values.link == Link::Off ? "CONNECT TO HPDG" : "DISCONNECT HPDG", values.link != Link::Off);
    const auto lampColour = linked ? juce::Colour(0xff4dff7a) : values.link == Link::Searching ? juce::Colour(0xffffb020) : juce::Colour(0xffff3b2d);
    lamp(g, kLinkLamp, values.link != Link::Off, 0.7f, lampColour);
    lcdText(g, kLinkLcd.reduced(8.0f, 0.0f), values.linkText, 14.0f, juce::Justification::centredLeft);

    for (int i = 0; i < simple::kGenres; ++i)
    {
        juce::String name = simple::genreName(i);
        if (i == simple::AutoGenre && values.genre == simple::AutoGenre)
            name << ": " << simple::genreName(values.shownGenre);
        crtButton(g, genreRect(i), name, values.genre == i);
    }
    crtButton(g, kAdvanced, "ADVANCED", false);

    for (int k = 0; k < NumKnobs; ++k)
    {
        const auto& info = kKnobs[static_cast<size_t>(k)];
        const double v = juce::jlimit(0.0, 1.0, values.knobs[static_cast<size_t>(k)]);
        const float angle = (-135.0f + static_cast<float>(v) * 270.0f) * kPi / 180.0f;
        brassKnob(g, { info.x, info.y }, angle, info.s, 1.0f, k == Mix ? KnobAsset::AluminumBrassInsert : KnobAsset::LatheSteelRust);
        const juce::String text = k == Intent ? juce::String(simple::intentName(simple::intentOf(v))) : juce::String(juce::roundToInt(100.0 * v)) + " %";
        engraved(g, text, { info.x - 90.0f, info.y + info.s * 60.0f + 14.0f, 180.0f, 22.0f }, 15.0f, stencilDim);
    }

    for (int f = 0; f < simple::kFills; ++f)
        crtButton(g, fillRect(f), simple::fillName(f), values.fill == f);
    for (int t = 0; t < simple::kTargets; ++t)
        crtButton(g, targetRect(t), simple::targetName(t), values.target == t);

    // GENERATE (the red button), LOCK, the summary
    {
        juce::ColourGradient red(juce::Colour(0xffff5a4a), kGenerate.x - 10.0f, kGenerate.y - 20.0f, juce::Colour(0xff7a0d08), kGenerate.x + 20.0f,
                                 kGenerate.y + 30.0f, true);
        g.setColour(juce::Colour(0xff0b0c0b));
        g.fillEllipse(kGenerate.x - 38.0f, kGenerate.y - 38.0f, 76.0f, 76.0f);
        g.setGradientFill(red);
        g.fillEllipse(kGenerate.x - 31.0f, kGenerate.y - 31.0f, 62.0f, 62.0f);
    }
    crtButton(g, kLock, values.lock ? "LOCKED" : "LOCK", values.lock);
    crtButton(g, kRise, "RISE", false);
    crtButton(g, kFill, "FILL", false);
    const juce::String line1 = juce::String(simple::genreName(values.shownGenre)) + "  /  " + simple::intentName(values.shownIntent) + "  /  AMOUNT "
                               + juce::String(juce::roundToInt(100.0 * values.knobs[Amount])) + " %";
    lcdText(g, kInfoLcd.withHeight(kInfoLcd.getHeight() * 0.5f).translated(0.0f, 4.0f), line1, 14.0f);
    lcdText(g, kInfoLcd.withTrimmedTop(kInfoLcd.getHeight() * 0.5f).translated(0.0f, -4.0f), values.source, 11.5f);

    // the preview: what the generator wrote (TIME / GLITCH actions, the levels of the others), the band, the play head
    for (int l = 0; l < rev::kLanes; ++l)
    {
        const float y = kPreviewY + kPreviewRow * static_cast<float>(l);
        const auto ink = kLaneInk[l];
        if (values.bands[static_cast<size_t>(l)] != rev::Full)
        {
            g.setFont(monoFont(11.0f).withStyle("Bold"));
            g.setColour(ink.withAlpha(0.85f));
            g.drawText(rev::bandName(values.bands[static_cast<size_t>(l)]), juce::Rectangle<float>(150.0f, y, 70.0f, 34.0f), juce::Justification::centred);
        }
        for (int s = 0; s < rev::kSteps; ++s)
        {
            const juce::Rectangle<float> r { kCellX + kCellPitch * static_cast<float>(s), y, kCellPitch - 4.0f, 34.0f };
            const int v = values.preview[static_cast<size_t>(l)][static_cast<size_t>(s)];
            if (s == values.step)
            {
                g.setColour(juce::Colours::white.withAlpha(0.10f));
                g.fillRoundedRectangle(r, 4.0f);
            }
            if (v == 0)
                continue;
            if (l < rev::Motion)
            {
                g.setFont(stencilFont(15.0f));
                g.setColour(ink);
                g.drawText(l == rev::Glitch && values.glitchBlock ? rev::blockActionShortName(v) : rev::valueShortName(l, v), r,
                           juce::Justification::centred);
            }
            else
            {
                const float h = (r.getHeight() - 8.0f) * static_cast<float>(v) / 15.0f;
                g.setColour(ink.withAlpha(0.85f));
                g.fillRoundedRectangle(r.getX() + 6.0f, r.getBottom() - 4.0f - h, r.getWidth() - 12.0f, h, 2.0f);
            }
        }
    }
}

juce::String HamloRevolutionSimplePage::helpKeyAt(juce::Point<float> local) const
{
    const auto d = toDesign(local);
    if (kConnect.contains(d) || kLinkLcd.contains(d) || d.getDistanceFrom(kLinkLamp) <= 16.0f)
        return "connect";
    if (kAdvanced.contains(d))
        return "advancedPage";
    if (d.getDistanceFrom(kGenerate) <= 38.0f)
        return "simpleGenerate";
    if (kLock.contains(d))
        return "simpleLock";
    if (kRise.contains(d))
        return "simpleRise";
    if (kFill.contains(d))
        return "simpleFill2";
    for (int i = 0; i < simple::kGenres; ++i)
        if (genreRect(i).contains(d))
            return "simpleGenre";
    for (int f = 0; f < simple::kFills; ++f)
        if (fillRect(f).contains(d))
            return "simpleFill";
    for (int t = 0; t < simple::kTargets; ++t)
        if (targetRect(t).contains(d))
            return "simpleTarget";
    for (int k = 0; k < NumKnobs; ++k)
        if (d.getDistanceFrom({ kKnobs[static_cast<size_t>(k)].x, kKnobs[static_cast<size_t>(k)].y }) <= 62.0f * kKnobs[static_cast<size_t>(k)].s)
            return keyFor(static_cast<Knob>(k));
    if (d.y >= kPreviewY && d.y < kPreviewY + kPreviewRow * static_cast<float>(rev::kLanes))
        return "preview";
    return {};
}

juce::String HamloRevolutionSimplePage::getTooltip()
{
    if(kBold.contains(toDesign(getMouseXYRelative().toFloat())))return "Generate a strong full-band Chaos recipe: Amount/Mix 100%, fills every bar; genre preserved. Existing projects change only when you press it."; return revolutionHelp(helpKeyAt(getMouseXYRelative().toFloat()));
}

void HamloRevolutionSimplePage::pressGenerate()
{
    if (onGenerate)
        onGenerate();
}

void HamloRevolutionSimplePage::pressConnect()
{
    if (onConnect)
        onConnect();
}

void HamloRevolutionSimplePage::pressTransition(int kind)
{
    if (onTransition)
        onTransition(kind);
}

void HamloRevolutionSimplePage::pressAdvanced()
{
    if (onAdvanced)
        onAdvanced();
}

void HamloRevolutionSimplePage::chooseGenre(int g)
{
    values.genre = juce::jlimit(0, simple::kGenres - 1, g);
    if (onChoice)
        onChoice("simpleGenre", values.genre);
    repaint();
}

void HamloRevolutionSimplePage::chooseFill(int f)
{
    values.fill = juce::jlimit(0, simple::kFills - 1, f);
    if (onChoice)
        onChoice("simpleFill", values.fill);
    repaint();
}

void HamloRevolutionSimplePage::chooseTarget(int t)
{
    values.target = juce::jlimit(0, simple::kTargets - 1, t);
    if (onChoice)
        onChoice("simpleTarget", values.target);
    repaint();
}

void HamloRevolutionSimplePage::toggleLock()
{
    values.lock = !values.lock;
    if (onChoice)
        onChoice("simpleLock", values.lock ? 1 : 0);
    repaint();
}

void HamloRevolutionSimplePage::setKnob(Knob k, double v)
{
    values.knobs[static_cast<size_t>(k)] = juce::jlimit(0.0, 1.0, v);
    if (onKnobChange)
        onKnobChange(k, values.knobs[static_cast<size_t>(k)]);
    repaint();
}

void HamloRevolutionSimplePage::mouseDown(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    if(kBold.contains(d)){if(onBold)onBold();return;}
    if (kConnect.contains(d) || d.getDistanceFrom(kLinkLamp) <= 16.0f)
    {
        if (e.mods.isPopupMenu())
        {
            if (onConnectMenu)
                onConnectMenu();
        }
        else
            pressConnect();
        return;
    }
    if (kAdvanced.contains(d))
        return pressAdvanced();
    if (d.getDistanceFrom(kGenerate) <= 38.0f)
        return pressGenerate();
    if (kLock.contains(d))
        return toggleLock();
    if (kRise.contains(d))
        return pressTransition(0);
    if (kFill.contains(d))
        return pressTransition(1);
    for (int i = 0; i < simple::kGenres; ++i)
        if (genreRect(i).contains(d))
            return chooseGenre(i);
    for (int f = 0; f < simple::kFills; ++f)
        if (fillRect(f).contains(d))
            return chooseFill(f);
    for (int t = 0; t < simple::kTargets; ++t)
        if (targetRect(t).contains(d))
            return chooseTarget(t);
    for (int k = 0; k < NumKnobs; ++k)
    {
        const auto& info = kKnobs[static_cast<size_t>(k)];
        if (d.getDistanceFrom({ info.x, info.y }) <= 62.0f * info.s)
        {
            dragKnob = k;
            dragStart = values.knobs[static_cast<size_t>(k)];
            dragStartY = e.position.y;
            if (onKnobGestureStart)
                onKnobGestureStart(static_cast<Knob>(k));
            return;
        }
    }
}

void HamloRevolutionSimplePage::mouseDrag(const juce::MouseEvent& e)
{
    if (dragKnob < 0)
        return;
    const float pixelsPerRange = (e.mods.isShiftDown() ? 900.0f : 220.0f) * scale;
    setKnob(static_cast<Knob>(dragKnob), dragStart + static_cast<double>((dragStartY - e.position.y) / pixelsPerRange));
}

void HamloRevolutionSimplePage::mouseUp(const juce::MouseEvent&)
{
    if (dragKnob >= 0 && onKnobGestureEnd)
        onKnobGestureEnd();
    dragKnob = -1;
}

void HamloRevolutionSimplePage::mouseDoubleClick(const juce::MouseEvent& e)
{
    const auto d = toDesign(e.position);
    for (int k = 0; k < NumKnobs; ++k)
    {
        const auto& info = kKnobs[static_cast<size_t>(k)];
        if (d.getDistanceFrom({ info.x, info.y }) <= 62.0f * info.s)
            return setKnob(static_cast<Knob>(k), defaultFor(static_cast<Knob>(k)));
    }
}
} // namespace hpeq::ui
