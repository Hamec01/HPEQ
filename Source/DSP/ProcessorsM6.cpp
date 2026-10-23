#include "DSP/ProcessorsM6.h"

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

float linkedPeak(float* const* channels, int numChannels, int i) noexcept
{
    float level = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        level = std::max(level, std::abs(channels[ch][i]));
    return level;
}

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
} // namespace

// ------------------------------------------------------------------------------------- Saturator
int SaturatorProcessor::latencySamples(int oversamplingIndex)
{
    // Measured, not taken from Oversampling::getLatencyInSamples(): for 4x (two half-band stages) that
    // reports 60 samples while the chain delays by 61 (M6 test "measured latency equals reported").
    // White noise through up -> down without shaping; the latency is the lag of maximum correlation.
    static const std::array<int, 3> table = []
    {
        using OS = juce::dsp::Oversampling<float>;
        constexpr int n = 4096, maxLag = 400;
        std::vector<float> input(static_cast<size_t>(n));
        std::uint32_t state = 12345u;
        for (auto& v : input)
        {
            state = state * 1664525u + 1013904223u;
            v = static_cast<float>(state >> 8) / static_cast<float>(1u << 24) - 0.5f;
        }
        std::array<int, 3> result { 0, 0, 0 };
        for (int stages = 1; stages <= 2; ++stages)
        {
            OS os(1, static_cast<size_t>(stages), OS::filterHalfBandFIREquiripple, true, true);
            os.initProcessing(static_cast<size_t>(n));
            auto output = input;
            float* channel[] { output.data() };
            juce::dsp::AudioBlock<float> block(channel, 1, static_cast<size_t>(n));
            os.processSamplesUp(block);
            os.processSamplesDown(block);
            double best = -1.0;
            for (int lag = 0; lag < maxLag; ++lag)
            {
                double c = 0.0;
                for (int i = 0; i + lag < n; ++i)
                    c += static_cast<double>(output[static_cast<size_t>(i + lag)]) * input[static_cast<size_t>(i)];
                if (c > best)
                {
                    best = c;
                    result[static_cast<size_t>(stages)] = lag;
                }
            }
        }
        return result;
    }();
    return table[static_cast<size_t>(std::clamp(oversamplingIndex, 0, 2))];
}

int SaturatorProcessor::latencySamples(const FxSlotParams& p, double sampleRate)
{
    return latencySamples(static_cast<int>(std::lround(p.values[slot::kOversampling]))) + MagProcessor::latencySamples(p, sampleRate);
}

void SaturatorProcessor::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    using OS = juce::dsp::Oversampling<float>;
    for (int i = 0; i < 2; ++i)
    {
        oversamplers[static_cast<size_t>(i)] = std::make_unique<OS>(kMaxChannels, static_cast<size_t>(i + 1), OS::filterHalfBandFIREquiripple, true, true);
        oversamplers[static_cast<size_t>(i)]->initProcessing(static_cast<size_t>(std::min(maxBlockSize, kOversamplingChunk)));
    }
    dcCoefficient = 1.0 - 2.0 * kPi * 10.0 / sampleRate;
    latencySamples(0); // build the latency table here, never on the audio thread
    mag.prepare(sampleRate, 1);
    magFactor = 1;
    reset();
}

void SaturatorProcessor::reset() noexcept
{
    for (auto& os : oversamplers)
        if (os != nullptr)
            os->reset();
    dcIn = {};
    dcOut = {};
    mag.reset();
}

void SaturatorProcessor::shape(float* data, int n, float g, float bias, float norm, int curve) noexcept
{
    const auto f = [curve](float u)
    {
        switch (curve)
        {
            case 1: return u >= 0.0f ? std::tanh(u) : std::tanh(1.4f * u) / 1.4f;
            case 2: return std::clamp(u, -1.0f, 1.0f);
            default: return std::tanh(u);
        }
    };
    const float offset = f(bias);
    for (int i = 0; i < n; ++i)
        data[i] = (f(g * data[i] + bias) - offset) * norm;
}

void SaturatorProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    if (std::lround(p.values[slot::kSatModel]) > 0)
    {
        const int os = std::clamp(static_cast<int>(std::lround(p.values[slot::kOversampling])), 0, 2);
        if (os != activeOversampling)
        {
            if (os > 0)
                oversamplers[static_cast<size_t>(os - 1)]->reset();
            activeOversampling = os;
        }
        const int factor = 1 << os;
        mag.beginBlock(p, factor);
        if (factor != magFactor)
        {
            mag.prepareCores(sampleRate * factor); // coefficients only, no allocation
            magFactor = factor;
        }
        for (int ch = 0; ch < numChannels; ++ch)
            mag.pre(channels[ch], numSamples, ch);
        if (os == 0)
            for (int ch = 0; ch < numChannels; ++ch)
                mag.core(channels[ch], numSamples, ch);
        else
        {
            auto& sampler = *oversamplers[static_cast<size_t>(os - 1)];
            for (int start = 0; start < numSamples; start += kOversamplingChunk)
            {
                const int n = std::min(kOversamplingChunk, numSamples - start);
                juce::dsp::AudioBlock<float> block(channels, static_cast<size_t>(numChannels), static_cast<size_t>(start), static_cast<size_t>(n));
                auto up = sampler.processSamplesUp(block);
                for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
                    mag.core(up.getChannelPointer(ch), static_cast<int>(up.getNumSamples()), static_cast<int>(ch));
                sampler.processSamplesDown(block);
            }
        }
        mag.post(channels, numChannels, numSamples);
        return;
    }

    const float g = std::pow(10.0f, std::clamp(p.values[slot::kDriveDb], 0.0f, 48.0f) / 20.0f);
    const int curve = std::clamp(static_cast<int>(std::lround(p.values[slot::kCurve])), 0, 2);
    const float bias = std::clamp(p.values[slot::kBias], -0.9f, 0.9f);
    const float trim = std::pow(10.0f, std::clamp(p.values[slot::kTrimDb], -24.0f, 24.0f) / 20.0f);
    float slopeAtBias = 1.0f;
    if (curve == 0 || (curve == 1 && bias >= 0.0f))
        slopeAtBias = 1.0f - std::tanh(bias) * std::tanh(bias);
    else if (curve == 1)
        slopeAtBias = 1.0f - std::tanh(1.4f * bias) * std::tanh(1.4f * bias);
    const float norm = (p.values[slot::kAutoGain] > 0.5f ? 1.0f / (g * slopeAtBias) : 1.0f) * trim;

    const int os = std::clamp(static_cast<int>(std::lround(p.values[slot::kOversampling])), 0, 2);
    if (os != activeOversampling)
    {
        if (os > 0)
            oversamplers[static_cast<size_t>(os - 1)]->reset();
        activeOversampling = os;
    }

    if (os == 0)
        for (int ch = 0; ch < numChannels; ++ch)
            shape(channels[ch], numSamples, g, bias, norm, curve);
    else
    {
        auto& sampler = *oversamplers[static_cast<size_t>(os - 1)];
        for (int start = 0; start < numSamples; start += kOversamplingChunk)
        {
            const int n = std::min(kOversamplingChunk, numSamples - start);
            juce::dsp::AudioBlock<float> block(channels, static_cast<size_t>(numChannels), static_cast<size_t>(start), static_cast<size_t>(n));
            auto up = sampler.processSamplesUp(block);
            for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
                shape(up.getChannelPointer(ch), static_cast<int>(up.getNumSamples()), g, bias, norm, curve);
            sampler.processSamplesDown(block);
        }
    }

    // 10 Hz DC blocker: y = x - x1 + R y1
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& x1 = dcIn[static_cast<size_t>(ch)];
        auto& y1 = dcOut[static_cast<size_t>(ch)];
        for (int i = 0; i < numSamples; ++i)
        {
            const double x = channels[ch][i];
            const double y = x - x1 + dcCoefficient * y1;
            x1 = x;
            y1 = y;
            channels[ch][i] = static_cast<float>(y);
        }
    }
}

