// The plug-in around the shaper: parameters and state of HPEQ's HamloUdarnik module VST3, the fixed latency,
// MIX / ON, block-size independence, mono, programs, the page.

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloUdarnikEditor.h"
#include "Plugin/HamloUdarnikProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"
#include "TestSignals.h"

namespace hamlo::test
{
using udarnik::HamloUdarnikProcessor;

namespace
{
juce::AudioBuffer<float> render(HamloUdarnikProcessor& p, const juce::AudioBuffer<float>& x, int block, int prepared = -1)
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
    PluginTests() : juce::UnitTest("Plug-in", "HamloUdarnik") {}

    void runTest() override
    {
        const auto loop = drums(2.0);

        beginTest("Parameters: the IDs, ranges and defaults of HPEQ's HamloUdarnik, then bypass");
        {
            HamloUdarnikProcessor p;
            expectEquals(p.getParameters().size(), static_cast<int>(udarnik::parameterSpecs().size()) + 1);
            for (const auto& spec : udarnik::parameterSpecs())
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

        beginTest("Latency: fixed at the SOYUZ look-ahead (5 ms = 240 samples at 48 kHz) whatever MODE / LOOK-AHEAD");
        {
            HamloUdarnikProcessor p;
            p.prepareToPlay(kFs, 512);
            expectEquals(p.getLatencySamples(), 240);
            p.setParameterValue("lookahead", 0.0);
            p.prepareToPlay(kFs, 512);
            expectEquals(p.getLatencySamples(), 240);
        }

        beginTest("A fresh instance (SNAP = BODY = 0) and MIX 0 and ON off: the input delayed by the latency, bit-exact; every look-ahead");
        for (int lookahead = 0; lookahead < 3; ++lookahead)
        {
            HamloUdarnikProcessor p;
            p.setParameterValue("lookahead", lookahead);
            expect(bitExact(render(p, loop, 512), delayed(loop, 240)), "fresh, look-ahead " + juce::String(lookahead));

            HamloUdarnikProcessor m;
            m.setParameterValue("lookahead", lookahead);
            m.setParameterValue("attackDb", 9.0);
            m.setParameterValue("mix", 0.0);
            expect(bitExact(render(m, loop, 512), delayed(loop, 240)), "MIX 0, look-ahead " + juce::String(lookahead));

            HamloUdarnikProcessor o;
            o.setParameterValue("lookahead", lookahead);
            o.setParameterValue("attackDb", 9.0);
            o.setParameterValue("bypass", 1.0);
            expect(bitExact(render(o, loop, 512), delayed(loop, 240)), "ON off, look-ahead " + juce::String(lookahead));
        }

        beginTest("SNAP +9 dB changes the loop and is aligned: its hits stay within one sample of the delayed input's");
        {
            HamloUdarnikProcessor p;
            p.setParameterValue("attackDb", 9.0);
            p.setParameterValue("style", 1.0);
            const auto y = render(p, loop, 512);
            const auto d = delayed(loop, 240);
            expect(!bitExact(y, d));
            const auto onset = [](const juce::AudioBuffer<float>& b)
            {
                for (int i = 0; i < b.getNumSamples(); ++i)
                    if (std::abs(b.getSample(0, i)) > 0.05f)
                        return i;
                return -1;
            };
            logMessage("  first sample above 0.05: delayed input " + juce::String(onset(d)) + ", output " + juce::String(onset(y)));
            expect(std::abs(onset(y) - onset(d)) <= 1);
        }

        beginTest("State: a state saved by HPEQ's module VST3 (tag HamloUdarnik, PARAM id / value) loads; save / load round trip");
        {
            HamloUdarnikProcessor p;
            juce::XmlElement xml("HamloUdarnik");
            const auto add = [&](const char* id, double v)
            {
                auto* e = xml.createNewChildElement("PARAM");
                e->setAttribute("id", id);
                e->setAttribute("value", v);
            };
            add("attackDb", 7.5);
            add("style", 2.0);
            add("rise", 1.5);
            add("bypass", 1.0);
            juce::MemoryBlock block;
            juce::AudioProcessor::copyXmlToBinary(xml, block);
            p.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
            expectWithinAbsoluteError(p.parameterValue("attackDb"), 7.5, 1.0e-4);
            expectEquals(juce::roundToInt(p.parameterValue("style")), 2);
            expectWithinAbsoluteError(p.parameterValue("rise"), 1.5, 1.0e-3);
            expect(!p.isOn());
            juce::MemoryBlock saved;
            p.getStateInformation(saved);
            HamloUdarnikProcessor q;
            q.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            for (const auto& spec : udarnik::parameterSpecs())
                expectWithinAbsoluteError(q.parameterValue(spec.id), p.parameterValue(spec.id), 1.0e-6, spec.id);
        }

        beginTest("Host blocks 1, 64, 127, 512 and 4096 after prepare(256) bit-identical (UDARNIK, SNAP +6, BODY -3, MIX 0.7)");
        {
            const auto set = [](HamloUdarnikProcessor& p)
            {
                p.setParameterValue("attackDb", 6.0);
                p.setParameterValue("sustainDb", -3.0);
                p.setParameterValue("style", 1.0);
                p.setParameterValue("mix", 0.7);
            };
            HamloUdarnikProcessor ref;
            set(ref);
            const auto expected = render(ref, loop, 256);
            for (int block : { 1, 64, 127, 512 })
            {
                HamloUdarnikProcessor p;
                set(p);
                expect(bitExact(render(p, loop, block), expected), "block " + juce::String(block));
            }
            HamloUdarnikProcessor big;
            set(big);
            expect(bitExact(render(big, loop, 4096, 256), expected), "block 4096 after prepare(256)");
        }

        beginTest("Mono: the mono layout is accepted and SNAP changes it");
        {
            HamloUdarnikProcessor p;
            juce::AudioProcessor::BusesLayout mono;
            mono.inputBuses.add(juce::AudioChannelSet::mono());
            mono.outputBuses.add(juce::AudioChannelSet::mono());
            expect(p.setBusesLayout(mono));
            p.setParameterValue("attackDb", 9.0);
            juce::AudioBuffer<float> m(1, loop.getNumSamples());
            m.copyFrom(0, 0, loop, 0, 0, loop.getNumSamples());
            expect(!bitExact(render(p, m, 512), delayed(m, 240)));
        }

        beginTest("Programs: Default plus the 6 HPEQ transient presets (CLASSIC) and 12 V2 presets; Default restores");
        {
            HamloUdarnikProcessor p;
            expectEquals(p.getNumPrograms(), 19);
            expectEquals(p.getProgramName(2), juce::String("Snare Crack"));
            p.setCurrentProgram(2);
            expectWithinAbsoluteError(p.parameterValue("attackDb"), 8.0, 1.0e-3);
            expectEquals(juce::roundToInt(p.parameterValue("style")), 1);
            p.setCurrentProgram(0);
            for (const auto& spec : udarnik::parameterSpecs())
                expectWithinAbsoluteError(p.parameterValue(spec.id), static_cast<double>(spec.defaultValue), 1.0e-3, spec.id);
        }

        beginTest("Every preset on the drum loop at -1 dBFS peak: finite, changes it; the V2 ones (PROTECT on) stay <= 0 dBFS");
        {
            auto hot = loop;
            hot.applyGain(static_cast<float>(std::pow(10.0, -1.0 / 20.0) / hot.getMagnitude(0, hot.getNumSamples())));
            for (int program = 1; program < 19; ++program)
            {
                HamloUdarnikProcessor p;
                p.setCurrentProgram(program);
                const auto y = render(p, hot, 512);
                bool finite = true;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < y.getNumSamples(); ++i)
                        finite = finite && std::isfinite(y.getSample(ch, i));
                const double peak = 20.0 * std::log10(y.getMagnitude(0, y.getNumSamples()));
                const bool v2 = juce::roundToInt(p.parameterValue("engine")) == 1;
                logMessage("  " + p.getProgramName(program) + (v2 ? " (V2)" : " (CLASSIC)") + ": peak " + juce::String(peak, 2) + " dBFS");
                expect(finite && !bitExact(y, delayed(hot, 240)) && (!v2 || peak <= 0.0), p.getProgramName(program));
            }
        }

        beginTest("Page: 1180 x 820, resizable; knob, MODE, levers and ON write the parameters; host changes come back");
        {
            HamloUdarnikProcessor p;
            std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
            auto* editor = dynamic_cast<udarnik::HamloUdarnikEditor*>(e.get());
            expect(editor != nullptr);
            if (editor == nullptr)
                return;
            expectEquals(editor->getWidth(), udarnik::kEditorWidth);
            expectEquals(editor->getHeight(), udarnik::kEditorHeight);
            auto& panel = editor->getPanel();
            using P = ui::HamloUdarnikPanel;
            panel.onKnobGestureStart(P::Snap);
            panel.onKnobChange(P::Snap, 5.5);
            panel.onKnobGestureEnd();
            panel.onKnobChange(P::Mode, 2.0);
            panel.onSwitchChange(P::Lookahead, 2);
            panel.onSwitchChange(P::Listen, 1);
            panel.onSwitchChange(P::Engine, 0);
            panel.onSwitchChange(P::Protect, 0);
            panel.onPowerChange(false);
            expectWithinAbsoluteError(p.parameterValue("attackDb"), 5.5, 1.0e-3);
            expectEquals(juce::roundToInt(p.parameterValue("style")), 2);
            expectEquals(juce::roundToInt(p.parameterValue("lookahead")), 2);
            expectEquals(juce::roundToInt(p.parameterValue("listen")), 1);
            expectEquals(juce::roundToInt(p.parameterValue("engine")), 0);
            expectEquals(juce::roundToInt(p.parameterValue("protect")), 0);
            expect(!p.isOn());
            p.setParameterValue("sustainDb", -4.0);
            p.setParameterValue("bypass", 0.0);
            editor->setSize(590, 410);
            expect(panel.getWidth() == 590 && panel.getHeight() == 410);
            juce::MessageManager::getInstance()->runDispatchLoopUntil(120);
            expectWithinAbsoluteError(panel.getValues().knobs[P::Body], -4.0, 1.0e-3);
            expect(panel.getValues().on);
        }
    }
};

static PluginTests pluginTests;
} // namespace hamlo::test
