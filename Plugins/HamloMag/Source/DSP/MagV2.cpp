#include "DSP/MagV2.h"

#include <algorithm>
#include <cmath>

namespace hamlo::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kButterworthQ = 0.70710678118654752;

struct Device
{
    double rate, antiAliasHz, outputHz;
    bool companded;
};
// docs/MAG_V2.md section 2: index 1 SP-1200 (no anti-alias filter: it aliases), 2 MPC60, 3 S950
constexpr std::array<Device, 4> kDevices { { { 0.0, 0.0, 0.0, false },
                                             { 26040.0, 0.0, 12000.0, false },
                                             { 40000.0, 18000.0, 18000.0, true },
                                             { 32000.0, 14000.0, 14000.0, false } } };
constexpr double kMu = 7.0;

double quantize12(double x, bool companded) noexcept
{
    x = std::clamp(x, -1.0, 1.0); // the device clips at full scale
    if (companded)
    {
        const double c = std::copysign(std::log1p(kMu * std::abs(x)) / std::log1p(kMu), x);
        const double q = std::round(c * 2047.0) / 2047.0;
        return std::copysign(std::expm1(std::abs(q) * std::log1p(kMu)) / kMu, q);
    }
    return std::round(x * 2047.0) / 2047.0;
}
} // namespace

void MagV2Processor::Biquad::lowPass(double hz, double q, double sampleRate) noexcept
{
    const double w = 2.0 * kPi * std::clamp(hz, 10.0, 0.49 * sampleRate) / sampleRate, c = std::cos(w), a = std::sin(w) / (2.0 * q);
    const double n = 1.0 / (1.0 + a);
    b0 = b2 = (1.0 - c) * 0.5 * n;
    b1 = (1.0 - c) * n;
    a1 = -2.0 * c * n;
    a2 = (1.0 - a) * n;
}

void MagV2Processor::Biquad::highPass(double hz, double q, double sampleRate) noexcept
{
    const double w = 2.0 * kPi * std::clamp(hz, 10.0, 0.49 * sampleRate) / sampleRate, c = std::cos(w), a = std::sin(w) / (2.0 * q);
    const double n = 1.0 / (1.0 + a);
    b0 = b2 = (1.0 + c) * 0.5 * n;
    b1 = -(1.0 + c) * n;
    a1 = -2.0 * c * n;
    a2 = (1.0 - a) * n;
}

bool MagV2Processor::usesV2(const FxSlotParams& p) noexcept
{
    return p.values[slot::kSatSubHz] > 0.0f || p.values[slot::kSatPhone] > 0.0f || p.values[slot::kSatDynamics] != 0.0f;
}

bool MagV2Processor::usesSampler(const FxSlotParams& p) noexcept
{
    return std::lround(p.values[slot::kSatSampler]) > 0;
}

int MagV2Processor::latencySamples(const FxSlotParams& p, double sampleRate)
{
    return SaturatorProcessor::latencySamples(p, sampleRate) + (usesSampler(p) ? samplerLatency() : 0);
}

