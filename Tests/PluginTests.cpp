// The plug-in around the limiter: parameters and state of HPEQ's HamloRedWall module VST3, the fixed latency,
// MIX / ON, block-size independence, mono, programs, the page.

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloRedWallEditor.h"
#include "Plugin/HamloRedWallProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"
#include "TestSignals.h"

namespace hamlo::test
{
using redwall::HamloRedWallProcessor;

namespace
{
juce::AudioBuffer<float> render(HamloRedWallProcessor& p, const juce::AudioBuffer<float>& x, int block, int prepared = -1)
{
    p.prepareToPlay(kFs, prepared > 0 ? prepared : block);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), y.getNumChannels(), start, n);
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

class PluginTests final : public juce::UnitTest
{
public:
    PluginTests() : juce::UnitTest("Plug-in", "HamloRedWall") {}

    void runTest() override
    {
        const auto loop = drums(2.0);
        // the fixed latency: the 10 ms look-ahead + 8 (true peak) + the colour stage's oversampler
        const int colourLatency = []
        {
            dsp::RedWallColour c;
            c.prepare(kFs, 512);
            return c.latency();
        }();
        const int latency = 488 + colourLatency;

        beginTest("Parameters: the IDs, ranges and defaults of HPEQ's HamloRedWall, then bypass");
        {
            HamloRedWallProcessor p;
            expectEquals(p.getParameters().size(), static_cast<int>(redwall::parameterSpecs().size()) + 1);
            for (const auto& spec : redwall::parameterSpecs())
            {
                auto* r = p.parameter(spec.id);
                expect(r != nullptr, spec.id);
                if (r == nullptr)
                    continue;
                expectWithinAbsoluteError(p.parameterValue(spec.id), static_cast<double>(spec.defaultValue), 1.0e-4, spec.id);
                expectWithinAbsoluteError(static_cast<double>(r->getNormalisableRange().start), static_cast<double>(spec.minimum), 1.0e-6);
                expectWithinAbsoluteError(static_cast<double>(r->getNormalisableRange().end), static_cast<double>(spec.maximum), 1.0e-6);
            }
            expect(p.getBypassParameter() != nullptr && !p.getBypassParameter()->get());
        }

        beginTest("Latency: fixed at the 10 ms look-ahead + 8 (true peak) = 488 samples at 48 kHz + the colour stage, whatever LOOKAHEAD / TRUE PEAK / DRIVE / TUBE");
        {
            logMessage("  colour stage " + juce::String(colourLatency) + " samples");
            expect(colourLatency > 0 && colourLatency < 100);
            HamloRedWallProcessor p;
            p.prepareToPlay(kFs, 512);
            expectEquals(p.getLatencySamples(), latency);
            p.setParameterValue("lookaheadMs", 0.5);
            p.setParameterValue("truePeak", 0.0);
            p.setParameterValue("drive", 0.7);
            p.setParameterValue("tube", 0.4);
            p.prepareToPlay(kFs, 512);
            expectEquals(p.getLatencySamples(), latency);
        }

        beginTest("MIX 0 and ON off: the input delayed by the latency, bit-exact; a quiet loop under the ceiling passes delayed (every lookahead)");
        for (const double lookahead : { 0.5, 2.0, 10.0 })
        {
            auto quiet = loop;
            quiet.applyGain(0.25f); // about -11 dBFS peak, under the -1 dB LIMIT
            HamloRedWallProcessor p;
            p.setParameterValue("lookaheadMs", lookahead);
            expect(bitExact(render(p, quiet, 512), delayed(quiet, latency)), "quiet, lookahead " + juce::String(lookahead));

            HamloRedWallProcessor m;
            m.setParameterValue("lookaheadMs", lookahead);
            m.setParameterValue("thresholdDb", -9.0);
            m.setParameterValue("mix", 0.0);
            expect(bitExact(render(m, loop, 512), delayed(loop, latency)), "MIX 0, lookahead " + juce::String(lookahead));

            HamloRedWallProcessor o;
            o.setParameterValue("lookaheadMs", lookahead);
            o.setParameterValue("thresholdDb", -9.0);
            o.setParameterValue("bypass", 1.0);
            expect(bitExact(render(o, loop, 512), delayed(loop, latency)), "ON off, lookahead " + juce::String(lookahead));
        }

        beginTest("Pushed 9 dB the output never passes the -1 dB LIMIT, and it is louder than the input");
        {
            HamloRedWallProcessor p;
            p.setParameterValue("thresholdDb", -9.0);
            const auto y = render(p, loop, 512);
            const double peak = 20.0 * std::log10(y.getMagnitude(0, y.getNumSamples()));
            logMessage("  peak " + juce::String(peak, 3) + " dBFS, level change " + juce::String(rmsDb(y) - rmsDb(loop), 2) + " dB");
            expect(peak <= -1.0 + 1.0e-4 && rmsDb(y) > rmsDb(loop) + 3.0);
        }

        beginTest("State: a state saved by HPEQ's module VST3 (tag HamloRedWall, PARAM id / value) loads; save / load round trip");
        {
            HamloRedWallProcessor p;
            juce::XmlElement xml("HamloRedWall");
            const auto add = [&](const char* id, double v)
            {
                auto* e = xml.createNewChildElement("PARAM");
                e->setAttribute("id", id);
                e->setAttribute("value", v);
            };
            add("thresholdDb", -7.5);
            add("style", 2.0);
            add("releaseMs", 150.0);
            add("bypass", 1.0);
            juce::MemoryBlock block;
            juce::AudioProcessor::copyXmlToBinary(xml, block);
            p.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
            expectWithinAbsoluteError(p.parameterValue("thresholdDb"), -7.5, 1.0e-4);
            expectEquals(juce::roundToInt(p.parameterValue("style")), 2);
            expectWithinAbsoluteError(p.parameterValue("releaseMs"), 150.0, 1.0e-2);
            expect(!p.isOn());
            juce::MemoryBlock saved;
            p.getStateInformation(saved);
            HamloRedWallProcessor q;
            q.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            for (const auto& spec : redwall::parameterSpecs())
                expectWithinAbsoluteError(q.parameterValue(spec.id), p.parameterValue(spec.id), 1.0e-6, spec.id);
        }

        beginTest("Migration: a state without engine / drive / tube (HPEQ, HamloRedWall 0.1.0) loads as CLASSIC with no colour; a new instance is V2");
        {
            HamloRedWallProcessor fresh;
            expectEquals(juce::roundToInt(fresh.parameterValue("engine")), 1);
            HamloRedWallProcessor p;
            p.setParameterValue("drive", 0.5);
            juce::XmlElement xml("HamloRedWall");
            auto* e = xml.createNewChildElement("PARAM");
            e->setAttribute("id", "thresholdDb");
            e->setAttribute("value", -3.0);
            juce::MemoryBlock block;
            juce::AudioProcessor::copyXmlToBinary(xml, block);
            p.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
            expectEquals(juce::roundToInt(p.parameterValue("engine")), 0);
            expectEquals(p.parameterValue("drive"), 0.0);
            expectEquals(p.parameterValue("tube"), 0.0);
        }

        beginTest("Host blocks 1, 64, 127, 512 and 4096 after prepare(256) bit-identical (STAKHANOV, pushed 6 dB, MIX 0.7)");
        {
            const auto set = [](HamloRedWallProcessor& p)
            {
                p.setParameterValue("thresholdDb", -6.0);
                p.setParameterValue("style", 1.0);
                p.setParameterValue("mix", 0.7);
            };
            HamloRedWallProcessor ref;
            set(ref);
            const auto expected = render(ref, loop, 256);
            for (int block : { 1, 64, 127, 512 })
            {
                HamloRedWallProcessor p;
                set(p);
                expect(bitExact(render(p, loop, block), expected), "block " + juce::String(block));
            }
            HamloRedWallProcessor big;
            set(big);
            expect(bitExact(render(big, loop, 4096, 256), expected), "block 4096 after prepare(256)");
        }

        beginTest("DRIVE / TUBE: host blocks 1, 64, 127, 512 and 4096 after prepare(256) bit-identical (every style, pushed 6 dB)");
        for (const double style : { 0.0, 1.0, 2.0 })
        {
            const auto set = [style](HamloRedWallProcessor& p)
            {
                p.setParameterValue("thresholdDb", -6.0);
                p.setParameterValue("style", style);
                p.setParameterValue("drive", 0.6);
                p.setParameterValue("tube", 0.8);
            };
            HamloRedWallProcessor ref;
            set(ref);
            const auto expected = render(ref, loop, 256);
            for (int block : { 1, 64, 127, 512 })
            {
                HamloRedWallProcessor p;
                set(p);
                expect(bitExact(render(p, loop, block), expected), "style " + juce::String(style) + ", block " + juce::String(block));
            }
            HamloRedWallProcessor big;
            set(big);
            expect(bitExact(render(big, loop, 4096, 256), expected), "style " + juce::String(style) + ", block 4096 after prepare(256)");
        }

        beginTest("DRIVE / TUBE: LIMIT holds at 100 % (drums +18 dB, TRUE PEAK on and off); the stage colours (output differs from DRIVE = TUBE = 0)");
        for (const double tp : { 0.0, 1.0 })
        {
            auto hot = loop;
            hot.applyGain(juce::Decibels::decibelsToGain(18.0f));
            HamloRedWallProcessor p, plain;
            for (auto* q : { &p, &plain })
                q->setParameterValue("truePeak", tp);
            p.setParameterValue("drive", 1.0);
            p.setParameterValue("tube", 1.0);
            const auto y = render(p, hot, 512);
            expect(y.getMagnitude(0, y.getNumSamples()) <= std::pow(10.0f, -1.0f / 20.0f) + 1.0e-5f, "true peak " + juce::String(tp));
            expect(!bitExact(y, render(plain, hot, 512)));
        }

        beginTest("DRIVE / TUBE back to 0: after the 10 ms fade the colour stage is the delayed input again, bit-exact");
        {
            // (checked on the stage: the limiter after it saw a different signal and settles only asymptotically)
            dsp::RedWallColour c;
            c.prepare(kFs, 512);
            dsp::LimiterSettings s;
            s.values[dsp::slot::kLimTube] = 0.5f;
            s.values[dsp::slot::kLimDrive] = 0.5f;
            auto y = loop;
            const int switchAt = loop.getNumSamples() / 4;
            for (int start = 0; start < y.getNumSamples(); start += 512)
            {
                if (start >= switchAt)
                    s.values[dsp::slot::kLimTube] = s.values[dsp::slot::kLimDrive] = 0.0f;
                float* ptr[2] { y.getWritePointer(0, start), y.getWritePointer(1, start) };
                c.process(ptr, 2, juce::jmin(512, y.getNumSamples() - start), s);
            }
            const auto expected = delayed(loop, colourLatency);
            const int from = switchAt + 1024; // the 10 ms (480 samples) fade done
            expect(bitExact(segment(y, from, y.getNumSamples() - from), segment(expected, from, y.getNumSamples() - from)));
            expect(!bitExact(segment(y, 0, switchAt), segment(expected, 0, switchAt)));
        }

        beginTest("Mono: the mono layout is accepted and the limiter works on it");
        {
            HamloRedWallProcessor p;
            juce::AudioProcessor::BusesLayout mono;
            mono.inputBuses.add(juce::AudioChannelSet::mono());
            mono.outputBuses.add(juce::AudioChannelSet::mono());
            expect(p.setBusesLayout(mono));
            p.setParameterValue("thresholdDb", -9.0);
            juce::AudioBuffer<float> m(1, loop.getNumSamples());
            m.copyFrom(0, 0, loop, 0, 0, loop.getNumSamples());
            const auto y = render(p, m, 512);
            expect(!bitExact(y, delayed(m, latency)) && y.getMagnitude(0, y.getNumSamples()) <= std::pow(10.0f, -1.0f / 20.0f) + 1.0e-5f);
        }

        beginTest("Programs: Default, the 6 HPEQ limiter presets (CLASSIC) and 12 v2 presets; Default restores");
        {
            HamloRedWallProcessor p;
            expectEquals(p.getNumPrograms(), 19);
            for (int i = 1; i <= 6; ++i)
            {
                p.setCurrentProgram(i);
                expectEquals(juce::roundToInt(p.parameterValue("engine")), 0, p.getProgramName(i));
            }
            expectEquals(p.getProgramName(3), juce::String("Loud Master"));
            p.setCurrentProgram(3);
            expectWithinAbsoluteError(p.parameterValue("thresholdDb"), -6.0, 1.0e-3);
            expectEquals(juce::roundToInt(p.parameterValue("style")), 1);
            p.setCurrentProgram(0);
            for (const auto& spec : redwall::parameterSpecs())
                expectWithinAbsoluteError(p.parameterValue(spec.id), static_cast<double>(spec.defaultValue), 1.0e-3, spec.id);
        }

        beginTest("Every program at MIX 100 %: drums +12 dB stay under its LIMIT (sample peak); a parallel one (MIX < 100 %) passes the dry part by design");
        for (int i = 1; i < 19; ++i)
        {
            HamloRedWallProcessor p;
            p.setCurrentProgram(i);
            if (p.parameterValue("mix") < 1.0)
                continue;
            auto hot = loop;
            hot.applyGain(juce::Decibels::decibelsToGain(12.0f));
            const auto y = render(p, hot, 512);
            const double peak = 20.0 * std::log10(y.getMagnitude(0, y.getNumSamples()));
            expect(peak <= p.parameterValue("ceilingDb") + 1.0e-3, p.getProgramName(i) + ": " + juce::String(peak, 3) + " dBFS");
        }

        beginTest("Page: 1180 x 820, resizable; knobs, STYLE, levers and ON write the parameters; host changes come back");
        {
            HamloRedWallProcessor p;
            std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
            auto* editor = dynamic_cast<redwall::HamloRedWallEditor*>(e.get());
            expect(editor != nullptr);
            if (editor == nullptr)
                return;
            expectEquals(editor->getWidth(), redwall::kEditorWidth);
            expectEquals(editor->getHeight(), redwall::kEditorHeight);
            auto& panel = editor->getPanel();
            using P = ui::HamloRedWallPanel;
            panel.onKnobGestureStart(P::Threshold);
            panel.onKnobChange(P::Threshold, -5.5);
            panel.onKnobGestureEnd();
            panel.onKnobChange(P::Style, 2.0);
            panel.onSwitchChange(P::TruePeak, 0);
            panel.onSwitchChange(P::Monitor, 2);
            panel.onPowerChange(false);
            expectWithinAbsoluteError(p.parameterValue("thresholdDb"), -5.5, 1.0e-3);
            expectEquals(juce::roundToInt(p.parameterValue("style")), 2);
            expectEquals(juce::roundToInt(p.parameterValue("truePeak")), 0);
            expectEquals(juce::roundToInt(p.parameterValue("monitor")), 2);
            expect(!p.isOn());
            p.setParameterValue("ceilingDb", -4.0);
            p.setParameterValue("bypass", 0.0);
            editor->setSize(590, 350);
            expect(panel.getWidth() == 590 && panel.getHeight() == 350);
            juce::MessageManager::getInstance()->runDispatchLoopUntil(120);
            expectWithinAbsoluteError(panel.getValues().knobs[P::Ceiling], -4.0, 1.0e-3);
            expect(panel.getValues().on);
        }
    }
};

static PluginTests pluginTests;
} // namespace hamlo::test
