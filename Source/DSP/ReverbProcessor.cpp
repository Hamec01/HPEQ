#include "DSP/ReverbProcessor.h"

#include <algorithm>
#include <cmath>

namespace hpeq::dsp
{
namespace
{
constexpr std::array<double, ClassicReverb::kLines> kBaseDelayMs { 31.3, 37.9, 41.7, 47.3, 53.9, 59.3, 67.1, 73.7 };
constexpr std::array<double, ClassicReverb::kLines> kOutLeft { 1, 1, 1, 1, -1, -1, -1, -1 };
constexpr std::array<double, ClassicReverb::kLines> kOutRight { 1, -1, 1, -1, 1, -1, 1, -1 };
constexpr std::array<double, ClassicReverb::kLines> kInSign { 1, 1, -1, -1, 1, 1, -1, -1 };
constexpr double kOutputScale = 0.35;   // keeps the wet near the input level for typical decays
constexpr double kGainSmoothing = 0.05; // s, decay/damping changes
constexpr double kSizeSmoothing = 0.20; // s, size changes glide the delay lengths

// Delay-line read without integer division: one conditional wrap. Whole-sample delays (the settled
// case) skip the interpolation entirely.
inline float readDelay(const float* line, int size, int writePos, double delay) noexcept
{
    const int whole = static_cast<int>(delay);
    const double frac = delay - whole;
    int i0 = writePos - whole;
    if (i0 < 0)
        i0 += size;
    if (frac == 0.0)
        return line[i0];
    int i1 = i0 - 1; // one sample further back
    if (i1 < 0)
        i1 += size;
    return static_cast<float>(line[i0] + frac * (line[i1] - line[i0]));
}
} // namespace

void ClassicReverb::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    const auto lineSize = static_cast<size_t>(std::ceil(kBaseDelayMs.back() * kMaxSize * 1.0e-3 * sampleRate)) + 4;
    for (auto& l : lines)
        l.assign(lineSize, 0.0f);
    for (auto& p : predelay)
        p.assign(static_cast<size_t>(std::ceil(kMaxPredelayMs * 1.0e-3 * sampleRate)) + 4, 0.0f);
    reset();
}

void ClassicReverb::reset() noexcept
{
    for (auto& l : lines)
        std::fill(l.begin(), l.end(), 0.0f);
    for (auto& p : predelay)
        std::fill(p.begin(), p.end(), 0.0f);
    writePos = {};
    absorption = {};
    lowCutState = {};
    predelayPos = 0;
    blockPeak = 0.0f;
    primed = false;
}

double ClassicReverb::tailSeconds(const FxSlotParams& p) noexcept
{
    return p.values[slot::kPredelayMs] * 1.0e-3 + p.values[slot::kDecaySeconds];
}

void ClassicReverb::updateTargets(const FxSlotParams& p) noexcept
{
    const double t60 = std::clamp(static_cast<double>(p.values[slot::kDecaySeconds]), 0.1, 20.0);
    const double hfRatio = std::clamp(static_cast<double>(p.values[slot::kHfRatio]), 0.05, 1.0);
    const double size = std::clamp(static_cast<double>(p.values[slot::kSize]), kMinSize, kMaxSize);
    for (int i = 0; i < kLines; ++i)
    {
        // Whole-sample lengths: a fractional read (linear interpolation) low-passes every pass of
        // the loop and shortened the measured RT60 by ~13 % (M4 T07); lengths only glide fractionally.
        const double d = std::round(kBaseDelayMs[static_cast<size_t>(i)] * size * 1.0e-3 * sampleRate);
        lengthTarget[static_cast<size_t>(i)] = d;
        const double g = std::pow(10.0, -3.0 * d / (t60 * sampleRate));
        const double gHigh = std::pow(10.0, -3.0 * d / (t60 * hfRatio * sampleRate));
        const double r = gHigh / g;
        gainTarget[static_cast<size_t>(i)] = g;
        poleTarget[static_cast<size_t>(i)] = (1.0 - r) / (1.0 + r);
    }
    predelayTarget = std::clamp(static_cast<double>(p.values[slot::kPredelayMs]), 0.0, kMaxPredelayMs) * 1.0e-3 * sampleRate;
    width = std::clamp(static_cast<double>(p.values[slot::kWidth]), 0.0, 1.5);
    const double lowCut = std::clamp(static_cast<double>(p.values[slot::kLowCutHz]), 10.0, 1000.0);
    lowCutCoefficient = 1.0 - std::exp(-2.0 * 3.14159265358979323846 * lowCut / sampleRate);

    if (!primed)
    {
        length = lengthTarget;
        gain = gainTarget;
        pole = poleTarget;
        predelaySamples = predelayTarget;
        primed = true;
    }
}

