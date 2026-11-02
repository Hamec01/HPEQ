// R1 lanes and scenes (docs/REVOLUTION_V2.md section 6): GLITCH GAP (silence) marks where a lane plays, so the
// lane's length, condition, probability and rate are read straight off the output of a steady tone.

#include <cmath>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/Revolution.h"
#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include "UI/HamloRevolutionPanel.h"

namespace hamlo::test
{
namespace
{
using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;
namespace rev = hpeq::revolution;
using State = ModulePluginProcessor::RevolutionState;

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

std::unique_ptr<ModulePluginProcessor> gapsOnly()
{
    auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
    p->setParameterValue("timeOn", 0.0);
    p->setParameterValue("motionOn", 0.0);
    return p;
}

State gapAt(std::initializer_list<int> steps)
{
    State s;
    for (int k : steps)
        s.pattern.steps[rev::Glitch][static_cast<size_t>(k)] = rev::Gap;
    return s;
}

juce::AudioBuffer<float> tone(double seconds)
{
    juce::AudioBuffer<float> x(2, static_cast<int>(seconds * kFs));
    for (int i = 0; i < x.getNumSamples(); ++i)
        x.setSample(0, i, 0.5f * static_cast<float>(std::sin(2.0 * 3.14159265358979 * 997.0 * i / kFs))), x.setSample(1, i, x.getSample(0, i));
    return x;
}

juce::AudioBuffer<float> play(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x, int block = 512)
{
    Head head;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, juce::jmin(block, 512));
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        head.ppq = static_cast<double>(start) * 2.0 / kFs; // 120 BPM
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
    }
    p.setPlayHead(nullptr);
    return y;
}

// Is step k (of `stepSeconds`) silent in its middle half?
bool silentStep(const juce::AudioBuffer<float>& y, int k, double stepSeconds)
{
    const int a = static_cast<int>((k + 0.25) * stepSeconds * kFs), n = static_cast<int>(0.5 * stepSeconds * kFs);
    return y.getRMSLevel(0, a, n) < 1.0e-4f;
}
} // namespace

class LaneTests final : public juce::UnitTest
{
public:
    LaneTests() : juce::UnitTest("Lanes", "HamloRevolution") {}

