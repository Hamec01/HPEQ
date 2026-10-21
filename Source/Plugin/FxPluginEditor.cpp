#include "Plugin/FxPluginEditor.h"

#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginProcessor.h"

namespace hpeq
{
namespace
{
const juce::Colour kBackground { 0xff12151a };
const juce::Colour kPanel { 0xff1b2028 };
const juce::Colour kOutline { 0xff2c3440 };
const juce::Colour kText { 0xffd7dde6 };
const juce::Colour kDim { 0xff7c8796 };
const juce::Colour kAccent { 0xff35c6d6 };

float toDb(float linear) { return juce::Decibels::gainToDecibels(linear, -100.0f); }
} // namespace

void FxPluginEditor::LevelMeter::setLevels(float peakLinear, float rmsLinear)
{
    peakDb = toDb(peakLinear);
    rmsDb = toDb(rmsLinear);
    heldPeakDb = std::max(peakDb, heldPeakDb - 1.0f); // ~30 dB/s fall at 30 Hz refresh
    repaint();
}

void FxPluginEditor::LevelMeter::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour(kDim);
    g.setFont(11.0f);
    g.drawText(label, area.removeFromLeft(30.0f), juce::Justification::centredLeft);
    auto readout = area.removeFromRight(64.0f);
    g.drawText(heldPeakDb <= -99.0f ? juce::String("-inf") : juce::String(heldPeakDb, 1) + " dB",
               readout, juce::Justification::centredRight);

    g.setColour(kPanel);
    g.fillRect(area);
    const auto position = [&](float db) { return area.getWidth() * juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f); };
    g.setColour(kAccent.withAlpha(0.45f));
    g.fillRect(area.withWidth(position(rmsDb)));
    g.setColour(heldPeakDb > 0.0f ? juce::Colours::red : kAccent);
    g.fillRect(area.getX() + position(heldPeakDb) - 1.0f, area.getY(), 2.0f, area.getHeight());
}

FxPluginEditor::FxPluginEditor(FxPluginProcessor& p)
    : juce::AudioProcessorEditor(p), processor(p)
{
    title.setText("HPEQ  Beta", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, kText);
    addAndMakeVisible(title);

    status.setText("M1 skeleton: audio pass-through, output gain, bypass. No frequency processing yet.",
                   juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, kDim);
    addAndMakeVisible(status);

    for (auto* label : { &workspaceNote, &inspectorNote })
    {
        label->setColour(juce::Label::textColourId, kDim);
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*label);
    }
    workspaceNote.setText("Spectrum workspace (PRE-EQ + FX regions) - not implemented yet (M5)", juce::dontSendNotification);
    inspectorNote.setText("Selected processor inspector - not implemented yet (M5)", juce::dontSendNotification);

    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    for (int i = 0; i < static_cast<int>(layerButtons.size()); ++i)
    {
        auto& button = layerButtons[static_cast<size_t>(i)];
        button.setButtonText(juce::String(i + 1));
        button.setClickingTogglesState(false);
        button.setColour(juce::TextButton::buttonOnColourId, kAccent.darker(0.4f));
        if (i == 0)
        {
            button.setTooltip("Layer 1 (the only layer processed until M7)");
            button.onClick = [this] { selectLayer(0); };
        }
        else
        {
            button.setEnabled(false);
            button.setTooltip("Layer " + juce::String(i + 1) + ": not yet implemented (M7)");
        }
        addAndMakeVisible(button);
    }
    // Show the stored selection without writing it back: opening the editor must not change state.
    const int selected = processor.getProject().selectedLayer;
    for (int i = 0; i < static_cast<int>(layerButtons.size()); ++i)
        layerButtons[static_cast<size_t>(i)].setToggleState(i == selected, juce::dontSendNotification);

    outputGain.setSliderStyle(juce::Slider::LinearHorizontal);
    outputGain.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
    outputGain.setTextValueSuffix(" dB");
    outputGainLabel.setText("Output", juce::dontSendNotification);
    outputGainLabel.setColour(juce::Label::textColourId, kText);
    addAndMakeVisible(outputGain);
    addAndMakeVisible(outputGainLabel);
    addAndMakeVisible(bypass);

    auto& apvts = processor.getParameters();
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, params::outputGain.getParamID(), outputGain);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, params::bypass.getParamID(), bypass);

    setResizable(true, true);
    setResizeLimits(760, 460, 2400, 1500);
    setSize(980, 600);
    startTimerHz(30);
}

FxPluginEditor::~FxPluginEditor()
{
    stopTimer();
}

void FxPluginEditor::selectLayer(int index)
{
    auto project = processor.getProject();
    project.selectedLayer = index;
    processor.setProject(project);
    for (int i = 0; i < static_cast<int>(layerButtons.size()); ++i)
        layerButtons[static_cast<size_t>(i)].setToggleState(i == index, juce::dontSendNotification);
}

void FxPluginEditor::timerCallback()
{
    const auto meters = processor.getMeters();
    inputMeter.setLevels(meters.inputPeak, meters.inputRms);
    outputMeter.setLevels(meters.outputPeak, meters.outputRms);
}

void FxPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBackground);
    g.setColour(kPanel);
    g.fillRect(workspaceArea);
    g.fillRect(inspectorArea);
    g.setColour(kOutline);
    g.drawRect(workspaceArea);
    g.drawRect(inspectorArea);
}

void FxPluginEditor::resized()
{
    auto area = getLocalBounds().reduced(12);

    auto header = area.removeFromTop(32);
    title.setBounds(header.removeFromLeft(160));
    status.setBounds(header);
    area.removeFromTop(8);

    auto strip = area.removeFromTop(64);
    auto layerGrid = strip.removeFromRight(5 * 40);
    const int rowHeight = layerGrid.getHeight() / 2;
    for (int i = 0; i < static_cast<int>(layerButtons.size()); ++i)
    {
        const int row = i / 5, column = i % 5;
        layerButtons[static_cast<size_t>(i)].setBounds(layerGrid.getX() + column * 40, layerGrid.getY() + row * rowHeight, 38, rowHeight - 2);
    }
    strip.removeFromRight(12);
    auto meters = strip.removeFromRight(260);
    inputMeter.setBounds(meters.removeFromTop(meters.getHeight() / 2).reduced(0, 6));
    outputMeter.setBounds(meters.reduced(0, 6));
    area.removeFromTop(8);

    auto bottom = area.removeFromBottom(150);
    inspectorArea = bottom;
    auto controls = bottom.reduced(12).removeFromBottom(28);
    outputGainLabel.setBounds(controls.removeFromLeft(60));
    outputGain.setBounds(controls.removeFromLeft(320));
    controls.removeFromLeft(16);
    bypass.setBounds(controls.removeFromLeft(100));
    inspectorNote.setBounds(inspectorArea.reduced(12).withTrimmedBottom(36));

    area.removeFromBottom(8);
    workspaceArea = area;
    workspaceNote.setBounds(workspaceArea);
}
} // namespace hpeq
