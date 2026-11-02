#pragma once
#include "Standalone/ScoreParameters.h"

namespace hpeq::standalone
{
// Canonical host-normalised recipe. Shared mix/output/routing controls stay live.
struct QuickRecipe
{
    std::array<float,scoreparam::BaseCount> normalised{};
    std::array<bool,scoreparam::BaseCount> applies{};
    float actual(int index) const noexcept;
};
QuickRecipe quickRecipe(int style,float time,float motion,float space,bool sync) noexcept;
const char* quickName(int style) noexcept;
const char* quickDescription(int style) noexcept;
double quickMilliseconds(float time) noexcept;
int quickDivision(float time) noexcept;
float quickTimeForDivision(int division) noexcept;
}
