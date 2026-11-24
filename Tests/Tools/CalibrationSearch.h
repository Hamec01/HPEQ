#pragma once
#include <cmath>

namespace hpeq::test {
// Heavy compression has an asymmetric admissible band (target 8, band 6--12).
// A legal measurement must win over a closer but illegal one (e.g. 11.9 vs 5.9).
inline bool preferCompressorMeasurement(double candidate,double current,double target,double lower,double upper)
{
    const bool candidateValid=candidate>=lower&&candidate<=upper;
    const bool currentValid=current>=lower&&current<=upper;
    if(candidateValid!=currentValid)return candidateValid;
    return std::abs(candidate-target)<std::abs(current-target);
}
}