    void runTest() override
    {
        const double sixteenth = 0.125; // seconds at 120 BPM

        beginTest("LENGTH 3: the lane repeats every 3 steps (a polymeter against STEPS 16)");
        {
            auto p = gapsOnly();
            auto s = gapAt({ 0 });
            s.lanes[rev::Glitch].length = 3;
            p->setRevolutionState(s);
            const auto y = play(*p, tone(4.0));
            juce::String wrong;
            for (int k = 0; k < 30; ++k)
                if (silentStep(y, k, sixteenth) != (k % 3 == 0))
                    wrong << k << " ";
            expect(wrong.isEmpty(), "steps out of place: " + wrong);
        }

        beginTest("LENGTH 32: steps 17 .. 32 are their own values");
        {
            auto p = gapsOnly();
            auto s = gapAt({ 2 });
            s.lanes[rev::Glitch].length = 32;
            s.lanes[rev::Glitch].extra[5] = rev::Gap; // step 22
            p->setRevolutionState(s);
            const auto y = play(*p, tone(8.5));
            juce::String wrong;
            for (int k = 0; k < 64; ++k)
                if (silentStep(y, k, sixteenth) != (k % 32 == 2 || k % 32 == 21))
                    wrong << k << " ";
            expect(wrong.isEmpty(), "steps out of place: " + wrong);
        }

        beginTest("CONDITION 1:2 (length 4): the even cycles play, the odd ones pass; 4:4 only every 4th cycle");
        for (const auto& [condition, period, which] : { std::tuple<int, int, int> { rev::OneOfTwo, 2, 0 }, { rev::FourOfFour, 4, 3 } })
        {
            auto p = gapsOnly();
            auto s = gapAt({ 0 });
            s.lanes[rev::Glitch].length = 4;
            s.lanes[rev::Glitch].condition = condition;
            p->setRevolutionState(s);
            const auto y = play(*p, tone(5.0));
            juce::String wrong;
            for (int k = 0; k < 36; ++k)
                if (silentStep(y, k, sixteenth) != (k % 4 == 0 && (k / 4) % period == which))
                    wrong << k << " ";
            expect(wrong.isEmpty(), juce::String(rev::laneConditionName(condition)) + ": steps out of place: " + wrong);
        }

        beginTest("PROBABILITY 50 %: 40 .. 60 % of the events over 64 cycles; 100 %: all; the same on a second run");
        {
            const auto measure = [&](int code)
            {
                auto p = gapsOnly();
                auto s = gapAt({ 0, 2 });
                s.lanes[rev::Glitch].length = 4;
                s.lanes[rev::Glitch].rate = 4; // 1/32: 64 cycles of 4 steps in 16 s (after a first cycle)
                s.lanes[rev::Glitch].probability[0] = s.lanes[rev::Glitch].probability[2] = static_cast<std::uint8_t>(code);
                p->setRevolutionState(s);
                const auto y = play(*p, tone(16.6));
                juce::String pattern;
                int hits = 0;
                for (int k = 4; k < 260; k += 2)
                {
                    const bool g = silentStep(y, k, sixteenth / 2.0);
                    hits += g ? 1 : 0;
                    pattern << (g ? "x" : ".");
                }
                return std::pair<int, juce::String> { hits, pattern };
            };
            const auto half = measure(4), again = measure(4), all = measure(0);
            logMessage("  50 %: " + juce::String(half.first) + " / 128 events " + half.second);
            logMessage("  100 %: " + all.second);
            expect(half.first >= 51 && half.first <= 77, juce::String(half.first));
            expect(half.second == again.second, "deterministic");
            expectEquals(all.first, 128);
        }

        beginTest("V2: RATE 1/8 on a 1/16 grid with swing: the gap starts within 1 sample of the lane's swung step");
        {
            auto p = gapsOnly();
            p->setParameterValue("swing", 0.6); // swing 0.65 of a pair
            auto s = gapAt({ 1 });
            s.lanes[rev::Glitch].length = 2;
            s.lanes[rev::Glitch].rate = 2; // 1/8
            p->setRevolutionState(s);
            const auto x = tone(2.0);
            const auto y = play(*p, x, 128);
            // lane steps of 0.5 quarter (0.25 s); odd steps start 0.65 x a pair (0.5 s) into the pair: 0.325 s, 0.825 s
            for (const double t : { 0.325, 0.825, 1.325 })
            {
                const int from = static_cast<int>((t - 0.05) * kFs);
                int first = -1;
                for (int i = from; i < from + static_cast<int>(0.1 * kFs) && first < 0; ++i)
                    if (std::abs(y.getSample(0, i) - x.getSample(0, i)) > 1.0e-6f)
                        first = i;
                const int expected = static_cast<int>(std::ceil(t * kFs - 1.0e-4));
                logMessage("  " + juce::String(t, 3) + " s: first changed sample " + juce::String(first) + ", step at " + juce::String(expected));
                expect(std::abs(first - expected) <= 1, juce::String(first) + " vs " + juce::String(expected));
            }
        }

        beginTest("SCENE: scene 2 plays its own pattern; an unedited scene = scene 1 bit for bit");
        {
            auto p = gapsOnly();
            p->setRevolutionState(gapAt({}));          // scene 1: nothing
            p->setScene(1, gapAt({ 0, 4, 8, 12 }));     // scene 2: gaps on the beats
            p->setParameterValue("scene", 1.0);
            const auto x = tone(2.0);
            const auto y = play(*p, x);
            int gaps = 0;
            for (int k = 0; k < 15; ++k)
                gaps += silentStep(y, k, sixteenth) ? 1 : 0;
            expectEquals(gaps, 4);
            p->setParameterValue("scene", 0.0);
            expect(differenceDb(play(*p, x), x) < -100.0, "scene 1 = dry");
            auto q = gapsOnly();
            q->setRevolutionState(gapAt({ 3 }));
            const auto one = play(*q, x);
            q->setParameterValue("scene", 5.0);
            expect(bitExact(play(*q, x), one), "scene 6 (never edited) = scene 1");
            q->setScene(5, gapAt({ 7 }));
            q->clearScene(5);
            expect(bitExact(play(*q, x), one), "cleared = scene 1 again");
        }

        beginTest("V3: blocks 1, 64, 127, 4096 after prepare(256) bit-identical with lanes, rates, conditions, chance on");
        {
            const auto setUp = [](ModulePluginProcessor& p)
            {
                p.setCurrentProgram(2);
                auto s = p.getRevolutionState();
                s.lanes[rev::Time] = { 5, 6, 0, {}, {} };            // 5 steps of 1/8T
                s.lanes[rev::Glitch] = { 3, 0, rev::NotOneOfFour, {}, {} };
                s.lanes[rev::Glitch].probability = { 3, 0, 5 };
                s.lanes[rev::Motion].length = 32;
                s.lanes[rev::Motion].extra = { 15, 0, 7, 0, 3 };
                p.setRevolutionState(s);
            };
            const auto x = drums(3.0);
            const auto render = [&](int block)
            {
                auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
                setUp(*p);
                Head head;
                p->setPlayHead(&head);
                p->prepareToPlay(kFs, 256);
                auto y = x;
                juce::MidiBuffer midi;
                for (int start = 0; start < y.getNumSamples(); start += block)
                {
                    const int n = juce::jmin(block, y.getNumSamples() - start);
                    head.ppq = static_cast<double>(start) * 2.0 / kFs;
                    juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
                    p->processBlock(view, midi);
                }
                return y;
            };
            const auto expected = render(256);
            expect(differenceDb(expected, x) > -40.0, "the lanes do something");
            for (int block : { 1, 64, 127, 4096 })
                expect(bitExact(render(block), expected), "block " + juce::String(block));
        }

        beginTest("V6: latency 0 with lanes; V13: scenes and lanes round trip, an R0 state loads with lanes at defaults");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            p->prepareToPlay(kFs, 512);
            expectEquals(p->getLatencySamples(), 0);
            auto s = gapAt({ 1, 5 });
            s.lanes[rev::Glitch] = { 7, 3, rev::EightOfEight, {}, {} };
            s.lanes[rev::Glitch].extra[3] = rev::Ratchet;
            s.lanes[rev::Glitch].probability[1] = 6;
            s.lanes[rev::Time].rate = 8;
            p->setRevolutionState(s);
            p->setScene(3, gapAt({ 9 }));
            juce::MemoryBlock saved;
            p->getStateInformation(saved);
            auto q = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            q->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            expect(q->getScene(0).lanes == s.lanes, "lanes");
            expect(q->getScene(0).pattern == s.pattern, "scene 1 pattern");
            expect(q->isSceneUsed(3) && q->getScene(3).pattern == gapAt({ 9 }).pattern, "scene 4");
            expect(!q->isSceneUsed(2), "scene 3 still follows");
            expect(q->currentSlot(true).values == p->currentSlot(true).values, "the slots");

            // R0's saved form: no lane attributes, no scenes
            juce::XmlElement xml("HamloRevolution");
            auto* e = xml.createNewChildElement("REVOLUTION");
            juce::NamedValueSet set;
            rev::writePattern(set, gapAt({ 2 }).pattern);
            set.set("revSeed", 7);
            for (const auto& v : set)
                e->setAttribute(v.name, v.value.toString());
            juce::MemoryBlock old;
            juce::AudioProcessor::copyXmlToBinary(xml, old);
            q->setStateInformation(old.getData(), static_cast<int>(old.getSize()));
            expect(q->getScene(0).lanes == rev::Lanes {}, "lanes at defaults");
            expect(q->getScene(0).pattern.steps[rev::Glitch][2] == rev::Gap);
            expect(!q->isSceneUsed(3), "scenes cleared");
        }

