#include "DSP/HamloShirota.h"

#include <algorithm>
#include <cmath>

namespace hpeq::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

double coefficientFor(double ms, double sampleRate) noexcept
{
    return std::exp(-1.0 / (std::max(0.01, ms) * 1.0e-3 * sampleRate));
}

// RBJ cookbook sections. kind: 0 high-pass, 1 all-pass, 2 peaking (gainDb).
void rbj(double& b0, double& b1, double& b2, double& a1, double& a2, int kind, double hz, double q, double fs, double gainDb = 0.0) noexcept
{
    const double w = 2.0 * kPi * std::clamp(hz, 5.0, 0.45 * fs) / fs, cw = std::cos(w), alpha = std::sin(w) / (2.0 * q);
    double a0 = 1.0 + alpha;
    if (kind == 0)
    {
        b0 = (1.0 + cw) / 2.0;
        b1 = -(1.0 + cw);
        b2 = b0;
        a1 = -2.0 * cw;
        a2 = 1.0 - alpha;
    }
    else if (kind == 1)
    {
        b0 = 1.0 - alpha;
        b1 = -2.0 * cw;
        b2 = 1.0 + alpha;
        a1 = -2.0 * cw;
        a2 = 1.0 - alpha;
    }
    else
    {
        const double A = std::pow(10.0, gainDb / 40.0);
        a0 = 1.0 + alpha / A;
        b0 = 1.0 + alpha * A;
        b1 = -2.0 * cw;
        b2 = 1.0 - alpha * A;
        a1 = -2.0 * cw;
        a2 = 1.0 - alpha / A;
    }
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
}
} // namespace

void ShirotaCore::prepare(double sampleRate)
{
    fs = sampleRate;
    safeDelay = safeLookaheadSamples(fs);
    ringSize = 2 * safeDelay + 1;
    for (auto* ring : { &ringM, &ringS, &ringM2, &ringS2 })
        ring->assign(static_cast<size_t>(ringSize), 0.0);
    scopeL = std::make_unique<std::atomic<float>[]>(kScopePoints);
    scopeR = std::make_unique<std::atomic<float>[]>(kScopePoints);
    for (int i = 0; i < kScopePoints; ++i)
    {
        scopeL[static_cast<size_t>(i)].store(0.0f);
        scopeR[static_cast<size_t>(i)].store(0.0f);
    }
    scopeDecimation = std::max(1, static_cast<int>(std::lround(fs / 24000.0)));
    designedBass = designedSpace = -1.0;
    design(0.0, 0.0);
    reset();
}

void ShirotaCore::reset() noexcept
{
    for (auto* group : { &bass })
        for (auto& s : *group)
            s.z1 = s.z2 = 0.0;
    for (auto& s : allPass)
        s.z1 = s.z2 = 0.0;
    for (auto* group : { &eqMid, &eqSide })
        for (auto& s : *group)
            s.z1 = s.z2 = 0.0;
    stereoizeHp.z1 = stereoizeHp.z2 = 0.0;
    guard = 1.0;
    for (auto* ring : { &ringM, &ringS, &ringM2, &ringS2 })
        std::fill(ring->begin(), ring->end(), 0.0);
    ringPos = 0;
    sumM2 = sumS2 = 0.0;
    safeRunning = false;
    sumLR = sumLL = sumRR = 0.0;
    scopeCount = 0;
}

void ShirotaCore::design(double monoBassHz, double space) noexcept
{
    if (monoBassHz != designedBass && monoBassHz > 0.0)
        for (auto& s : bass)
            rbj(s.b0, s.b1, s.b2, s.a1, s.a2, 0, monoBassHz, 0.7071067811865476, fs);
    if (designedSpace < 0.0) // fixed networks, once
    {
        rbj(stereoizeHp.b0, stereoizeHp.b1, stereoizeHp.b2, stereoizeHp.a1, stereoizeHp.a2, 0, 150.0, 0.7071067811865476, fs);
        static constexpr double hz[] { 270.0, 520.0, 950.0, 1700.0, 2900.0, 4800.0, 7600.0, 11000.0 };
        for (size_t i = 0; i < allPass.size(); ++i)
            rbj(allPass[i].b0, allPass[i].b1, allPass[i].b2, allPass[i].a1, allPass[i].a2, 1, hz[i], 0.7, fs);
    }
    if (space != designedSpace)
    {
        static constexpr double hz[] { 1000.0, 11000.0, 4500.0 }, q[] { 1.2, 1.2, 0.9 }, sign[] { 1.0, 1.0, -1.0 };
        for (size_t i = 0; i < 3; ++i)
            for (auto* group : { &eqMid, &eqSide })
            {
                auto& s = (*group)[i];
                rbj(s.b0, s.b1, s.b2, s.a1, s.a2, 2, hz[i], q[i], fs, sign[i] * 6.0 * space);
            }
    }
    designedBass = monoBassHz;
    designedSpace = space;
}

ShirotaCore::Meters ShirotaCore::meters() const noexcept
{
    Meters m;
    m.correlation = meterCorrelation.load(std::memory_order_relaxed);
    m.guardDb = meterGuard.load(std::memory_order_relaxed);
    m.oneChannel = meterOneChannel.load(std::memory_order_relaxed);
    return m;
}

void ShirotaCore::scope(std::array<float, kScopePoints>& left, std::array<float, kScopePoints>& right) const noexcept
{
    if (scopeL == nullptr)
    {
        left.fill(0.0f);
        right.fill(0.0f);
        return;
    }
    const int start = scopeWrite.load(std::memory_order_acquire);
    for (int i = 0; i < kScopePoints; ++i)
    {
        const auto k = static_cast<size_t>((start + i) % kScopePoints);
        left[static_cast<size_t>(i)] = scopeL[k].load(std::memory_order_relaxed);
        right[static_cast<size_t>(i)] = scopeR[k].load(std::memory_order_relaxed);
    }
}

