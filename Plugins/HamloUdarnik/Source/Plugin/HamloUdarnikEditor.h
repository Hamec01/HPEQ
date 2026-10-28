#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "UI/HamloUdarnikPanel.h"
#include "UI/PresetScreen.h"

namespace hamlo::udarnik
{
class HamloUdarnikProcessor;

// The HamloUdarnik page (Black Steel, 1180 x 820, scaled 0.5 .. 2 x) with the preset screen top left. Panel edits become host gestures and
// parameter writes; a 30 Hz timer brings host automation back; the panel polls the meters and the last hit.
class HamloUdarnikEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HamloUdarnikEditor(HamloUdarnikProcessor&);
    ~HamloUdarnikEditor() override;

    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    ui::HamloUdarnikPanel& getPanel() noexcept { return *panel; }
    ui::PresetScreen& getPresetScreen() noexcept { return *presets; }

private:
    void beginGesture(const juce::String& id);
    void endGesture();
    void setImmediate(const juce::String& id, double value);
    void refresh();
    void selectSoftwareRenderer();
    void timerCallback() override;

    HamloUdarnikProcessor& processor;
    std::unique_ptr<ui::HamloUdarnikPanel> panel;
    std::unique_ptr<ui::PresetScreen> presets;
    juce::RangedAudioParameter* activeGesture = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamloUdarnikEditor)
};
} // namespace hamlo::udarnik
