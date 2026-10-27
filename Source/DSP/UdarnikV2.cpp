#include "DSP/UdarnikV2.h"

#include <algorithm>
#include <cmath>

namespace hamlo::dsp
{
namespace
{
double coefficient(double ms, double fs) noexcept
{
    return std::exp(-1.0 / (std::max(0.01, ms) * 1.0e-3 * fs));
}
double follow(double env, double x, double attack, double release) noexcept
{
    return x + (x > env ? attack : release) * (env - x);
}
constexpr double kTiny = 1.0e-10;
} // namespace

void UdarnikV2::prepare(double sampleRate)
{
    fs = sampleRate;
    capacity = UdarnikCore::lookaheadSamples(2, fs) + 2;
    for (auto& a : audio)
        a.assign(static_cast<size_t>(capacity), 0.0f);
    gainDb.assign(static_cast<size_t>(capacity), 0.0f);
    attackShare.assign(static_cast<size_t>(capacity), 0.0f);
    reset();
}

void UdarnikV2::reset() noexcept
{
    counter = 0;
    lookahead = -1;
    meanSquare = fast = slow = hold = gainSmooth = peakHold = impact = 0.0;
    inHit = false;
    hitStart = riseEnd = -1;
    riseMax = 0.0;
    hitFloor = hitPeak = 0.0;
    bodyFade = 0.0;
    for (auto& a : audio)
        std::fill(a.begin(), a.end(), 0.0f);
    std::fill(gainDb.begin(), gainDb.end(), 0.0f);
    std::fill(attackShare.begin(), attackShare.end(), 0.0f);
    capT0 = -1;
    capBins = 0;
    events = 0;
    meterEvents.store(0, std::memory_order_relaxed);
}

void UdarnikV2::process(float* const* channels, int numChannels, int numSamples, const TransientSettings& p) noexcept
{
    const int L = UdarnikCore::lookaheadSamples(std::clamp(static_cast<int>(std::lround(p.values[slot::kTrLookahead])), 0, 2), fs);
    if (L != lookahead)
    {
        reset();
        lookahead = L;
    }
    const int style = std::clamp(static_cast<int>(std::lround(p.values[slot::kTrStyle])), 0, 2);
    const double snap = std::clamp(p.values[slot::kAttackGainDb], -24.0f, 24.0f);
    const double body = std::clamp(p.values[slot::kSustainGainDb], -24.0f, 24.0f);
    const double speed = 0.5 + std::clamp(p.values[slot::kTransientSpeed], 0.0f, 1.0f);
    const double rise = style == 2 ? std::exp2(std::clamp(p.values[slot::kTrRiseLog2], -1.0f, 1.0f)) : 1.0;
    const double decayK = style == 2 ? std::exp2(std::clamp(p.values[slot::kTrDecayLog2], -1.0f, 1.0f)) : 1.0;
    const double sensitivity = 1.0 - std::clamp(p.values[slot::kTrStrictness], 0.0f, 1.0f);
    const double preserve = std::clamp(p.values[slot::kTrPreserve], 0.0f, 1.0f);
    const int listen = std::clamp(static_cast<int>(std::lround(p.values[slot::kTrListen])), 0, 2);
    const double outputGain = std::pow(10.0, std::clamp(p.values[slot::kTrOutputDb], -24.0f, 12.0f) / 20.0);

    const double rmsC = coefficient(3.0, fs);
    const double fastA = coefficient(0.1, fs), fastR = coefficient(8.0 * speed, fs);
    const double slowA = coefficient(6.0 * speed * rise, fs), slowR = coefficient(150.0 * speed, fs);
    const double smoothC = coefficient(0.5, fs), impactC = coefficient(300.0, fs), peakRelease = coefficient(50.0, fs);
    const double dead = 1.5 + 6.0 * (1.0 - preserve) + 12.0 * (1.0 - sensitivity);
    const int lead = std::min(L, static_cast<int>(leadMs * 1.0e-3 * fs));
    const int bin = std::max(1, static_cast<int>(0.0025 * fs));
    const double decayFactor = 1.0 - 1.0 / decayK;
    const double attackWindow = 0.012 * speed * rise * fs, bodyLength = 0.500 * speed * fs, bodyRamp = 0.003 * fs;
    const double bodyFadeStep = 1.0 / (0.050 * fs);

    float blockInput = 0.0f;
    double gMin = 0.0, gMax = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        // ---------------------------------------------------------------- detector (time n)
        const long long n = counter;
        double x2 = 0.0;
        float peak = 0.0f;
        for (int c = 0; c < numChannels; ++c)
        {
            const double v = channels[c][i];
            x2 = std::max(x2, v * v);
            peak = std::max(peak, std::abs(channels[c][i]));
        }
        blockInput = std::max(blockInput, peak);
        peakHold = std::max(static_cast<double>(peak), peakHold * peakRelease); // the sample peak seen so far (50 ms release)
        meanSquare = x2 + rmsC * (meanSquare - x2);
        const double level = std::sqrt(meanSquare) + kTiny;
        fast = follow(fast, level, fastA, fastR);
        slow = follow(slow, level, slowA, slowR);
        hold = follow(hold, level, fastA, slowR); // the sustain pair: fast attack, slow release
        const double A = 20.0 * std::log10((fast + kTiny) / (slow + kTiny));
        // a hit: the attack envelope stands out of the slow one by more than `dead` (re-armed below dead - 1 dB)
        if (!inHit && A > dead)
        {
            inHit = true;
            hitStart = n;
            riseEnd = n;
            riseMax = A;
            hitFloor = slow;
            hitPeak = level;
            bodyFade = 1.0;
            ++events;
            meterEvents.store(events, std::memory_order_relaxed);
            meterStrength.store(static_cast<float>(20.0 * std::log10(level)), std::memory_order_relaxed);
            capT0 = n - L; // the hit in audio time
            capBins = 0;
            capIn.fill(0.0f);
            capOut.fill(0.0f);
        }
        else if (inHit && A < dead - 1.0)
            inHit = false;
        double wa = 0.0, wb = 0.0, fall = 0.0;
        if (hitStart >= 0)
        {
            // the attack window counts from the attack envelope's peak (a slow attack keeps it open while it rises)
            if (inHit && A >= riseMax)
            {
                riseMax = A;
                riseEnd = n;
            }
            const double since = static_cast<double>(n - riseEnd);
            hitPeak = std::max(hitPeak, level);
            if (since < 0.7 * attackWindow)
                wa = 1.0;
            else if (since < attackWindow)
                wa = 0.5 + 0.5 * std::cos(3.14159265358979323846 * (since - 0.7 * attackWindow) / (0.3 * attackWindow));
            // the body: from the attack window's end, while the hit has not fallen back to the level before it
            // (+3 dB) and for at most 500 ms x SPEED; it fades out over 50 ms
            if (since >= 0.7 * attackWindow)
            {
                const bool alive = since < bodyLength;
                if (!alive)
                    bodyFade = std::max(0.0, bodyFade - bodyFadeStep);
                wb = (1.0 - wa) * std::min(1.0, (since - 0.7 * attackWindow) / bodyRamp) * bodyFade;
                fall = 20.0 * std::log10(hitPeak / level);
            }
        }
        double G = snap * wa + body * wb;
        if (style == 1 && snap > 0.0)
            G -= 0.5 * snap * wb; // UDARNIK: the body gives back half the SNAP, so the contrast is the SNAP and the peak stays
        if (style == 2)
            G += std::max(0.0, fall) * decayFactor * wb; // the decay's slope in dB divided by DECAY
        const int w = static_cast<int>(n % capacity);
        for (int c = 0; c < numChannels; ++c)
            audio[static_cast<size_t>(c)][static_cast<size_t>(w)] = channels[c][i];
        gainDb[static_cast<size_t>(w)] = static_cast<float>(G);
        attackShare[static_cast<size_t>(w)] = static_cast<float>(wa);

        // ---------------------------------------------------------------- audio (time m = n - L)
        const long long m = n - L;
        const int r = static_cast<int>(((m % capacity) + capacity) % capacity);
        const int g = static_cast<int>((((m + lead) % capacity) + capacity) % capacity);
        const double target = m >= 0 ? gainDb[static_cast<size_t>(g)] : 0.0;
        gainSmooth = target + smoothC * (gainSmooth - target);
        if (std::abs(gainSmooth) < 1.0e-9)
            gainSmooth = 0.0;
        const double gain = gainSmooth == 0.0 ? 1.0 : std::pow(10.0, gainSmooth / 20.0);
        // UDARNIK, peak-neutral: a boost may not lift a sample over the hit's own peak `room` (the sample peak the
        // detector has seen, look-ahead included); the excess is soft-limited, so the attack gets denser, not
        // higher. Without a boost (gain <= 1) it is the plain gain.
        const bool peakNeutral = style == 1 && gain > 1.0 && attackShare[static_cast<size_t>(g)] > 0.0f; // the attack's boost only
        const double share = attackShare[static_cast<size_t>(g)];
        gMin = std::min(gMin, gainSmooth);
        gMax = std::max(gMax, gainSmooth);
        double inPeak = 0.0, outPeak = 0.0;
        for (int c = 0; c < numChannels; ++c)
        {
            const double xin = m >= 0 ? audio[static_cast<size_t>(c)][static_cast<size_t>(r)] : 0.0;
            double y = xin * gain;
            if (peakNeutral)
            {
                const double a = std::abs(xin), room = std::max(0.0, peakHold - a), boost = a * (gain - 1.0);
                y = std::copysign(a + (room > 0.0 ? room * std::tanh(boost / room) : 0.0), xin);
            }
            if (listen == 1)
                y *= share;
            else if (listen == 2)
                y *= 1.0 - share;
            y *= outputGain;
            inPeak = std::max(inPeak, std::abs(xin));
            outPeak = std::max(outPeak, std::abs(y));
            channels[c][i] = static_cast<float>(y);
        }
        // contrast meter: the gain's attack share, 300 ms
        impact = (gainSmooth * share) + impactC * (impact - gainSmooth * share);
        // last-hit graph: 64 bins of 2.5 ms from the hit (audio time)
        if (capT0 >= 0 && m >= capT0)
        {
            const long long since = m - capT0;
            const int b = static_cast<int>(since / bin);
            if (b < kGraphPoints)
            {
                capIn[static_cast<size_t>(b)] = std::max(capIn[static_cast<size_t>(b)], static_cast<float>(inPeak));
                capOut[static_cast<size_t>(b)] = std::max(capOut[static_cast<size_t>(b)], static_cast<float>(outPeak));
                capBins = b + 1;
            }
            else if (capBins == kGraphPoints)
            {
                for (int k = 0; k < kGraphPoints; ++k)
                {
                    hitIn[static_cast<size_t>(k)].store(capIn[static_cast<size_t>(k)], std::memory_order_relaxed);
                    hitOut[static_cast<size_t>(k)].store(capOut[static_cast<size_t>(k)], std::memory_order_relaxed);
                }
                hitSequence.fetch_add(1, std::memory_order_release);
                capT0 = -1;
                capBins = 0;
            }
        }
        ++counter;
    }
    meterImpact.store(static_cast<float>(impact), std::memory_order_relaxed);
    meterGainMin.store(static_cast<float>(gMin), std::memory_order_relaxed);
    meterGainMax.store(static_cast<float>(gMax), std::memory_order_relaxed);
    meterInput.store(blockInput > 0.0f ? 20.0f * std::log10(blockInput) : -120.0f, std::memory_order_relaxed);
}

UdarnikCore::Meters UdarnikV2::meters() const noexcept
{
    UdarnikCore::Meters m;
    m.impactDb = meterImpact.load(std::memory_order_relaxed);
    m.gainMinDb = meterGainMin.load(std::memory_order_relaxed);
    m.gainMaxDb = meterGainMax.load(std::memory_order_relaxed);
    m.inputDb = meterInput.load(std::memory_order_relaxed);
    m.events = meterEvents.load(std::memory_order_relaxed);
    m.confidence = 1.0f;
    m.strengthDb = meterStrength.load(std::memory_order_relaxed);
    return m;
}

int UdarnikV2::lastHit(std::array<float, kGraphPoints>& in, std::array<float, kGraphPoints>& out) const noexcept
{
    const int seq = hitSequence.load(std::memory_order_acquire);
    for (int k = 0; k < kGraphPoints; ++k)
    {
        in[static_cast<size_t>(k)] = hitIn[static_cast<size_t>(k)].load(std::memory_order_relaxed);
        out[static_cast<size_t>(k)] = hitOut[static_cast<size_t>(k)].load(std::memory_order_relaxed);
    }
    return seq;
}
} // namespace hamlo::dsp
