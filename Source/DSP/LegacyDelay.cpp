#include "DSP/LegacyDelay.h"
#include <algorithm>
#include <cmath>

namespace hpeq::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
float readLinear(const std::vector<float>& line, int writePos, double delay) noexcept
{
    const int size = static_cast<int>(line.size());
    const int whole = static_cast<int>(delay);
    const double frac = delay - whole;
    int i0 = writePos - whole;
    if (i0 < 0)
        i0 += size;
    int i1 = i0 - 1;
    if (i1 < 0)
        i1 += size;
    return static_cast<float>(line[static_cast<size_t>(i0)] + frac * (line[static_cast<size_t>(i1)] - line[static_cast<size_t>(i0)]));
}
}
namespace
{
constexpr std::array<double, 12> kDivisionBeats { 0.0, 0.125, 0.25, 0.375, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0, 4.0 };
}

double ClassicDelay::delaySeconds(const FxSlotParams& p, double bpm) noexcept
{
    const int sync = std::clamp(static_cast<int>(std::lround(p.values[slot::kDelaySync])), 0, 11);
    const double seconds = sync == 0 ? p.values[slot::kDelayTimeMs] * 1.0e-3 : kDivisionBeats[static_cast<size_t>(sync)] * 60.0 / std::clamp(bpm, 20.0, 999.0);
    return std::clamp(seconds, 0.001, kMaxSeconds - 0.01);
}

double ClassicDelay::tailSeconds(const FxSlotParams& p, double bpm) noexcept
{
    const double t = delaySeconds(p, bpm);
    const double fb = std::clamp(static_cast<double>(p.values[slot::kFeedback]), 0.0, 0.95);
    return fb < 0.01 ? t : t + t * std::log(1.0e-3) / std::log(fb);
}

void ClassicDelay::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    for (auto& l : lines)
        l.assign(static_cast<size_t>(std::ceil(kMaxSeconds * sampleRate)) + 4, 0.0f);
    reset();
}

void ClassicDelay::reset() noexcept
{
    for (auto& l : lines)
        std::fill(l.begin(), l.end(), 0.0f);
    writePos = 0;
    delay = -1.0;
    lowState = {};
    highState = {};
    blockPeak = 0.0f;
}

void ClassicDelay::process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams& p, const Transport& transport) noexcept
{
    const double target = delaySeconds(p, transport.bpm) * sampleRate;
    if (delay < 0.0)
        delay = target;
    const double glide = 1.0 - std::exp(-1.0 / (0.1 * sampleRate));
    const double feedback = std::clamp(static_cast<double>(p.values[slot::kFeedback]), 0.0, 0.95);
    const bool pingPong = p.values[slot::kPingPong] > 0.5f;
    const double lowCut = 1.0 - std::exp(-2.0 * kPi * std::clamp(static_cast<double>(p.values[slot::kDelayLowCutHz]), 10.0, 2000.0) / sampleRate);
    const double damping = 1.0 - std::exp(-2.0 * kPi * std::clamp(static_cast<double>(p.values[slot::kDampingHz]), 500.0, 20000.0) / sampleRate);
    const double width = std::clamp(static_cast<double>(p.values[slot::kDelayWidth]), 0.0, 1.5);
    const int size = static_cast<int>(lines[0].size());
    float peak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        delay += glide * (target - delay);
        std::array<double, 2> y { readLinear(lines[0], writePos, delay), readLinear(lines[1], writePos, delay) };
        std::array<double, 2> fb {};
        for (int c = 0; c < 2; ++c)
        {
            lowState[static_cast<size_t>(c)] += lowCut * (y[static_cast<size_t>(c)] - lowState[static_cast<size_t>(c)]);
            const double hp = y[static_cast<size_t>(c)] - lowState[static_cast<size_t>(c)];
            highState[static_cast<size_t>(c)] += damping * (hp - highState[static_cast<size_t>(c)]);
            fb[static_cast<size_t>(c)] = highState[static_cast<size_t>(c)] * feedback;
        }
        const double inL = in[0][i];
        const double inR = numChannels > 1 ? in[1][i] : inL;
        if (pingPong)
        {
            lines[0][static_cast<size_t>(writePos)] = static_cast<float>(0.5 * (inL + inR) + fb[1]);
            lines[1][static_cast<size_t>(writePos)] = static_cast<float>(fb[0]);
        }
        else
        {
            lines[0][static_cast<size_t>(writePos)] = static_cast<float>(inL + fb[0]);
            lines[1][static_cast<size_t>(writePos)] = static_cast<float>(inR + fb[1]);
        }
        if (++writePos == size)
            writePos = 0;

        const double m = 0.5 * (y[0] + y[1]), s = 0.5 * (y[0] - y[1]) * width;
        if (numChannels > 1)
        {
            wet[0][i] = static_cast<float>(m + s);
            wet[1][i] = static_cast<float>(m - s);
        }
        else
            wet[0][i] = static_cast<float>(m);
        peak = std::max({ peak, std::abs(wet[0][i]), std::abs(wet[numChannels > 1 ? 1 : 0][i]) });
    }
    blockPeak = peak;
}

void DelayProcessor::prepare(double sampleRate)
{
    classic.prepare(sampleRate);
    echo.prepare(sampleRate);
    fadeSamples = std::max(1, static_cast<int>(std::lround(0.010 * sampleRate)));
    reset();
}

void DelayProcessor::reset() noexcept
{
    classic.reset();
    echo.reset();
    engine = -1;
    fade = 1.0;
    fadingOut = false;
    blockPeak = 0.0f;
    meterOutDb.store(-120.0f, std::memory_order_relaxed);
}

double DelayProcessor::tailSeconds(const FxSlotParams& p, double bpm) noexcept
{
    return std::lround(p.values[slot::kDlyCharacter]) <= 0 ? ClassicDelay::tailSeconds(p, bpm) : EchoProcessor::tailSeconds(p, bpm);
}

void DelayProcessor::process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams& p,
                             const Transport& transport) noexcept
{
    const int wanted = std::lround(p.values[slot::kDlyCharacter]) <= 0 ? 0 : 1;
    if (engine < 0)
        engine = wanted;
    else if (wanted != engine && !fadingOut)
        fadingOut = true;
    else if (wanted == engine && fadingOut)
        fadingOut = false;
    if (fadingOut && fade <= 0.0)
    {
        engine = wanted;
        if (engine == 0)
            classic.reset();
        else
            echo.reset();
        fadingOut = false;
    }

    if (engine == 0)
        classic.process(in, wet, numChannels, numSamples, p, transport);
    else
    {
        auto q = p; // an engine fading out keeps its own character
        if (q.values[slot::kDlyCharacter] < 0.5f)
            q.values[slot::kDlyCharacter] = static_cast<float>(echo::Tape);
        echo.process(in, wet, numChannels, numSamples, q, transport);
    }

    const bool settled = !fadingOut && fade >= 1.0;
    const double step = 1.0 / fadeSamples;
    float peak = 0.0f;
    double energy = 0.0;
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
    // settled: the engine's own peak (Classic: exactly the M6 spill-over behaviour)
    blockPeak = settled ? (engine == 0 ? classic.lastBlockPeak() : echo.lastBlockPeak()) : peak;
    if (numSamples > 0)
        meterOutDb.store(static_cast<float>(10.0 * std::log10(energy / (numSamples * std::max(1, numChannels)) + 1.0e-12)),
                         std::memory_order_relaxed);
}

}
