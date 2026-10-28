// The saturator copied from HPEQ (DSP/Saturator.h, DSP/HamloMag.h): the CLASSIC law, the models, latency, blocks.

#include <cmath>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/Saturator.h"
#include "TestSignals.h"

namespace hamlo::test
{
namespace
{
dsp::FxSlotParams settings(int model, float driveDb, int oversampling)
{
    dsp::FxSlotParams p;
    p.active = true;
    p.values[dsp::slot::kSatModel] = static_cast<float>(model);
    p.values[dsp::slot::kDriveDb] = driveDb;
    p.values[dsp::slot::kOversampling] = static_cast<float>(oversampling);
    p.values[dsp::slot::kAutoGain] = 1.0f;
    p.values[dsp::slot::kSatHarmonics] = 0.5f;
    p.values[dsp::slot::kSatTapeBias] = 0.5f;
    p.values[dsp::slot::kSatLowCutHz] = 20.0f;
    p.values[dsp::slot::kSatHighCutHz] = 20000.0f;
    return p;
}

juce::AudioBuffer<float> run(const dsp::FxSlotParams& p, const juce::AudioBuffer<float>& x, int block, int prepared = 512)
{
    dsp::SaturatorProcessor s;
    s.prepare(kFs, prepared);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        float* ch[2] { y.getWritePointer(0, start), y.getWritePointer(1, start) };
        s.process(ch, 2, n, p);
    }
    return y;
}
} // namespace

class SaturatorTests final : public juce::UnitTest
{
public:
    SaturatorTests() : juce::UnitTest("Saturator", "HamloMag") {}

    void runTest() override
    {
        const auto loop = drums(2.0);

        beginTest("CLASSIC (model 0), 1x: y = DC-block((tanh(g x) - tanh(0)) / g) with auto gain - HPEQ's M6 law");
        {
            auto p = settings(0, 12.0f, 0);
            const auto x = sine(0.5, 220.0, -12.0);
            const auto y = run(p, x, 512);
            const double g = std::pow(10.0, 12.0 / 20.0), r = 1.0 - 2.0 * 3.14159265358979323846 * 10.0 / kFs;
            double x1 = 0.0, y1 = 0.0, worst = 0.0;
            for (int i = 0; i < x.getNumSamples(); ++i)
            {
                const double shaped = static_cast<double>(static_cast<float>(std::tanh(static_cast<float>(g) * x.getSample(0, i)) * (1.0f / static_cast<float>(g))));
                const double out = shaped - x1 + r * y1;
                x1 = shaped;
                y1 = out;
                worst = std::max(worst, std::abs(out - y.getSample(0, i)));
            }
            logMessage("  worst difference " + juce::String(worst, 9));
            expect(worst < 1.0e-5);
        }

        beginTest("Every model and oversampling: finite, bounded, and not the input (it colours)");
        for (int model = 0; model < 6; ++model)
            for (int os = 0; os < 3; ++os)
            {
                const auto y = run(settings(model, 18.0f, os), loop, 512);
                bool finite = true;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < y.getNumSamples(); ++i)
                        finite = finite && std::isfinite(y.getSample(ch, i));
                expect(finite && y.getMagnitude(0, y.getNumSamples()) < 8.0f, "model " + juce::String(model) + " os " + juce::String(os));
                expect(!bitExact(y, loop));
            }

        beginTest("Latency: 1x 0, 2x / 4x the measured oversampler delay; TAPE / VINYL add the 4 ms wow delay");
        {
            expectEquals(dsp::SaturatorProcessor::latencySamples(0), 0);
            expect(dsp::SaturatorProcessor::latencySamples(1) > 0 && dsp::SaturatorProcessor::latencySamples(2) > dsp::SaturatorProcessor::latencySamples(1));
            auto tape = settings(1, 6.0f, 0);
            tape.values[dsp::slot::kSatWow] = 0.5f;
            expect(dsp::SaturatorProcessor::latencySamples(tape, kFs) >= 192);
            expectEquals(dsp::SaturatorProcessor::latencySamples(settings(2, 6.0f, 0), kFs), dsp::MagProcessor::latencySamples(settings(2, 6.0f, 0), kFs));
        }

        beginTest("Blocks 1, 64, 127 and 4096 (after prepare(512)) bit-identical to 512, every model at 4x");
        for (int model = 0; model < 6; ++model)
        {
            auto p = settings(model, 12.0f, 2);
            p.values[dsp::slot::kSatWow] = 0.3f;
            p.values[dsp::slot::kSatHiss] = 0.2f;
            const auto expected = run(p, loop, 512);
            for (int block : { 1, 64, 127, 4096 })
                expect(bitExact(run(p, loop, block), expected), "model " + juce::String(model) + " block " + juce::String(block));
        }
    }
};

static SaturatorTests saturatorTests;
} // namespace hamlo::test
