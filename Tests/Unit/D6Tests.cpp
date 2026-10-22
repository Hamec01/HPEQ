// D6 (owner feedback F1-F3): modules on EQ points, stable engine slots, module-grid inspector.
// Thresholds fixed here before the runs.

#include <cmath>
#include <memory>

#include "DSP/ProjectEngine.h"
#include "Harness/AudioMetrics.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"

namespace hpeq::test
{
namespace
{
constexpr double kFs = 48000.0;

FxSlotState module(FxType type, std::initializer_list<std::pair<const char*, double>> values = {})
{
    FxSlotState s;
    s.type = type;
    for (const auto& [key, value] : values)
        s.parameters.set(juce::Identifier(key), value);
    return s;
}

juce::AudioBuffer<float> render(const FxProjectState& project, const juce::AudioBuffer<float>& x,
                                const std::function<void(dsp::ProjectEngine&, int)>& beforeBlock = {})
{
    auto e = std::make_unique<dsp::ProjectEngine>();
    e->prepare(kFs, 512);
    e->publish(project);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        if (beforeBlock)
            beforeBlock(*e, start);
        float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        e->process(c, 2, juce::jmin(512, y.getNumSamples() - start));
    }
    return y;
}

void collectButtons(juce::Component& root, juce::Array<juce::TextButton*>& out)
{
    for (auto* child : root.getChildren())
    {
        if (auto* b = dynamic_cast<juce::TextButton*>(child))
            out.add(b);
        collectButtons(*child, out);
    }
}
} // namespace

class D6Tests final : public juce::UnitTest
{
public:
    D6Tests() : juce::UnitTest("Modules on points D6", "HPEQ") {}

