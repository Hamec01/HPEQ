// The compressor's own behaviour (ported from HPEQ Tests/Unit/CompressorPassTests.cpp, same thresholds).

#include <cstring>
#include <vector>

#include <juce_core/juce_core.h>

#include "DSP/Compressor.h"
#include "TestSignals.h"

namespace hamlo::test
{
using dsp::CompressorProcessor;
using dsp::CompressorSettings;

namespace
{
CompressorSettings comp(float threshold, float ratio, float knee, float attack, float release, float detector = 0, float character = 0, float autoMakeup = 0)
{
    CompressorSettings s;
    s.thresholdDb = threshold;
    s.ratio = ratio;
    s.kneeDb = knee;
    s.attackMs = attack;
    s.releaseMs = release;
    s.makeupDb = 0.0f;
    s.detector = detector;
    s.character = character;
    s.autoMakeup = autoMakeup;
    return s;
}

juce::AudioBuffer<float> run(CompressorProcessor& c, const juce::AudioBuffer<float>& x, const CompressorSettings& p, int block = 256,
                             std::vector<float>* inputDb = nullptr)
{
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* ch[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        c.process(ch, 2, juce::jmin(block, y.getNumSamples() - start), p, true);
        if (inputDb != nullptr)
            inputDb->push_back(c.getInputLevelDb());
    }
    return y;
}
} // namespace

class CompressorTests final : public juce::UnitTest
{
public:
    CompressorTests() : juce::UnitTest("Compressor", "HamloComp") {}

