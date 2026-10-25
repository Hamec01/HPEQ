#include "UI/SteelLookAndFeel.h"

#include "UI/BlackSteel.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
namespace
{
using namespace steel;
const juce::Colour kAmber { 0xffffb347 }, kAmberDim { 0xff8a5a1c }, kGlass { 0xff0b0d0c }, kSteelLight { 0xff4a4a44 }, kSteelDark { 0xff1c1c19 };
} // namespace

SteelLookAndFeel::SteelLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, colours::background);
    setColour(juce::TextButton::buttonColourId, colours::panelRaised);
    setColour(juce::TextButton::textColourOffId, colours::text);
    setColour(juce::TextButton::textColourOnId, colours::text);
    setColour(juce::ComboBox::backgroundColourId, kGlass);
    setColour(juce::ComboBox::textColourId, kAmber);
    setColour(juce::ComboBox::arrowColourId, kAmber);
    setColour(juce::ComboBox::outlineColourId, juce::Colours::black);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff151613));
    setColour(juce::PopupMenu::textColourId, stencil);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kAmber.withAlpha(0.22f));
    setColour(juce::PopupMenu::highlightedTextColourId, kAmber);
    setColour(juce::PopupMenu::headerTextColourId, kAmber);
    setColour(juce::Label::textColourId, stencil);
    setColour(juce::Slider::textBoxTextColourId, kAmber);
    setColour(juce::Slider::textBoxBackgroundColourId, kGlass);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
    setColour(juce::TextEditor::backgroundColourId, kGlass);
    setColour(juce::TextEditor::textColourId, kAmber);
    setColour(juce::TextEditor::outlineColourId, juce::Colours::black);
    setColour(juce::TextEditor::focusedOutlineColourId, kAmberDim);
    setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff151613));
    setColour(juce::TooltipWindow::textColourId, stencil);
    setColour(juce::ScrollBar::thumbColourId, kSteelLight);
    setColour(juce::ToggleButton::textColourId, stencil);
    setColour(juce::ToggleButton::tickColourId, kAmber);
}

void SteelLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& background, bool over, bool down)
{
    // A steel key; a button the editor colours (selected tab, layer, module tile) is "lit": a lamp
    // stripe in its colour along the bottom and a warmer face.
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    const bool lit = background.getAlpha() > 0 && background.withAlpha(1.0f) != colours::panelRaised.withAlpha(1.0f);
    const auto lampColour = lit ? background.withAlpha(1.0f).withMultipliedSaturation(1.2f) : juce::Colours::transparentBlack;
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillRoundedRectangle(r.translated(0.0f, 1.5f), 4.0f);
    juce::ColourGradient face(down ? kSteelDark : kSteelLight.brighter(over ? 0.15f : 0.0f), r.getX(), r.getY(),
                              down ? kSteelLight : kSteelDark, r.getX(), r.getBottom(), false);
    if (lit)
    {
        face = juce::ColourGradient(kSteelLight.interpolatedWith(lampColour, 0.25f).brighter(over ? 0.1f : 0.0f), r.getX(), r.getY(),
                                    kSteelDark.interpolatedWith(lampColour, 0.15f), r.getX(), r.getBottom(), false);
    }
    g.setGradientFill(face);
    g.fillRoundedRectangle(r, 4.0f);
    // brushed grain
    g.setColour(juce::Colours::white.withAlpha(0.025f));
    for (float x = r.getX() + 2.0f; x < r.getRight() - 2.0f; x += 3.0f)
        g.drawVerticalLine(juce::roundToInt(x), r.getY() + 2.0f, r.getBottom() - 2.0f);
    g.setColour(juce::Colours::white.withAlpha(down ? 0.04f : 0.12f));
    g.drawHorizontalLine(juce::roundToInt(r.getY() + 1.0f), r.getX() + 3.0f, r.getRight() - 3.0f);
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.drawRoundedRectangle(r, 4.0f, 1.0f);
    if (lit)
    {
        const auto stripe = juce::Rectangle<float>(r.getX() + 5.0f, r.getBottom() - 4.5f, r.getWidth() - 10.0f, 2.5f);
        g.setColour(lampColour.withAlpha(0.35f));
        g.fillRoundedRectangle(stripe.expanded(1.5f, 1.5f), 2.0f);
        g.setColour(lampColour);
        g.fillRoundedRectangle(stripe, 1.2f);
    }
}

