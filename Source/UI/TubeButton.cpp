#include "UI/TubeButton.h"

#include <BinaryData.h>

#include "UI/BlackSteel.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
namespace
{
juce::Image loadPortrait(const juce::String& name)
{
    int size = 0;
    if (const char* data = HpeqAssets::getNamedResource(name.toRawUTF8(), size); data != nullptr && size > 0)
        return juce::ImageCache::getFromMemory(data, size);
    return {};
}
const juce::Colour kSovietRed { 0xffd8231a };
} // namespace

TubeButton::TubeButton(const juce::String& portrait, const juce::String& text)
    : imageOff(loadPortrait(portrait + "_off_png")), imageOn(loadPortrait(portrait + "_on_png")), caption(text)
{
    setRepaintsOnMouseActivity(false);
}

void TubeButton::setState(bool newOn, double newMix)
{
    if (dragging)
        return;
    if (newOn != on || std::abs(newMix - mix) > 1.0e-9)
    {
        on = newOn;
        mix = newMix;
        repaint();
    }
}

void TubeButton::setMix(double m)
{
    mix = juce::jlimit(0.0, 1.0, m);
    if (onMix)
        onMix(mix);
    repaint();
}

void TubeButton::paint(juce::Graphics& g)
{
    using namespace steel;
    const auto r = getLocalBounds().toFloat().reduced(1.0f);
    // the steel frame
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(r.translated(0.0f, 2.0f), 5.0f);
    juce::ColourGradient frame(juce::Colour(0xff55564f), r.getX(), r.getY(), juce::Colour(0xff1d1d1a), r.getX(), r.getBottom(), false);
    g.setGradientFill(frame);
    g.fillRoundedRectangle(r, 5.0f);
    const auto picture = r.reduced(4.0f);
    {
        juce::Graphics::ScopedSaveState state(g);
        juce::Path clip;
        clip.addRoundedRectangle(picture, 3.0f);
        g.reduceClipRegion(clip);
        const auto& image = on ? imageOn : imageOff;
        if (image.isValid())
            g.drawImage(image, picture, juce::RectanglePlacement::fillDestination | juce::RectanglePlacement::xMid | juce::RectanglePlacement::yTop);
        if (!on)
        {
            g.setColour(juce::Colours::black.withAlpha(hover ? 0.35f : 0.5f)); // off: darkened
            g.fillRect(picture);
        }
        // the caption on a dark band at the bottom
        const auto band = picture.withTop(picture.getBottom() - juce::jlimit(13.0f, 18.0f, picture.getHeight() * 0.2f));
        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillRect(band);
        g.setFont(stencilFont(juce::jlimit(8.5f, 12.0f, band.getHeight() * 0.72f)));
        g.setColour(on ? juce::Colour(0xfff6e2c4) : stencilDim);
        g.drawFittedText(caption, band.reduced(3.0f, 0.0f).toNearestInt(), juce::Justification::centred, 1, 0.7f);
        // MIX: a thin amber gauge on the right edge; its value while it changes
        const auto gauge = juce::Rectangle<float>(picture.getRight() - 5.0f, picture.getY() + 3.0f, 3.0f, band.getY() - picture.getY() - 6.0f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRect(gauge);
        g.setColour(colours::accent.withAlpha(on ? 1.0f : 0.45f));
        g.fillRect(gauge.withTop(gauge.getBottom() - gauge.getHeight() * static_cast<float>(mix)));
        if (dragging || (hover && mix < 0.999))
        {
            const auto tag = juce::Rectangle<float>(picture.getX() + 4.0f, picture.getY() + 4.0f, picture.getWidth() - 14.0f, 15.0f);
            g.setColour(juce::Colour(0xe0050605));
            g.fillRoundedRectangle(tag, 2.0f);
            g.setFont(juce::FontOptions("Consolas", 11.0f, juce::Font::bold));
            g.setColour(colours::accent);
            g.drawText("MIX " + juce::String(juce::roundToInt(mix * 100.0)) + " %", tag, juce::Justification::centred);
        }
    }
    // lit: a red glow around the picture and a lamp in the corner
    if (on)
    {
        g.setColour(kSovietRed.withAlpha(0.55f));
        g.drawRoundedRectangle(picture.expanded(1.5f), 4.0f, 2.0f);
        lamp(g, { r.getX() + 9.0f, r.getY() + 9.0f }, true, 0.14f, kSovietRed);
    }
    else
    {
        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(picture.expanded(0.5f), 3.5f, 1.0f);
    }
}

void TubeButton::mouseEnter(const juce::MouseEvent&)
{
    hover = true;
    repaint();
}

void TubeButton::mouseExit(const juce::MouseEvent&)
{
    hover = false;
    repaint();
}

void TubeButton::mouseDown(const juce::MouseEvent& e)
{
    dragStartY = e.position.y;
    dragStartMix = mix;
    moved = false;
}

void TubeButton::mouseDrag(const juce::MouseEvent& e)
{
    if (!moved && std::abs(e.position.y - dragStartY) < 4.0f)
        return;
    if (!moved)
    {
        moved = dragging = true;
        if (onMixGestureStart)
            onMixGestureStart();
    }
    setMix(dragStartMix + (dragStartY - e.position.y) / (e.mods.isShiftDown() ? 600.0 : 150.0));
}

void TubeButton::mouseUp(const juce::MouseEvent&)
{
    if (moved)
    {
        dragging = false;
        if (onMixGestureEnd)
            onMixGestureEnd();
        repaint();
        return;
    }
    on = !on;
    if (onToggle)
        onToggle(on);
    repaint();
}

void TubeButton::mouseDoubleClick(const juce::MouseEvent&)
{
    if (onMixGestureStart)
        onMixGestureStart();
    setMix(1.0);
    if (onMixGestureEnd)
        onMixGestureEnd();
}

void TubeButton::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (onMixGestureStart)
        onMixGestureStart();
    setMix(mix + (wheel.deltaY > 0 ? 1.0 : -1.0) * (e.mods.isShiftDown() ? 0.01 : 0.05));
    if (onMixGestureEnd)
        onMixGestureEnd();
}
} // namespace hpeq::ui
