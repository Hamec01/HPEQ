#pragma once

#include <vector>

#include <juce_dsp/juce_dsp.h>

namespace hpeq
{
// FFT analyser for the spectrum workspace (UI thread). Calibration: Hann window, amplitude spectrum
// normalised by the window's coherent gain, so a sine whose frequency sits on a bin reads its peak
// level in dBFS (a 0 dBFS-peak sine reads 0 dB). Off-bin sines read up to 1.42 dB low (Hann
// scalloping); the display states FFT size and window. Power is averaged with an exponential time
// constant; `difference` gives 20 log10(|Y| / |X|) only where |X| is above a fixed mask, else NaN.
class SpectrumAnalyzer
{
public:
    explicit SpectrumAnalyzer(int fftOrder = 13);

    int size() const noexcept { return fftSize; }

    // Instantaneous level per bin (dBFS) of the last `size()` samples in `signal`.
    void analyse(const float* signal, std::vector<float>& levelDb);

    // Exponential average in the power domain; alpha = 1 means no smoothing.
    static void average(std::vector<float>& smoothedDb, const std::vector<float>& newDb, float alpha);

    // Per-bin difference out - in in dB where in is above maskDb; NaN where it is not.
    static void difference(const std::vector<float>& outDb, const std::vector<float>& inDb, float maskDb, std::vector<float>& diffDb);

    double binFrequency(int bin, double sampleRate) const noexcept { return bin * sampleRate / fftSize; }

private:
    int fftSize;
    juce::dsp::FFT fft;
    std::vector<float> window, work;
    float windowGain = 1.0f;
};
} // namespace hpeq
