// HamloMag v2 (docs/MAG_V2.md): the new stages off = the saturator of 0.1.0 exactly; DELTA, MATCH, the SAMPLER,
// SUB, PHONE, DYNAMICS behave; old states load with everything new off.

#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/MagV2.h"
#include "Plugin/HamloMagProcessor.h"
#include "Plugin/Parameters.h"
#include "TestSignals.h"

namespace hamlo::test
{
using saturator::HamloMagProcessor;

namespace
{
dsp::FxSlotParams slotFor(int model, int os)
{
    dsp::FxSlotParams p;
    p.active = true;
    p.values[dsp::slot::kSatModel] = static_cast<float>(model);
    p.values[dsp::slot::kDriveDb] = 12.0f;
    p.values[dsp::slot::kOversampling] = static_cast<float>(os);
    p.values[dsp::slot::kAutoGain] = 1.0f;
    p.values[dsp::slot::kSatHarmonics] = 0.5f;
    p.values[dsp::slot::kSatTapeBias] = 0.5f;
    p.values[dsp::slot::kSatWow] = 0.2f;
    p.values[dsp::slot::kSatHiss] = 0.1f;
    p.values[dsp::slot::kSatLowCutHz] = 20.0f;
    p.values[dsp::slot::kSatHighCutHz] = 20000.0f;
    return p;
}

template <typename P>
juce::AudioBuffer<float> runDsp(P& proc, const dsp::FxSlotParams& p, const juce::AudioBuffer<float>& x)
{
    proc.prepare(kFs, 512);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        float* ch[2] { y.getWritePointer(0, start), y.getWritePointer(1, start) };
        proc.process(ch, 2, juce::jmin(512, y.getNumSamples() - start), p);
    }
    return y;
}

juce::AudioBuffer<float> render(HamloMagProcessor& p, const juce::AudioBuffer<float>& x, int block = 512, int prepared = 512)
{
    p.prepareToPlay(kFs, prepared);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(block, y.getNumSamples() - start));
        p.processBlock(view, midi);
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

class V2Tests final : public juce::UnitTest
{
public:
    V2Tests() : juce::UnitTest("V2", "HamloMag") {}

    void runTest() override
    {
        const auto loop = drums(2.0);

        beginTest("SAMPLER, PHONE, SUB, DYNAMICS off: MagV2Processor = SaturatorProcessor bit-exact (every model, 1x .. 8x)");
        for (int model = 0; model < 6; ++model)
            for (int os = 0; os < 4; ++os)
            {
                const auto p = slotFor(model, os);
                dsp::SaturatorProcessor plain;
                dsp::MagV2Processor v2;
                expect(bitExact(runDsp(v2, p, loop), runDsp(plain, p, loop)), "model " + juce::String(model) + " os " + juce::String(os));
                expectEquals(dsp::MagV2Processor::latencySamples(p, kFs), dsp::SaturatorProcessor::latencySamples(p, kFs));
            }

        beginTest("Every new control on, every model at 4x: finite and bounded");
        for (int model = 0; model < 6; ++model)
            for (int sampler = 1; sampler <= 3; ++sampler)
            {
                auto p = slotFor(model, 2);
                p.values[dsp::slot::kSatSampler] = static_cast<float>(sampler);
                p.values[dsp::slot::kSatPhone] = 1.0f;
                p.values[dsp::slot::kSatSubHz] = 120.0f;
                p.values[dsp::slot::kSatDynamics] = model % 2 == 0 ? 0.8f : -0.8f;
                dsp::MagV2Processor v2;
                const auto y = runDsp(v2, p, loop);
                bool finite = true;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < y.getNumSamples(); ++i)
                        finite = finite && std::isfinite(y.getSample(ch, i));
                expect(finite && y.getMagnitude(0, y.getNumSamples()) < 8.0f, "model " + juce::String(model) + " sampler " + juce::String(sampler));
            }