juce::Font SteelLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(stencilFont(juce::jlimit(11.0f, 17.0f, static_cast<float>(buttonHeight) * 0.55f)).withKerningFactor(0.06f));
}

void SteelLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setFont(getTextButtonFont(b, b.getHeight()));
    auto r = b.getLocalBounds().reduced(4, 2).translated(0, down ? 1 : 0);
    const auto colour = b.findColour(b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                            .withMultipliedAlpha(b.isEnabled() ? 1.0f : 0.45f);
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.drawFittedText(b.getButtonText(), r.translated(0, 1), juce::Justification::centred, 1, 0.8f);
    g.setColour(colour);
    g.drawFittedText(b.getButtonText(), r, juce::Justification::centred, 1, 0.8f);
}

void SteelLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    const float h = static_cast<float>(b.getHeight());
    const float lampSize = juce::jmin(h, 22.0f);
    lamp(g, { 4.0f + lampSize * 0.5f, h * 0.5f }, b.getToggleState(), lampSize / 68.0f, kAmber);
    g.setFont(stencilFont(juce::jlimit(11.0f, 15.0f, h * 0.6f)));
    g.setColour(stencil.withMultipliedAlpha(b.isEnabled() ? 1.0f : 0.45f));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().withTrimmedLeft(juce::roundToInt(lampSize) + 10), juce::Justification::centredLeft, 1);
}

void SteelLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    lcdWindow(g, r);
    juce::Path arrow;
    const float ax = r.getRight() - 14.0f, ay = r.getCentreY();
    arrow.addTriangle(ax - 4.5f, ay - 2.5f, ax + 4.5f, ay - 2.5f, ax, ay + 3.5f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId).withMultipliedAlpha(box.isEnabled() ? 1.0f : 0.4f));
    g.fillPath(arrow);
}

juce::Font SteelLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return juce::Font(monoFont(juce::jlimit(11.0f, 16.0f, static_cast<float>(box.getHeight()) * 0.55f)).withStyle("Bold"));
}

void SteelLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(8, 1, box.getWidth() - 28, box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
    label.setColour(juce::Label::textColourId, box.findColour(juce::ComboBox::textColourId).withMultipliedAlpha(box.isEnabled() ? 1.0f : 0.45f));
}

void SteelLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto r = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    juce::ColourGradient steelFace(juce::Colour(0xff26261f), 0.0f, 0.0f, juce::Colour(0xff121210), 0.0f, r.getBottom(), false);
    g.setGradientFill(steelFace);
    g.fillRect(r);
    g.setColour(juce::Colours::white.withAlpha(0.02f));
    for (int x = 0; x < width; x += 3)
        g.drawVerticalLine(x, 0.0f, r.getBottom());
    g.setColour(juce::Colours::black);
    g.drawRect(r, 1.5f);
    g.setColour(kAmber.withAlpha(0.35f));
    g.drawHorizontalLine(1, 2.0f, r.getRight() - 2.0f);
}

void SteelLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool separator, bool active, bool highlighted,
                                         bool ticked, bool hasSubMenu, const juce::String& text, const juce::String& shortcut,
                                         const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (separator)
    {
        const auto y = area.getCentreY();
        g.setColour(juce::Colours::black);
        g.drawHorizontalLine(y, 8.0f, static_cast<float>(area.getRight() - 8));
        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.drawHorizontalLine(y + 1, 8.0f, static_cast<float>(area.getRight() - 8));
        return;
    }
    auto r = area.reduced(2, 1);
    if (highlighted && active)
    {
        g.setColour(kAmber.withAlpha(0.18f));
        g.fillRoundedRectangle(r.toFloat(), 3.0f);
        g.setColour(kAmber.withAlpha(0.6f));
        g.fillRect(r.getX(), r.getY() + 3, 3, r.getHeight() - 6); // a lit edge, as a lamp
    }
    auto colour = textColour != nullptr ? *textColour : (highlighted ? kAmber : stencil);
    if (!active)
        colour = colour.withAlpha(0.35f);
    auto inner = r.reduced(10, 0);
    if (ticked)
        lamp(g, { static_cast<float>(inner.getX()) + 5.0f, static_cast<float>(inner.getCentreY()) }, true, 0.13f, kAmber);
    inner.removeFromLeft(16);
    if (icon != nullptr)
    {
        const auto iconArea = inner.removeFromLeft(36).toFloat().reduced(2.0f, 3.0f);
        icon->drawWithin(g, iconArea, juce::RectanglePlacement::centred, active ? 1.0f : 0.4f);
        inner.removeFromLeft(6);
    }
    g.setFont(getPopupMenuFont());
    g.setColour(colour);
    g.drawFittedText(text, inner, juce::Justification::centredLeft, 1);
    if (hasSubMenu)
    {
        juce::Path arrow;
        const float ax = static_cast<float>(r.getRight() - 12), ay = static_cast<float>(r.getCentreY());
        arrow.addTriangle(ax - 3.0f, ay - 4.5f, ax - 3.0f, ay + 4.5f, ax + 3.0f, ay);
        g.fillPath(arrow);
    }
    if (shortcut.isNotEmpty())
    {
        g.setColour(stencilDim);
        g.drawText(shortcut, inner, juce::Justification::centredRight);
    }
}

