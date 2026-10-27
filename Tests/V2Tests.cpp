// The v2 engine (docs/UDARNIK_V2.md): identities, block sizes, level independence, extremes, state migration, the
// output protection. The measured targets U1 .. U8 are HamloUdarnik_Lab's.

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/TransientShaper.h"
#include "Plugin/HamloUdarnikProcessor.h"
#include "TestSignals.h"

namespace hamlo::test
{
using dsp::TransientProcessor;
using dsp::TransientSettings;
namespace s = dsp::slot;

namespace
{
TransientSettings v2(float snap, float body, int style, int lookahead)
{
    TransientSettings p;
    p.values[s::kAttackGainDb] = snap;
    p.values[s::kSustainGainDb] = body;
    p.values[s::kTransientSpeed] = 0.5f;
    p.values[s::kTrStyle] = static_cast<float>(style);
    p.values[s::kTrLookahead] = static_cast<float>(lookahead);
    p.values[s::kTrPreserve] = 1.0f;
    p.values[s::kTrEngine] = 1.0f;
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

bool finite(const juce::AudioBuffer<float>& x)
{
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        for (int i = 0; i < x.getNumSamples(); ++i)
            if (!std::isfinite(x.getSample(ch, i)))
                return false;
    return true;
}

juce::AudioBuffer<float> renderPlugin(udarnik::HamloUdarnikProcessor& p, const juce::AudioBuffer<float>& x)
{
    p.prepareToPlay(kFs, 512);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(512, y.getNumSamples() - start));
        p.processBlock(view, midi);
    }
    return y;
}
} // namespace

class V2Tests final : public juce::UnitTest
{
public:
    V2Tests() : juce::UnitTest("V2 engine", "HamloUdarnik") {}

