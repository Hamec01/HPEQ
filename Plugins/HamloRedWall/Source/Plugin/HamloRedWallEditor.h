#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "UI/HamloRedWallPanel.h"
#include "UI/PresetScreen.h"

namespace hamlo::redwall
{
class HamloRedWallProcessor;

// The HamloRedWall page (Black Steel, 1180 x 820, scaled 0.5 .. 2 x) with the preset screen top left. Panel edits become host gestures and
// parameter writes; a 30 Hz timer brings host automation back; the panel polls the meters and the last hit.
class HamloRedWallEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HamloRedWallEditor(HamloRedWallProcessor&);
    ~HamloRedWallEditor() override;

    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    ui::HamloRedWallPanel& getPanel() noexcept { return *panel; }
    ui::PresetScreen& getPresetScreen() noexcept { return *presets; }

private:
    void beginGesture(const juce::String& id);
    void endGesture();
    void setImmediate(const juce::String& id, double value);
    void refresh();
    void selectSoftwareRenderer();
    void timerCallback() override;

    HamloRedWallProcessor& processor;
    std::unique_ptr<ui::HamloRedWallPanel> panel;
    std::unique_ptr<ui::PresetScreen> presets;
    juce::RangedAudioParameter* activeGesture = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamloRedWallEditor)
};
} // namespace hamlo::redwall
