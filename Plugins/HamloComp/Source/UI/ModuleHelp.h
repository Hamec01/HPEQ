#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace hamlo::ui
{
// Owner (2026-10-26): "choosing a knob on any module, write at the side what it does and what it is for".
// A module page answers which control is under a point (its state key) and where its drawing sits; the
// inspector shows the text from moduleHelp() in the free margin left of the page.
struct HelpTarget
{
    virtual ~HelpTarget() = default;
    // The state key of the knob / switch under `local` (component coordinates), empty if none.
    virtual juce::String helpKeyAt(juce::Point<float> local) const = 0;
    // The drawn page in component coordinates (the rest of the component is free margin).
    virtual juce::Rectangle<float> contentBounds() const = 0;
};

struct HelpText
{
    juce::String title, text;
};

// module: "comp" (HamloComp carries only the compressor's table; HPEQ has the other modules).
// Unknown keys give an empty text.
HelpText moduleHelp(const juce::String& module, const juce::String& key);
} // namespace hamlo::ui
