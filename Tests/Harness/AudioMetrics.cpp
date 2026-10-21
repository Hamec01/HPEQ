#include "Harness/AudioMetrics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <limits>

#include <juce_dsp/juce_dsp.h>

namespace hpeq::test
{
namespace
{
bool isSubnormal(float v) noexcept
{
    return v != 0.0f && std::fpclassify(v) == FP_SUBNORMAL;
}

int nextPowerOfTwoOrder(int length)
{
    int order = 1;
    while ((1 << order) < length)
        ++order;
    return order;
}

// Complex spectrum of one channel, zero-padded to 2^order.
std::vector<std::complex<double>> spectrumOf(const juce::AudioBuffer<float>& buffer, int channel, int order)
{
    const int size = 1 << order;
    juce::dsp::FFT fft(order);
    std::vector<std::complex<float>> in(static_cast<size_t>(size)), out(static_cast<size_t>(size));
    const auto* data = buffer.getReadPointer(juce::jmin(channel, buffer.getNumChannels() - 1));
    for (int i = 0; i < juce::jmin(size, buffer.getNumSamples()); ++i)
        in[static_cast<size_t>(i)] = { data[i], 0.0f };
    fft.perform(in.data(), out.data(), false);
    std::vector<std::complex<double>> result(static_cast<size_t>(size / 2 + 1));
    for (size_t k = 0; k < result.size(); ++k)
        result[k] = { out[k].real(), out[k].imag() };
    return result;
}
} // namespace

double toDb(double linear) noexcept
{
    return linear > 0.0 ? juce::jmax(kSilenceDb, 20.0 * std::log10(linear)) : kSilenceDb;
}

static LevelStats finish(double peak, double sumSquares, double sum, long long count, bool finite, int subnormals)
{
    LevelStats stats;
    stats.allFinite = finite;
    stats.subnormalCount = subnormals;
    stats.peak = peak;
    stats.rms = count > 0 ? std::sqrt(sumSquares / static_cast<double>(count)) : 0.0;
    stats.dc = count > 0 ? sum / static_cast<double>(count) : 0.0;
    stats.peakDbfs = toDb(stats.peak);
    stats.rmsDbfs = toDb(stats.rms);
    stats.crestDb = stats.rms > 0.0 ? stats.peakDbfs - stats.rmsDbfs : 0.0;
    return stats;
}

LevelStats measureChannel(const juce::AudioBuffer<float>& buffer, int channel)
{
    double peak = 0.0, sumSquares = 0.0, sum = 0.0;
    bool finite = true;
    int subnormals = 0;
    const auto* data = buffer.getReadPointer(channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float v = data[i];
        if (!std::isfinite(v)) { finite = false; continue; }
        subnormals += isSubnormal(v) ? 1 : 0;
        peak = std::max(peak, static_cast<double>(std::abs(v)));
        sumSquares += static_cast<double>(v) * v;
        sum += v;
    }
    return finish(peak, sumSquares, sum, buffer.getNumSamples(), finite, subnormals);
}

LevelStats measureAll(const juce::AudioBuffer<float>& buffer)
{
    double peak = 0.0, sumSquares = 0.0, sum = 0.0;
    bool finite = true;
    int subnormals = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float v = data[i];
            if (!std::isfinite(v)) { finite = false; continue; }
            subnormals += isSubnormal(v) ? 1 : 0;
            peak = std::max(peak, static_cast<double>(std::abs(v)));
            sumSquares += static_cast<double>(v) * v;
            sum += v;
        }
    }
    return finish(peak, sumSquares, sum, static_cast<long long>(buffer.getNumSamples()) * buffer.getNumChannels(), finite, subnormals);
}