void MagV2Processor::prepare(double sampleRate, int maxBlockSize)
{
    fs = sampleRate;
    maxBlock = std::max(1, maxBlockSize);
    core.prepare(sampleRate, maxBlock);
    using OS = juce::dsp::Oversampling<float>;
    samplerOs = std::make_unique<OS>(kMaxChannels, 2, OS::filterHalfBandFIREquiripple, true, true);
    samplerOs->initProcessing(static_cast<size_t>(std::min(maxBlock, SaturatorProcessor::kOversamplingChunk)));
    FxSlotParams largest; // the most the saturator can delay: 8x, TAPE, WOW
    largest.values[slot::kOversampling] = 3.0f;
    largest.values[slot::kSatModel] = 1.0f;
    largest.values[slot::kSatWow] = 1.0f;
    const int capacity = SaturatorProcessor::latencySamples(largest, sampleRate) + 8;
    lowDelay.prepare(capacity);
    dryDelay.prepare(capacity);
    phoneDelay.prepare(capacity);
    modDelay.prepare(capacity);
    for (auto* set : { &low, &phone, &source, &dryPart })
        for (auto& v : *set)
            v.assign(static_cast<size_t>(maxBlock), 0.0f);
    modulation.assign(static_cast<size_t>(maxBlock), 1.0f);
    // the anti-alias FIRs at 4x: windowed sinc, Kaiser beta 7.86, unity gain at DC
    for (int mode = 2; mode <= 3; ++mode)
    {
        const double cutoff = kDevices[static_cast<size_t>(mode)].antiAliasHz / (4.0 * fs);
        double sum = 0.0;
        auto& h = fir[static_cast<size_t>(mode)];
        for (int i = 0; i < kFirTaps; ++i)
        {
            const double t = i - kFirDelay, r = (2.0 * i) / (kFirTaps - 1) - 1.0;
            const double sinc = t == 0.0 ? 2.0 * cutoff : std::sin(2.0 * kPi * cutoff * t) / (kPi * t);
            const double window = std::cyl_bessel_i(0.0, 7.86 * std::sqrt(std::max(0.0, 1.0 - r * r))) / std::cyl_bessel_i(0.0, 7.86);
            h[static_cast<size_t>(i)] = static_cast<float>(sinc * window);
            sum += sinc * window;
        }
        for (auto& v : h)
            v = static_cast<float>(v / sum);
    }
    reset();
}

void MagV2Processor::reset() noexcept
{
    core.reset();
    if (samplerOs != nullptr)
        samplerOs->reset();
    lowDelay.reset();
    dryDelay.reset();
    phoneDelay.reset();
    modDelay.reset();
    for (auto& c : state)
    {
        for (auto* set : { &c.subLow, &c.subHigh, &c.phoneLow, &c.phoneHigh, &c.phoneTop, &c.output })
            for (auto& b : *set)
                b.reset();
        c.ring.fill(0.0f);
        c.write = 0;
        c.phoneEnvelope = c.dcIn = c.dcOut = c.held = 0.0;
    }
    fastEnv = slowEnv = devicePhase = 0.0;
}

void MagV2Processor::design(const FxSlotParams& p) noexcept
{
    const double sub = p.values[slot::kSatSubHz];
    if (sub != designedSub)
    {
        designedSub = sub;
        for (auto& c : state)
            for (int k = 0; k < 2; ++k)
            {
                c.subLow[static_cast<size_t>(k)].lowPass(std::max(20.0, sub), kButterworthQ, fs);
                c.subHigh[static_cast<size_t>(k)].highPass(std::max(20.0, sub), kButterworthQ, fs);
            }
    }
    const int mode = std::clamp(static_cast<int>(std::lround(p.values[slot::kSatSampler])), 0, 3);
    if (mode != designedSampler)
    {
        designedSampler = mode;
        const auto& d = kDevices[static_cast<size_t>(mode)];
        const double fs4 = 4.0 * fs;
        for (auto& c : state)
        {
            c.output[0].lowPass(std::max(20.0, d.outputHz), 0.54119610014619698, fs4); // 4th-order Butterworth
            c.output[1].lowPass(std::max(20.0, d.outputHz), 1.3065629648763766, fs4);
        }
    }
}

