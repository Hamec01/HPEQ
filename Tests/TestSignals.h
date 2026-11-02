#pragma once

#include <cmath>
#include <cstring>

#include <juce_audio_basics/juce_audio_basics.h>

namespace hamlo::test
{
constexpr double kFs = 48000.0;

inline juce::AudioBuffer<float> sine(double seconds, double hz, double dbfs, int channels = 2)
{
    const int n = static_cast<int>(seconds * kFs);
    juce::AudioBuffer<float> x(channels, n);
    const double a = std::pow(10.0, dbfs / 20.0);
    for (int i = 0; i < n; ++i)
        for (int ch = 0; ch < channels; ++ch)
            x.setSample(ch, i, static_cast<float>(a * std::sin(2.0 * juce::MathConstants<double>::pi * hz * i / kFs)));
    return x;
}

// Seeded synthetic drums at 120 BPM: a pitched kick on every beat, a noise snare on 2 and 4, hats on the
// eighths (alternating sides), peaks near -1 dBFS.
inline juce::AudioBuffer<float> drums(double seconds, int seed = 7)
{
    const int n = static_cast<int>(seconds * kFs), beat = static_cast<int>(kFs / 2);
    juce::AudioBuffer<float> x(2, n);
    juce::Random rng(seed);
    for (int i = 0; i < n; ++i)
    {
        const int inBeat = i % beat, which = (i / beat) % 4, inEighth = i % (beat / 2);
        const double tb = inBeat / kFs, te = inEighth / kFs;
        double v = 0.7 * std::exp(-tb / 0.09) * std::sin(2.0 * juce::MathConstants<double>::pi * (50.0 + 90.0 * std::exp(-tb / 0.012)) * tb);
        const double noise = 2.0 * rng.nextDouble() - 1.0;
        if (which == 1 || which == 3)
            v += 0.35 * std::exp(-tb / 0.07) * noise;
        const double hat = 0.1 * std::exp(-te / 0.012) * noise;
        const bool left = (i / (beat / 2)) % 2 == 0;
        x.setSample(0, i, static_cast<float>(v + hat * (left ? 1.0 : 0.35)));
        x.setSample(1, i, static_cast<float>(v + hat * (left ? 0.35 : 1.0)));
    }
    return x;
}

inline juce::AudioBuffer<float> segment(const juce::AudioBuffer<float>& x, int start, int length)
{
    juce::AudioBuffer<float> y(x.getNumChannels(), length);
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, 0, x, ch, start, length);
    return y;
}

inline double rmsDb(const juce::AudioBuffer<float>& x)
{
    double s = 0.0;
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        for (int i = 0; i < x.getNumSamples(); ++i)
            s += static_cast<double>(x.getSample(ch, i)) * x.getSample(ch, i);
    return 10.0 * std::log10(s / (x.getNumChannels() * static_cast<double>(x.getNumSamples())) + 1.0e-30);
}

inline bool bitExact(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
        return false;
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        if (std::memcmp(a.getReadPointer(ch), b.getReadPointer(ch), sizeof(float) * static_cast<size_t>(a.getNumSamples())) != 0)
            return false;
    return true;
}

// Difference a - b relative to b's RMS, dB (-inf as -400).
inline double differenceDb(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    double d = 0.0, r = 0.0;
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        for (int i = 0; i < a.getNumSamples(); ++i)
        {
            const double e = static_cast<double>(a.getSample(ch, i)) - b.getSample(ch, i);
            d += e * e;
            r += static_cast<double>(b.getSample(ch, i)) * b.getSample(ch, i);
        }
    return d == 0.0 ? -400.0 : 10.0 * std::log10(d / (r + 1.0e-30));
}
} // namespace hamlo::test
