#pragma once

#include <optional>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

namespace hpeq::test
{
// Metrics from 04_ACCEPTANCE_MATRIX.md §3. All levels are relative to digital full scale 1.0
// (0 dBFS); RMS is plain unweighted RMS, so a 0 dBFS-peak sine reads -3.01 dB RMS.
constexpr double kSilenceDb = -400.0; // reported instead of -inf

double toDb(double linear) noexcept;

struct LevelStats
{
    double peak = 0.0;
    double rms = 0.0;
    double dc = 0.0;
    double peakDbfs = kSilenceDb;
    double rmsDbfs = kSilenceDb;
    double crestDb = 0.0;
    bool allFinite = true;
    int subnormalCount = 0;
};

LevelStats measureChannel(const juce::AudioBuffer<float>&, int channel);
LevelStats measureAll(const juce::AudioBuffer<float>&); // all channels pooled

// error_dB = 20 log10( RMS(y - x) / max(RMS(x), eps) ), channels pooled.
// `y` is compared from sample `delay` on (y[n + delay] vs x[n]) - the alignment is explicit.
struct NullResult
{
    bool sameShape = false;
    bool bitExact = false;
    double errorDb = kSilenceDb;   // relative to the reference RMS
    double maxAbsError = 0.0;
    double referenceRmsDbfs = kSilenceDb;
};

NullResult compareAligned(const juce::AudioBuffer<float>& y, const juce::AudioBuffer<float>& x, int delay = 0);

// Lag (in samples, 0..maxLag) maximising the cross-correlation of channel 0 of y against x.
int estimateDelay(const juce::AudioBuffer<float>& y, const juce::AudioBuffer<float>& x, int maxLag);

// H(f) = FFT(y)/FFT(x) on channel `channel`, zero-padded to a power of two.
// Bins where |X| is more than `maskDb` below max|X| are unreliable and reported as nullopt.
struct TransferPoint
{
    double frequencyHz = 0.0;
    std::optional<double> magnitudeDb;
    std::optional<double> phaseRadians;
};

std::vector<TransferPoint> measureTransfer(const juce::AudioBuffer<float>& y,
                                           const juce::AudioBuffer<float>& x,
                                           double sampleRate,
                                           const std::vector<double>& frequencies,
                                           int channel = 0,
                                           double maskDb = -100.0);

// Frequency of the strongest FFT bin (parabolic interpolation) of one channel.
double dominantFrequency(const juce::AudioBuffer<float>&, double sampleRate, int channel = 0);

// Power-weighted spectral centroid of one channel.
double spectralCentroid(const juce::AudioBuffer<float>&, double sampleRate, int channel = 0);

// Mid/side energies with M = (L+R)/2, S = (L-R)/2 (01 §4 Channel routing).
struct StereoStats
{
    double midRmsDbfs = kSilenceDb;
    double sideRmsDbfs = kSilenceDb;
    double correlation = 0.0; // Pearson L/R, 0 if either side is silent
};

StereoStats measureStereo(const juce::AudioBuffer<float>&);

struct TimingStats
{
    double p50 = 0.0, p95 = 0.0, p99 = 0.0, max = 0.0, mean = 0.0;
    int count = 0;
};

TimingStats summarise(std::vector<double> values);
} // namespace hpeq::test