void MagV2Processor::sample(float* const* channels, int numChannels, int numSamples, int mode) noexcept
{
    const auto& d = kDevices[static_cast<size_t>(mode)];
    const double step = d.rate / (4.0 * fs);
    for (int start = 0; start < numSamples; start += SaturatorProcessor::kOversamplingChunk)
    {
        const int n = std::min(SaturatorProcessor::kOversamplingChunk, numSamples - start);
        juce::dsp::AudioBlock<float> block(channels, static_cast<size_t>(numChannels), static_cast<size_t>(start), static_cast<size_t>(n));
        auto up = samplerOs->processSamplesUp(block);
        const auto count = up.getNumSamples();
        for (size_t i = 0; i < count; ++i)
        {
            devicePhase += step;
            const bool tick = devicePhase >= 1.0;
            const double back = tick ? (devicePhase - 1.0) / step : 0.0; // how far before this sample the tick was
            if (tick)
                devicePhase -= 1.0;
            for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
            {
                auto& c = state[ch];
                float* data = up.getChannelPointer(ch);
                constexpr int size = 512;
                c.ring[static_cast<size_t>(c.write)] = data[i];
                // the (filtered) input kFirDelay samples ago, and one sample before that, only at a tick
                if (tick)
                {
                    const auto at = [&](int age) // filtered value `age` samples before the newest, delayed by kFirDelay
                    {
                        if (d.antiAliasHz <= 0.0)
                            return static_cast<double>(c.ring[static_cast<size_t>((c.write - kFirDelay - age + 2 * size) % size)]);
                        const auto& h = fir[static_cast<size_t>(mode)];
                        double acc = 0.0;
                        for (int k = 0; k < kFirTaps; ++k)
                            acc += static_cast<double>(h[static_cast<size_t>(k)]) * c.ring[static_cast<size_t>((c.write - age - k + 2 * size) % size)];
                        return acc;
                    };
                    const double now = at(0), before = at(1);
                    c.held = quantize12(now + back * (before - now), d.companded);
                }
                c.write = (c.write + 1) % size;
                double y = c.held;
                for (auto& b : c.output)
                    y = b.process(y);
                data[i] = static_cast<float>(y);
            }
        }
        samplerOs->processSamplesDown(block);
    }
}

