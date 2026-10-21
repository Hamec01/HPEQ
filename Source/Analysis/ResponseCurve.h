#pragma once

#include <vector>

#include "Core/FxProjectState.h"
#include "DSP/EqDesign.h"

namespace hpeq
{
// The PRE-EQ response drawn in the workspace: the product of `dsp::bandResponse()` of every enabled
// EQ node, i.e. the exact digital response of the coefficients the DSP uses (03 Part B). Drawn for
// the Stereo path; bands in Left/Right/Mid/Side modes are included as if they acted on the drawn
// channel (the workspace says which channel view it shows). FX processing is never drawn as an EQ curve.
inline double eqResponseDb(const LayerState& layer, double sampleRate, double frequencyHz)
{
    std::complex<double> h(1.0, 0.0);
    for (const auto& node : layer.eqNodes)
        if (node.enabled && !(dsp::isGainShape(node.shape) && node.gainDb == 0.0))
            h *= dsp::bandResponse(dsp::designBand(node.shape, node.slopeDbPerOct, sampleRate, node.frequencyHz, node.gainDb, node.q), sampleRate, frequencyHz);
    return 20.0 * std::log10(std::max(std::abs(h), 1.0e-12));
}

inline double bandResponseDb(const EqNodeState& node, double sampleRate, double frequencyHz)
{
    const auto h = dsp::bandResponse(dsp::designBand(node.shape, node.slopeDbPerOct, sampleRate, node.frequencyHz, node.gainDb, node.q), sampleRate, frequencyHz);
    return 20.0 * std::log10(std::max(std::abs(h), 1.0e-12));
}
} // namespace hpeq
