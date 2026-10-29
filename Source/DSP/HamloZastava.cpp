#include "DSP/HamloZastava.h"

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
} // namespace

int ZastavaCore::lookaheadSamples(int index, double sampleRate) noexcept
{
    static constexpr double ms[] { 0.0, 2.0, 5.0 };
    return static_cast<int>(std::lround(ms[std::clamp(index, 0, 2)] * 1.0e-3 * sampleRate));
}

void ZastavaCore::prepare(double sampleRate)
{
    fs = sampleRate;
    capacity = lookaheadSamples(2, fs) + 2;
    for (auto* group : { &audio, &key })
        for (auto& c : *group)
            c.assign(static_cast<size_t>(capacity), 0.0f);
    dequeValue.assign(static_cast<size_t>(capacity + 1), 0.0f);
    dequeIndex.assign(static_cast<size_t>(capacity + 1), 0);
    lookahead = -1;
    designedHp = designedLp = -1.0;
    reset();
}

void ZastavaCore::reset() noexcept
{
    clearState();
    meterOpenings.store(0, std::memory_order_relaxed);
}

void ZastavaCore::clearState() noexcept
{
    counter = 0;
    detector = attenuationDb = 0.0;
    open = false;
    holdLeft = openings = 0;
    hp.z = {};
    lp.z = {};
    for (auto* group : { &audio, &key })
        for (auto& c : *group)
            std::fill(c.begin(), c.end(), 0.0f);
    dequeHead = dequeTail = 0;
}

void ZastavaCore::design(double hpHz, double lpHz) noexcept
{
    // RBJ cookbook, Q = 1/sqrt(2) (Butterworth); a frequency of 0 = off
    const auto make = [this](Biquad& f, double hz, bool highPass)
    {
        const double w = 2.0 * kPi * std::clamp(hz, 10.0, 0.45 * fs) / fs;
        const double cw = std::cos(w), alpha = std::sin(w) / (2.0 * 0.7071067811865476), a0 = 1.0 + alpha;
        f.b0 = (highPass ? (1.0 + cw) : (1.0 - cw)) / 2.0 / a0;
        f.b1 = (highPass ? -(1.0 + cw) : (1.0 - cw)) / a0;
        f.b2 = f.b0;
        f.a1 = -2.0 * cw / a0;
        f.a2 = (1.0 - alpha) / a0;
    };
    hpOn = hpHz > 0.0;
    lpOn = lpHz > 0.0;
    if (hpOn)
        make(hp, hpHz, true);
    if (lpOn)
        make(lp, lpHz, false);
    designedHp = hpHz;
    designedLp = lpHz;
}

ZastavaCore::Meters ZastavaCore::meters() const noexcept
{
    Meters m;
    m.attenuationDb = meterNow.load(std::memory_order_relaxed);
    m.maxAttenuationDb = meterMax.load(std::memory_order_relaxed);
    m.keyDb = meterKey.load(std::memory_order_relaxed);
    m.open = meterOpen.load(std::memory_order_relaxed);
    m.openings = meterOpenings.load(std::memory_order_relaxed);
    return m;
}