void SteelLookAndFeel::drawPopupMenuSectionHeader(juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name)
{
    g.setFont(juce::Font(stencilFont(12.5f).withKerningFactor(0.12f)));
    g.setColour(kAmber);
    g.drawFittedText(name.toUpperCase(), area.reduced(12, 0), juce::Justification::bottomLeft, 1);
}

juce::Font SteelLookAndFeel::getPopupMenuFont()
{
    return juce::Font(stencilFont(15.0f, false));
}

void SteelLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float position, float, float, juce::Slider& s)
{
    const float size = static_cast<float>(juce::jmin(width, height));
    const juce::Point<float> c(static_cast<float>(x) + static_cast<float>(width) * 0.5f, static_cast<float>(y) + static_cast<float>(height) * 0.5f);
    const float scale = size / 150.0f;
    knobScale(g, c, scale);
    const float angle = (-135.0f + position * 270.0f) * juce::MathConstants<float>::pi / 180.0f;
    brassKnob(g, c, angle, scale, s.isEnabled() ? 1.0f : 0.45f);
}

void SteelLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float position, float, float,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
{
    const bool horizontal = style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar;
    const auto r = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
    // the slot
    const auto slot = horizontal ? juce::Rectangle<float>(r.getX(), r.getCentreY() - 3.0f, r.getWidth(), 6.0f)
                                 : juce::Rectangle<float>(r.getCentreX() - 3.0f, r.getY(), 6.0f, r.getHeight());
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.fillRoundedRectangle(slot.translated(0.0f, 1.0f), 3.0f);
    g.setColour(juce::Colour(0xff050505));
    g.fillRoundedRectangle(slot, 3.0f);
    // the cap
    const auto cap = horizontal ? juce::Rectangle<float>(0.0f, 0.0f, 14.0f, juce::jmin(22.0f, r.getHeight())).withCentre({ position, r.getCentreY() })
                                : juce::Rectangle<float>(0.0f, 0.0f, juce::jmin(26.0f, r.getWidth()), 14.0f).withCentre({ r.getCentreX(), position });
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(cap.translated(1.5f, 2.5f), 3.0f);
    juce::ColourGradient body(juce::Colour(0xffd9d9d2), cap.getX(), cap.getY(), juce::Colour(0xff4b4b47), cap.getX(), cap.getBottom(), false);
    body.addColour(0.5, juce::Colour(0xff8a8a84));
    g.setGradientFill(body);
    g.fillRoundedRectangle(cap, 3.0f);
    g.setColour(juce::Colour(0xffb3261e).withMultipliedAlpha(s.isEnabled() ? 1.0f : 0.4f));
    if (horizontal)
        g.fillRect(cap.getCentreX() - 0.8f, cap.getY() + 3.0f, 1.6f, cap.getHeight() - 6.0f);
    else
        g.fillRect(cap.getX() + 3.0f, cap.getCentreY() - 0.8f, cap.getWidth() - 6.0f, 1.6f);
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.drawRoundedRectangle(cap, 3.0f, 1.0f);
}

juce::Font SteelLookAndFeel::getLabelFont(juce::Label& label)
{
    const float size = label.getFont().getHeight();
    return juce::Font(stencilFont(juce::jlimit(10.0f, 30.0f, size), label.getFont().isBold()));
}

void SteelLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    drawPopupMenuBackground(g, width, height);
    g.setFont(juce::Font(stencilFont(13.0f, false)));
    g.setColour(stencil);
    g.drawFittedText(text, juce::Rectangle<int>(width, height).reduced(6, 3), juce::Justification::centredLeft, 4);
}
} // namespace hpeq::ui
