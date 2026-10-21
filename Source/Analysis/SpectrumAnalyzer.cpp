#include "Analysis/SpectrumAnalyzer.h"

#include <cmath>
#include <limits>

namespace hpeq
{
SpectrumAnalyzer::SpectrumAnalyzer(int fftOrder)
    : fftSize(1 << fftOrder), fft(fftOrder), window(static_cast<size_t>(fftSize)), work(static_cast<size_t>(2 * fftSize))
{
    double sum = 0.0;
    for (int i = 0; i < fftSize; ++i)
    {
        window[static_cast<size_t>(i)] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi * i / fftSize));
        sum += window[static_cast<size_t>(i)];
    }
    windowGain = static_cast<float>(sum / 2.0); // one-sided amplitude: |X_k| = A * sum(w) / 2
}

void SpectrumAnalyzer::analyse(const float* signal, std::vector<float>& levelDb)
{
    std::fill(work.begin(), work.end(), 0.0f);
    for (int i = 0; i < fftSize; ++i)
        work[static_cast<size_t>(i)] = signal[i] * window[static_cast<size_t>(i)];
    fft.performFrequencyOnlyForwardTransform(work.data(), true);
    levelDb.resize(static_cast<size_t>(fftSize / 2 + 1));
    for (size_t k = 0; k < levelDb.size(); ++k)
        levelDb[k] = 20.0f * std::log10(std::max(work[k] / windowGain, 1.0e-10f));
}

void SpectrumAnalyzer::average(std::vector<float>& smoothedDb, const std::vector<float>& newDb, float alpha)
{
    if (smoothedDb.size() != newDb.size())
    {
        smoothedDb = newDb;
        return;
    }
    for (size_t k = 0; k < newDb.size(); ++k)
    {
        const float p = std::pow(10.0f, smoothedDb[k] / 10.0f), q = std::pow(10.0f, newDb[k] / 10.0f);
        smoothedDb[k] = 10.0f * std::log10(std::max(p + alpha * (q - p), 1.0e-20f));
    }
}

void SpectrumAnalyzer::difference(const std::vector<float>& outDb, const std::vector<float>& inDb, float maskDb, std::vector<float>& diffDb)
{
    diffDb.resize(outDb.size());
    for (size_t k = 0; k < outDb.size() && k < inDb.size(); ++k)
        diffDb[k] = inDb[k] > maskDb ? outDb[k] - inDb[k] : std::numeric_limits<float>::quiet_NaN();
}
} // namespace hpeq
