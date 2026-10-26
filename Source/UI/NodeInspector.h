#pragma once

#include <functional>

#include "Plugin/FxPluginProcessor.h"
#include "UI/Theme.h"
#include "UI/HamloCompPanel.h"
#include "UI/HamloEchoPanel.h"
#include "UI/HamloFaderPanel.h"
#include "UI/HamloMagPanel.h"
#include "UI/HamloRedWallPanel.h"
#include "UI/HamloRevolutionPanel.h"
#include "UI/HamloUdarnikPanel.h"
#include "UI/HamloZastavaPanel.h"
#include "UI/HamloShirotaPanel.h"
#include "UI/ModuleHelp.h"
#include "UI/HamloTochkaPanel.h"
#include "UI/HamloVerbPanel.h"

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
    // True while a module page needs the whole editor body (HamloComp, HamloVerb, HamloEcho, HamloMag).
    bool wantsLargeView() const noexcept { return largePage && !showingGrid; }
    std::function<void()> onLayoutChange;
    // Live readouts from the engine (status, needles); the timer calls it at 15 Hz, tests directly.
    // settle = put the compressor / reverb needles on the current readings at once (tests, screenshots).
    void updateLiveReadouts(bool settle = false);

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
    enum class ChainKind { Region, Point, Mix };
    void buildModules(const std::vector<FxSlotState>& chain, ChainKind);
    // The engine node of the selection (a region's or point's slot, or the layer's MIX node).
    const dsp::FxNodeProcessor* selectedNodeProcessor(const FxProjectState&) const;
    // Points into the given project: the caller keeps it alive while using the result.
    const std::vector<FxSlotState>* selectedChain(const FxProjectState&) const;
    int selectedEngineSlot(const FxProjectState&) const;
    void editChain(const juce::String& name, const std::function<void(std::vector<FxSlotState>&)>&);
    void addModule(FxType);
    void moveModule(int index, int delta);
    // Presets pass: the module page's preset menu (default, factory, user, save).
    void showModulePresets(int index, juce::Component& target);
    void saveModulePresetAs(int index);
    std::unique_ptr<juce::AlertWindow> saveDialog;
    void buildReverbPage(int k);
    void buildDelayPage(int k);
    void buildSaturatorPage(int k);
    void buildFaderPage(int k);
    void buildLimiterPage(int k);
    void buildTransientPage(int k);
    void buildGatePage(int k);
    void buildStereoPage(int k);
    void buildRevolutionPage(int k);

    FxPluginProcessor& processor;
    Selection& selection;
    juce::OwnedArray<juce::Component> controls;
    juce::OwnedArray<juce::Label> labels;
    std::vector<std::function<void()>> refreshers;
    juce::Rectangle<int> chainArea, paramArea;
    juce::String title, status;
    bool dragging = false;
    int nodeIndex = -1;
    bool showingGrid = true, largePage = false;
    HamloCompPanel* hamloPanel = nullptr; // owned by controls
    HamloVerbPanel* verbPanel = nullptr;  // owned by controls
    HamloEchoPanel* echoPanel = nullptr;  // owned by controls
    HamloMagPanel* magPanel = nullptr;    // owned by controls
    HamloFaderPanel* faderPanel = nullptr; // owned by controls
    HamloRedWallPanel* wallPanel = nullptr; // owned by controls
    HamloUdarnikPanel* udarnikPanel = nullptr; // owned by controls
    HamloZastavaPanel* zastavaPanel = nullptr; // owned by controls
    HamloShirotaPanel* shirotaPanel = nullptr; // owned by controls
    // Knob help (owner, 2026-10-26): the large page under the mouse tells which control it is; the text is
    // drawn in the free margin left of the page, the margins also carry scratched pictures.
    struct HelpWatcher final : juce::MouseListener
    {
        std::function<void(const juce::MouseEvent&)> onEvent;
        void mouseMove(const juce::MouseEvent& e) override { if (onEvent) onEvent(e); }
        void mouseDown(const juce::MouseEvent& e) override { if (onEvent) onEvent(e); }
        void mouseDrag(const juce::MouseEvent& e) override { if (onEvent) onEvent(e); }
    };
    void attachHelp();
public:
    // The help text for a control of the page shown now (as pointing at it; tests and screenshots).
    void showHelp(const juce::String& key) { helpKey = key; repaint(); }
    const juce::String& shownHelpKey() const noexcept { return helpKey; }
private:
    void drawMargins(juce::Graphics&) const;
    HelpWatcher helpWatcher;
    juce::Component* helpPanel = nullptr; // owned by controls
    juce::String helpModule, helpKey;
    HamloRevolutionPanel* revolutionPanel = nullptr; // owned by controls
    HamloTochkaPanel* tochkaPanel = nullptr;         // owned by controls (EQ point page)
};
} // namespace hpeq::ui