    void runTest() override
    {
        const auto loop = drums(2.0);

        beginTest("SNAP = BODY = 0 (every mode, look-ahead): the input delayed by the look-ahead, bit-exact");
        for (int style = 0; style < 3; ++style)
            for (int lookahead = 0; lookahead < 3; ++lookahead)
            {
                const auto p = v2(0.0f, 0.0f, style, lookahead);
                expect(bitExact(run(p, loop, 256), delayed(loop, TransientProcessor::lookaheadSamples(p, kFs))),
                       "style " + juce::String(style) + ", look-ahead " + juce::String(lookahead));
            }

        // The tone's start is a hit (its body lasts 500 ms + a 50 ms fade at SPEED 0.5); first written from 0.3 s,
        // which fell inside that body.
        beginTest("A steady tone (220 Hz + 1 kHz) is left alone: SNAP +12, BODY -12, every mode - the input delayed, bit-exact after 0.7 s");
        {
            auto tone = sine(2.0, 220.0, -20.0);
            const auto other = sine(2.0, 1000.0, -20.0);
            for (int ch = 0; ch < 2; ++ch)
                tone.addFrom(ch, 0, other, ch, 0, tone.getNumSamples());
            for (int style = 0; style < 3; ++style)
            {
                const auto p = v2(12.0f, -12.0f, style, 1);
                const int l = TransientProcessor::lookaheadSamples(p, kFs);
                const auto y = run(p, tone, 256), d = delayed(tone, l);
                const int from = static_cast<int>(0.7 * kFs);
                expect(bitExact(segment(y, from, y.getNumSamples() - from), segment(d, from, d.getNumSamples() - from)), "style " + juce::String(style));
            }
        }

        beginTest("Host blocks 1, 64, 127, 512 bit-identical to 256 (every mode, SNAP +9, BODY -6, every look-ahead)");
        for (int style = 0; style < 3; ++style)
            for (int lookahead = 0; lookahead < 3; ++lookahead)
            {
                const auto p = v2(9.0f, -6.0f, style, lookahead);
                const auto expected = run(p, loop, 256);
                for (int block : { 1, 64, 127, 512 })
                    expect(bitExact(run(p, loop, block), expected), "style " + juce::String(style) + ", look-ahead " + juce::String(lookahead) + ", block " + juce::String(block));
            }

        beginTest("Level independence: the loop 24 dB quieter gives the output 24 dB quieter (PRAVDA and PERESTROIKA, within 1e-4 relative)");
        for (const int style : { 0, 2 })
        {
            auto quiet = loop;
            const float k = static_cast<float>(std::pow(10.0, -24.0 / 20.0));
            quiet.applyGain(k);
            auto p = v2(9.0f, -6.0f, style, 1);
            p.values[s::kTrDecayLog2] = 0.5f;
            auto ref = run(p, loop, 256);
            ref.applyGain(k);
            const double d = differenceDb(run(p, quiet, 256), ref);
            logMessage("  style " + juce::String(style) + ": difference " + juce::String(d, 1) + " dB");
            expect(d <= -80.0);
        }

        beginTest("Extremes stay finite: SNAP / BODY +-24, every mode, SPEED 0 / 1, RISE / DECAY x0.5 / x2");
        for (int style = 0; style < 3; ++style)
            for (const float snap : { -24.0f, 24.0f })
                for (const float speed : { 0.0f, 1.0f })
                {
                    auto p = v2(snap, -snap, style, 2);
                    p.values[s::kTransientSpeed] = speed;
                    p.values[s::kTrRiseLog2] = snap > 0 ? 1.0f : -1.0f;
                    p.values[s::kTrDecayLog2] = snap > 0 ? -1.0f : 1.0f;
                    expect(finite(run(p, loop, 256)));
                }

        beginTest("UDARNIK is peak-neutral: SNAP +12 does not raise the sample peak (+0.1 dB); PRAVDA does");
        {
            const auto peakDb = [](const juce::AudioBuffer<float>& b) { return 20.0 * std::log10(b.getMagnitude(0, b.getNumSamples())); };
            const double in = peakDb(loop), udarnik = peakDb(run(v2(12.0f, 0.0f, 1, 2), loop, 256)), pravda = peakDb(run(v2(12.0f, 0.0f, 0, 2), loop, 256));
            logMessage("  peak: input " + juce::String(in, 2) + ", UDARNIK " + juce::String(udarnik, 2) + ", PRAVDA " + juce::String(pravda, 2) + " dBFS");
            expect(udarnik <= in + 0.1 && pravda > in + 3.0);
        }

        beginTest("Plug-in: a new instance is V2 with PROTECT on and leaves the loop untouched (the input delayed, bit-exact)");
        {
            udarnik::HamloUdarnikProcessor p;
            expectEquals(juce::roundToInt(p.parameterValue("engine")), 1);
            expectEquals(juce::roundToInt(p.parameterValue("protect")), 1);
            expect(bitExact(renderPlugin(p, loop), delayed(loop, 240)));
        }

        beginTest("Plug-in: a state without `engine` (HPEQ, HamloUdarnik 0.1.0) loads as CLASSIC without PROTECT; HPEQ's presets are CLASSIC");
        {
            udarnik::HamloUdarnikProcessor p;
            juce::XmlElement xml("HamloUdarnik");
            auto* e = xml.createNewChildElement("PARAM");
            e->setAttribute("id", "attackDb");
            e->setAttribute("value", 6.0);
            juce::MemoryBlock block;
            juce::AudioProcessor::copyXmlToBinary(xml, block);
            p.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
            expectEquals(juce::roundToInt(p.parameterValue("engine")), 0);
            expectEquals(juce::roundToInt(p.parameterValue("protect")), 0);
            udarnik::HamloUdarnikProcessor q;
            q.setCurrentProgram(1);
            expectEquals(juce::roundToInt(q.parameterValue("engine")), 0);
        }

        beginTest("Plug-in PROTECT: SNAP +12 (PRAVDA) on the loop at -1 dBFS peak stays <= 0 dBFS; off it goes over");
        {
            auto hot = loop;
            hot.applyGain(static_cast<float>(std::pow(10.0, -1.0 / 20.0) / hot.getMagnitude(0, hot.getNumSamples())));
            udarnik::HamloUdarnikProcessor on, off;
            for (auto* p : { &on, &off })
            {
                p->setParameterValue("attackDb", 12.0);
                p->setParameterValue("style", 0.0);
            }
            off.setParameterValue("protect", 0.0);
            const double a = 20.0 * std::log10(renderPlugin(on, hot).getMagnitude(0, hot.getNumSamples()));
            const double b = 20.0 * std::log10(renderPlugin(off, hot).getMagnitude(0, hot.getNumSamples()));
            logMessage("  peak with PROTECT " + juce::String(a, 2) + ", without " + juce::String(b, 2) + " dBFS");
            expect(a <= 0.0 && b > 0.0);
        }
    }
};

static V2Tests v2Tests;
} // namespace hamlo::test
