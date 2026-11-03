#include "Core/ModulePresets.h"
namespace hpeq::presets
{
std::vector<ModulePreset> genreModulePresets()
{
    return {
#include "Presets/GeneratedZastavaPresets.h"
    };
}
}
