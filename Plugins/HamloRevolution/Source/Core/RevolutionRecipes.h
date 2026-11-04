#pragma once
#include "Core/RevolutionSimple.h"
namespace hpeq::revolution::simple
{
// R10 explicit generation only. The R6 program-loading generator stays frozen.
Result levelRecipe(int lane, int target, int variant, const Settings&, const Hits&);
void addSpatialRecipes(Result&, const Settings&, const Hits&);
}
