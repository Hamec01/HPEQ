#pragma once

#include <functional>

#include "Plugin/FxPluginProcessor.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
// Left panel: every EQ point and FX region of the selected layer, in processing order (PRE-EQ first, decision B).
// Click selects; the dot toggles enable; x deletes; "+" adds. Painted, no child components.
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
        const auto drawRow = [&](juce::Colour colour, const juce::String& text, bool enabled, bool selected)
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
            g.drawText(text, r.withTrimmedLeft(24).withTrimmedRight(44), juce::Justification::centredLeft);
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
            drawRow(colours::eqNode, text, n.enabled, selection.isEq(n.id));
        }
        for (int i = 0; i < static_cast<int>(layer.fxNodes.size()); ++i)
        {
            const auto& n = layer.fxNodes[static_cast<size_t>(i)];
            drawRow(colours::forNode(i), "FX " + juce::String(i + 1) + "  " + nodeTitle(n), n.enabled, selection.isFx(n.id));
        }
        if (row == 0)
        {
            g.setColour(colours::dim);
            g.drawText("No nodes yet.", rowArea(0), juce::Justification::centred);
        }
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
        if (row < 0 || row >= total)
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
            selection.slot = 0;
        }
        if (onSelectionChanged)
            onSelectionChanged();
        repaint();
    }

private:
    static constexpr float kRow = 24.0f;

    juce::Rectangle<float> rowArea(int row) const
    {
        return juce::Rectangle<float>(6.0f, kRow + 6.0f + kRow * static_cast<float>(row), static_cast<float>(getWidth()) - 12.0f, kRow - 2.0f);
    }

    void showAddMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Add EQ Bell at 1 kHz");
        menu.addItem(2, "Add EQ Low Cut at 80 Hz");
        menu.addItem(3, "Add FX Compressor 250 - 700 Hz");
        menu.addItem(4, "Add FX Reverb 3 - 8 kHz");
        juce::Component::SafePointer<NodeList> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safe](int result)
        {
            if (safe == nullptr || result == 0)
                return;
            std::uint32_t newId = 0;
            bool eq = result <= 2;
            safe->processor.getUndoManager().beginNewTransaction("Add node");
            safe->processor.editProject("Add node", [&](FxProjectState& p)
            {
                auto& l = selectedLayer(p);
                if (eq && l.eqNodes.size() < static_cast<size_t>(kMaxEqNodesPerLayer))
                {
                    EqNodeState n;
                    n.id = newId = p.nextNodeId++;
                    n.shape = result == 1 ? EqShape::Bell : EqShape::LowCut;
                    n.frequencyHz = result == 1 ? 1000.0 : 80.0;
                    n.gainDb = result == 1 ? 3.0 : 0.0;
                    n.slopeDbPerOct = 24;
                    l.eqNodes.push_back(n);
                }
                else if (!eq && l.fxNodes.size() < static_cast<size_t>(kMaxFxNodesPerLayer))
                {
                    FxNodeState n;
                    n.id = newId = p.nextNodeId++;
                    n.lowHz = result == 3 ? 250.0 : 3000.0;
                    n.highHz = result == 3 ? 700.0 : 8000.0;
                    n.chain.front().type = result == 3 ? FxType::Compressor : FxType::Reverb;
                    l.fxNodes.push_back(n);
                }
            });
            if (newId != 0)
            {
                safe->selection.kind = eq ? Selection::Kind::Eq : Selection::Kind::Fx;
                safe->selection.id = newId;
                safe->selection.slot = 0;
                if (safe->onSelectionChanged)
                    safe->onSelectionChanged();
            }
        });
    }

    FxPluginProcessor& processor;
    Selection& selection;
};
} // namespace hpeq::ui
