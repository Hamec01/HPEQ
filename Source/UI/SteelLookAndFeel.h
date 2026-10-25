#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace hpeq::ui
{
// EQ pass: the whole HPEQ window in the Black Steel style of the module pages. Buttons are steel keys
// (a lit one carries a lamp stripe in its colour), combo boxes are amber LCD windows, menus are dark
// steel with amber text (and the shape pictures), rotary sliders are brass knobs on an engraved scale,
// linear sliders a steel slot with a fader cap, toggles a pilot lamp with stencil text.
class SteelLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SteelLookAndFeel();

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& background, bool over, bool down) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area, bool separator, bool active, bool highlighted, bool ticked,
                           bool hasSubMenu, const juce::String& text, const juce::String& shortcut, const juce::Drawable* icon,
                           const juce::Colour* textColour) override;
    void drawPopupMenuSectionHeader(juce::Graphics&, const juce::Rectangle<int>&, const juce::String&) override;
    juce::Font getPopupMenuFont() override;

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float position, float start, float end, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height, float position, float min, float max,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Font getLabelFont(juce::Label&) override;
    void drawTooltip(juce::Graphics&, const juce::String& text, int width, int height) override;
};
} // namespace hpeq::ui
