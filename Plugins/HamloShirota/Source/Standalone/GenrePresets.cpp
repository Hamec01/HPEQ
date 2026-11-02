#include "Standalone/GenrePresets.h"
namespace hpeq::standalone
{
const std::vector<GenrePreset>& genrePresets()
{
    static const std::vector<GenrePreset> presets {
#include "GenrePresets.inc"
    };
    return presets;
}
}
