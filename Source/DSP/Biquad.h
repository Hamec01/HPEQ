#pragma once

#include <array>

#include "DSP/DspConfig.h"

namespace hpeq::dsp
{
// Normalised second-order section, a0 == 1. Designed in double, run in double.
struct BiquadCoefficients
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

    bool isIdentity() const noexcept { return b0 == 1.0 && b1 == 0.0 && b2 == 0.0 && a1 == 0.0 && a2 == 0.0; }
};

// RBJ "Audio EQ Cookbook" designs (bilinear transform with frequency pre-warping). All functions are
// allocation-free and safe on the audio thread. Frequencies are clamped to [1 Hz, kNyquistMargin*fs].
namespace design
{
double clampFrequency(double frequencyHz, double sampleRate) noexcept;

BiquadCoefficients peak(double sampleRate, double frequencyHz, double q, double gainDb) noexcept;
BiquadCoefficients lowPass(double sampleRate, double frequencyHz, double q) noexcept;
BiquadCoefficients highPass(double sampleRate, double frequencyHz, double q) noexcept;
BiquadCoefficients allPass(double sampleRate, double frequencyHz, double q) noexcept;

constexpr double kButterworthQ = 0.70710678118654752440;
} // namespace design

// Transposed direct form II, one state pair per channel.
class Biquad
{
public:
    void setCoefficients(const BiquadCoefficients& c) noexcept { coefficients = c; }
    const BiquadCoefficients& getCoefficients() const noexcept { return coefficients; }
    void reset() noexcept { state = {}; }

    double processSample(int channel, double x) noexcept
    {
        auto& s = state[static_cast<size_t>(channel)];
        const double y = coefficients.b0 * x + s[0];
        s[0] = coefficients.b1 * x - coefficients.a1 * y + s[1];
        s[1] = coefficients.b2 * x - coefficients.a2 * y;
        return y;
    }

private:
    BiquadCoefficients coefficients;
    std::array<std::array<double, 2>, kMaxChannels> state {};
};

// |H(e^jw)| in dB of one section, for analytic checks and the future EQ display (M5).
double magnitudeDb(const BiquadCoefficients&, double sampleRate, double frequencyHz) noexcept;
} // namespace hpeq::dsp
