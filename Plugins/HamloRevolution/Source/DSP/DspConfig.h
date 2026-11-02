#pragma once

namespace hpeq::dsp
{
// Engine-wide constants. Everything that sizes memory is fixed here so the audio thread never
// allocates (01 §10).
constexpr int kMaxChannels = 2;

// Coefficient-rate updates happen every kControlInterval samples, counted on a running sample clock
// (not per host block), so a render does not depend on the host's buffer size (T10).
constexpr int kControlInterval = 32;

constexpr double kParameterRampSeconds = 0.030; // frequency / gain / Q glides
constexpr double kMixRampSeconds = 0.020;       // mix and on/off fades

// Upper frequency for any filter design: keeps bilinear-transform designs away from Nyquist.
constexpr double kNyquistMargin = 0.45;
} // namespace hpeq::dsp
