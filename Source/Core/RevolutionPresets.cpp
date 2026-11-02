#include "Core/RevolutionPresets.h"

namespace hpeq::revolution
{
const std::vector<GenrePreset>& genrePresets()
{
    static const std::vector<GenrePreset> presets {
#include "Core/RevolutionPresets.inc"
    };
    return presets;
}
} // namespace hpeq::revolution
