#include "DSP/Biquad.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace hpeq::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

struct Prewarp
{
    double cosW, alpha;
};

Prewarp prewarp(double sampleRate, double frequencyHz, double q) noexcept
{
    const double w = 2.0 * kPi * design::clampFrequency(frequencyHz, sampleRate) / sampleRate;
    return { std::cos(w), std::sin(w) / (2.0 * std::max(q, 1.0e-3)) };
}

BiquadCoefficients normalise(double b0, double b1, double b2, double a0, double a1, double a2) noexcept
{
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}
} // namespace

namespace design
{
double clampFrequency(double frequencyHz, double sampleRate) noexcept
{
    const double upper = kNyquistMargin * sampleRate;
    return std::isfinite(frequencyHz) ? std::clamp(frequencyHz, 1.0, upper) : 1000.0;
}

BiquadCoefficients peak(double sampleRate, double frequencyHz, double q, double gainDb) noexcept
{
    if (gainDb == 0.0)
        return {}; // exact identity: a 0 dB bell must not touch the signal
    const double a = std::pow(10.0, gainDb / 40.0);
    const auto [cosW, alpha] = prewarp(sampleRate, frequencyHz, q);
    return normalise(1.0 + alpha * a, -2.0 * cosW, 1.0 - alpha * a, 1.0 + alpha / a, -2.0 * cosW, 1.0 - alpha / a);
}

BiquadCoefficients lowPass(double sampleRate, double frequencyHz, double q) noexcept
{
    const auto [cosW, alpha] = prewarp(sampleRate, frequencyHz, q);
    return normalise((1.0 - cosW) * 0.5, 1.0 - cosW, (1.0 - cosW) * 0.5, 1.0 + alpha, -2.0 * cosW, 1.0 - alpha);
}

BiquadCoefficients highPass(double sampleRate, double frequencyHz, double q) noexcept
{
    const auto [cosW, alpha] = prewarp(sampleRate, frequencyHz, q);
    return normalise((1.0 + cosW) * 0.5, -(1.0 + cosW), (1.0 + cosW) * 0.5, 1.0 + alpha, -2.0 * cosW, 1.0 - alpha);
}

BiquadCoefficients allPass(double sampleRate, double frequencyHz, double q) noexcept
{
    const auto [cosW, alpha] = prewarp(sampleRate, frequencyHz, q);
    return normalise(1.0 - alpha, -2.0 * cosW, 1.0 + alpha, 1.0 + alpha, -2.0 * cosW, 1.0 - alpha);
}
} // namespace design

double magnitudeDb(const BiquadCoefficients& c, double sampleRate, double frequencyHz) noexcept
{
    const double w = 2.0 * kPi * frequencyHz / sampleRate;
    const std::complex<double> z1 = std::polar(1.0, -w), z2 = z1 * z1;
    const auto h = (c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2);
    return 20.0 * std::log10(std::max(std::abs(h), 1.0e-20));
}
} // namespace hpeq::dsp