    void runTest() override
    {
        beginTest("Point module band: edges of a band-pass with the point's frequency and Q (centre f, f / bandwidth = Q)");
        {
            for (const auto& [f, q] : { std::pair { 1000.0, 1.0 }, std::pair { 80.0, 0.5 }, std::pair { 6000.0, 4.0 } })
            {
                EqNodeState n;
                n.frequencyHz = f;
                n.q = q;
                const auto [lo, hi] = pointModuleBandHz(n);
                expectWithinAbsoluteError(std::sqrt(lo * hi), f, f * 1.0e-12);
                expectWithinAbsoluteError(f / (hi - lo), q, q * 1.0e-12);
            }
            EqNodeState n;
            const auto [lo, hi] = pointModuleBandHz(n); // 1 kHz, Q 0.707
            logMessage("  1 kHz, Q 0.707: " + juce::String(lo, 1) + " - " + juce::String(hi, 1) + " Hz");
        }

        beginTest("A compressor on an EQ point sounds bit-exactly like an FX region with the same band");
        {
            FxProjectState onPoint;
            EqNodeState bell;
            bell.id = 1;
            bell.frequencyHz = 900.0;
            bell.gainDb = 4.0;
            bell.q = 1.2;
            bell.chain = { module(FxType::Compressor, { { "thresholdDb", -35.0 }, { "ratio", 6.0 } }) };
            onPoint.layers[0].eqNodes = { bell };
            onPoint.nextNodeId = 3;
            sanitize(onPoint);

            FxProjectState asRegion;
            auto plainBell = bell;
            plainBell.chain.clear();
            asRegion.layers[0].eqNodes = { plainBell };
            FxNodeState region;
            region.id = 2;
            std::tie(region.lowHz, region.highHz) = pointModuleBandHz(bell);
            region.chain = bell.chain;
            asRegion.layers[0].fxNodes = { region };
            asRegion.nextNodeId = 3;
            sanitize(asRegion);

            const auto drums = makeSyntheticDrums({ kFs, 2, 2.0, kFixtureSeed });
            const auto a = render(onPoint, drums), b = render(asRegion, drums);
            expect(compareAligned(a, b).bitExact, "point modules differ from the equivalent region");
            FxProjectState bellOnly;
            bellOnly.layers[0].eqNodes = { plainBell };
            sanitize(bellOnly);
            expect(compareAligned(a, render(bellOnly, drums)).errorDb > -60.0, "the point's compressor had no audible effect");
        }

        beginTest("Point off = its modules off; point modules follow the point's channel component");
        {
            LayerState layer;
            EqNodeState p;
            p.id = 1;
            p.enabled = false;
            p.channel = ChannelMode::Side;
            p.chain = { module(FxType::Gain, { { "gainDb", -6.0 } }) };
            layer.eqNodes = { p };
            assignEngineSlots(layer);
            const auto s = dsp::makeSnapshot(layer);
            expect(!s.fx[0].active && s.fx[0].numSlots == 1 && s.fx[0].channel == ChannelMode::Side);
        }

        beginTest("Stable engine slots: deleting a region does not move the others; a new one takes the free slot; points share the 8");
        {
            FxProjectState p;
            for (int i = 0; i < 3; ++i)
            {
                FxNodeState n;
                n.id = static_cast<std::uint32_t>(10 + i);
                n.lowHz = 200.0 * (i + 1);
                n.highHz = n.lowHz * 2.0;
                p.layers[0].fxNodes.push_back(n);
            }
            sanitize(p);
            expect(p.layers[0].fxNodes[0].engineSlot == 0 && p.layers[0].fxNodes[1].engineSlot == 1 && p.layers[0].fxNodes[2].engineSlot == 2);
            auto deleted = p;
            deleted.layers[0].fxNodes.erase(deleted.layers[0].fxNodes.begin());
            sanitize(deleted);
            expect(deleted.layers[0].fxNodes[0].engineSlot == 1 && deleted.layers[0].fxNodes[1].engineSlot == 2, "remaining regions moved");
            FxNodeState added;
            added.id = 20;
            deleted.layers[0].fxNodes.push_back(added);
            sanitize(deleted);
            expectEquals(deleted.layers[0].fxNodes[2].engineSlot, 0);

            // In the engine: region 2 (slot 1) keeps running while region 1 is deleted.
            const auto drums = makeSyntheticDrums({ kFs, 2, 1.0, kFixtureSeed });
            bool slot1RunningAfter = false, slot0StoppedAfter = false;
            auto withComp = p;
            for (auto& n : withComp.layers[0].fxNodes)
                n.chain = { module(FxType::Compressor, { { "thresholdDb", -40.0 } }) };
            auto withoutFirst = withComp;
            withoutFirst.layers[0].fxNodes.erase(withoutFirst.layers[0].fxNodes.begin());
            sanitize(withoutFirst);
            render(withComp, drums, [&](dsp::ProjectEngine& e, int start)
            {
                if (start == 12288)
                    e.publish(withoutFirst);
                if (start == 36864)
                {
                    slot1RunningAfter = e.getLayer(0).getNode(1).isRunning();
                    slot0StoppedAfter = !e.getLayer(0).getNode(0).isRunning();
                }
            });
            expect(slot1RunningAfter && slot0StoppedAfter);

            // 8 regions use every engine node: a point's modules do not fit and are dropped by sanitize.
            FxProjectState full;
            for (int i = 0; i < kMaxFxNodesPerLayer; ++i)
            {
                FxNodeState n;
                n.id = static_cast<std::uint32_t>(30 + i);
                full.layers[0].fxNodes.push_back(n);
            }
            EqNodeState point;
            point.id = 50;
            point.chain = { module(FxType::Reverb) };
            full.layers[0].eqNodes = { point };
            expectEquals(freeEngineSlots(full.layers[0]), 0);
            sanitize(full);
            expect(full.layers[0].eqNodes[0].chain.empty() && full.layers[0].eqNodes[0].engineSlot == -1);
        }

        beginTest("State: point modules and engine slots round-trip through the saved state");
        {
            FxProjectState p;
            EqNodeState point;
            point.id = 1;
            point.chain = { module(FxType::Saturator, { { "driveDb", 9.5 } }), module(FxType::Delay, { { "sync", 6 } }) };
            p.layers[2].eqNodes = { point };
            FxNodeState region;
            region.id = 2;
            region.engineSlot = 5;
            p.layers[2].fxNodes = { region };
            p.nextNodeId = 3;
            sanitize(p);
            const auto xml = toValueTree(p).createXml();
            const auto back = fromValueTree(juce::ValueTree::fromXml(*xml));
            expect(back.has_value());
            if (back)
            {
                expect(toValueTree(*back).isEquivalentTo(toValueTree(p)));
                expect(back->layers[2].eqNodes[0].chain.size() == 2 && back->layers[2].eqNodes[0].engineSlot == 0);
                expectEquals(back->layers[2].fxNodes[0].engineSlot, 5);
            }
        }

        beginTest("Inspector: a point shows a 3 x 2 module grid ('+' for empty tiles); a tile opens the module page with a back arrow");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            EqNodeState point;
            point.id = 1;
            point.chain = { module(FxType::Compressor), module(FxType::Reverb) };
            p.layers[0].eqNodes = { point };
            p.nextNodeId = 2;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1400, 900);
            const auto buttonsWithText = [&](const juce::String& text, bool exact)
            {
                juce::Array<juce::TextButton*> all;
                collectButtons(*editor, all);
                int n = 0;
                for (auto* b : all)
                    n += (exact ? b->getButtonText() == text : b->getButtonText().contains(text)) && b->isVisible() ? 1 : 0;
                return n;
            };
            editor->selectNode(false, 1, -1);
            expectEquals(buttonsWithText("+", true), 4);
            expectEquals(buttonsWithText("Compressor", false), 1);
            expectEquals(buttonsWithText("Reverb", false), 1);
            editor->selectNode(false, 1, 1);
            expectEquals(buttonsWithText("Modules", false), 1); // back arrow
            {
                // The module page shows the module's values (type, On).
                juce::Array<juce::Component*> stack { editor.get() };
                juce::StringArray comboTexts;
                bool onTicked = false;
                while (!stack.isEmpty())
                {
                    auto* c = stack.removeAndReturn(stack.size() - 1);
                    for (auto* child : c->getChildren())
                        stack.add(child);
                    if (auto* combo = dynamic_cast<juce::ComboBox*>(c))
                        comboTexts.add(combo->getText());
                    if (auto* toggle = dynamic_cast<juce::ToggleButton*>(c); toggle != nullptr && toggle->getButtonText() == "On")
                        onTicked = onTicked || toggle->getToggleState();
                }
                logMessage("  combos on the module page: " + comboTexts.joinIntoString(" | "));
                expect(comboTexts.contains("Reverb"), "module type not shown");
                expect(onTicked, "module On not shown");
            }
            expectEquals(buttonsWithText("+", true), 0);
        }
    }
};

static D6Tests d6Tests;
} // namespace hpeq::test
