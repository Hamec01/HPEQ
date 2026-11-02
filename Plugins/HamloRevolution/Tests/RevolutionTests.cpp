// HamloRevolution R0 (docs/REVOLUTION_V2.md, V1): the plug-in's slot = the slot HPEQ makes from the same module
// (LayerSnapshot.cpp from presetBaseModule + the preset, the pattern generated as Plugin/Presets.cpp makeModule),
// for Default and every factory preset; the pattern / feel / seed state; the page's GENERATE reaches the DSP.

#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/ModulePresets.h"
#include "Core/Revolution.h"
#include "DSP/LayerSnapshot.h"
#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include "UI/HamloRevolutionPanel.h"

namespace hamlo::test
{
using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;
namespace rev = hpeq::revolution;

namespace
{
// HPEQ's module for program `index` (0 = Default = presetBaseModule), as Plugin/Presets.cpp makeModule builds it.
hpeq::FxSlotState hpeqModule(int index)
{
    auto m = hpeq::presetBaseModule(hpeq::FxType::MonstaFx);
    if (index == 0)
        return m;
    const auto& preset = hpeq::presets::factoryModulePresets(hpeq::FxType::MonstaFx)[static_cast<size_t>(index - 1)];
    m.mix = preset.mix;
    for (const auto& [key, value] : preset.values)
        m.parameters.set(key, value);
    const auto seed = static_cast<std::uint64_t>(hpeq::presets::presetValue(preset, "revSeed", 1.0));
    const int character = juce::roundToInt(hpeq::presets::presetValue(preset, "character", static_cast<double>(rev::Musical)));
    rev::writePattern(m.parameters, rev::generate({}, seed, character));
    return m;
}

hpeq::dsp::FxSlotParams hpeqSlot(const hpeq::FxSlotState& module)
{
    hpeq::LayerState layer;
    layer.mixChain.push_back(module);
    return hpeq::dsp::makeSnapshot(layer).mix.slots[0];
}

std::unique_ptr<ModulePluginProcessor> make()
{
    return std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution); // large: heap
}

// Playing at 120 BPM from bar 1 (the pattern runs only with the transport).
class PlayHead final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm(120.0);
        info.setIsPlaying(true);
        info.setPpqPosition(ppq);
        return info;
    }
    double ppq = 0.0;
};

juce::AudioBuffer<float> renderPlaying(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x, int block)
{
    PlayHead head;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, block);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        head.ppq = static_cast<double>(start) * 120.0 / (60.0 * kFs);
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
    }
    p.setPlayHead(nullptr);
    return y;
}
} // namespace

class RevolutionTests final : public juce::UnitTest
{
public:
    RevolutionTests() : juce::UnitTest("Revolution", "HamloRevolution") {}

