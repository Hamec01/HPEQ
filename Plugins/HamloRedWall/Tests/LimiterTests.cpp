// The limiter (LimiterProcessor + RedWallCore, as in HPEQ): the M6 path, the ceiling, true peak, LINK, OUTPUT and
// block sizes. Same laws as HPEQ's Tests/Unit/LimiterPassTests.cpp (1, 2, 3, 8, 10, 13).

#include <juce_core/juce_core.h>

#include "DSP/Limiter.h"
#include "TestSignals.h"

namespace hamlo::test
{
using dsp::LimiterProcessor;
using dsp::LimiterSettings;
namespace s = dsp::slot;

namespace
{
LimiterSettings settings(float threshold, float ceiling, int style, int truePeak, int autoRelease, float link = 1.0f, float outputDb = 0.0f)
{
    LimiterSettings p;
    p.values[s::kCeilingDb] = ceiling;
    p.values[s::kLimiterReleaseMs] = 60.0f;
    p.values[s::kLookaheadMs] = 2.0f;
    p.values[s::kLimThresholdDb] = threshold;
    p.values[s::kLimTruePeak] = static_cast<float>(truePeak);
    p.values[s::kLimAutoRelease] = static_cast<float>(autoRelease);
    p.values[s::kLimStyle] = static_cast<float>(style);
    p.values[s::kLimLink] = link;
    p.values[s::kLimOutputDb] = outputDb;
    return p;
}

juce::AudioBuffer<float> run(const LimiterSettings& p, const juce::AudioBuffer<float>& x, int block)
{
    LimiterProcessor l;
    l.prepare(kFs);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* ch[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        l.process(ch, 2, juce::jmin(block, y.getNumSamples() - start), p);
    }
    return y;
}

// The largest inter-sample peak (8x upsampling with a windowed sinc, 64 taps each side).
double truePeakDb(const juce::AudioBuffer<float>& x)
{
    double peak = 0.0;
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        for (int i = 64; i < x.getNumSamples() - 64; ++i)
            for (int k = 0; k < 8; ++k)
            {
                const double frac = k / 8.0;
                double v = 0.0;
                for (int j = -63; j <= 64; ++j)
                {
                    const double t = j - frac;
                    const double sinc = std::abs(t) < 1.0e-12 ? 1.0 : std::sin(juce::MathConstants<double>::pi * t) / (juce::MathConstants<double>::pi * t);
                    const double w = 0.5 + 0.5 * std::cos(juce::MathConstants<double>::pi * t / 64.5);
                    v += x.getSample(ch, i + j) * sinc * w;
                }
                peak = std::max(peak, std::abs(v));
            }
    return 20.0 * std::log10(peak + 1.0e-12);
}
} // namespace

class LimiterTests final : public juce::UnitTest
{
public:
    LimiterTests() : juce::UnitTest("Limiter", "HamloRedWall") {}

