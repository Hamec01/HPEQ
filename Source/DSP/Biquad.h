#pragma once

#include <array>
#include <cmath>

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

// Topology-preserving-transform state-variable filter (Zavalishin; Simper's trapezoidal SVF).
// Same bilinear, pre-warped transfer functions as the RBJ designs above, but it stays stable and
// click-free when its frequency is modulated quickly, including from/to 1 Hz. Used by the
// crossover tree, whose edges glide over many octaves.
class TptSvf
{
public:
    enum class Mode { LowPass, HighPass, AllPass };

    void setMode(Mode m) noexcept { mode = m; }
    void setFrequency(double sampleRate, double frequencyHz, double q) noexcept
    {
        const double g = std::tan(3.14159265358979323846 * design::clampFrequency(frequencyHz, sampleRate) / sampleRate);
        k = 1.0 / q;
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() noexcept { state = {}; }

    double processSample(int channel, double v0) noexcept
    {
        auto& s = state[static_cast<size_t>(channel)];
        const double v3 = v0 - s[1];
        const double v1 = a1 * s[0] + a2 * v3;
        const double v2 = s[1] + a2 * s[0] + a3 * v3;
        s[0] = 2.0 * v1 - s[0];
        s[1] = 2.0 * v2 - s[1];
        switch (mode)
        {
            case Mode::LowPass:  return v2;
            case Mode::HighPass: return v0 - k * v1 - v2;
            case Mode::AllPass:  return v0 - 2.0 * k * v1;
        }
        return v0;
    }

private:
    Mode mode = Mode::AllPass;
    double k = 1.4142135623730951, a1 = 1.0, a2 = 0.0, a3 = 0.0;
    std::array<std::array<double, 2>, kMaxChannels> state {};
};

// |H(e^jw)| in dB of one section, for analytic checks and the future EQ display (M5).
double magnitudeDb(const BiquadCoefficients&, double sampleRate, double frequencyHz) noexcept;
} // namespace hpeq::dsp