    void runTest() override
    {
        beginTest("V1: the slot of Default and of every preset = the slot HPEQ makes from the same module (bit-exact)");
        {
            auto probe = make();
            for (int i = 0; i < probe->getNumPrograms() && probe->genrePresetFor(i) == nullptr; ++i) // HPEQ's programs (R6)
            {
                auto p = make();
                p->setCurrentProgram(i);
                const auto mine = p->currentSlot(true);
                const auto ref = hpeqSlot(hpeqModule(i));
                int differing = 0;
                double largest = 0.0;
                // HPEQ's slot is values 0 .. 47; 48 on are R1 / R2 (lanes, shapes, bands) - their sound at defaults is
                // checked by GoldenTests
                for (size_t k = 0; k < 48; ++k)
                    if (mine.values[k] != ref.values[k])
                    {
                        ++differing;
                        largest = juce::jmax(largest, static_cast<double>(std::abs(mine.values[k] - ref.values[k])));
                        logMessage("  " + p->getProgramName(i) + ": value " + juce::String(static_cast<int>(k)) + " " + juce::String(mine.values[k], 9)
                                   + " vs HPEQ " + juce::String(ref.values[k], 9));
                    }
                expect(differing == 0 && mine.mix == ref.mix && mine.type == ref.type && mine.active == ref.active,
                       p->getProgramName(i) + ": " + juce::String(differing) + " values differ (largest " + juce::String(largest) + ")");
            }
        }

        beginTest("State: the pattern, its locks and allowed actions, the feel and the seed round trip; GENERATE keeps them");
        {
            auto p = make();
            ModulePluginProcessor::RevolutionState s;
            s.pattern = rev::generate({}, 4242, rev::Experimental);
            s.pattern.locks = { 0x0f0f, 0x00ff, 0x8001 };
            s.pattern.allow = { 0x003f, 0x00f1, 0xffff };
            for (size_t i = 0; i < s.feel.size(); ++i)
                s.feel[i] = static_cast<std::uint8_t>((i * 5) % 16);
            s.seed = 0x7fffffffLL + 77; // above 32 bits
            p->setRevolutionState(s);
            juce::MemoryBlock saved;
            p->getStateInformation(saved);
            auto q = make();
            q->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            const auto r = q->getRevolutionState();
            expect(r.pattern == s.pattern, "pattern");
            expect(r.feel == s.feel, "feel");
            expectEquals(r.seed, s.seed);
            const auto a = p->currentSlot(true), b = q->currentSlot(true);
            expect(a.values == b.values, "the slots");
            // saved twice: one REVOLUTION element, not two
            juce::MemoryBlock again;
            q->getStateInformation(again);
            const auto xml = juce::AudioProcessor::getXmlFromBinary(again.getData(), static_cast<int>(again.getSize()));
            int count = 0;
            for (auto* e : xml->getChildWithTagNameIterator("REVOLUTION"))
                count += e != nullptr ? 1 : 0;
            expectEquals(count, 1);
        }

        beginTest("State without the pattern element (parameters only): the Default pattern (Musical, seed 1)");
        {
            auto p = make();
            p->setRevolutionState({ rev::generate({}, 99, rev::Destruction), {}, 99 });
            juce::XmlElement xml("HamloRevolution");
            auto* e = xml.createNewChildElement("PARAM");
            e->setAttribute("id", "swing");
            e->setAttribute("value", 0.25);
            juce::MemoryBlock block;
            juce::AudioProcessor::copyXmlToBinary(xml, block);
            auto q = make();
            q->setStateInformation(block.getData(), static_cast<int>(block.getSize()));
            expect(q->getRevolutionState().pattern == rev::generate({}, 1, rev::Musical));
            expectWithinAbsoluteError(q->parameterValue("swing"), 0.25, 1.0e-6);
        }

        beginTest("The pattern drives the DSP: a playing loop changes with the pattern, all-pass pattern = dry at MIX 1");
        {
            const auto loop = drums(4.0);
            auto a = make();
            const auto y = renderPlaying(*a, loop, 512);
            expect(differenceDb(y, loop) > -40.0, "the Default pattern changes the loop: " + juce::String(differenceDb(y, loop)));
            auto b = make();
            b->setRevolutionState({ rev::Pattern {}, {}, 1 }); // every step TimePass / GlitchPass / level 0
            const auto z = renderPlaying(*b, loop, 512);
            logMessage("  empty pattern vs dry: " + juce::String(differenceDb(z, loop), 1) + " dB");
            expect(differenceDb(z, loop) < -60.0, juce::String(differenceDb(z, loop)));
        }

        beginTest("Host blocks 64, 127, 4096 after prepare(256), playing: bit-identical (block 1: PluginTests, stopped)");
        {
            const auto loop = drums(2.0);
            auto ref = make();
            const auto expected = renderPlaying(*ref, loop, 256);
            for (int block : { 64, 127, 4096 })
            {
                auto p = make();
                PlayHead head;
                p->setPlayHead(&head);
                p->prepareToPlay(kFs, 256);
                auto y = loop;
                juce::MidiBuffer midi;
                for (int start = 0; start < y.getNumSamples(); start += block)
                {
                    const int n = juce::jmin(block, y.getNumSamples() - start);
                    head.ppq = static_cast<double>(start) * 120.0 / (60.0 * kFs);
                    juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
                    p->processBlock(view, midi);
                }
                p->setPlayHead(nullptr);
                expect(bitExact(y, expected), "block " + juce::String(block));
            }
        }

        beginTest("Page: GENERATE / NEW SEED / a step click reach the processor; the page shows the processor's pattern");
        {
            auto p = make();
            p->setPage(ModulePluginProcessor::Page::Advanced); // R4: a new instance opens SIMPLE; this is the ADVANCED page
            std::unique_ptr<juce::AudioProcessorEditor> e(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(e.get());
            auto* page = editor != nullptr ? dynamic_cast<hpeq::ui::HamloRevolutionPanel*>(editor->getPanel()) : nullptr;
            expect(page != nullptr);
            if (page == nullptr)
                return;
            expect(page->getValues().pattern == p->getRevolutionState().pattern, "shown");
            const auto before = p->getRevolutionState();
            page->pressNewSeed();
            expect(p->getRevolutionState().seed != before.seed, "new seed");
            page->pressGenerate();
            const auto afterGen = p->getRevolutionState();
            page->clickStep(rev::Glitch, 3);
            expect(p->getRevolutionState().pattern.steps[rev::Glitch][3] != afterGen.pattern.steps[rev::Glitch][3], "step click");
            // a program from the preset screen: the page follows
            int last = 1; // HPEQ's last program (R6's genre presets open SIMPLE: PresetTests)
            while (p->genrePresetFor(last + 1) == nullptr && last + 1 < p->getNumPrograms())
                ++last;
            p->setCurrentProgram(last);
            editor->getPresetScreen().setCurrent(last, false);
            p->setCurrentProgram(last);
            juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
            expect(page->getValues().pattern == p->getRevolutionState().pattern, "after a program");
        }
    }
};

static RevolutionTests revolutionTests;
} // namespace hamlo::test
