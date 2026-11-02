// R6 (docs/REVOLUTION_V2.md section 11): the 200 genre presets, painting steps on the ADVANCED page, the help texts.

#include <cmath>
#include <memory>
#include <set>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/Revolution.h"
#include "Core/RevolutionPresets.h"
#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include "UI/HamloRevolutionPanel.h"
#include "UI/HamloRevolutionSimplePage.h"
#include "UI/RevolutionHelp.h"

namespace hamlo::test
{
namespace
{
using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;
namespace rev = hpeq::revolution;
constexpr int kLegacy = 24;

class Head final : public juce::AudioPlayHead
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

juce::AudioBuffer<float> play(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x)
{
    Head head;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, 512);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        const int n = juce::jmin(512, y.getNumSamples() - start);
        head.ppq = static_cast<double>(start) * 2.0 / kFs;
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
    }
    p.setPlayHead(nullptr);
    return y;
}

juce::MouseEvent mouse(juce::Component& c, juce::Point<float> at, juce::Point<float> down, bool pressed)
{
    auto source = juce::Desktop::getInstance().getMainMouseSource();
    return juce::MouseEvent(source, at, pressed ? juce::ModifierKeys::leftButtonModifier : juce::ModifierKeys(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, &c, &c,
                            juce::Time::getCurrentTime(), down, juce::Time::getCurrentTime(), 1, false);
}
} // namespace

class PresetTests final : public juce::UnitTest
{
public:
    PresetTests() : juce::UnitTest("Presets", "HamloRevolution") {}

    void runTest() override
    {
        beginTest("225 programs; unique names; HPEQ's 24 where they were");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            expectEquals(p->getNumPrograms(), 1 + kLegacy + 200);
            std::set<juce::String> names;
            for (int i = 1; i < p->getNumPrograms(); ++i)
                names.insert(p->getProgramName(i));
            expectEquals(static_cast<int>(names.size()), p->getNumPrograms() - 1);
            expect(p->genrePresetFor(kLegacy) == nullptr && p->genrePresetFor(kLegacy + 1) != nullptr);
        }