    void runTest() override
    {
        auto hot = drums(2.0);
        hot.applyGain(static_cast<float>(std::pow(10.0, 18.0 / 20.0))); // drums +18 dB

        beginTest("Every option at its default: the M6 limiter (usesRedWall false)");
        {
            LimiterSettings p;
            p.values[s::kCeilingDb] = -1.0f;
            p.values[s::kLimiterReleaseMs] = 60.0f;
            p.values[s::kLookaheadMs] = 2.0f;
            p.values[s::kLimLink] = 1.0f;
            expect(!LimiterProcessor::usesRedWall(p));
            expect(LimiterProcessor::usesRedWall(settings(-1.0f, -1.0f, 0, 0, 0)));
        }

        beginTest("Ceiling: output sample peak <= ceiling for every style, true peak, auto release, link (drums +18 dB)");
        for (int style = 0; style < 3; ++style)
            for (int truePeak = 0; truePeak < 2; ++truePeak)
                for (int autoRelease = 0; autoRelease < 2; ++autoRelease)
                    for (const float link : { 0.0f, 1.0f })
                    {
                        const auto y = run(settings(-3.0f, -1.0f, style, truePeak, autoRelease, link), hot, 256);
                        const double peak = 20.0 * std::log10(y.getMagnitude(0, y.getNumSamples()));
                        expect(peak <= -1.0 + 1.0e-4, "style " + juce::String(style) + " tp " + juce::String(truePeak) + ": " + juce::String(peak, 3));
                    }

        beginTest("True peak: with TRUE PEAK the inter-sample peak stays <= ceiling + 0.3 dB; without, the same signal goes over by > 0.5 dB");
        {
            // fs / 4 with a 45 degree phase: every sample lands at 0.707 of the waveform's peak, the classic
            // inter-sample over (first written with 0.97 fs / 4, whose overs were too small to show the difference)
            juce::AudioBuffer<float> x(2, static_cast<int>(kFs));
            for (int i = 0; i < x.getNumSamples(); ++i)
                for (int ch = 0; ch < 2; ++ch)
                    x.setSample(ch, i, static_cast<float>(1.5 * std::sin(juce::MathConstants<double>::pi * (0.5 * i + 0.25))));
            const double on = truePeakDb(run(settings(0.0f, -1.0f, 0, 1, 0), x, 256)), off = truePeakDb(run(settings(0.0f, -1.0f, 0, 0, 1), x, 256));
            logMessage("  true peak: on " + juce::String(on, 2) + ", off " + juce::String(off, 2) + " dBTP (ceiling -1)");
            expect(on <= -0.7 && off > -0.5);
        }

        beginTest("LINK 0: a left-only burst leaves the right channel exactly delayed; LINK 1 ducks it");
        {
            auto x = sine(1.0, 220.0, -12.0);
            for (int i = 12000; i < 24000; ++i)
                x.setSample(0, i, x.getSample(0, i) * 8.0f);
            const auto p0 = settings(0.0f, -6.0f, 0, 0, 0, 0.0f), p1 = settings(0.0f, -6.0f, 0, 0, 0, 1.0f);
            const int l = LimiterProcessor::lookaheadSamples(p0, kFs);
            const auto y0 = run(p0, x, 256), y1 = run(p1, x, 256);
            double right0 = 0.0, right1 = 0.0;
            for (int i = l; i < x.getNumSamples(); ++i)
            {
                right0 = std::max(right0, std::abs(static_cast<double>(y0.getSample(1, i)) - x.getSample(1, i - l)));
                right1 = std::max(right1, std::abs(static_cast<double>(y1.getSample(1, i)) - x.getSample(1, i - l)));
            }
            expect(right0 < 1.0e-6 && right1 > 0.01, juce::String(right0) + " / " + juce::String(right1));
        }

        beginTest("OUTPUT -6 dB: the 0 dB output x 10^(-6/20) (1e-6 relative); peak <= LIMIT + OUTPUT");
        {
            const auto a = run(settings(-6.0f, -1.0f, 1, 1, 1), hot, 256), b = run(settings(-6.0f, -1.0f, 1, 1, 1, 1.0f, -6.0f), hot, 256);
            const double g = std::pow(10.0, -6.0 / 20.0);
            double worst = 0.0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < a.getNumSamples(); ++i)
                    worst = std::max(worst, std::abs(b.getSample(ch, i) - g * a.getSample(ch, i)));
            expect(worst <= 1.0e-6 && 20.0 * std::log10(b.getMagnitude(0, b.getNumSamples())) <= -7.0 + 1.0e-4);
        }

        beginTest("Host blocks 1, 64, 512 bit-identical to 256 (every style, true peak on, auto release)");
        for (int style = 0; style < 3; ++style)
        {
            const auto p = settings(-6.0f, -1.0f, style, 1, 1);
            const auto expected = run(p, hot, 256);
            for (int block : { 1, 64, 512 })
                expect(bitExact(run(p, hot, block), expected), "style " + juce::String(style) + ", block " + juce::String(block));
        }
    }
};

static LimiterTests limiterTests;
} // namespace hamlo::test
