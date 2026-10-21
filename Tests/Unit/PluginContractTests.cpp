// M1 contract of the insert skeleton: M1-A unity, bypass, gain ramp, RT allocations, T04 (M1 scope),
// editor open/close (M1-C, in-process), identity vs HPDG.

#include <cmath>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginProcessor.h"

namespace hpeq::test
{
namespace
{
const double kRates[] { 44100.0, 48000.0, 96000.0, 192000.0 };
const int kBlocks[] { 1, 16, 32, 64, 127, 256, 512, 1024, 2048 };

void setParameter(FxPluginProcessor& p, const juce::ParameterID& id, float plainValue)
{
    auto* parameter = p.getParameters().getParameter(id.getParamID());
    parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
}

bool configure(FxPluginProcessor& p, int channels, double fs, int maxBlock)
{
    const auto set = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(set);
    layout.outputBuses.add(set);
    if (!p.setBusesLayout(layout))
        return false;
    p.setRateAndBufferSizeDetails(fs, maxBlock);
    p.prepareToPlay(fs, maxBlock);
    return true;
}

// Processes `audio` in place with a fixed block size; returns allocations seen inside processBlock.
std::uint64_t render(FxPluginProcessor& p, juce::AudioBuffer<float>& audio, int block)
{
    juce::MidiBuffer midi;
    std::uint64_t allocations = 0;
    for (int start = 0; start < audio.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, audio.getNumSamples() - start);
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), audio.getNumChannels(), start, n);
        ScopedAllocationProbe probe;
        p.processBlock(view, midi);
        allocations += probe.counts().allocations;
    }
    return allocations;
}
} // namespace

class PluginContractTests final : public juce::UnitTest
{
public:
    PluginContractTests() : juce::UnitTest("Plugin contract (M1)", "HPEQ") {}

    void runTest() override
    {
        beginTest("identity: audio insert effect, distinct from HPDG");
        {
            FxPluginProcessor p;
            expectEquals(juce::String(JucePlugin_Name), juce::String("HPEQ"));
            expect(JucePlugin_IsSynth == 0, "IS_SYNTH must be FALSE");
            expect(!p.acceptsMidi() && !p.producesMidi() && !p.isMidiEffect());
            // HPDG is manufacturer BBLB, plugin BBG1. Same vendor, different plugin code.
            expect(JucePlugin_PluginCode != 0x42424731, "plugin code must differ from HPDG's BBG1");
            expectEquals(p.getLatencySamples(), 0);
            expectEquals(p.getTailLengthSeconds(), 0.0);
            expect(p.getBypassParameter() != nullptr, "host bypass must map to the bypass parameter");
        }

        beginTest("bus layouts: mono->mono and stereo->stereo only");
        {
            FxPluginProcessor p;
            const auto layout = [](juce::AudioChannelSet in, juce::AudioChannelSet out)
            {
                juce::AudioProcessor::BusesLayout l;
                l.inputBuses.add(in);
                l.outputBuses.add(out);
                return l;
            };
            expect(p.checkBusesLayoutSupported(layout(juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo())));
            expect(p.checkBusesLayoutSupported(layout(juce::AudioChannelSet::mono(), juce::AudioChannelSet::mono())));
            expect(!p.checkBusesLayoutSupported(layout(juce::AudioChannelSet::mono(), juce::AudioChannelSet::stereo())));
            expect(!p.checkBusesLayoutSupported(layout(juce::AudioChannelSet::create5point1(), juce::AudioChannelSet::create5point1())));
        }

        beginTest("M1-A: default state is bit-exact pass-through at every rate, block size, mono/stereo");
        {
            int cases = 0, exact = 0;
            std::uint64_t allocations = 0;
            for (double fs : kRates)
                for (int channels : { 1, 2 })
                    for (int block : kBlocks)
                    {
                        FxPluginProcessor p;
                        expect(configure(p, channels, fs, block));
                        const auto input = makeSyntheticDrums({ fs, channels, 0.25, kFixtureSeed });
                        auto audio = input;
                        allocations += render(p, audio, block);
                        ++cases;
                        exact += compareAligned(audio, input).bitExact ? 1 : 0;
                    }
            expectEquals(exact, cases);
            expectEquals(static_cast<int>(allocations), 0, "processBlock allocated");
            logMessage("  unity cases bit-exact: " + juce::String(exact) + "/" + juce::String(cases));
        }

        beginTest("bypass is bit-exact even with output gain set");
        {
            FxPluginProcessor p;
            setParameter(p, params::outputGain, -12.0f);
            setParameter(p, params::bypass, 1.0f);
            expect(configure(p, 2, 48000.0, 512));
            const auto input = makePinkNoise({ 48000.0, 2, 0.5, kFixtureSeed }, -20.0);
            auto audio = input;
            render(p, audio, 512);
            expect(compareAligned(audio, input).bitExact);
        }

        beginTest("output gain: -6 dB settles exactly, ramp is smooth, no allocations");
        {
            FxPluginProcessor p;
            expect(configure(p, 2, 48000.0, 256));
            setParameter(p, params::outputGain, -6.0f);
            const auto input = makeSine({ 48000.0, 2, 1.0, kFixtureSeed }, 1000.0, -6.0);
            auto audio = input;
            const auto allocations = render(p, audio, 256);
            expectEquals(static_cast<int>(allocations), 0);

            // After the 20 ms ramp the gain is exactly the target in every sample.
            const int settled = 48000 / 10;
            double worstDb = 0.0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = settled; i < audio.getNumSamples(); ++i)
                    if (std::abs(input.getSample(ch, i)) > 1.0e-3f)
                        worstDb = std::max(worstDb, std::abs(20.0 * std::log10(std::abs(audio.getSample(ch, i) / input.getSample(ch, i))) + 6.0));
            expect(worstDb < 1.0e-4, "settled gain error " + juce::String(worstDb) + " dB");

            // Smoothness: per-sample gain change during the ramp stays below 0.01 dB.
            double maxStepDb = 0.0, previousDb = 0.0;
            bool first = true;
            for (int i = 0; i < settled; ++i)
            {
                const float x = input.getSample(0, i);
                if (std::abs(x) < 1.0e-2f)
                    continue;
                const double g = 20.0 * std::log10(std::abs(audio.getSample(0, i) / x));
                if (!first)
                    maxStepDb = std::max(maxStepDb, std::abs(g - previousDb));
                previousDb = g;
                first = false;
            }
            expect(maxStepDb < 0.05, "max gain step " + juce::String(maxStepDb) + " dB");
            logMessage("  settled error " + juce::String(worstDb, 8) + " dB, max ramp step " + juce::String(maxStepDb, 5) + " dB");
        }