        beginTest("Page: lane menus, probability, steps 17 .. 32 and SCENE reach the processor");
        {
            auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
            std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(ed.get());
            auto* page = editor != nullptr ? dynamic_cast<hpeq::ui::HamloRevolutionPanel*>(editor->getPanel()) : nullptr;
            expect(page != nullptr);
            if (page == nullptr)
                return;
            const auto patternBefore = p->getRevolutionState().pattern;
            page->setLaneLength(rev::Glitch, 24);
            page->setLaneRate(rev::Glitch, 2);
            page->setLaneCondition(rev::Time, rev::TwoOfTwo);
            page->setStepProbability(rev::Motion, 30, 5);
            page->showSecondHalf(rev::Glitch, true);
            page->clickStep(rev::Glitch, 2); // step 19
            const auto s = p->getRevolutionState();
            expectEquals(s.lanes[rev::Glitch].length, 24);
            expectEquals(s.lanes[rev::Glitch].rate, 2);
            expectEquals(s.lanes[rev::Time].condition, static_cast<int>(rev::TwoOfTwo));
            expectEquals(static_cast<int>(s.lanes[rev::Motion].probability[30]), 5);
            expectEquals(static_cast<int>(s.lanes[rev::Glitch].extra[2]), 1);
            expect(s.pattern == patternBefore, "steps 1 .. 16 untouched");
            page->pressGenerate(); // GENERATE keeps the lanes
            expect(p->getRevolutionState().lanes == s.lanes, "lanes kept by GENERATE");
            page->selectScene(2);
            expectEquals(p->currentScene(), 2);
            expect(p->getRevolutionState().lanes == s.lanes, "scene 3 (unedited) shows scene 1");
            page->setLaneLength(rev::Time, 5); // editing scene 3 makes it its own
            expect(p->isSceneUsed(2) && p->getScene(0).lanes[rev::Time].length == 0, "scene 1 unchanged");
        }
    }
};

static LaneTests laneTests;
} // namespace hamlo::test