// ------------------------------------------------------------------------------------- Transient
void TransientProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    const double attackDb = std::clamp(p.values[slot::kAttackGainDb], -24.0f, 24.0f);
    const double sustainDb = std::clamp(p.values[slot::kSustainGainDb], -24.0f, 24.0f);
    const double s = 0.5 + std::clamp(p.values[slot::kTransientSpeed], 0.0f, 1.0f);
    const double fastAttack = coefficientFor(0.5, sampleRate_), fastRelease = coefficientFor(20.0 * s, sampleRate_);
    const double slowAttack = coefficientFor(20.0 * s, sampleRate_), slowRelease = coefficientFor(250.0 * s, sampleRate_);
    const bool identity = attackDb == 0.0 && sustainDb == 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        const double level = linkedPeak(channels, numChannels, i);
        fast = level + (level > fast ? fastAttack : fastRelease) * (fast - level);
        slow = level + (level > slow ? slowAttack : slowRelease) * (slow - level);
        if (identity)
            continue;
        const double d = 20.0 * std::log10((fast + 1.0e-9) / (slow + 1.0e-9));
        const double gainDb = attackDb * std::clamp(d / 6.0, 0.0, 1.0) + sustainDb * std::clamp(-d / 6.0, 0.0, 1.0);
        if (gainDb == 0.0)
            continue;
        const auto g = static_cast<float>(std::pow(10.0, gainDb / 20.0));
        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][i] *= g;
    }
}

// ------------------------------------------------------------------------------------- Stereo
void StereoProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    const float width = std::clamp(p.values[slot::kStereoWidth], 0.0f, 2.0f);
    const float balance = std::clamp(p.values[slot::kStereoBalance], -1.0f, 1.0f);
    if (numChannels < 2 || (width == 1.0f && balance == 0.0f))
        return;
    const float gl = balance > 0.0f ? 1.0f - balance : 1.0f;
    const float gr = balance < 0.0f ? 1.0f + balance : 1.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float m = 0.5f * (channels[0][i] + channels[1][i]);
        const float sd = 0.5f * (channels[0][i] - channels[1][i]) * width;
        channels[0][i] = (m + sd) * gl;
        channels[1][i] = (m - sd) * gr;
    }
}

// ------------------------------------------------------------------------------------- Gate
void GateProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    const double threshold = std::clamp(p.values[slot::kGateThresholdDb], -90.0f, 0.0f);
    const double ratio = std::clamp(p.values[slot::kGateRatio], 1.0f, 100.0f);
    const double range = std::clamp(p.values[slot::kGateRangeDb], 0.0f, 90.0f);
    const double attack = coefficientFor(p.values[slot::kGateAttackMs], sampleRate_);
    const double release = coefficientFor(p.values[slot::kGateReleaseMs], sampleRate_);
    const int hold = static_cast<int>(std::max(0.0f, p.values[slot::kGateHoldMs]) * 1.0e-3 * sampleRate_);
    const double hysteresis = std::clamp(p.values[slot::kGateHysteresisDb], 0.0f, 24.0f);
    const double detectorRelease = coefficientFor(kDetectorReleaseMs, sampleRate_);
    for (int i = 0; i < numSamples; ++i)
    {
        // Peak detector: instant attack, 10 ms release, so zero crossings of a tone are not "silence".
        const double level = linkedPeak(channels, numChannels, i);
        detector = std::max(level, detector * detectorRelease);
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
        transitions += wasOpen != open ? 1 : 0;
        const double target = open ? 0.0 : std::min(range, (threshold - x) * (ratio - 1.0));
        attenuationDb = target + (target < attenuationDb ? attack : release) * (attenuationDb - target);
        if (attenuationDb < 1.0e-9)
            attenuationDb = 0.0;
        if (attenuationDb == 0.0)
            continue;
        const auto g = static_cast<float>(std::pow(10.0, -attenuationDb / 20.0));
        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][i] *= g;
    }
}