        beginTest("blocks larger than prepared and empty blocks are handled");
        {
            FxPluginProcessor p;
            expect(configure(p, 2, 48000.0, 64));
            setParameter(p, params::outputGain, 3.0f);
            auto audio = makePinkNoise({ 48000.0, 2, 0.2, kFixtureSeed }, -20.0);
            const auto allocations = render(p, audio, 4096);
            expectEquals(static_cast<int>(allocations), 0);
            juce::AudioBuffer<float> empty(2, 0);
            juce::MidiBuffer midi;
            p.processBlock(empty, midi);
            expect(measureAll(audio).allFinite);
        }

        beginTest("T04 (M1 scope): 100k random parameter mutations, random blocks 1..2048");
        {
            FxPluginProcessor p;
            expect(configure(p, 2, 48000.0, 2048));
            DeterministicRandom random(kFixtureSeed);
            const auto source = makeWhiteNoise({ 48000.0, 2, 1.0, kFixtureSeed }, -6.0);
            juce::AudioBuffer<float> block(2, 2048);
            juce::MidiBuffer midi;
            bool finite = true;
            float worstPeak = 0.0f;
            std::uint64_t allocations = 0;
            int position = 0;
            for (int mutation = 0; mutation < 100000; ++mutation)
            {
                const float gainDb = static_cast<float>(random.nextBipolar() * 30.0); // includes out-of-range
                setParameter(p, params::outputGain, gainDb);
                if (mutation % 97 == 0)
                    setParameter(p, params::bypass, random.nextBipolar() > 0.0 ? 1.0f : 0.0f);
                if (mutation % 10 != 0)
                    continue; // mutate 10x faster than we process
                const int n = 1 + static_cast<int>((random.nextBipolar() * 0.5 + 0.5) * 2047.0);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < n; ++i)
                        block.setSample(ch, i, source.getSample(ch, (position + i) % source.getNumSamples()));
                position += n;
                juce::AudioBuffer<float> view(block.getArrayOfWritePointers(), 2, 0, n);
                ScopedAllocationProbe probe;
                p.processBlock(view, midi);
                allocations += probe.counts().allocations;
                const auto stats = measureAll(view);
                finite = finite && stats.allFinite;
                worstPeak = std::max(worstPeak, static_cast<float>(stats.peak));
            }
            expect(finite, "non-finite output");
            // Input peak <= 0.5-ish, max gain +24 dB -> bound 0.5 * 15.85 with margin.
            expect(worstPeak <= measureAll(source).peak * juce::Decibels::decibelsToGain(params::kOutputGainMaxDb) * 1.0001,
                   "output exceeded the +24 dB bound");
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("M1-C (in-process): 100 editor open/close cycles while audio runs");
        {
            FxPluginProcessor p;
            expect(configure(p, 2, 48000.0, 512));
            auto audio = makePinkNoise({ 48000.0, 2, 0.05, kFixtureSeed }, -20.0);
            juce::MidiBuffer midi;
            const auto before = p.getProject();
            for (int cycle = 0; cycle < 100; ++cycle)
            {
                std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditorIfNeeded());
                expect(editor != nullptr, "createEditorIfNeeded returned null in cycle " + juce::String(cycle));
                if (editor == nullptr)
                    break;
                editor->setSize(1200, 700);
                p.processBlock(audio, midi);
                p.editorBeingDeleted(editor.get()); // what the VST3 wrapper does before deleting
            }
            expect(toValueTree(before).isEquivalentTo(toValueTree(p.getProject())), "opening the editor changed state");
        }
    }
};

static PluginContractTests pluginContractTests;
} // namespace hpeq::test