    void runTest() override
    {
        const auto loop = drums(3.0);

        // First written with a sine, which a peak detector does not hold between peaks (measured -17.04 vs
        // -18.00 dB); a square has a constant |x|, so Peak must follow the static law exactly.
        beginTest("Static law: a steady square above threshold is reduced by the law within 0.2 dB (Peak, hard knee)");
        {
            auto x = sine(2.0, 997.0, 0.0);
            for (int ch = 0; ch < x.getNumChannels(); ++ch)
                for (int i = 0; i < x.getNumSamples(); ++i)
                    x.setSample(ch, i, x.getSample(ch, i) >= 0.0f ? 0.5f : -0.5f); // -6.02 dBFS
            const double levelDb = 20.0 * std::log10(0.5);
            CompressorProcessor c;
            c.prepare(kFs);
            const auto y = run(c, x, comp(-30, 4, 0, 5, 80));
            const double gainDb = rmsDb(segment(y, 48000, 48000)) - rmsDb(segment(x, 48000, 48000));
            const double expected = -CompressorProcessor::staticGainReductionDb(levelDb, -30, 4, 0);
            logMessage("  measured " + juce::String(gainDb, 2) + " dB, law " + juce::String(expected, 2) + " dB");
            expect(std::abs(gainDb - expected) <= 0.2);
        }

        beginTest("Below the knee with 0 dB makeup: an exact identity");
        {
            const auto x = sine(1.0, 997.0, -50.0);
            for (float character : { 0.0f, 1.0f, 2.0f })
            {
                CompressorProcessor c;
                c.prepare(kFs);
                expect(bitExact(run(c, x, comp(-30, 4, 6, 5, 80, 0, character)), x), "character " + juce::String(character));
            }
        }

        beginTest("Detector values round: 0.2 is Peak (bit-exact with 0)");
        {
            CompressorProcessor a, b;
            a.prepare(kFs);
            b.prepare(kFs);
            auto p = comp(-30, 4, 6, 5, 80);
            const auto ya = run(a, loop, p);
            p.detector = 0.2f;
            expect(bitExact(run(b, loop, p), ya));
        }

        beginTest("Input meter: a steady -20 dBFS sine reads -20.0 (Peak) and -23.0 (RMS) +-0.1 dB");
        {
            const auto x = sine(1.0, 997.0, -20.0);
            for (float detector : { 0.0f, 1.0f })
            {
                CompressorProcessor c;
                c.prepare(kFs);
                std::vector<float> levels;
                run(c, x, comp(-60, 1, 0, 5, 80, detector), 256, &levels);
                const float reading = levels.back();
                logMessage(juce::String(detector > 0.5f ? "  RMS" : "  Peak") + " reads " + juce::String(reading, 2) + " dB");
                expect(std::abs(reading - (detector > 0.5f ? -23.01f : -20.0f)) <= 0.1f);
            }
        }

        beginTest("RMS detector: steady sine compresses by the static law at its RMS level, +-0.2 dB");
        {
            const auto x = sine(2.0, 997.0, -6.0);
            CompressorProcessor c;
            c.prepare(kFs);
            const auto y = run(c, x, comp(-30, 4, 0, 5, 80, 1));
            const double gainDb = rmsDb(segment(y, 48000, 48000)) - rmsDb(segment(x, 48000, 48000));
            const double expected = -CompressorProcessor::staticGainReductionDb(-6.0 - 3.0103, -30, 4, 0);
            logMessage("  measured " + juce::String(gainDb, 2) + " dB, law " + juce::String(expected, 2) + " dB");
            expect(std::abs(gainDb - expected) <= 0.2);
        }

        beginTest("Auto makeup: on steady compression the long-term output level returns to the input within 0.5 dB after 10 s");
        {
            const auto x = sine(12.0, 220.0, -10.0);
            CompressorProcessor c;
            c.prepare(kFs);
            const auto y = run(c, x, comp(-30, 4, 6, 5, 80, 0, 0, 1));
            const double diff = rmsDb(segment(y, static_cast<int>(10.0 * kFs), static_cast<int>(kFs))) - rmsDb(segment(x, static_cast<int>(10.0 * kFs), static_cast<int>(kFs)));
            logMessage("  level change after 10 s: " + juce::String(diff, 2) + " dB");
            expect(std::abs(diff) <= 0.5);
        }

        beginTest("Punch: with gain reduction it adds a 3rd harmonic >= 20 dB above Clean's");
        {
            const auto loud = sine(1.0, 997.0, -6.0);
            const auto harmonicDb = [&](float character)
            {
                CompressorProcessor k;
                k.prepare(kFs);
                const auto y = run(k, loud, comp(-30, 4, 6, 5, 80, 0, character));
                double f1r = 0, f1i = 0, f3r = 0, f3i = 0;
                for (int i = 24000; i < 48000; ++i)
                {
                    const double v = y.getSample(0, i), t = 2.0 * juce::MathConstants<double>::pi * 997.0 * i / kFs;
                    f1r += v * std::cos(t); f1i += v * std::sin(t);
                    f3r += v * std::cos(3 * t); f3i += v * std::sin(3 * t);
                }
                return 10.0 * std::log10((f3r * f3r + f3i * f3i + 1e-30) / (f1r * f1r + f1i * f1i));
            };
            const double clean = harmonicDb(0), punch = harmonicDb(1);
            logMessage("  3rd harmonic: Clean " + juce::String(clean, 1) + " dB, Punch " + juce::String(punch, 1) + " dB");
            expect(punch > clean + 20.0);
        }

        beginTest("Glue: after a short peak it recovers faster than Clean; after a long compression slower");
        {
            const auto recovery = [&](double burstSeconds, float character)
            {
                const int burst = static_cast<int>(burstSeconds * kFs), n = burst + static_cast<int>(0.5 * kFs);
                juce::AudioBuffer<float> x(2, n);
                for (int i = 0; i < n; ++i)
                {
                    const float a = i < burst ? 0.5f : 0.001f;
                    const float v = a * static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * 997.0 * i / kFs));
                    x.setSample(0, i, v);
                    x.setSample(1, i, v);
                }
                CompressorProcessor c;
                c.prepare(kFs);
                const auto p = comp(-30, 4, 0, 1, 100, 0, character);
                auto y = x;
                const int until = burst + static_cast<int>(0.15 * kFs);
                for (int start = 0; start < until; start += 64)
                {
                    float* ch[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    c.process(ch, 2, juce::jmin(64, until - start), p, true);
                }
                return c.getGainReductionDb();
            };
            const float cleanShort = recovery(0.03, 0), glueShort = recovery(0.03, 2);
            const float cleanLong = recovery(3.0, 0), glueLong = recovery(3.0, 2);
            logMessage("  GR 150 ms after: short Clean " + juce::String(cleanShort, 2) + " / Glue " + juce::String(glueShort, 2)
                       + " dB; long Clean " + juce::String(cleanLong, 2) + " / Glue " + juce::String(glueLong, 2) + " dB");
            expect(glueShort < cleanShort);
            expect(glueLong > cleanLong);
        }

        beginTest("Block size does not change the result: 1, 64, 127, 256, 512 bit-identical (every option on)");
        {
            const auto p = comp(-28, 6, 8, 3, 120, 1, 2, 1);
            CompressorProcessor ref;
            ref.prepare(kFs);
            const auto expected = run(ref, loop, p, 256);
            for (int block : { 1, 64, 127, 512 })
            {
                CompressorProcessor c;
                c.prepare(kFs);
                expect(bitExact(run(c, loop, p, block), expected), "block " + juce::String(block));
            }
        }
    }
};

static CompressorTests compressorTests;
} // namespace hamlo::test
