#pragma once

#include <functional>

#include "Plugin/FxPluginProcessor.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
// Layer controls of the selected layer (01 §8): On / Mute / Solo, Gain, Mix, Pan, Width, channel
// component, and the project routing (Parallel / Serial). Copy / Paste / Duplicate / Clear act on
// the selected layer. Every change is one undoable edit (drags grouped per gesture).
class LayerControls final : public juce::Component
{
public:
    explicit LayerControls(FxPluginProcessor&);

    // Re-read the project (after edits, undo, A/B, layer selection).
    void refresh();
    // Called after Duplicate selected another layer.
    std::function<void()> onLayerChanged;

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    void editLayer(const juce::String& name, const std::function<void(LayerState&)>&);
    void setupSlider(juce::Slider&, juce::Label&, const juce::String& name, double min, double max, double mid, const juce::String& suffix,
                     int decimals, std::function<void(LayerState&, double)> apply);

    FxPluginProcessor& processor;
    bool dragging = false;
    juce::Label title;
    juce::TextButton on { "On" }, mute { "M" }, solo { "S" };
    juce::Slider gain, mix, pan, width;
    juce::Label gainLabel, mixLabel, panLabel, widthLabel;
    juce::ComboBox mode, routing;
    juce::TextButton copy { "Copy" }, paste { "Paste" }, duplicate { "Dup" }, clear { "Clear" };
};
} // namespace hpeq::ui
