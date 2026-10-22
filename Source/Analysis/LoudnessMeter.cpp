#include "Analysis/LoudnessMeter.h"

#include <algorithm>
#include <cmath>

namespace hpeq
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

double besselI0(double x) noexcept
{
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 40; ++k)
    {
        term *= (x / (2.0 * k)) * (x / (2.0 * k));
        sum += term;
    }
    return sum;
}

double toLufs(double meanSquare) noexcept
{
    return meanSquare > 0.0 ? -0.691 + 10.0 * std::log10(meanSquare) : LoudnessMeter::kSilence;
}
} // namespace

void LoudnessMeter::prepare(double newSampleRate, bool measureTruePeak)
{
    sampleRate = newSampleRate;
    truePeakOn = measureTruePeak;
    factor = sampleRate < 96000.0 ? 4 : sampleRate < 192000.0 ? 2 : 1;

    // K-weighting, BS.1770-4 analog prototypes (as in libebur128); at 48 kHz these reproduce the
    // coefficients printed in the standard.
    {
        const double f0 = 1681.974450955533, g = 3.999843853973347, q = 0.7071752369554196;
        const double k = std::tan(kPi * f0 / sampleRate);
        const double vh = std::pow(10.0, g / 20.0), vb = std::pow(vh, 0.4996667741545416);
        const double a0 = 1.0 + k / q + k * k;
        shelf.b0 = (vh + vb * k / q + k * k) / a0;
        shelf.b1 = 2.0 * (k * k - vh) / a0;
        shelf.b2 = (vh - vb * k / q + k * k) / a0;
        shelf.a1 = 2.0 * (k * k - 1.0) / a0;
        shelf.a2 = (1.0 - k / q + k * k) / a0;
    }
    {
        const double f0 = 38.13547087602444, q = 0.5003270373238773;
        const double k = std::tan(kPi * f0 / sampleRate);
        const double a0 = 1.0 + k / q + k * k;
        highPass.b0 = 1.0;
        highPass.b1 = -2.0;
        highPass.b2 = 1.0;
        highPass.a1 = 2.0 * (k * k - 1.0) / a0;
        highPass.a2 = (1.0 - k / q + k * k) / a0;
    }
    hopLength = std::max(1, static_cast<int>(std::lround(0.1 * sampleRate)));

    // Polyphase interpolator: phase p estimates x at (n - 8 + p / factor) from x[n - 15] ... x[n].
    phaseTaps.assign(static_cast<size_t>(factor * kTapsPerPhase), 0.0f);
    const double fc = 0.48, beta = 9.0, half = kTapsPerPhase / 2.0;
    for (int p = 0; p < factor; ++p)
    {
        double sum = 0.0;
        std::array<double, kTapsPerPhase> c {};
        for (int k = 0; k < kTapsPerPhase; ++k)
        {
            const double d = k - half + static_cast<double>(p) / factor;
            const double x = 2.0 * fc * d;
            const double sinc = std::abs(x) < 1.0e-12 ? 1.0 : std::sin(kPi * x) / (kPi * x);
            const double r = d / (half + 0.5);
            const double w = std::abs(r) < 1.0 ? besselI0(beta * std::sqrt(1.0 - r * r)) / besselI0(beta) : 0.0;
            c[static_cast<size_t>(k)] = 2.0 * fc * sinc * w;
            sum += c[static_cast<size_t>(k)];
        }
        for (int k = 0; k < kTapsPerPhase; ++k)
            phaseTaps[static_cast<size_t>(p * kTapsPerPhase + k)] = static_cast<float>(c[static_cast<size_t>(k)] / sum);
    }
    reset();
}

void LoudnessMeter::reset() noexcept
{
    shelf.z = {};
    highPass.z = {};
    hopFill = 0;
    hopSum = {};
    hops = {};
    hopWrite = hopsSeen = 0;
    histCount = {};
    histEnergy = {};
    history = {};
    historyPos = 0;
    peakTrue = peakSample = 0.0;
    momentary.store(kSilence, std::memory_order_relaxed);
    shortTerm.store(kSilence, std::memory_order_relaxed);
    integrated.store(kSilence, std::memory_order_relaxed);
    truePeak.store(kSilence, std::memory_order_relaxed);
    samplePeak.store(kSilence, std::memory_order_relaxed);
}