// ------------------------------------------------------------------------------------- Limiter
int LimiterProcessor::maxLookaheadSamples(double sampleRate) noexcept
{
    return std::clamp(static_cast<int>(std::ceil(kMaxLookaheadMs * 1.0e-3 * sampleRate)) + 1, 2, kMaxLookahead);
}

int LimiterProcessor::baseLookaheadSamples(const FxSlotParams& p, double sampleRate) noexcept
{
    const double ms = std::clamp(static_cast<double>(p.values[slot::kLookaheadMs]), 0.1, kMaxLookaheadMs);
    return std::clamp(static_cast<int>(std::lround(ms * 1.0e-3 * sampleRate)), 1, maxLookaheadSamples(sampleRate));
}

int LimiterProcessor::lookaheadSamples(const FxSlotParams& p, double sampleRate) noexcept
{
    return baseLookaheadSamples(p, sampleRate) + (p.values[slot::kLimTruePeak] > 0.5f ? RedWallCore::kTruePeakDelay : 0);
}

bool LimiterProcessor::usesRedWall(const FxSlotParams& p) noexcept
{
    return p.values[slot::kLimThresholdDb] != 0.0f || p.values[slot::kLimTruePeak] > 0.5f || p.values[slot::kLimAutoRelease] > 0.5f
        || std::lround(p.values[slot::kLimStyle]) != 0 || p.values[slot::kLimLink] != 1.0f || std::lround(p.values[slot::kLimMonitor]) != 0;
}

void LimiterProcessor::prepare(double sampleRate)
{
    sampleRate_ = sampleRate;
    const int maxL = maxLookaheadSamples(sampleRate);
    for (auto& a : audio)
        a.assign(static_cast<size_t>(maxL + 1), 0.0f);
    averaged.assign(static_cast<size_t>(maxL + 1), 1.0);
    capacity = maxL + 2;
    dequeValue.assign(static_cast<size_t>(capacity), 1.0f);
    dequeIndex.assign(static_cast<size_t>(capacity), 0);
    lookahead = -1;
    core.prepare(sampleRate, maxL);
    redWallActive = false;
    reset();
}

void LimiterProcessor::reset() noexcept
{
    for (auto& a : audio)
        std::fill(a.begin(), a.end(), 0.0f);
    std::fill(averaged.begin(), averaged.end(), 1.0);
    dequeHead = dequeTail = 0;
    counter = 0;
    writePos = 0;
    released = 1.0;
    runningSum = lookahead > 0 ? static_cast<double>(lookahead) : 0.0;
    meter.store(0.0f, std::memory_order_relaxed);
    meterOutput.store(-120.0f, std::memory_order_relaxed);
    core.reset();
}

void LimiterProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p) noexcept
{
    const bool wantRedWall = usesRedWall(p);
    if (wantRedWall != redWallActive)
    {
        redWallActive = wantRedWall; // switching between the two paths starts clean
        lookahead = -1;
        reset();
    }
    if (redWallActive)
    {
        core.process(channels, numChannels, numSamples, p, baseLookaheadSamples(p, sampleRate_));
        return;
    }
    if (averaged.empty())
        return; // not prepared
    const int l = lookaheadSamples(p, sampleRate_);
    if (l != lookahead)
    {
        lookahead = l;
        reset();
    }
    const float ceiling = std::pow(10.0f, std::clamp(p.values[slot::kCeilingDb], -60.0f, 0.0f) / 20.0f);
    const double releaseCoefficient = coefficientFor(std::max(1.0f, p.values[slot::kLimiterReleaseMs]), sampleRate_);
    const int ring = lookahead + 1;
    float minGain = 1.0f, outPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float level = linkedPeak(channels, numChannels, i);
        const float target = level > ceiling ? ceiling / level : 1.0f;

        // Sliding minimum over the last L+1 targets (monotonic deque in fixed arrays).
        while (dequeTail != dequeHead && dequeValue[static_cast<size_t>((dequeTail - 1 + capacity) % capacity)] >= target)
            dequeTail = (dequeTail - 1 + capacity) % capacity;
        dequeIndex[static_cast<size_t>(dequeTail)] = counter;
        dequeValue[static_cast<size_t>(dequeTail)] = target;
        dequeTail = (dequeTail + 1) % capacity;
        while (counter - dequeIndex[static_cast<size_t>(dequeHead)] > lookahead) // older than L samples
            dequeHead = (dequeHead + 1) % capacity;
        const double held = dequeValue[static_cast<size_t>(dequeHead)];

        // Release on rising gain only; never above the held minimum.
        released = held < released ? held : released + (held - released) * (1.0 - releaseCoefficient);

        // Moving average over the last L values.
        const int avgPos = static_cast<int>(counter % lookahead);
        runningSum += released - averaged[static_cast<size_t>(avgPos)];
        averaged[static_cast<size_t>(avgPos)] = released;
        if (avgPos == 0 && counter % (1LL << 16) == 0)
        {
            runningSum = 0.0; // re-sum now and then: no slow floating-point drift
            for (int k = 0; k < lookahead; ++k)
                runningSum += averaged[static_cast<size_t>(k)];
        }
        const float gain = static_cast<float>(std::min(1.0, runningSum / lookahead));
        minGain = std::min(minGain, gain);

        // Delay the audio by L and apply the gain.
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& a = audio[static_cast<size_t>(ch)];
            const float delayed = a[static_cast<size_t>((writePos + 1) % ring)];
            a[static_cast<size_t>(writePos)] = channels[ch][i];
            channels[ch][i] = delayed * gain;
            outPeak = std::max(outPeak, std::abs(channels[ch][i]));
        }
        writePos = (writePos + 1) % ring;
        ++counter;
    }
    meter.store(-20.0f * std::log10(std::max(minGain, 1.0e-6f)), std::memory_order_relaxed);
    meterOutput.store(outPeak > 0.0f ? 20.0f * std::log10(outPeak) : -120.0f, std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------------- Delay
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

// ------------------------------------------------------------------------------------- Monsta
std::uint32_t MonstaProcessor::hash(std::uint32_t seed, std::uint32_t value) noexcept
{
    std::uint32_t h = seed ^ (value * 0x9E3779B9u);
    h ^= h >> 16;
    h *= 0x85EBCA6Bu;
    h ^= h >> 13;
    h *= 0xC2B2AE35u;
    h ^= h >> 16;
    return h;
}

MonstaProcessor::Action MonstaProcessor::actionFor(const FxSlotParams& p, long long slice) noexcept
{
    const auto seed = static_cast<std::uint32_t>(std::max(0.0f, p.values[slot::kSeed]));
    const auto h = hash(seed, static_cast<std::uint32_t>(slice));
    const double u1 = (h & 0xFFFFu) / 65536.0;
    if (u1 >= std::clamp(static_cast<double>(p.values[slot::kChance]), 0.0, 1.0))
        return Action::Pass;
    const double u2 = ((h >> 16) & 0xFFu) / 256.0;
    const double reverseShare = std::clamp(static_cast<double>(p.values[slot::kReverseShare]), 0.0, 1.0);
    if (u2 < reverseShare)
        return Action::Reverse;
    return u2 < reverseShare + (1.0 - reverseShare) * 0.35 ? Action::Stutter : Action::Repeat;
}

void MonstaProcessor::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    historySize = static_cast<int>(std::ceil(kHistorySeconds * sampleRate));
    for (auto& h : history)
        h.assign(static_cast<size_t>(historySize), 0.0f);
    reset();
}

