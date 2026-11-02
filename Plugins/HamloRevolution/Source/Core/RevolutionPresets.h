#pragma once

#include <cstdint>
#include <vector>

// R6 (docs/REVOLUTION_V2.md section 11): HamloRevolution's 200 genre presets - SIMPLE recipes (Tools/revolution_presets.py
// writes RevolutionPresets.inc). Loading one resets the parameters, opens SIMPLE and runs the generator.
namespace hpeq::revolution
{
struct GenrePreset
{
    const char* genre;
    const char* group;
    const char* name;
    const char* note;
    int simpleGenre;                          // simple::Genre
    float intent, amount, variation, groove;
    int fill, target;                         // simple::Fill, simple::Target
    float mix;
    std::uint32_t seed;
};
const std::vector<GenrePreset>& genrePresets();
} // namespace hpeq::revolution