NullResult compareAligned(const juce::AudioBuffer<float>& y, const juce::AudioBuffer<float>& x, int delay)
{
    NullResult result;
    result.sameShape = y.getNumChannels() == x.getNumChannels() && y.getNumSamples() == x.getNumSamples();
    const int channels = juce::jmin(y.getNumChannels(), x.getNumChannels());
    const int length = juce::jmin(y.getNumSamples() - delay, x.getNumSamples());
    if (channels <= 0 || length <= 0)
        return result;

    double errorSquares = 0.0, referenceSquares = 0.0;
    bool exact = true;
    for (int ch = 0; ch < channels; ++ch)
    {
        const auto* yd = y.getReadPointer(ch) + delay;
        const auto* xd = x.getReadPointer(ch);
        for (int i = 0; i < length; ++i)
        {
            exact = exact && (yd[i] == xd[i]);
            const double e = static_cast<double>(yd[i]) - xd[i];
            errorSquares += e * e;
            referenceSquares += static_cast<double>(xd[i]) * xd[i];
            result.maxAbsError = std::max(result.maxAbsError, std::abs(e));
        }
    }
    const double n = static_cast<double>(length) * channels;
    const double referenceRms = std::sqrt(referenceSquares / n);
    const double errorRms = std::sqrt(errorSquares / n);
    result.bitExact = exact && delay == 0 && result.sameShape;
    result.referenceRmsDbfs = toDb(referenceRms);
    // Below ~-300 dBFS the reference is silence: report the absolute error instead of a ratio.
    constexpr double epsilon = 1.0e-15;
    result.errorDb = errorRms == 0.0 ? kSilenceDb : toDb(errorRms / std::max(referenceRms, epsilon));
    return result;
}

int estimateDelay(const juce::AudioBuffer<float>& y, const juce::AudioBuffer<float>& x, int maxLag)
{
    const auto* yd = y.getReadPointer(0);
    const auto* xd = x.getReadPointer(0);
    const int length = juce::jmin(y.getNumSamples(), x.getNumSamples());
    int bestLag = 0;
    double best = -std::numeric_limits<double>::infinity();
    for (int lag = 0; lag <= maxLag && lag < length; ++lag)
    {
        double sum = 0.0;
        for (int i = 0; i + lag < length; ++i)
            sum += static_cast<double>(yd[i + lag]) * xd[i];
        if (sum > best)
        {
            best = sum;
            bestLag = lag;
        }
    }
    return bestLag;
}

std::vector<TransferPoint> measureTransfer(const juce::AudioBuffer<float>& y,
                                           const juce::AudioBuffer<float>& x,
                                           double sampleRate,
                                           const std::vector<double>& frequencies,
                                           int channel,
                                           double maskDb)
{
    const int order = nextPowerOfTwoOrder(juce::jmax(y.getNumSamples(), x.getNumSamples()));
    const auto Y = spectrumOf(y, channel, order);
    const auto X = spectrumOf(x, channel, order);
    double maxMagnitude = 0.0;
    for (const auto& v : X)
        maxMagnitude = std::max(maxMagnitude, std::abs(v));
    const double floor = maxMagnitude * std::pow(10.0, maskDb / 20.0);
    const double binHz = sampleRate / static_cast<double>(1 << order);

    std::vector<TransferPoint> points;
    for (double f : frequencies)
    {
        TransferPoint point;
        point.frequencyHz = f;
        const auto bin = static_cast<size_t>(std::llround(f / binHz));
        if (bin < X.size() && std::abs(X[bin]) > floor && floor > 0.0)
        {
            const auto h = Y[bin] / X[bin];
            point.magnitudeDb = toDb(std::abs(h));
            point.phaseRadians = std::arg(h);
        }
        points.push_back(point);
    }
    return points;
}

double dominantFrequency(const juce::AudioBuffer<float>& buffer, double sampleRate, int channel)
{
    const int order = nextPowerOfTwoOrder(buffer.getNumSamples());
    const auto spectrum = spectrumOf(buffer, channel, order);
    size_t best = 1;
    for (size_t k = 1; k + 1 < spectrum.size(); ++k)
        if (std::abs(spectrum[k]) > std::abs(spectrum[best]))
            best = k;
    double offset = 0.0;
    if (best > 0 && best + 1 < spectrum.size())
    {
        const double a = std::log(std::abs(spectrum[best - 1]) + 1e-30);
        const double b = std::log(std::abs(spectrum[best]) + 1e-30);
        const double c = std::log(std::abs(spectrum[best + 1]) + 1e-30);
        const double denominator = a - 2.0 * b + c;
        if (denominator != 0.0)
            offset = 0.5 * (a - c) / denominator;
    }
    return (static_cast<double>(best) + offset) * sampleRate / static_cast<double>(1 << order);
}