void ShirotaCore::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    if (numChannels < 2)
    {
        meterOneChannel.store(true, std::memory_order_relaxed);
        return; // nothing to widen on one channel: the signal passes untouched
    }
    meterOneChannel.store(false, std::memory_order_relaxed);
    const int style = std::clamp(static_cast<int>(std::lround(p.values[slot::kStStyle])), 0, 2);
    const double width = std::clamp(p.values[slot::kStereoWidth], 0.0f, 2.0f);
    const double balance = std::clamp(p.values[slot::kStereoBalance], -1.0f, 1.0f);
    const double amount = style == Edinstvo ? 0.0 : std::clamp(p.values[slot::kStStereoize], 0.0f, 1.0f);
    const double space = style == Kosmos ? std::clamp(p.values[slot::kStSpace], 0.0f, 1.0f) : 0.0;
    const double monoBassHz = p.values[slot::kStMonoBassHz] > 0.0f ? std::clamp(p.values[slot::kStMonoBassHz], 20.0f, 500.0f) : 0.0;
    const double theta = std::clamp(p.values[slot::kStRotation], -45.0f, 45.0f) * kPi / 180.0;
    const bool safe = p.values[slot::kStSafe] != 0.0f, mono = p.values[slot::kStMonoListen] != 0.0f;
    const double outputGain = std::pow(10.0, std::clamp(p.values[slot::kStOutputDb], -24.0f, 12.0f) / 20.0);
    if (monoBassHz != designedBass || space != designedSpace)
        design(monoBassHz, space);
    const double cosT = std::cos(theta), sinT = std::sin(theta);
    const double gl = balance > 0.0 ? 1.0 - balance : 1.0, gr = balance < 0.0 ? 1.0 + balance : 1.0;
    const double recover = coefficientFor(100.0, fs), corrA = coefficientFor(300.0, fs);
    if (safe != safeRunning) // the latency changes: start the look-ahead from silence
    {
        for (auto* ring : { &ringM, &ringS, &ringM2, &ringS2 })
            std::fill(ring->begin(), ring->end(), 0.0);
        ringPos = 0;
        sumM2 = sumS2 = 0.0;
        guard = 1.0;
        safeRunning = safe;
    }
    const double sideScale = 1.0 + space;

    for (int i = 0; i < numSamples; ++i)
    {
        const double l = channels[0][i], r = channels[1][i];
        double m = 0.5 * (l + r), s = 0.5 * (l - r);
        if (monoBassHz > 0.0)
            for (auto& section : bass)
                s = section.process(s);
        s *= width;
        if (amount > 0.0)
        {
            double d = stereoizeHp.process(m);
            for (auto& section : allPass)
                d = section.process(d);
            s += amount * d;
        }
        if (space > 0.0)
        {
            for (auto& section : eqMid)
                m = section.process(m);
            for (auto& section : eqSide)
                s = section.process(s);
            s *= sideScale;
        }
        if (safe)
        {
            // the sample leaving the 5 ms window, the new one entering; the output is the centre (D samples ago)
            const auto w = static_cast<size_t>(ringPos);
            sumM2 += m * m - ringM2[w];
            sumS2 += s * s - ringS2[w];
            ringM[w] = m;
            ringS[w] = s;
            ringM2[w] = m * m;
            ringS2[w] = s * s;
            ringPos = (ringPos + 1) % ringSize;
            const auto centre = static_cast<size_t>((ringPos + safeDelay) % ringSize);
            m = ringM[centre];
            s = ringS[centre];
            const double target = sumS2 > sumM2 && sumS2 > 1.0e-20 ? std::sqrt(std::max(0.0, sumM2) / sumS2) : 1.0;
            guard = target < guard ? target : target + recover * (guard - target); // reduce at once, recover over 100 ms
            s *= guard;
        }
        double outL = m + s, outR = m - s;
        if (theta != 0.0)
        {
            const double rl = cosT * outL - sinT * outR, rr = sinT * outL + cosT * outR;
            outL = rl;
            outR = rr;
        }
        outL *= gl;
        outR *= gr;
        if (mono)
            outL = outR = 0.5 * (outL + outR);
        outL *= outputGain;
        outR *= outputGain;
        channels[0][i] = static_cast<float>(outL);
        channels[1][i] = static_cast<float>(outR);

        sumLR = outL * outR + corrA * (sumLR - outL * outR);
        sumLL = outL * outL + corrA * (sumLL - outL * outL);
        sumRR = outR * outR + corrA * (sumRR - outR * outR);
        if (scopeL != nullptr && ++scopeCount >= scopeDecimation)
        {
            scopeCount = 0;
            const int w = scopeWrite.load(std::memory_order_relaxed);
            scopeL[static_cast<size_t>(w)].store(static_cast<float>(outL), std::memory_order_relaxed);
            scopeR[static_cast<size_t>(w)].store(static_cast<float>(outR), std::memory_order_relaxed);
            scopeWrite.store((w + 1) % kScopePoints, std::memory_order_release);
        }
    }
    const double energy = std::sqrt(sumLL * sumRR);
    meterCorrelation.store(energy > 1.0e-20 ? static_cast<float>(sumLR / energy) : 1.0f, std::memory_order_relaxed);
    meterGuard.store(static_cast<float>(20.0 * std::log10(std::max(1.0e-6, guard))), std::memory_order_relaxed);
}
} // namespace hpeq::dsp