void ClassicReverb::process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    updateTargets(p);
    const double gainK = 1.0 - std::exp(-1.0 / (kGainSmoothing * sampleRate));
    const double sizeK = 1.0 - std::exp(-1.0 / (kSizeSmoothing * sampleRate));
    const int predelaySize = static_cast<int>(predelay[0].size());
    float peak = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        for (int i = 0; i < kLines; ++i)
        {
            gain[static_cast<size_t>(i)] += gainK * (gainTarget[static_cast<size_t>(i)] - gain[static_cast<size_t>(i)]);
            pole[static_cast<size_t>(i)] += gainK * (poleTarget[static_cast<size_t>(i)] - pole[static_cast<size_t>(i)]);
            auto& len = length[static_cast<size_t>(i)];
            const double target = lengthTarget[static_cast<size_t>(i)];
            len += sizeK * (target - len);
            if (std::abs(target - len) < 1.0e-4)
                len = target; // land on the integer length: no interpolation loss once settled
        }
        predelaySamples += sizeK * (predelayTarget - predelaySamples);

        const float inL = in[0][n];
        const float inR = numChannels > 1 ? in[1][n] : inL;
        predelay[0][static_cast<size_t>(predelayPos)] = inL;
        predelay[1][static_cast<size_t>(predelayPos)] = inR;
        const double pl = readDelay(predelay[0].data(), predelaySize, predelayPos, predelaySamples);
        const double pr = readDelay(predelay[1].data(), predelaySize, predelayPos, predelaySamples);
        if (++predelayPos == predelaySize)
            predelayPos = 0;

        std::array<double, kLines> z {};
        double sum = 0.0, outL = 0.0, outR = 0.0;
        for (int i = 0; i < kLines; ++i)
        {
            const auto idx = static_cast<size_t>(i);
            const double y = readDelay(lines[idx].data(), static_cast<int>(lines[idx].size()), writePos[idx], length[idx]);
            absorption[idx] = gain[idx] * (1.0 - pole[idx]) * y + pole[idx] * absorption[idx];
            z[idx] = absorption[idx];
            sum += z[idx];
            outL += kOutLeft[idx] * z[idx];
            outR += kOutRight[idx] * z[idx];
        }
        const double householder = 2.0 / kLines * sum;
        for (int i = 0; i < kLines; ++i)
        {
            const auto idx = static_cast<size_t>(i);
            const double input = (i % 2 == 0 ? pl : pr) * kInSign[idx];
            lines[idx][static_cast<size_t>(writePos[idx])] = static_cast<float>(z[idx] - householder + input);
            if (++writePos[idx] == static_cast<int>(lines[idx].size()))
                writePos[idx] = 0;
        }

        outL *= kOutputScale;
        outR *= kOutputScale;
        const double mid = 0.5 * (outL + outR), side = 0.5 * (outL - outR) * width;
        double l = mid + side, r = mid - side;
        lowCutState[0] += lowCutCoefficient * (l - lowCutState[0]);
        lowCutState[1] += lowCutCoefficient * (r - lowCutState[1]);
        l -= lowCutState[0];
        r -= lowCutState[1];

        if (numChannels > 1)
        {
            wet[0][n] = static_cast<float>(l);
            wet[1][n] = static_cast<float>(r);
        }
        else
            wet[0][n] = static_cast<float>(0.5 * (l + r));
        peak = std::max({ peak, std::abs(static_cast<float>(l)), std::abs(static_cast<float>(r)) });
    }
    blockPeak = peak;
}

