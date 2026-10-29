#pragma once

#include <array>
#include <emmintrin.h>
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
    // `interpolate`: glide g linearly over the next kControlInterval samples (call advance() once per
    // sample), instead of stepping it; a stepped g leaks zipper products into the output.
    void setFrequency(double sampleRate, double frequencyHz, double q, bool interpolate = false) noexcept
    {
        const double target = std::tan(3.14159265358979323846 * design::clampFrequency(frequencyHz, sampleRate) / sampleRate);
        k = 1.0 / q;
        if (!interpolate)
        {
            g = gTarget = target;
            remaining = 0;
            derive();
            return;
        }
        gTarget = target;
        gStep = (target - g) / kControlInterval;
        remaining = kControlInterval;
    }
    void advance() noexcept
    {
        if (remaining <= 0)
            return;
        g = --remaining == 0 ? gTarget : g + gStep;
        derive();
    }
    void reset() noexcept
    {
        s0 = {};
        s1 = {};
    }

    double processSample(int channel, double v0) noexcept
    {
        auto& z0 = s0[static_cast<size_t>(channel)];
        auto& z1 = s1[static_cast<size_t>(channel)];
        const double v3 = v0 - z1;
        const double v1 = a1 * z0 + a2 * v3;
        const double v2 = z1 + a2 * z0 + a3 * v3;
        z0 = 2.0 * v1 - z0;
        z1 = 2.0 * v2 - z1;
        switch (mode)
        {
            case Mode::LowPass:  return v2;
            case Mode::HighPass: return v0 - k * v1 - v2;
            case Mode::AllPass:  return v0 - 2.0 * k * v1;
        }
        return v0;
    }

    // Both channels at once (SSE2), the same operations in the same order as processSample (no FMA
    // contraction under /fp:precise), so the result is bit-identical to two scalar calls (M8).
    __m128d processStereo(__m128d v0) noexcept
    {
        const __m128d z0 = _mm_loadu_pd(s0.data()), z1 = _mm_loadu_pd(s1.data());
        const __m128d c1 = _mm_set1_pd(a1), c2 = _mm_set1_pd(a2), c3 = _mm_set1_pd(a3);
        const __m128d v3 = _mm_sub_pd(v0, z1);
        const __m128d v1 = _mm_add_pd(_mm_mul_pd(c1, z0), _mm_mul_pd(c2, v3));
        const __m128d v2 = _mm_add_pd(_mm_add_pd(z1, _mm_mul_pd(c2, z0)), _mm_mul_pd(c3, v3));
        const __m128d two = _mm_set1_pd(2.0);
        _mm_storeu_pd(s0.data(), _mm_sub_pd(_mm_mul_pd(two, v1), z0));
        _mm_storeu_pd(s1.data(), _mm_sub_pd(_mm_mul_pd(two, v2), z1));
        switch (mode)
        {
            case Mode::LowPass:  return v2;
            case Mode::HighPass: return _mm_sub_pd(_mm_sub_pd(v0, _mm_mul_pd(_mm_set1_pd(k), v1)), v2);
            case Mode::AllPass:  return _mm_sub_pd(v0, _mm_mul_pd(_mm_set1_pd(2.0 * k), v1));
        }
        return v0;
    }

private:
    void derive() noexcept
    {
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    Mode mode = Mode::AllPass;
    double k = 1.4142135623730951, a1 = 1.0, a2 = 0.0, a3 = 0.0;
    double g = 0.0, gTarget = 0.0, gStep = 0.0;
    int remaining = 0;
    std::array<double, 2> s0 {}, s1 {}; // integrator states per channel (L, R adjacent for SSE2)
};

// |H(e^jw)| in dB of one section, for analytic checks and the future EQ display (M5).
double magnitudeDb(const BiquadCoefficients&, double sampleRate, double frequencyHz) noexcept;
} // namespace hpeq::dsp
