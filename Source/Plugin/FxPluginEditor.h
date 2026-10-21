#pragma once

#include <array>

#include <juce_audio_processors/juce_audio_processors.h>

namespace hpeq
{
class FxPluginProcessor;

// M1 editor: the sketch's frame (header, top strip with meters and 10 layer buttons, workspace,
// inspector) with only what really works wired up. Later milestones fill the placeholders.
class FxPluginEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FxPluginEditor(FxPluginProcessor&);
    ~FxPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class LevelMeter final : public juce::Component
    {
    public:
        explicit LevelMeter(juce::String labelText) : label(std::move(labelText)) {}
        void setLevels(float peakLinear, float rmsLinear);
        void paint(juce::Graphics&) override;

    private:
        juce::String label;
        float peakDb = -100.0f, rmsDb = -100.0f, heldPeakDb = -100.0f;
    };

    void timerCallback() override;
    void selectLayer(int index);

    FxPluginProcessor& processor;

    juce::Label title, status, workspaceNote, inspectorNote;
    LevelMeter inputMeter { "IN" }, outputMeter { "OUT" };
    std::array<juce::TextButton, 10> layerButtons;
    juce::Slider outputGain;
    juce::Label outputGainLabel;
    juce::ToggleButton bypass { "Bypass" };
    juce::TooltipWindow tooltips { this };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::Rectangle<int> workspaceArea, inspectorArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPluginEditor)
};
} // namespace hpeq
