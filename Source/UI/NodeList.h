#pragma once

#include <functional>

#include "Plugin/FxPluginProcessor.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
// Left panel: every EQ point and FX region of the selected layer, in processing order (PRE-EQ first,
// decision B), then the layer's MIX chain (D7: modules on the whole layer). Click selects; the dot
// toggles enable; x deletes; "+" adds an EQ point, an FX region or a module on the whole layer, each
// with the full list. Painted, no child components.
class NodeList final : public juce::Component
{
public:
    NodeList(FxPluginProcessor& p, Selection& s) : processor(p), selection(s) {}

    std::function<void()> onSelectionChanged;

    void paint(juce::Graphics& g) override
    {
        const auto project = processor.getProject();
        const auto& layer = selectedLayer(project);
        auto area = getLocalBounds().toFloat();
        g.setColour(colours::panel);
        g.fillRoundedRectangle(area, 6.0f);
        auto header = area.removeFromTop(kRow + 4.0f).reduced(10.0f, 4.0f);
        g.setColour(colours::text);
        g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
        g.drawText("NODES  (Layer " + juce::String(project.selectedLayer + 1) + ")", header, juce::Justification::centredLeft);
        g.drawText("+", header.removeFromRight(20.0f), juce::Justification::centred);

        int row = 0;
        g.setFont(12.0f);
        const auto drawRow = [&](juce::Colour colour, const juce::String& text, bool enabled, bool selected, bool controls)
        {
            auto r = rowArea(row++);
            if (selected)
            {
                g.setColour(colours::panelRaised);
                g.fillRoundedRectangle(r, 4.0f);
                g.setColour(colour.withAlpha(0.8f));
                g.drawRoundedRectangle(r, 4.0f, 1.0f);
            }
            g.setColour(enabled ? colour : colours::dim.withAlpha(0.4f));
            g.fillEllipse(r.getX() + 6, r.getCentreY() - 5, 10, 10);
            g.setColour(enabled ? colours::text : colours::dim);
            g.drawText(text, r.withTrimmedLeft(24.0f).withTrimmedRight(controls ? 44.0f : 6.0f), juce::Justification::centredLeft);
            if (!controls)
                return;
            g.setColour(colours::dim);
            g.drawText(enabled ? "on" : "off", r.withLeft(r.getRight() - 44).withWidth(22), juce::Justification::centred);
            g.drawText("x", r.withLeft(r.getRight() - 20), juce::Justification::centred);
        };
        for (int i = 0; i < static_cast<int>(layer.eqNodes.size()); ++i)
        {
            const auto& n = layer.eqNodes[static_cast<size_t>(i)];
            juce::String text = "EQ " + juce::String(i + 1) + "  " + shapeName(n.shape) + "  " + formatHz(n.frequencyHz);
            if (isGainShapeUi(n.shape))
                text << "  " << juce::String(n.gainDb, 1) << " dB";
            if (!n.chain.empty())
                text << "  +" << static_cast<int>(n.chain.size()) << (n.chain.size() == 1 ? " module" : " modules");
            drawRow(colours::eqNode, text, n.enabled, selection.isEq(n.id), true);
        }
        for (int i = 0; i < static_cast<int>(layer.fxNodes.size()); ++i)
        {
            const auto& n = layer.fxNodes[static_cast<size_t>(i)];
            drawRow(colours::forNode(i), "FX " + juce::String(i + 1) + "  " + nodeTitle(n), n.enabled, selection.isFx(n.id), true);
        }
        juce::StringArray names;
        for (const auto& m : layer.mixChain)
            names.add(fxName(m.type));
        drawRow(colours::accent, "MIX  whole layer" + (names.isEmpty() ? juce::String("  (empty)") : ":  " + names.joinIntoString(" > ")),
                !layer.mixChain.empty(), selection.kind == Selection::Kind::Mix, false);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.position.y < kRow + 4.0f && e.position.x > static_cast<float>(getWidth()) - 34.0f)
        {
            showAddMenu();
            return;
        }
        const auto project = processor.getProject();
        const auto& layer = selectedLayer(project);
        const int row = static_cast<int>((e.position.y - (kRow + 6.0f)) / kRow);
        const int eqCount = static_cast<int>(layer.eqNodes.size());
        const int total = eqCount + static_cast<int>(layer.fxNodes.size());
        if (row == total)
        {
            selection.kind = Selection::Kind::Mix;
            selection.id = 0;
            selection.slot = -1;
            if (onSelectionChanged)
                onSelectionChanged();
            repaint();
            return;
        }
        if (row < 0 || row > total)
            return;
        const bool isEq = row < eqCount;
        const auto id = isEq ? layer.eqNodes[static_cast<size_t>(row)].id : layer.fxNodes[static_cast<size_t>(row - eqCount)].id;
        const auto r = rowArea(row);
        if (e.position.x > r.getRight() - 20)
        {
            processor.getUndoManager().beginNewTransaction("Delete node");
            processor.editProject("Delete node", [&](FxProjectState& p)
            {
                auto& l = selectedLayer(p);
                if (isEq) l.eqNodes.erase(l.eqNodes.begin() + row);
                else l.fxNodes.erase(l.fxNodes.begin() + (row - eqCount));
            });
            selection = {};
        }
        else if (e.position.x > r.getRight() - 44)
        {
            processor.getUndoManager().beginNewTransaction("Enable node");
            processor.editProject("Enable node", [&](FxProjectState& p)
            {
                auto& l = selectedLayer(p);
                if (isEq) l.eqNodes[static_cast<size_t>(row)].enabled = !l.eqNodes[static_cast<size_t>(row)].enabled;
                else l.fxNodes[static_cast<size_t>(row - eqCount)].enabled = !l.fxNodes[static_cast<size_t>(row - eqCount)].enabled;
            });
        }
        else
        {
            selection.kind = isEq ? Selection::Kind::Eq : Selection::Kind::Fx;
            selection.id = id;
            selection.slot = -1;
        }
        if (onSelectionChanged)
            onSelectionChanged();
        repaint();
    }

    // The menu "+" opens (also used by tests): ids 100 + shape, 200 + module (region), 300 + module (whole layer).
    static juce::PopupMenu buildAddMenu(const LayerState& layer)
    {
        juce::PopupMenu eq, region, mix, menu;
        for (size_t i = 0; i < kAllShapes.size(); ++i)
            eq.addItem(static_cast<int>(100 + i), shapeName(kAllShapes[i]));
        for (size_t i = 0; i < kAllModules.size(); ++i)
        {
            region.addItem(static_cast<int>(200 + i), fxName(kAllModules[i]));
            mix.addItem(static_cast<int>(300 + i), fxName(kAllModules[i]));
        }
        menu.addSubMenu("EQ point at 1 kHz", eq, layer.eqNodes.size() < static_cast<size_t>(kMaxEqNodesPerLayer));
        menu.addSubMenu("FX region 250 - 700 Hz", region, freeEngineSlots(layer) > 0);
        menu.addSubMenu("Module on the whole layer", mix, layer.mixChain.size() < static_cast<size_t>(kMaxFxPerNode));
        return menu;
    }

    // Applies a result of buildAddMenu; returns what to select.
    static Selection applyAddMenu(FxProjectState& p, int result)
    {
        Selection next;
        auto& l = selectedLayer(p);
        if (result >= 100 && result < 200 && l.eqNodes.size() < static_cast<size_t>(kMaxEqNodesPerLayer))
        {
            EqNodeState n;
            n.id = p.nextNodeId++;
            n.shape = kAllShapes[static_cast<size_t>(result - 100)];
            n.frequencyHz = 1000.0;
            n.gainDb = isGainShapeUi(n.shape) ? 3.0 : 0.0;
            n.slopeDbPerOct = 24;
            l.eqNodes.push_back(n);
            next = { Selection::Kind::Eq, n.id, -1 };
        }
        else if (result >= 200 && result < 300 && freeEngineSlots(l) > 0)
        {
            FxNodeState n;
            n.id = p.nextNodeId++;
            n.lowHz = 250.0;
            n.highHz = 700.0;
            n.chain.front() = defaultModule(kAllModules[static_cast<size_t>(result - 200)]);
            l.fxNodes.push_back(n);
            next = { Selection::Kind::Fx, n.id, -1 };
        }
        else if (result >= 300 && l.mixChain.size() < static_cast<size_t>(kMaxFxPerNode))
        {
            l.mixChain.push_back(defaultModule(kAllModules[static_cast<size_t>(result - 300)]));
            next = { Selection::Kind::Mix, 0, static_cast<int>(l.mixChain.size()) - 1 };
        }
        return next;
    }