void LoudnessMeter::process(const float* const* channels, int numChannels, int numSamples) noexcept
{
    if (resetRequested.exchange(false, std::memory_order_relaxed))
        reset();
    numChannels = std::min(numChannels, 2);
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const double x = channels[ch][i];
            const double k = highPass.process(ch, shelf.process(ch, x));
            hopSum[static_cast<size_t>(ch)] += k * k;
            peakSample = std::max(peakSample, std::abs(x));
            if (truePeakOn)
            {
                auto& h = history[static_cast<size_t>(ch)];
                h[static_cast<size_t>(historyPos)] = static_cast<float>(x);
                h[static_cast<size_t>(historyPos + kTapsPerPhase)] = static_cast<float>(x);
                const float* window = h.data() + historyPos + 1; // oldest ... newest
                for (int p = 0; p < factor; ++p)
                {
                    const float* c = phaseTaps.data() + p * kTapsPerPhase;
                    float y = 0.0f;
                    for (int t = 0; t < kTapsPerPhase; ++t)
                        y += c[t] * window[kTapsPerPhase - 1 - t];
                    peakTrue = std::max(peakTrue, static_cast<double>(std::abs(y)));
                }
            }
        }
        if (truePeakOn && ++historyPos == kTapsPerPhase)
            historyPos = 0;
        if (++hopFill == hopLength)
            endHop();
    }
    const auto db = [](double v) { return v > 0.0 ? 20.0 * std::log10(v) : kSilence; };
    samplePeak.store(db(peakSample), std::memory_order_relaxed);
    truePeak.store(db(std::max(peakTrue, peakSample)), std::memory_order_relaxed);
}

void LoudnessMeter::endHop() noexcept
{
    hops[static_cast<size_t>(hopWrite)] = (hopSum[0] + hopSum[1]) / hopLength;
    hopWrite = (hopWrite + 1) % kShortHops;
    hopSum = {};
    hopFill = 0;
    ++hopsSeen;

    const auto meanOf = [this](int count)
    {
        double sum = 0.0;
        for (int k = 1; k <= count; ++k)
            sum += hops[static_cast<size_t>((hopWrite - k + kShortHops) % kShortHops)];
        return sum / count;
    };
    if (hopsSeen >= kMomentaryHops)
    {
        const double block = meanOf(kMomentaryHops);
        const double l = toLufs(block);
        momentary.store(l, std::memory_order_relaxed);
        if (l > -70.0)
        {
            const int bin = std::clamp(static_cast<int>((l + 70.0) * 10.0), 0, kHistogramBins - 1);
            ++histCount[static_cast<size_t>(bin)];
            histEnergy[static_cast<size_t>(bin)] += block;
            updateIntegrated();
        }
    }
    if (hopsSeen >= kShortHops)
        shortTerm.store(toLufs(meanOf(kShortHops)), std::memory_order_relaxed);
}

void LoudnessMeter::updateIntegrated() noexcept
{
    double energy = 0.0;
    long long count = 0;
    for (int b = 0; b < kHistogramBins; ++b)
    {
        energy += histEnergy[static_cast<size_t>(b)];
        count += histCount[static_cast<size_t>(b)];
    }
    if (count == 0)
        return;
    // Relative gate on the bin grid: bins whose lower edge is at or above the gate (0.1 LU resolution).
    const double relativeGate = toLufs(energy / static_cast<double>(count)) - 10.0;
    const int first = std::clamp(static_cast<int>(std::ceil((relativeGate + 70.0) * 10.0)), 0, kHistogramBins);
    energy = 0.0;
    count = 0;
    for (int b = first; b < kHistogramBins; ++b)
    {
        energy += histEnergy[static_cast<size_t>(b)];
        count += histCount[static_cast<size_t>(b)];
    }
    integrated.store(count > 0 ? toLufs(energy / static_cast<double>(count)) : kSilence, std::memory_order_relaxed);
}
} // namespace hpeq
