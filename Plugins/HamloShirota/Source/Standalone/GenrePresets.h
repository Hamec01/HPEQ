#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <utility>
namespace hpeq::standalone
{
struct GenrePreset
{
    const char* id;
    const char* name;
    const char* genre;
    const char* group;
    const char* note;
    const char* material;
    const char* depth;
    double bpm;
    std::vector<std::pair<juce::String,double>> values;
};
const std::vector<GenrePreset>& genrePresets();
}