void ZastavaCore::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    const int style = std::clamp(static_cast<int>(std::lround(p.values[slot::kGateStyle])), 0, 2);
    const int L = std::min(capacity - 2, lookaheadSamples(static_cast<int>(std::lround(p.values[slot::kGateLookahead])), fs));
    if (L != lookahead)
    {
        lookahead = L;
        clearState();
    }
    const double hpHz = std::clamp(p.values[slot::kGateKeyHpHz], 0.0f, 20000.0f);
    const double lpHz = std::clamp(p.values[slot::kGateKeyLpHz], 0.0f, 20000.0f);
    if (hpHz != designedHp || lpHz != designedLp)
        design(hpHz, lpHz);
    const double threshold = std::clamp(p.values[slot::kGateThresholdDb], -90.0f, 0.0f);
    const double ratio = std::clamp(p.values[slot::kGateRatio], 1.0f, 100.0f);
    const double range = std::clamp(p.values[slot::kGateRangeDb], 0.0f, 90.0f);
    const double attack = coefficientFor(p.values[slot::kGateAttackMs], fs);
    const double release = coefficientFor(p.values[slot::kGateReleaseMs], fs);
    const int hold = static_cast<int>(std::max(0.0f, p.values[slot::kGateHoldMs]) * 1.0e-3 * fs);
    const double hysteresis = std::clamp(p.values[slot::kGateHysteresisDb], 0.0f, 24.0f);
    const double detectorRelease = coefficientFor(10.0, fs);
    const bool flip = p.values[slot::kGateFlip] != 0.0f, listen = p.values[slot::kGateKeyListen] != 0.0f;
    const float outputGain = static_cast<float>(std::pow(10.0, std::clamp(p.values[slot::kGateOutputDb], -24.0f, 12.0f) / 20.0));
    const bool duck = style == Trevoga;
    const int cap = static_cast<int>(dequeValue.size());

    float keyPeak = 0.0f;
    double maxApplied = 0.0, applied = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        // ------------------------------------------------------------ detector (time n)
        const long long n = counter;
        const int w = static_cast<int>(n % capacity);
        float level = 0.0f;
        for (int c = 0; c < numChannels; ++c)
        {
            const float x = channels[c][i];
            double k = x;
            if (hpOn)
                k = hp.process(c, k);
            if (lpOn)
                k = lp.process(c, k);
            audio[static_cast<size_t>(c)][static_cast<size_t>(w)] = x;
            key[static_cast<size_t>(c)][static_cast<size_t>(w)] = static_cast<float>(k);
            level = std::max(level, static_cast<float>(std::abs(k)));
        }
        keyPeak = std::max(keyPeak, level);
        detector = std::max(static_cast<double>(level), detector * detectorRelease);
        const double x = 20.0 * std::log10(detector + 1.0e-12);
        const bool wasOpen = open;
        if (x > threshold)
        {
            open = true;
            holdLeft = hold;
        }
        else if (x < threshold - hysteresis)
        {
            if (holdLeft > 0)
                --holdLeft;
            else
                open = false;
        }
        openings += !wasOpen && open ? 1 : 0;
        double target;
        if (style == Patrul)
            target = std::clamp((threshold - x) * (ratio - 1.0), 0.0, range);
        else if (duck)
            target = open ? range : 0.0;
        else
            target = open ? 0.0 : std::min(range, (threshold - x) * (ratio - 1.0));
        if (threshold <= -90.0)
            target = 0.0; // the knob's end is OFF: never closes, never ducks (owner: a new gate changes nothing)
        const bool fast = duck ? target > attenuationDb : target < attenuationDb;
        attenuationDb = target + (fast ? attack : release) * (attenuationDb - target);
        if (attenuationDb < 1.0e-9)
            attenuationDb = 0.0;

        // look-ahead: min (duck: max) of the attenuation over the last L + 1 detector samples
        const auto value = static_cast<float>(attenuationDb);
        while (dequeTail != dequeHead)
        {
            const float last = dequeValue[static_cast<size_t>((dequeTail - 1 + cap) % cap)];
            if (duck ? last > value : last < value)
                break;
            dequeTail = (dequeTail - 1 + cap) % cap;
        }
        dequeValue[static_cast<size_t>(dequeTail)] = value;
        dequeIndex[static_cast<size_t>(dequeTail)] = n;
        dequeTail = (dequeTail + 1) % cap;
        while (dequeIndex[static_cast<size_t>(dequeHead)] < n - L)
            dequeHead = (dequeHead + 1) % cap;
        applied = dequeValue[static_cast<size_t>(dequeHead)];
        maxApplied = std::max(maxApplied, applied);

        // ------------------------------------------------------------ audio (time m = n - L)
        const int r = static_cast<int>((((n - L) % capacity) + capacity) % capacity);
        const auto g = applied == 0.0 ? 1.0f : static_cast<float>(std::pow(10.0, -applied / 20.0));
        for (int c = 0; c < numChannels; ++c)
        {
            float y;
            if (listen)
                y = key[static_cast<size_t>(c)][static_cast<size_t>(r)];
            else
            {
                const float xd = audio[static_cast<size_t>(c)][static_cast<size_t>(r)];
                y = xd * g;
                if (flip)
                    y = xd - y;
            }
            if (outputGain != 1.0f)
                y *= outputGain;
            channels[c][i] = y;
        }
        ++counter;
    }
    meterNow.store(static_cast<float>(applied), std::memory_order_relaxed);
    meterMax.store(static_cast<float>(maxApplied), std::memory_order_relaxed);
    meterKey.store(static_cast<float>(20.0 * std::log10(keyPeak + 1.0e-6)), std::memory_order_relaxed);
    meterOpen.store(open, std::memory_order_relaxed);
    meterOpenings.store(openings, std::memory_order_relaxed);
}
} // namespace hpeq::dsp