        beginTest("Every genre preset: deterministic, SIMPLE, writes events, finite and bounded on the drum loop");
        {
            const auto x = drums(2.0);
            int good = 0, worstProgram = 0;
            double worstPeak = 0.0;
            for (int program = kLegacy + 1; program <= kLegacy + 200; ++program)
            {
                auto a = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
                a->setPage(ModulePluginProcessor::Page::Advanced);
                a->setCurrentProgram(program);
                auto b = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
                b->setCurrentProgram(program);
                b->setCurrentProgram(program);
                const bool same = a->currentSlot(true).values == b->currentSlot(true).values && a->getRevolutionState().lanes == b->getRevolutionState().lanes;
                const auto y = play(*a, x);
                bool finite = true;
                double peak = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < y.getNumSamples(); ++i)
                    {
                        const float v = y.getSample(ch, i);
                        finite = finite && std::isfinite(v);
                        peak = std::max(peak, static_cast<double>(std::abs(v)));
                    }
                if (peak > worstPeak)
                {
                    worstPeak = peak;
                    worstProgram = program;
                }
                const bool ok = same && a->getPage() == ModulePluginProcessor::Page::Simple && a->lastSimple().events >= 1 && finite && peak <= 2.0;
                good += ok;
                if (!ok)
                    logMessage("  " + a->getProgramName(program) + ": same " + juce::String(static_cast<int>(same)) + ", events "
                               + juce::String(a->lastSimple().events) + ", peak " + juce::String(20.0 * std::log10(peak + 1.0e-12), 1) + " dBFS");
            }
            logMessage("  " + juce::String(good) + " / 200 pass; the loudest: " + juce::String(20.0 * std::log10(worstPeak), 2) + " dBFS ("
                       + std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution)->getProgramName(worstProgram) + ")");
            expectEquals(good, 200);
        }

        beginTest("The preset screen: 4 genres + Factory; each genre 50 presets in 6 groups");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(ed.get());
            expect(editor != nullptr);
            if (editor == nullptr)
                return;
            auto& screen = editor->getPresetScreen();
            screen.setFilter(hpeq::ui::PresetScreen::kFactory, "All");
            expectEquals(static_cast<int>(screen.filtered().size()), kLegacy);
            const std::pair<const char*, int> groups[] { { "Drum Bus", 10 }, { "Hats / Percussion", 9 }, { "Loops / Samples", 9 },
                                                         { "Vocals", 7 }, { "Transitions / Fills", 9 }, { "Master FX", 6 } };
            for (const char* genre : { "Boom Bap", "Trap", "Techno", "DnB" })
            {
                screen.setFilter(genre, "All");
                expectEquals(static_cast<int>(screen.filtered().size()), 50, genre);
                for (const auto& [group, count] : groups)
                {
                    screen.setFilter(genre, group);
                    expectEquals(static_cast<int>(screen.filtered().size()), count, juce::String(genre) + " / " + group);
                }
            }
            // a preset chosen there loads (and opens SIMPLE)
            p->setPage(ModulePluginProcessor::Page::Advanced);
            screen.setFilter("Techno", "Drum Bus");
            screen.setCurrent(screen.filtered().front(), false);
            screen.step(1);
            expectEquals(p->getCurrentProgram(), screen.filtered()[1]);
            expect(p->getPage() == ModulePluginProcessor::Page::Simple);
        }

        beginTest("Painting: a drag over GLITCH steps 3 .. 7 sets all five to the first step's new value");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->setPage(ModulePluginProcessor::Page::Advanced);
            p->setRevolutionState({});
            std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(ed.get());
            auto* page = editor != nullptr ? dynamic_cast<hpeq::ui::HamloRevolutionPanel*>(editor->getPanel()) : nullptr;
            expect(page != nullptr);
            if (page == nullptr)
                return;
            const float scale = static_cast<float>(page->getWidth()) / 1180.0f;
            const auto cell = [scale](int s) { return juce::Point<float>(420.5f + 44.0f * static_cast<float>(s), 420.0f + 58.0f) * scale; };
            const auto down = cell(2);
            page->mouseDown(mouse(*page, down, down, true));
            for (int s = 3; s <= 6; ++s)
                page->mouseDrag(mouse(*page, cell(s), down, true));
            page->mouseUp(mouse(*page, cell(6), down, false));
            const auto& steps = p->getRevolutionState().pattern.steps[rev::Glitch];
            juce::String row;
            for (int s = 0; s < 8; ++s)
                row << static_cast<int>(steps[static_cast<size_t>(s)]) << " ";
            logMessage("  GLITCH steps 1 .. 8: " + row);
            expectEquals(static_cast<int>(steps[2]), 1);
            for (int s = 3; s <= 6; ++s)
                expectEquals(static_cast<int>(steps[static_cast<size_t>(s)]), static_cast<int>(steps[2]));
            expectEquals(static_cast<int>(steps[7]), 0);
        }

        beginTest("Help: every control of both pages has a text");
        {
            using Panel = hpeq::ui::HamloRevolutionPanel;
            using Simple = hpeq::ui::HamloRevolutionSimplePage;
            juce::StringArray missing;
            for (int i = 0; i < Panel::NumParams; ++i)
                if (hpeq::ui::revolutionHelp(Panel::keyFor(static_cast<Panel::Param>(i))).isEmpty())
                    missing.add(Panel::keyFor(static_cast<Panel::Param>(i)));
            for (int i = 0; i < Panel::NumSwitches; ++i)
                if (hpeq::ui::revolutionHelp(Panel::keyFor(static_cast<Panel::Switch>(i))).isEmpty())
                    missing.add(Panel::keyFor(static_cast<Panel::Switch>(i)));
            for (int i = 0; i < Simple::NumKnobs; ++i)
                if (hpeq::ui::revolutionHelp(Simple::keyFor(static_cast<Simple::Knob>(i))).isEmpty())
                    missing.add(Simple::keyFor(static_cast<Simple::Knob>(i)));
            for (const char* key : { "simpleGenre", "simpleFill", "simpleTarget", "simpleLock", "connect", "simpleGenerate", "advancedPage",
                                     "simplePage", "preview", "generate", "scene", "lanePlate", "laneCells", "laneGenerate", "laneLock" })
                if (hpeq::ui::revolutionHelp(key).isEmpty())
                    missing.add(key);
            logMessage("  missing: " + (missing.isEmpty() ? juce::String("none") : missing.joinIntoString(", ")));
            expect(missing.isEmpty());
        }
    }
};

static PresetTests presetTests;
} // namespace hamlo::test
