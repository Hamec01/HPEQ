#include "Harness/TestSignalFactory.h"

#include <cmath>

namespace hpeq::test
{
namespace
{
constexpr double kTwoPi = 6.283185307179586476925286766559;

int lengthFor(const SignalRequest& request)
{
    return juce::jmax(1, static_cast<int>(std::llround(request.durationSeconds * request.sampleRate)));
}

juce::AudioBuffer<float> blank(const SignalRequest& request)
{
    juce::AudioBuffer<float> buffer(juce::jmax(1, request.numChannels), lengthFor(request));
    buffer.clear();
    return buffer;
}

double dbToGain(double db) { return std::pow(10.0, db / 20.0); }

void copyChannelZeroToAll(juce::AudioBuffer<float>& buffer)
{
    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom(ch, 0, buffer, 0, 0, buffer.getNumSamples());
}

double rmsOf(const std::vector<double>& x)
{
    double sum = 0.0;
    for (double v : x)
        sum += v * v;
    return x.empty() ? 0.0 : std::sqrt(sum / static_cast<double>(x.size()));
}

std::vector<double> whiteSequence(int length, std::uint64_t seed)
{
    DeterministicRandom random(seed);
    std::vector<double> x(static_cast<size_t>(length));
    for (auto& v : x)
        v = random.nextBipolar();
    return x;
}

// Paul Kellett's refined pink filter: within ~0.05 dB of -3 dB/oct over the audio band at 44.1k+.
std::vector<double> pinkSequence(int length, std::uint64_t seed)
{
    auto x = whiteSequence(length, seed);
    double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    for (auto& v : x)
    {
        const double white = v;
        b0 = 0.99886 * b0 + white * 0.0555179;
        b1 = 0.99332 * b1 + white * 0.0750759;
        b2 = 0.96900 * b2 + white * 0.1538520;
        b3 = 0.86650 * b3 + white * 0.3104856;
        b4 = 0.55000 * b4 + white * 0.5329522;
        b5 = -0.7616 * b5 - white * 0.0168980;
        v = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362;
        b6 = white * 0.115926;
    }
    return x;
}

void writeNormalisedRms(juce::AudioBuffer<float>& buffer, int channel, const std::vector<double>& x, double rmsDbfs)
{
    const double scale = dbToGain(rmsDbfs) / juce::jmax(1.0e-12, rmsOf(x));
    auto* out = buffer.getWritePointer(channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        out[i] = static_cast<float>(x[static_cast<size_t>(i)] * scale);
}

double fadeGain(int index, int length, int fadeLength)
{
    if (fadeLength <= 0)
        return 1.0;
    const auto edge = juce::jmin(index, length - 1 - index);
    if (edge >= fadeLength)
        return 1.0;
    return 0.5 - 0.5 * std::cos(juce::MathConstants<double>::pi * edge / fadeLength);
}
} // namespace

double maxValidFrequency(double sampleRate) noexcept { return 0.45 * sampleRate; }

bool isFrequencyValid(double frequencyHz, double sampleRate) noexcept
{
    return frequencyHz > 0.0 && frequencyHz <= maxValidFrequency(sampleRate);
}

std::uint64_t DeterministicRandom::nextU64() noexcept
{
    std::uint64_t z = (state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

double DeterministicRandom::nextBipolar() noexcept
{
    // 53 random mantissa bits -> [0,1) -> [-1,1)
    const double unit = static_cast<double>(nextU64() >> 11) * (1.0 / 9007199254740992.0);
    return unit * 2.0 - 1.0;
}

juce::AudioBuffer<float> makeSilence(const SignalRequest& request) { return blank(request); }

juce::AudioBuffer<float> makeDc(const SignalRequest& request, float level)
{
    auto buffer = blank(request);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::fill(buffer.getWritePointer(ch), level, buffer.getNumSamples());
    return buffer;
}

juce::AudioBuffer<float> makeSubnormal(const SignalRequest& request)
{
    // Alternating-sign values well inside the float subnormal range (< 1.17549435e-38).
    auto buffer = blank(request);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            out[i] = (i % 2 == 0 ? 1.0f : -1.0f) * 1.0e-40f;
    }
    return buffer;
}

juce::AudioBuffer<float> makeImpulse(const SignalRequest& request, int positionSamples, float amplitude)
{
    auto buffer = blank(request);
    const int index = juce::jlimit(0, buffer.getNumSamples() - 1, positionSamples);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.setSample(ch, index, amplitude);
    return buffer;
}

juce::AudioBuffer<float> makeStep(const SignalRequest& request, int positionSamples, float amplitude)
{
    auto buffer = blank(request);
    const int start = juce::jlimit(0, buffer.getNumSamples(), positionSamples);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::fill(buffer.getWritePointer(ch) + start, amplitude, buffer.getNumSamples() - start);
    return buffer;
}

juce::AudioBuffer<float> makeSine(const SignalRequest& request, double frequencyHz, double peakDbfs)
{
    auto buffer = blank(request);
    if (!isFrequencyValid(frequencyHz, request.sampleRate))
        return buffer;
    const double amplitude = dbToGain(peakDbfs);
    auto* out = buffer.getWritePointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        out[i] = static_cast<float>(amplitude * std::sin(kTwoPi * frequencyHz * i / request.sampleRate));
    copyChannelZeroToAll(buffer);
    return buffer;
}

juce::AudioBuffer<float> makeExpSweep(const SignalRequest& request, double f0, double f1, double peakDbfs)
{
    auto buffer = blank(request);
    f1 = juce::jmin(f1, maxValidFrequency(request.sampleRate));
    const int length = buffer.getNumSamples();
    const double duration = length / request.sampleRate;
    const double k = std::log(f1 / f0);
    const double amplitude = dbToGain(peakDbfs);
    const int fade = static_cast<int>(0.005 * request.sampleRate);
    auto* out = buffer.getWritePointer(0);
    for (int i = 0; i < length; ++i)
    {
        const double t = i / request.sampleRate;
        const double phase = kTwoPi * f0 * duration / k * (std::exp(t * k / duration) - 1.0);
        out[i] = static_cast<float>(amplitude * fadeGain(i, length, fade) * std::sin(phase));
    }
    copyChannelZeroToAll(buffer);
    return buffer;
}

juce::AudioBuffer<float> makeWhiteNoise(const SignalRequest& request, double rmsDbfs)
{
    auto buffer = blank(request);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        writeNormalisedRms(buffer, ch, whiteSequence(buffer.getNumSamples(), request.seed + 101u * static_cast<unsigned>(ch)), rmsDbfs);
    return buffer;
}

juce::AudioBuffer<float> makePinkNoise(const SignalRequest& request, double rmsDbfs)
{
    auto buffer = blank(request);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        writeNormalisedRms(buffer, ch, pinkSequence(buffer.getNumSamples(), request.seed + 202u * static_cast<unsigned>(ch)), rmsDbfs);
    return buffer;
}

juce::AudioBuffer<float> makeDualToneImd(const SignalRequest& request, double peakDbfs)
{
    auto buffer = blank(request);
    const double peak = dbToGain(peakDbfs);
    const double lowAmp = peak * 0.8;  // 4:1 -> 0.8 + 0.2 = peak
    const double highAmp = peak * 0.2;
    const double highHz = juce::jmin(7000.0, maxValidFrequency(request.sampleRate));
    auto* out = buffer.getWritePointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const double t = i / request.sampleRate;
        out[i] = static_cast<float>(lowAmp * std::sin(kTwoPi * 60.0 * t) + highAmp * std::sin(kTwoPi * highHz * t));
    }
    copyChannelZeroToAll(buffer);
    return buffer;
}

juce::AudioBuffer<float> makeGatedBurst(const SignalRequest& request, double loudDbfs, double quietDbfs, double segmentSeconds)
{
    auto buffer = blank(request);
    const int segment = juce::jmax(1, static_cast<int>(std::llround(segmentSeconds * request.sampleRate)));
    const double loud = dbToGain(loudDbfs);
    const double quiet = dbToGain(quietDbfs);
    auto* out = buffer.getWritePointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // Starts quiet so the detector settles before the first loud step.
        const double amplitude = ((i / segment) % 2 == 0) ? quiet : loud;
        out[i] = static_cast<float>(amplitude * std::sin(kTwoPi * 1000.0 * i / request.sampleRate));
    }
    copyChannelZeroToAll(buffer);
    return buffer;
}

juce::AudioBuffer<float> makeStereoPlacement(const SignalRequest& request, StereoPlacement placement, double rmsDbfs)
{
    SignalRequest stereo = request;
    stereo.numChannels = 2;
    auto buffer = blank(stereo);
    const auto source = pinkSequence(buffer.getNumSamples(), request.seed + 303u);
    writeNormalisedRms(buffer, 0, source, rmsDbfs);
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float s = left[i];
        switch (placement)
        {
            case StereoPlacement::LeftOnly:  left[i] = s;    right[i] = 0.0f; break;
            case StereoPlacement::RightOnly: left[i] = 0.0f; right[i] = s;    break;
            case StereoPlacement::MidOnly:   left[i] = s;    right[i] = s;    break;
            case StereoPlacement::SideOnly:  left[i] = s;    right[i] = -s;   break;
        }
    }
    return buffer;
}

