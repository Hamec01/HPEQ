#pragma once

#include <array>
#include <complex>

#include "Core/FxProjectState.h"

namespace hpeq::dsp
{
// PRE-EQ band design (M3). Every band is a cascade of up to kMaxEqSections sections, each either a
// trapezoidal (TPT) state-variable filter or a TPT one-pole. Both are bilinear transforms of their
// analog prototypes with frequency pre-warping, so `bandResponse()` evaluates the exact digital
// response of the coefficients actually used (03 Part B "EQ response = real coefficients").
//
// 2nd-order mixing (Simper): y = m0*x + m1*band + m2*low, with prototype
//     H(s) = m0 + (m1*s + m2) / (s^2 + k*s + 1),   s = j*tan(pi f / fs) / g
// Shapes (A = 10^(gainDb/40), t = tan(pi fc / fs)):
//     Bell       g = t,       k = 1/(Q A), m = (1, k(A^2-1), 0)
//     LowShelf   g = t/sqrt A, k = 1/Q,    m = (1, k(A-1), A^2-1)
//     HighShelf  g = t*sqrt A, k = 1/Q,    m = (A^2, k(1-A)A, 1-A^2)
//     Notch      m = (1, -k, 0);  BandPass (0 dB peak) m = (0, k, 0);  AllPass m = (1, -2k, 0)
//     Low/High Cut: Butterworth of order N = slope / 6 (one-pole for odd N), Q ignored
//     Tilt: first-order (sqrt(g) s + 1) / (s + sqrt(g)), g = 10^(G/20): -G/2 lows, +G/2 highs, 0 dB at fc
// These equal the RBJ cookbook designs (checked by test against Biquad designs).
constexpr int kMaxEqSections = 8; // 96 dB/oct = 16th order = 8 second-order sections
constexpr int kEqSlopes[] { 6, 12, 18, 24, 36, 48, 72, 96 };

struct SectionSpec
{
    bool onePole = false;
    double g = 0.0, k = 1.4142135623730951;
    double m0 = 1.0, m1 = 0.0, m2 = 0.0; // one-pole: y = m1*high + m2*low (m0 unused)
};

struct BandDesign
{
    int count = 0;
    std::array<SectionSpec, kMaxEqSections> sections {};
};

int butterworthOrder(int slopeDbPerOct) noexcept; // slope snapped to kEqSlopes

BandDesign designBand(EqShape, int slopeDbPerOct, double sampleRate, double frequencyHz, double gainDb, double q) noexcept;

std::complex<double> sectionResponse(const SectionSpec&, double sampleRate, double frequencyHz) noexcept;
std::complex<double> bandResponse(const BandDesign&, double sampleRate, double frequencyHz) noexcept;

// Shapes whose 0 dB setting is an exact identity (they are faded by gain and skipped at 0 dB).
bool isGainShape(EqShape) noexcept;
} // namespace hpeq::dsp