void MagV2Processor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    numChannels = std::min(numChannels, kMaxChannels);
    design(p);
    const int mode = std::clamp(static_cast<int>(std::lround(p.values[slot::kSatSampler])), 0, 3);
    if (mode > 0)
        sample(channels, numChannels, numSamples, mode);
    if (!usesV2(p))
    {
        core.process(channels, numChannels, numSamples, p);
        return;
    }

    const int latency = SaturatorProcessor::latencySamples(p, fs);
    const double sub = p.values[slot::kSatSubHz];
    const double phoneAmount = std::clamp(static_cast<double>(p.values[slot::kSatPhone]), 0.0, 1.0);
    const double dynamics = std::clamp(static_cast<double>(p.values[slot::kSatDynamics]), -1.0, 1.0);
    const bool classic = std::lround(p.values[slot::kSatModel]) == 0;
    float* lowPtr[kMaxChannels] {};
    float* phonePtr[kMaxChannels] {};
    for (int ch = 0; ch < numChannels; ++ch)
    {
        lowPtr[ch] = low[static_cast<size_t>(ch)].data();
        phonePtr[ch] = phone[static_cast<size_t>(ch)].data();
        std::copy(channels[ch], channels[ch] + numSamples, source[static_cast<size_t>(ch)].data());
    }

    // PHONE: harmonics of the lows, from the input
    if (phoneAmount > 0.0)
    {
        const double attack = std::exp(-1.0 / (0.001 * fs)), release = std::exp(-1.0 / (0.100 * fs));
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& c = state[static_cast<size_t>(ch)];
            for (int k = 0; k < 2; ++k)
            {
                c.phoneLow[static_cast<size_t>(k)].lowPass(120.0, kButterworthQ, fs);
                c.phoneHigh[static_cast<size_t>(k)].highPass(70.0, kButterworthQ, fs);
                c.phoneTop[static_cast<size_t>(k)].lowPass(700.0, kButterworthQ, fs);
            }
            const float* x = source[static_cast<size_t>(ch)].data();
            for (int i = 0; i < numSamples; ++i)
            {
                double l = x[i];
                for (auto& b : c.phoneLow)
                    l = b.process(l);
                const double a = std::abs(l);
                c.phoneEnvelope = a > c.phoneEnvelope ? a + attack * (c.phoneEnvelope - a) : a + release * (c.phoneEnvelope - a);
                const double e = std::max(1.0e-5, c.phoneEnvelope);
                const double u = std::clamp(l / e, -1.0, 1.0), u2 = u * u;
                const double u3 = u2 * u, u4 = u2 * u2;
                double h = e * (0.22 * (2.0 * u2 - 1.0) + 0.22 * (4.0 * u3 - 3.0 * u) + 0.20 * (8.0 * u4 - 8.0 * u2 + 1.0)
                                + 0.20 * (16.0 * u4 * u - 20.0 * u3 + 5.0 * u) + 0.18 * (32.0 * u4 * u2 - 48.0 * u4 + 18.0 * u2 - 1.0));
                for (auto& b : c.phoneHigh)
                    h = b.process(h);
                for (auto& b : c.phoneTop)
                    h = b.process(h);
                phonePtr[ch][i] = static_cast<float>(phoneAmount * h);
            }
        }
    }
    else
        for (int ch = 0; ch < numChannels; ++ch)
            std::fill(phonePtr[ch], phonePtr[ch] + numSamples, 0.0f);

    // SUB: split
    if (sub > 0.0)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& c = state[static_cast<size_t>(ch)];
            for (int i = 0; i < numSamples; ++i)
            {
                double lo = channels[ch][i], hi = channels[ch][i];
                for (auto& b : c.subLow)
                    lo = b.process(lo);
                for (auto& b : c.subHigh)
                    hi = b.process(hi);
                lowPtr[ch][i] = static_cast<float>(lo);
                channels[ch][i] = static_cast<float>(hi);
            }
        }
    else
        for (int ch = 0; ch < numChannels; ++ch)
            std::fill(lowPtr[ch], lowPtr[ch] + numSamples, 0.0f);

    // DYNAMICS: the saturation follows the hit - w = the saturated share of the signal
    if (dynamics != 0.0)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            std::copy(channels[ch], channels[ch] + numSamples, dryPart[static_cast<size_t>(ch)].data());
        const double fa = std::exp(-1.0 / (0.001 * fs)), fr = std::exp(-1.0 / (0.030 * fs));
        const double sa = std::exp(-1.0 / (0.030 * fs)), sr = std::exp(-1.0 / (0.300 * fs));
        for (int i = 0; i < numSamples; ++i)
        {
            double a = 0.0;
            for (int ch = 0; ch < numChannels; ++ch)
                a = std::max(a, static_cast<double>(std::abs(source[static_cast<size_t>(ch)][static_cast<size_t>(i)])));
            fastEnv = a > fastEnv ? a + fa * (fastEnv - a) : a + fr * (fastEnv - a);
            slowEnv = a > slowEnv ? a + sa * (slowEnv - a) : a + sr * (slowEnv - a);
            const double d = 20.0 * std::log10((fastEnv + 1.0e-9) / (slowEnv + 1.0e-9));
            const double hit = std::clamp(d / 6.0, 0.0, 1.0);
            modulation[static_cast<size_t>(i)] = static_cast<float>(1.0 - std::abs(dynamics) * (1.0 - (dynamics > 0.0 ? hit : 1.0 - hit)));
        }
    }
    else
        std::fill(modulation.begin(), modulation.begin() + numSamples, 1.0f);

    core.process(channels, numChannels, numSamples, p);

    // the dry share, the low band and the phone harmonics, all delayed by the saturator's latency
    {
        float* mod[kMaxChannels] { modulation.data(), nullptr };
        modDelay.process(mod, mod, 1, numSamples, latency);
    }
    float* dryPtr[kMaxChannels] {};
    for (int ch = 0; ch < numChannels; ++ch)
        dryPtr[ch] = dryPart[static_cast<size_t>(ch)].data();
    if (dynamics != 0.0)
        dryDelay.process(dryPtr, dryPtr, numChannels, numSamples, latency);
    lowDelay.process(lowPtr, lowPtr, numChannels, numSamples, latency);
    phoneDelay.process(phonePtr, phonePtr, numChannels, numSamples, latency);
    const double dc = 1.0 - 2.0 * kPi * 10.0 / fs; // CLASSIC's DC blocker, also on the low band
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& c = state[static_cast<size_t>(ch)];
        for (int i = 0; i < numSamples; ++i)
        {
            double lo = lowPtr[ch][i];
            if (classic)
            {
                const double y = lo - c.dcIn + dc * c.dcOut;
                c.dcIn = lo;
                c.dcOut = y;
                lo = y;
            }
            const float w = modulation[static_cast<size_t>(i)];
            const float wet = w == 1.0f ? channels[ch][i] : dryPtr[ch][i] + w * (channels[ch][i] - dryPtr[ch][i]);
            channels[ch][i] = static_cast<float>(wet + lo + phonePtr[ch][i]);
        }
    }
}
} // namespace hamlo::dsp
