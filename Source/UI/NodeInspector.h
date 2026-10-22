#pragma once

#include <functional>

#include "Plugin/FxPluginProcessor.h"
#include "UI/Theme.h"
#include "UI/VintageStyle.h"

namespace hpeq::ui
{
// Bottom panel: controls of the ONE selected EQ point or FX region (03 "не отображать все ручки всех
// эффектов"). Values are read from the project on every change; edits go through editProject. Every
// slider accepts typed values. FX regions and EQ points show their modules as a 3 x 2 grid (D2, D6);
// a tile opens that module's page (a back arrow returns), with live gain reduction and pool starvation.
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
    // Live readouts from the engine (status, needles); the timer calls it at 15 Hz, tests directly.
    void updateLiveReadouts();

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
    // D6: the module chain of the selected FX region or EQ point (grid, or one module's page).
    void buildModules(const std::vector<FxSlotState>& chain, bool region);
    // Points into the given project: the caller keeps it alive while using the result.
    const std::vector<FxSlotState>* selectedChain(const FxProjectState&) const;
    int selectedEngineSlot(const FxProjectState&) const;
    void editChain(const juce::String& name, const std::function<void(std::vector<FxSlotState>&)>&);
    void addModule(FxType);
    void moveModule(int index, int delta);

    FxPluginProcessor& processor;
    Selection& selection;
    vintage::LookAndFeel vintageLook; // declared before the controls that use it (destroyed after them)
    juce::OwnedArray<juce::Component> controls;
    juce::OwnedArray<juce::Label> labels;
    std::vector<std::function<void()>> refreshers;
    juce::Rectangle<int> chainArea, paramArea;
    juce::String title, status;
    bool dragging = false;
    int nodeIndex = -1;
    bool showingGrid = true, vintagePage = false;
    juce::Rectangle<int> vintageArea;
    vintage::NeedleMeter* inputNeedle = nullptr; // owned by controls
    vintage::NeedleMeter* reductionNeedle = nullptr;
};
} // namespace hpeq::ui
