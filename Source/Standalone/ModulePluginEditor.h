#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Standalone/ModulePluginProcessor.h"
#include "UI/HamloRevolutionSimplePage.h"
#include "UI/PresetScreen.h"

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
    ui::HamloRevolutionSimplePage* getSimplePage() noexcept { return simplePage.get(); } // R4: HamloRevolution only

private:
    template <typename Panel>
    void installPanel(std::unique_ptr<Panel>);
    void beginGesture(const juce::String& id);
    void endGesture();
    void setImmediate(const juce::String& id, double value);
    void selectSoftwareRenderer();
    void timerCallback() override;
    void applyPage(); // R4: SIMPLE or ADVANCED, as the processor says
    void installSimplePage();

    ModulePluginProcessor& processor;
    std::unique_ptr<juce::Component> panel;
    std::unique_ptr<ui::PresetScreen> presets;
    std::function<void()> refreshPanel, refreshSimple;
    std::unique_ptr<ui::HamloRevolutionSimplePage> simplePage;
    std::unique_ptr<juce::TooltipWindow> tooltips; // R6: HamloRevolution's help
    juce::RangedAudioParameter* activeGesture = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePluginEditor)
};
} // namespace hpeq::standalone
