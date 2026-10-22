#pragma once

#include <functional>

#include "Plugin/FxPluginProcessor.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
// Bottom panel: controls of the ONE selected EQ point or FX region (03 "не отображать все ручки всех
// эффектов"). Values are read from the project on every change; edits go through editProject. Every
// slider accepts typed values. FX regions show their chain (up to 6 slots, D2) and the parameters of
// the selected slot, plus live gain reduction (compressor) and reverb-pool starvation.
class NodeInspector final : public juce::Component, private juce::Timer
{
public:
    NodeInspector(FxPluginProcessor&, Selection&);
    ~NodeInspector() override;

    // Selection changed: rebuild the controls (rebuildAsync from inside a control's own callback).
    void rebuild();
    void rebuildAsync();
    // Project changed elsewhere (undo, drag in the workspace): refresh values only.
    void refresh();

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct ParamDef
    {
        const char* key;
        const char* label;
        double min, max, defaultValue, midPoint;
        const char* suffix;
        int decimals;
        const char* choices = nullptr; // "A|B|C": a combo box storing the item index
    };

    void timerCallback() override;
    juce::Slider& addKnob(const juce::String& label, double min, double max, double mid, const juce::String& suffix, int decimals,
                          std::function<void(double)> apply);
    juce::ComboBox& addCombo(const juce::String& label, const juce::StringArray& items, std::function<void(int)> apply);
    juce::Button& addButton(const juce::String& text, std::function<void()> onClick, bool toggle = false);
    void editEq(const juce::String& name, const std::function<void(EqNodeState&)>&);
    void editFx(const juce::String& name, const std::function<void(FxNodeState&)>&);
    void beginEdit(const juce::String& name);
    static const std::vector<ParamDef>& paramsFor(FxType);

    FxPluginProcessor& processor;
    Selection& selection;
    juce::OwnedArray<juce::Component> controls;
    juce::OwnedArray<juce::Label> labels;
    std::vector<std::function<void()>> refreshers;
    juce::Rectangle<int> chainArea, paramArea;
    juce::String title, status;
    bool dragging = false;
    int nodeIndex = -1;
};
} // namespace hpeq::ui