juce::AudioBuffer<float> makeSyntheticDrums(const SignalRequest& request)
{
    auto buffer = blank(request);
    const double fs = request.sampleRate;
    const int length = buffer.getNumSamples();
    const int sixteenth = static_cast<int>(std::llround(fs * 60.0 / 120.0 / 4.0));
    std::vector<double> mix(static_cast<size_t>(length), 0.0);
    DeterministicRandom random(request.seed + 404u);

    const auto addKick = [&](int start)
    {
        double phase = 0.0;
        for (int i = 0; start + i < length && i < static_cast<int>(0.45 * fs); ++i)
        {
            const double t = i / fs;
            const double freq = 45.0 + 75.0 * std::exp(-t / 0.035);
            phase += kTwoPi * freq / fs;
            mix[static_cast<size_t>(start + i)] += 0.9 * std::exp(-t / 0.18) * std::sin(phase);
        }
    };
    const auto addSnare = [&](int start)
    {
        for (int i = 0; start + i < length && i < static_cast<int>(0.25 * fs); ++i)
        {
            const double t = i / fs;
            const double body = 0.35 * std::exp(-t / 0.05) * std::sin(kTwoPi * 185.0 * t);
            const double noise = 0.45 * std::exp(-t / 0.07) * random.nextBipolar();
            mix[static_cast<size_t>(start + i)] += body + noise;
        }
    };
    const auto addHat = [&](int start)
    {
        double previous = 0.0;
        for (int i = 0; start + i < length && i < static_cast<int>(0.05 * fs); ++i)
        {
            const double t = i / fs;
            const double white = random.nextBipolar();
            const double highPassed = white - previous; // first difference: +6 dB/oct tilt
            previous = white;
            mix[static_cast<size_t>(start + i)] += 0.12 * std::exp(-t / 0.012) * highPassed;
        }
    };

    for (int step = 0; step * sixteenth < length; ++step)
    {
        const int start = step * sixteenth;
        const int inBar = step % 16;
        if (inBar == 0 || inBar == 10)
            addKick(start);
        if (inBar == 4 || inBar == 12)
            addSnare(start);
        if (inBar % 2 == 0)
            addHat(start);
    }

    auto* out = buffer.getWritePointer(0);
    for (int i = 0; i < length; ++i)
        out[i] = static_cast<float>(juce::jlimit(-1.0, 1.0, mix[static_cast<size_t>(i)] * 0.7));
    copyChannelZeroToAll(buffer);
    return buffer;
}

