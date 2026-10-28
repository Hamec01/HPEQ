// The transient shaper (TransientProcessor + UdarnikCore, as in HPEQ): identities, block sizes, LISTEN, OUTPUT.
// Same laws as HPEQ's Tests/Unit/TransientPassTests.cpp (T1, T2, T8, T10).

#include <juce_core/juce_core.h>

#include "DSP/TransientShaper.h"
#include "TestSignals.h"

namespace hamlo::test
{
using dsp::TransientProcessor;
using dsp::TransientSettings;
namespace s = dsp::slot;

namespace
{
TransientSettings settings(float snap, float body, int style, int lookahead, float preserve = 1.0f, int listen = 0, float outputDb = 0.0f)
{
    TransientSettings p;
    p.values[s::kAttackGainDb] = snap;
    p.values[s::kSustainGainDb] = body;
    p.values[s::kTransientSpeed] = 0.5f;
    p.values[s::kTrStyle] = static_cast<float>(style);
    p.values[s::kTrLookahead] = static_cast<float>(lookahead);
    p.values[s::kTrPreserve] = preserve;
    p.values[s::kTrListen] = static_cast<float>(listen);
    p.values[s::kTrOutputDb] = outputDb;
    return p;
}

juce::AudioBuffer<float> run(const TransientSettings& p, const juce::AudioBuffer<float>& x, int block)
{
    TransientProcessor t;
    t.prepare(kFs);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* ch[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        t.process(ch, 2, juce::jmin(block, y.getNumSamples() - start), p);
    }
    return y;
}

juce::AudioBuffer<float> delayed(const juce::AudioBuffer<float>& x, int samples)
{
    juce::AudioBuffer<float> y(x.getNumChannels(), x.getNumSamples());
    y.clear();
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, samples, x, ch, 0, x.getNumSamples() - samples);
    return y;
}
} // namespace

class ShaperTests final : public juce::UnitTest
{
public:
    ShaperTests() : juce::UnitTest("Transient shaper", "HamloUdarnik") {}

    void runTest() override
    {
        const auto loop = drums(2.0);

        beginTest("No option set (the M6 law): SNAP = BODY = 0 is an exact identity, latency 0");
        {
            TransientSettings p; // every value 0
            expect(!TransientProcessor::usesUdarnik(p));
            expectEquals(TransientProcessor::lookaheadSamples(p, kFs), 0);
            expect(bitExact(run(p, loop, 256), loop));
        }

        beginTest("Neutral settings in every mode and look-ahead: output = input delayed by the look-ahead, bit-exact");
        for (int style = 0; style < 3; ++style)
            for (int lookahead = 0; lookahead < 3; ++lookahead)
            {
                auto p = settings(0.0f, 0.0f, style, lookahead, 0.0f);
                p.values[s::kTrRiseLog2] = 0.0f;
                const int l = TransientProcessor::lookaheadSamples(p, kFs);
                if (style == 0 && lookahead == 0)
                    continue; // no option set: the M6 path (above)
                expect(bitExact(run(p, loop, 256), delayed(loop, l)), "style " + juce::String(style) + ", look-ahead " + juce::String(lookahead));
            }

        beginTest("Look-ahead: SPUTNIK 0 / VOSTOK 2 ms / SOYUZ 5 ms");
        {
            expectEquals(TransientProcessor::lookaheadSamples(settings(6, 0, 1, 1), kFs), 96);
            expectEquals(TransientProcessor::lookaheadSamples(settings(6, 0, 1, 2), kFs), 240);
        }

        beginTest("Host blocks 1, 64, 512 bit-identical to 256 (every mode, SNAP +6, BODY -3, SOYUZ)");
        for (int style = 0; style < 3; ++style)
        {
            const auto p = settings(6.0f, -3.0f, style, 2);
            const auto expected = run(p, loop, 256);
            for (int block : { 1, 64, 512 })
                expect(bitExact(run(p, loop, block), expected), "style " + juce::String(style) + ", block " + juce::String(block));
        }

        beginTest("LISTEN: ATTACK + BODY = FULL within 1e-6, every mode");
        for (int style = 0; style < 3; ++style)
        {
            const auto full = run(settings(6.0f, -3.0f, style, 1, 1.0f, 0), loop, 256);
            const auto attack = run(settings(6.0f, -3.0f, style, 1, 1.0f, 1), loop, 256);
            const auto body = run(settings(6.0f, -3.0f, style, 1, 1.0f, 2), loop, 256);
            double worst = 0.0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < full.getNumSamples(); ++i)
                    worst = std::max(worst, std::abs(static_cast<double>(attack.getSample(ch, i)) + body.getSample(ch, i) - full.getSample(ch, i)));
            expect(worst <= 1.0e-6, "style " + juce::String(style) + ": " + juce::String(worst));
        }

        beginTest("OUTPUT -6 / +6 dB = the output x 10^(O / 20) (1e-6 relative)");
        for (const float o : { -6.0f, 6.0f })
        {
            const auto ref = run(settings(6.0f, -3.0f, 1, 1), loop, 256);
            const auto y = run(settings(6.0f, -3.0f, 1, 1, 1.0f, 0, o), loop, 256);
            const double g = std::pow(10.0, o / 20.0);
            double worst = 0.0, peak = 0.0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < y.getNumSamples(); ++i)
                {
                    worst = std::max(worst, std::abs(y.getSample(ch, i) - g * ref.getSample(ch, i)));
                    peak = std::max(peak, std::abs(g * ref.getSample(ch, i)));
                }
            expect(worst <= 1.0e-6 * peak + 1.0e-9, juce::String(o) + " dB: " + juce::String(worst / peak));
        }

        beginTest("SNAP +12 dB (PRAVDA) raises the hits; SNAP -12 dB lowers them (peak over the loop, +- 1 dB)");
        {
            const auto up = run(settings(12.0f, 0.0f, 0, 1), loop, 256), down = run(settings(-12.0f, 0.0f, 0, 1), loop, 256);
            const auto peakDb = [](const juce::AudioBuffer<float>& b) { return 20.0 * std::log10(b.getMagnitude(0, b.getNumSamples())); };
            logMessage("  peak: input " + juce::String(peakDb(loop), 2) + ", SNAP +12 " + juce::String(peakDb(up), 2) + ", SNAP -12 " + juce::String(peakDb(down), 2) + " dBFS");
            expect(peakDb(up) > peakDb(loop) + 1.0 && peakDb(down) < peakDb(loop) - 1.0);
        }
    }
};

static ShaperTests shaperTests;
} // namespace hamlo::test
