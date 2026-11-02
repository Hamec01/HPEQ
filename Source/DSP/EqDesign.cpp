#include "DSP/EqDesign.h"

#include <algorithm>
#include <cmath>

#include "DSP/Biquad.h"

namespace hpeq::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

double prewarp(double sampleRate, double frequencyHz) noexcept
{
    return std::tan(kPi * design::clampFrequency(frequencyHz, sampleRate) / sampleRate);
}

SectionSpec svf(double g, double k, double m0, double m1, double m2) noexcept
{
    SectionSpec s;
    s.g = g;
    s.k = k;
    s.m0 = m0;
    s.m1 = m1;
    s.m2 = m2;
    return s;
}

SectionSpec onePole(double g, double highGain, double lowGain) noexcept
{
    SectionSpec s;
    s.onePole = true;
    s.g = g;
    s.m1 = highGain;
    s.m2 = lowGain;
    return s;
}

void butterworth(BandDesign& d, double t, int order, bool highPass) noexcept
{
    if (order % 2 == 1)
        d.sections[static_cast<size_t>(d.count++)] = highPass ? onePole(t, 1.0, 0.0) : onePole(t, 0.0, 1.0);
    // Pole pairs: Q_i = 1 / (2 cos(theta_i)), theta_i = pi (2i + 1) / (2N) for even N, pi i / N for odd N.
    for (int i = 0; i < order / 2; ++i)
    {
        const double theta = order % 2 == 0 ? kPi * (2.0 * i + 1.0) / (2.0 * order) : kPi * (i + 1.0) / order;
        const double k = 2.0 * std::cos(theta); // 1/Q
        d.sections[static_cast<size_t>(d.count++)] = highPass ? svf(t, k, 1.0, -k, -1.0) : svf(t, k, 0.0, 0.0, 1.0);
    }
}
} // namespace

int butterworthOrder(int slopeDbPerOct) noexcept
{
    int best = kEqSlopes[0];
    for (int s : kEqSlopes)
        if (std::abs(s - slopeDbPerOct) < std::abs(best - slopeDbPerOct))
            best = s;
    return best / 6;
}

bool isGainShape(EqShape shape) noexcept
{
    return shape == EqShape::Bell || shape == EqShape::LowShelf || shape == EqShape::HighShelf || shape == EqShape::Tilt;
}

BandDesign designBand(EqShape shape, int slope, double sampleRate, double frequencyHz, double gainDb, double q) noexcept
{
    BandDesign d;
    const double t = prewarp(sampleRate, frequencyHz);
    const double a = std::pow(10.0, gainDb / 40.0);
    const double k = 1.0 / std::max(q, 0.025);
    switch (shape)
    {
        case EqShape::Bell:      d.sections[0] = svf(t, k / a, 1.0, (k / a) * (a * a - 1.0), 0.0); d.count = 1; break;
        case EqShape::LowShelf:  d.sections[0] = svf(t / std::sqrt(a), k, 1.0, k * (a - 1.0), a * a - 1.0); d.count = 1; break;
        case EqShape::HighShelf: d.sections[0] = svf(t * std::sqrt(a), k, a * a, k * (1.0 - a) * a, 1.0 - a * a); d.count = 1; break;
        case EqShape::Notch:     d.sections[0] = svf(t, k, 1.0, -k, 0.0); d.count = 1; break;
        case EqShape::BandPass:  d.sections[0] = svf(t, k, 0.0, k, 0.0); d.count = 1; break;
        case EqShape::AllPass:   d.sections[0] = svf(t, k, 1.0, -2.0 * k, 0.0); d.count = 1; break;
        case EqShape::LowCut:    butterworth(d, t, butterworthOrder(slope), true); break;
        case EqShape::HighCut:   butterworth(d, t, butterworthOrder(slope), false); break;
        case EqShape::Tilt:
        {
            // H(s) = (sqrt(g) s + 1) / (s + sqrt(g)), g = 10^(G/20): -G/2 dB at DC, +G/2 dB at HF,
            // exactly 0 dB at fc. With s' = s / sqrt(g): sqrt(g) * high(s') + low(s') / sqrt(g).
            const double half = std::pow(10.0, gainDb / 40.0);
            d.sections[0] = onePole(t * half, half, 1.0 / half);
            d.count = 1;
            break;
        }
    }
    return d;
}

std::complex<double> sectionResponse(const SectionSpec& s, double sampleRate, double frequencyHz) noexcept
{
    const std::complex<double> sn(0.0, prewarp(sampleRate, frequencyHz) / s.g);
    if (s.onePole)
        return (s.m1 * sn + s.m2) / (sn + 1.0);
    return s.m0 + (s.m1 * sn + s.m2) / (sn * sn + s.k * sn + 1.0);
}

std::complex<double> bandResponse(const BandDesign& d, double sampleRate, double frequencyHz) noexcept
{
    std::complex<double> h(1.0, 0.0);
    for (int i = 0; i < d.count; ++i)
        h *= sectionResponse(d.sections[static_cast<size_t>(i)], sampleRate, frequencyHz);
    return h;
}
} // namespace hpeq::dsp
