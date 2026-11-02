#pragma once
#include "Standalone/ModuleVariant.h"

namespace hpeq::standalone
{
const std::vector<ParameterSpec>& v2Parameters();
void addV2Parameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
}