void MonstaProcessor::reset() noexcept
{
    for (auto& h : history)
        std::fill(h.begin(), h.end(), 0.0f);
    writePos = 0;
    freeRunning = 0;
}

void MonstaProcessor::process(float* const* channels, int numChannels, int numSamples, const FxSlotParams& p, const Transport& transport, int blockOffset) noexcept
{
    static constexpr double kBeats[] { 1.0, 0.5, 0.25, 0.125 };
    constexpr int kFade = 64;
    const int division = std::clamp(static_cast<int>(std::lround(p.values[slot::kDivision])), 0, 3);
    const double bpm = std::clamp(transport.bpm, 20.0, 999.0);
    long long sliceLength = std::llround(kBeats[division] * 60.0 / bpm * sampleRate);
    sliceLength = std::clamp(sliceLength, static_cast<long long>(4 * kFade), static_cast<long long>((historySize - 4) / 2));
    const int crushBits = std::clamp(static_cast<int>(std::lround(p.values[slot::kCrushBits])), 0, 16);
    const float crushScale = crushBits > 0 ? static_cast<float>(1 << (crushBits - 1)) : 0.0f;
    const float gateDepth = std::clamp(p.values[slot::kGateDepth], 0.0f, 1.0f);
    const auto seed = static_cast<std::uint32_t>(std::max(0.0f, p.values[slot::kSeed]));

    long long position = freeRunning;
    if (transport.playing && transport.hasPosition)
        position = std::llround(transport.ppqAtBlockStart * 60.0 / bpm * sampleRate) + blockOffset;

    const auto historyAt = [&](int ch, long long ago)
    {
        long long index = writePos - ago;
        while (index < 0)
            index += historySize;
        return history[static_cast<size_t>(ch)][static_cast<size_t>(index % historySize)];
    };

    for (int i = 0; i < numSamples; ++i, ++position)
    {
        // History first: "ago" 1 is the previous sample.
        for (int ch = 0; ch < numChannels; ++ch)
            history[static_cast<size_t>(ch)][static_cast<size_t>(writePos)] = channels[ch][i];

        const long long slice = position >= 0 ? position / sliceLength : (position - sliceLength + 1) / sliceLength;
        const long long phase = position - slice * sliceLength;
        const auto action = actionFor(p, slice);
        if (action != Action::Pass)
        {
            long long ago = sliceLength;                                   // Repeat
            if (action == Action::Reverse)
                ago = 2 * phase + 1;
            else if (action == Action::Stutter)
                ago = phase + sliceLength - (phase % std::max(1LL, sliceLength / 4));
            float w = 1.0f;
            if (phase < kFade)
                w = 0.5f - 0.5f * std::cos(static_cast<float>(kPi) * static_cast<float>(phase) / kFade);
            else if (phase > sliceLength - kFade)
                w = 0.5f - 0.5f * std::cos(static_cast<float>(kPi) * static_cast<float>(sliceLength - phase) / kFade);
            float gateGain = 1.0f;
            if (gateDepth > 0.0f)
            {
                const auto step = static_cast<std::uint32_t>(4 * phase / sliceLength);
                if (((hash(seed + 7u, static_cast<std::uint32_t>(slice)) >> (8 + step)) & 1u) == 0u)
                    gateGain = 1.0f - gateDepth;
            }
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float processed = historyAt(ch, ago);
                if (crushScale > 0.0f)
                    processed = std::round(processed * crushScale) / crushScale;
                processed *= gateGain;
                channels[ch][i] = channels[ch][i] + w * (processed - channels[ch][i]);
            }
        }
        if (++writePos == historySize)
            writePos = 0;
    }
    freeRunning += numSamples;
}
} // namespace hpeq::dsp