double spectralCentroid(const juce::AudioBuffer<float>& buffer, double sampleRate, int channel)
{
    const int order = nextPowerOfTwoOrder(buffer.getNumSamples());
    const auto spectrum = spectrumOf(buffer, channel, order);
    const double binHz = sampleRate / static_cast<double>(1 << order);
    double weighted = 0.0, total = 0.0;
    for (size_t k = 1; k < spectrum.size(); ++k)
    {
        const double power = std::norm(spectrum[k]);
        weighted += power * static_cast<double>(k) * binHz;
        total += power;
    }
    return total > 0.0 ? weighted / total : 0.0;
}

StereoStats measureStereo(const juce::AudioBuffer<float>& buffer)
{
    StereoStats stats;
    if (buffer.getNumChannels() < 2)
        return stats;
    const auto* l = buffer.getReadPointer(0);
    const auto* r = buffer.getReadPointer(1);
    double mm = 0.0, ss = 0.0, ll = 0.0, rr = 0.0, lr = 0.0;
    const int n = buffer.getNumSamples();
    for (int i = 0; i < n; ++i)
    {
        const double m = 0.5 * (static_cast<double>(l[i]) + r[i]);
        const double s = 0.5 * (static_cast<double>(l[i]) - r[i]);
        mm += m * m;
        ss += s * s;
        ll += static_cast<double>(l[i]) * l[i];
        rr += static_cast<double>(r[i]) * r[i];
        lr += static_cast<double>(l[i]) * r[i];
    }
    stats.midRmsDbfs = toDb(std::sqrt(mm / n));
    stats.sideRmsDbfs = toDb(std::sqrt(ss / n));
    stats.correlation = (ll > 0.0 && rr > 0.0) ? lr / std::sqrt(ll * rr) : 0.0;
    return stats;
}

double highBandRelativeDb(const juce::AudioBuffer<float>& audio, double sampleRate, double cutoffHz, int channel, double settleSeconds)
{
    const double w = 2.0 * juce::MathConstants<double>::pi * cutoffHz / sampleRate;
    const double alpha = std::sin(w) / (2.0 * 0.70710678118654752440);
    const double cosW = std::cos(w), a0 = 1.0 + alpha;
    const double b0 = (1.0 + cosW) * 0.5 / a0, b1 = -(1.0 + cosW) / a0, b2 = b0;
    const double a1 = -2.0 * cosW / a0, a2 = (1.0 - alpha) / a0;

    std::array<std::array<double, 2>, 4> state {};
    double hfSquares = 0.0, totalSquares = 0.0;
    const auto* data = audio.getReadPointer(channel);
    const int settle = static_cast<int>(settleSeconds * sampleRate);
    for (int i = 0; i < audio.getNumSamples(); ++i)
    {
        double v = data[i];
        const bool counted = i >= settle;
        totalSquares += counted ? v * v : 0.0;
        for (auto& s : state)
        {
            const double y = b0 * v + s[0];
            s[0] = b1 * v - a1 * y + s[1];
            s[1] = b2 * v - a2 * y;
            v = y;
        }
        hfSquares += counted ? v * v : 0.0;
    }
    return toDb(std::sqrt(hfSquares / std::max(totalSquares, 1.0e-300)));
}

TimingStats summarise(std::vector<double> values)
{
    TimingStats stats;
    stats.count = static_cast<int>(values.size());
    if (values.empty())
        return stats;
    std::sort(values.begin(), values.end());
    const auto at = [&](double q)
    {
        const auto index = static_cast<size_t>(std::ceil(q * static_cast<double>(values.size()))) - 1;
        return values[std::min(index, values.size() - 1)];
    };
    stats.p50 = at(0.50);
    stats.p95 = at(0.95);
    stats.p99 = at(0.99);
    stats.max = values.back();
    double sum = 0.0;
    for (double v : values)
        sum += v;
    stats.mean = sum / static_cast<double>(values.size());
    return stats;
}
} // namespace hpeq::test
