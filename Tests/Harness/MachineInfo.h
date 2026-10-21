#pragma once

#include <juce_core/juce_core.h>

namespace hpeq::test
{
// Context recorded with every measurement (04 §4): numbers from different machines/builds are not
// comparable, so each JSON carries where and how it was produced.
juce::var describeMachine();
juce::String buildConfiguration();
} // namespace hpeq::test