const std::vector<double>& fixtureSineFrequencies()
{
    static const std::vector<double> frequencies { 20, 50, 80, 100, 200, 450, 700, 1000, 5000, 10000, 15000 };
    return frequencies;
}

std::vector<Fixture> makeFixtureSet(const SignalRequest& request)
{
    std::vector<Fixture> set;
    const auto add = [&](std::string name, std::string description, juce::AudioBuffer<float> audio, bool valid = true)
    {
        set.push_back({ std::move(name), std::move(description), valid, std::move(audio) });
    };

    const int quarter = static_cast<int>(request.sampleRate * request.durationSeconds * 0.25);
    add("silence", "all zeros", makeSilence(request));
    add("dc", "constant +0.5", makeDc(request, 0.5f));
    add("subnormal", "alternating +/-1e-40 (float subnormal)", makeSubnormal(request));
    add("impulse", "unit impulse at 25% of length", makeImpulse(request, quarter, 1.0f));
    add("step", "0 -> 0.5 step at 25% of length", makeStep(request, quarter, 0.5f));
    for (double f : fixtureSineFrequencies())
    {
        const bool valid = isFrequencyValid(f, request.sampleRate);
        add("sine_" + std::to_string(static_cast<int>(f)), "sine -6 dBFS peak", makeSine(request, f, -6.0), valid);
    }
    add("exp_sweep", "20 Hz -> 0.45 fs exponential sweep, -6 dBFS peak", makeExpSweep(request, 20.0, maxValidFrequency(request.sampleRate), -6.0));
    add("white_noise", "white, -20 dBFS RMS per channel, decorrelated", makeWhiteNoise(request, -20.0));
    add("pink_noise", "pink (Kellett), -20 dBFS RMS per channel, decorrelated", makePinkNoise(request, -20.0));
    add("dual_tone_imd", "SMPTE 60 Hz + 7 kHz 4:1, -6 dBFS peak", makeDualToneImd(request, -6.0));
    add("gated_burst", "1 kHz, -40 / -6 dBFS alternating every 250 ms", makeGatedBurst(request, -6.0, -40.0, 0.25));
    add("stereo_left_only", "pink -20 dBFS RMS in L, R silent", makeStereoPlacement(request, StereoPlacement::LeftOnly, -20.0));
    add("stereo_right_only", "pink -20 dBFS RMS in R, L silent", makeStereoPlacement(request, StereoPlacement::RightOnly, -20.0));
    add("stereo_mid_only", "pink, L == R", makeStereoPlacement(request, StereoPlacement::MidOnly, -20.0));
    add("stereo_side_only", "pink, L == -R", makeStereoPlacement(request, StereoPlacement::SideOnly, -20.0));
    add("synthetic_drums", "kick/snare/hat 16th loop at 120 BPM", makeSyntheticDrums(request));
    return set;
}
} // namespace hpeq::test