private:
    static constexpr float kRow = 24.0f;
    static inline const std::array<EqShape, 9> kAllShapes { EqShape::Bell, EqShape::LowShelf, EqShape::HighShelf, EqShape::LowCut, EqShape::HighCut,
                                                            EqShape::Notch, EqShape::BandPass, EqShape::Tilt, EqShape::AllPass };
    static inline const std::array<FxType, 10> kAllModules { FxType::Compressor, FxType::Limiter, FxType::Reverb, FxType::Delay, FxType::Saturator,
                                                             FxType::TransientShaper, FxType::GateExpander, FxType::StereoProcessor, FxType::MonstaFx,
                                                             FxType::Gain };

    juce::Rectangle<float> rowArea(int row) const
    {
        return juce::Rectangle<float>(6.0f, kRow + 6.0f + kRow * static_cast<float>(row), static_cast<float>(getWidth()) - 12.0f, kRow - 2.0f);
    }

    void showAddMenu()
    {
        const auto project = processor.getProject();
        auto menu = buildAddMenu(selectedLayer(project));
        juce::Component::SafePointer<NodeList> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safe](int result)
        {
            if (safe == nullptr || result == 0)
                return;
            Selection next;
            safe->processor.getUndoManager().beginNewTransaction("Add");
            safe->processor.editProject("Add", [&](FxProjectState& p) { next = applyAddMenu(p, result); });
            if (next.kind != Selection::Kind::None)
            {
                safe->selection = next;
                if (safe->onSelectionChanged)
                    safe->onSelectionChanged();
            }
        });
    }

    FxPluginProcessor& processor;
    Selection& selection;
};
} // namespace hpeq::ui