void ReverbProcessor::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    classic.prepare(sampleRate);
    verb.prepare(sampleRate);
    fadeSamples = std::max(1, static_cast<int>(std::lround(0.010 * sampleRate)));
    reset();
}

void ReverbProcessor::reset() noexcept
{
    classic.reset();
    verb.reset();
    activeSpace = -1;
    fade = 1.0;
    fadingOut = false;
    blockPeak = 0.0f;
    meterOutDb.store(-120.0f, std::memory_order_relaxed);
}

double ReverbProcessor::tailSeconds(const FxSlotParams& p, double bpm) noexcept
{
    if (std::lround(p.values[slot::kRevSpace]) <= 0)
        return ClassicReverb::tailSeconds(p);
    return HamloVerb::tailSeconds(p, bpm);
}

void ReverbProcessor::runEngine(int space, const float* const* in, float* const* wet, int numChannels, int numSamples,
                                const FxSlotParams& p, const Transport& transport) noexcept
{
    if (space == verb::Classic)
        classic.process(in, wet, numChannels, numSamples, p);
    else
    {
        auto q = p; // an engine fading out keeps its own space
        q.values[slot::kRevSpace] = static_cast<float>(space);
        verb.process(in, wet, numChannels, numSamples, q, transport);
    }
}

void ReverbProcessor::process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams& p,
                              const Transport& transport) noexcept
{
    const int wanted = std::clamp(static_cast<int>(std::lround(p.values[slot::kRevSpace])), 0, verb::NumSpaces - 1);
    if (activeSpace < 0)
    {
        activeSpace = wanted;
        fade = 1.0;
    }
    else if (wanted != activeSpace && !fadingOut)
        fadingOut = true;
    else if (wanted == activeSpace && fadingOut)
        fadingOut = false; // changed back before the fade ended: fade in again

    if (fadingOut && fade <= 0.0)
    {
        // the old engine is silent: start the new one from a clean state
        if (activeSpace == verb::Classic)
            classic.reset();
        else
            verb.reset();
        activeSpace = wanted;
        if (activeSpace == verb::Classic)
            classic.reset();
        else
            verb.reset();
        fadingOut = false;
    }

    runEngine(activeSpace, in, wet, numChannels, numSamples, p, transport);

    const double step = 1.0 / fadeSamples;
    float peak = 0.0f;
    double energy = 0.0;
    const bool settled = !fadingOut && fade >= 1.0;
    for (int n = 0; n < numSamples; ++n)
    {
        if (!settled)
        {
            fade = std::clamp(fade + (fadingOut ? -step : step), 0.0, 1.0);
            for (int ch = 0; ch < numChannels; ++ch)
                wet[ch][n] = static_cast<float>(wet[ch][n] * fade);
        }
        for (int ch = 0; ch < numChannels; ++ch)
        {
            peak = std::max(peak, std::abs(wet[ch][n]));
            energy += static_cast<double>(wet[ch][n]) * wet[ch][n];
        }
    }
    // settled: the engine's own peak (Classic: exactly the M4 spill-over behaviour)
    blockPeak = settled ? (activeSpace == verb::Classic ? classic.lastBlockPeak() : verb.lastBlockPeak()) : peak;
    if (numSamples > 0)
        meterOutDb.store(static_cast<float>(10.0 * std::log10(energy / (numSamples * std::max(1, numChannels)) + 1.0e-12)),
                         std::memory_order_relaxed);
}
} // namespace hpeq::dsp
