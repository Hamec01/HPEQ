#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Standalone/ModulePluginProcessor.h"
#include "UI/PresetScreen.h"
#include "UI/ModernZastavaPanel.h"

namespace hpeq::standalone
{
class ModulePluginEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ModulePluginEditor(ModulePluginProcessor&);
    ~ModulePluginEditor() override;

    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;
    ui::PresetScreen& getPresetScreen() noexcept { return *presets; }
    juce::Component* getPanel() noexcept { return panel.get(); }
    ui::ModernZastavaPanel* getModernPanel() noexcept { return modernPanel.get(); }
    void showView(int);
    void refreshEditor();

private:
    template <typename Panel>
    void installPanel(std::unique_ptr<Panel>);
    void beginGesture(const juce::String& id);
    void endGesture();
    void setImmediate(const juce::String& id, double value);
    void selectSoftwareRenderer();
    void timerCallback() override;

    ModulePluginProcessor& processor;
    std::unique_ptr<juce::Component> panel;
    std::unique_ptr<ui::ModernZastavaPanel> modernPanel;
    std::array<juce::TextButton,3> navigation;
    std::unique_ptr<juce::TooltipWindow> tooltip;
    int lastEngine=1,lastSeenEngine=-1;
    std::unique_ptr<ui::PresetScreen> presets;
    std::function<void()> refreshPanel;
    juce::RangedAudioParameter* activeGesture = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePluginEditor)
};
} // namespace hpeq::standalone
