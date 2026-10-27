#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "UI/HamloCompPanel.h"

namespace hamlo::comp
{
class HamloCompProcessor;

// The HamloComp page (Black Steel, 1180 x 700, scaled 0.5 .. 2 x). Panel edits become host gestures and
// parameter writes; a 30 Hz timer brings host automation and the meters back into the panel.
class HamloCompEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HamloCompEditor(HamloCompProcessor&);
    ~HamloCompEditor() override;

    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    ui::HamloCompPanel& getPanel() noexcept { return *panel; }

private:
    void beginGesture(const juce::String& id);
    void endGesture();
    void setImmediate(const juce::String& id, double value);
    void refresh();
    void selectSoftwareRenderer();
    void timerCallback() override;

    HamloCompProcessor& processor;
    std::unique_ptr<ui::HamloCompPanel> panel;
    juce::RangedAudioParameter* activeGesture = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamloCompEditor)
};
} // namespace hamlo::comp
