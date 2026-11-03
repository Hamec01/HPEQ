#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Standalone/ModulePluginProcessor.h"
#include "UI/HamloRevolutionSimplePage.h"
#include "UI/HamloRevolutionTransitionPage.h"
#include "UI/PresetScreen.h"
#include "UI/RevolutionSampleBar.h"

namespace hpeq::standalone
{
class ModulePluginEditor final : public juce::AudioProcessorEditor, private juce::Timer, public juce::FileDragAndDropTarget
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
    ui::HamloRevolutionTransitionPage* getTransitionPage() noexcept { return transitionPage.get(); } // RISE / FILL

bool isInterestedInFileDrag(const juce::StringArray& files) override {return sampleBar&&sampleBar->isInterestedInFileDrag(files);}
    void filesDropped(const juce::StringArray& files,int,int) override {if(sampleBar)sampleBar->filesDropped(files,0,0);}
    ui::RevolutionSampleBar* getSampleBar() noexcept {return sampleBar.get();}
private:
    std::unique_ptr<ui::RevolutionSampleBar> sampleBar;
    template <typename Panel>
    void installPanel(std::unique_ptr<Panel>);
    void beginGesture(const juce::String& id);
    void endGesture();
    void setImmediate(const juce::String& id, double value);
    void selectSoftwareRenderer();
    void timerCallback() override;
    void applyPage(); // R4: SIMPLE or ADVANCED, as the processor says
    void installSimplePage();
    void installTransitionPage();
    void openTransition(int kind); // RISE / FILL from SIMPLE: the page, a fresh capture and render

    ModulePluginProcessor& processor;
    std::unique_ptr<juce::Component> panel;
    std::unique_ptr<ui::PresetScreen> presets;
    std::function<void()> refreshPanel, refreshSimple;
    std::unique_ptr<ui::HamloRevolutionSimplePage> simplePage;
    std::unique_ptr<ui::HamloRevolutionTransitionPage> transitionPage;
    std::function<void()> refreshTransition;
    std::unique_ptr<juce::TooltipWindow> tooltips; // R6: HamloRevolution's help
    juce::RangedAudioParameter* activeGesture = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulePluginEditor)
};
} // namespace hpeq::standalone
