#pragma once

#include <juce_core/juce_core.h>

namespace hpeq::ui
{
// R6: what a HamloRevolution control does and what it is for (both pages' tooltips). Keys: the parameter ids, plus
// connect, simpleGenerate, advancedPage, simplePage, preview, generate, lanePlate, laneCells, laneGenerate, laneLock.
juce::String revolutionHelp(const juce::String& key);
} // namespace hpeq::ui
