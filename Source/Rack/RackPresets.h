#pragma once
#include "Core/FxProjectState.h"

namespace hpeq::rack
{
struct Recipe
{
    juce::String id, bank, category, name, intent, rejection;
    double bpm = 120;
    juce::var data;
};
const std::vector<Recipe>& recipes();
juce::String bankSha256();
// All native defaults are applied before authored physical parameters. Fails
// atomically; no unknown key, scene event or unsupported topology is discarded.
bool makeProject(const Recipe&, FxProjectState&, juce::String& error,bool calibrated=true);
bool applyCalibration(const juce::var&,FxProjectState&,juce::String& error);
}