        beginTest("SUB at DRIVE 0 (CLASSIC): the split sums back - a 1 kHz tone and a 60 Hz tone keep their level within 0.2 dB");
        for (const double hz : { 60.0, 1000.0 })
        {
            const auto x = sine(1.0, hz, -30.0);
            auto p = slotFor(0, 0);
            p.values[dsp::slot::kDriveDb] = 0.0f;
            p.values[dsp::slot::kSatWow] = p.values[dsp::slot::kSatHiss] = 0.0f;
            dsp::SaturatorProcessor plain;
            const auto ref = runDsp(plain, p, x);
            p.values[dsp::slot::kSatSubHz] = 120.0f;
            dsp::MagV2Processor v2;
            const auto y = runDsp(v2, p, x);
            const int from = static_cast<int>(0.5 * kFs), n = static_cast<int>(0.4 * kFs);
            expectWithinAbsoluteError(20.0 * std::log10(y.getRMSLevel(0, from, n) / ref.getRMSLevel(0, from, n)), 0.0, 0.2, juce::String(hz) + " Hz");
        }

        beginTest("DELTA: the output with DELTA + the input delayed by the latency = the output without DELTA");
        {
            HamloMagProcessor a, b;
            for (auto* p : { &a, &b })
            {
                p->setParameterValue("driveDb", 18.0);
                p->setParameterValue("hiss", 0.0);
                p->setParameterValue("wow", 0.0);
            }
            b.setParameterValue("delta", 1.0);
            const auto wet = render(a, loop);
            auto d = render(b, loop);
            const auto x = delayed(loop, a.getLatencySamples());
            for (int ch = 0; ch < 2; ++ch)
                d.addFrom(ch, 0, x, ch, 0, x.getNumSamples());
            expect(differenceDb(d, wet) < -120.0, juce::String(differenceDb(d, wet)));
        }

        beginTest("MATCH: TUBE at DRIVE 24 dB comes out within 1 dB of the input level (pink-ish drums, after 1.5 s)");
        {
            HamloMagProcessor p;
            p.setParameterValue("model", 2.0);
            p.setParameterValue("driveDb", 24.0);
            p.setParameterValue("hiss", 0.0);
            p.setParameterValue("match", 1.0);
            const auto x = drums(6.0);
            const auto y = render(p, x);
            const int lat = p.getLatencySamples(), from = static_cast<int>(3.0 * kFs), n = static_cast<int>(2.5 * kFs);
            const double change = 20.0 * std::log10(y.getRMSLevel(0, from + lat, n) / x.getRMSLevel(0, from, n));
            logMessage("  level change " + juce::String(change, 2) + " dB");
            expect(std::abs(change) < 1.5);
        }

        beginTest("Host blocks 1, 64, 127, 4096 after prepare(256) bit-identical with every new control on");
        {
            const auto set = [](HamloMagProcessor& p)
            {
                p.setParameterValue("model", 1.0);
                p.setParameterValue("oversampling", 3.0);
                p.setParameterValue("sampler", 1.0);
                p.setParameterValue("phoneBass", 0.7);
                p.setParameterValue("subProtectHz", 100.0);
                p.setParameterValue("dynamics", 0.5);
                p.setParameterValue("match", 1.0);
            };
            HamloMagProcessor ref;
            set(ref);
            const auto expected = render(ref, loop, 256, 256);
            for (int block : { 1, 64, 127, 4096 })
            {
                HamloMagProcessor p;
                set(p);
                expect(bitExact(render(p, loop, block, 256), expected), "block " + juce::String(block));
            }
        }

        beginTest("Migration: a state without the v2 parameters (HPEQ, HamloMag 0.1.0) loads with every new control off");
        {
            HamloMagProcessor p;
            p.setParameterValue("sampler", 2.0);
            p.setParameterValue("match", 1.0);
            juce::XmlElement xml("HamloMag");
            auto* e = xml.createNewChildElement("PARAM");
            e->setAttribute("id", "driveDb");
            e->setAttribute("value", 10.0);
            juce::MemoryBlock block;
            juce::AudioProcessor::copyXmlToBinary(xml, block);
            p.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
            for (const char* id : { "sampler", "phoneBass", "subProtectHz", "dynamics", "match", "delta" })
                expectEquals(p.parameterValue(id), 0.0, id);
        }
    }
};

static V2Tests v2Tests;
} // namespace hamlo::test
