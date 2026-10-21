#include "Plugin/FxPluginEditor.h"

#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/AnalysisModel.h"
#include "UI/NodeInspector.h"
#include "UI/NodeList.h"
#include "UI/SpectrumWorkspace.h"
#include "UI/Theme.h"
#include "UI/WaveformPanel.h"

namespace hpeq
{
using namespace ui;

// Vertical peak/RMS meter of the last processed block (real levels from the audio thread).
class FxPluginEditor::LevelMeter final : public juce::Component
{
public:
    explicit LevelMeter(juce::String labelText) : label(std::move(labelText)) {}

    void setLevels(float peakLinear, float rmsLinear)
    {
        peakDb = juce::Decibels::gainToDecibels(peakLinear, -100.0f);
        rmsDb = juce::Decibels::gainToDecibels(rmsLinear, -100.0f);
        heldPeakDb = juce::jmax(peakDb, heldPeakDb - 1.0f); // ~30 dB/s fall at 30 Hz
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour(colours::dim);
        g.setFont(10.0f);
        g.drawText(label, area.removeFromTop(12.0f), juce::Justification::centred);
        g.drawText(heldPeakDb <= -99.0f ? juce::String("-inf") : juce::String(heldPeakDb, 1), area.removeFromBottom(12.0f), juce::Justification::centred);
        auto bar = area.reduced(area.getWidth() * 0.3f, 2.0f);
        g.setColour(colours::panelRaised);
        g.fillRect(bar);
        const auto yFor = [&](float db) { return bar.getBottom() - bar.getHeight() * juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f); };
        g.setColour(colours::accent.withAlpha(0.7f));
        g.fillRect(bar.withTop(yFor(rmsDb)));
        g.setColour(heldPeakDb > 0.0f ? colours::danger : colours::text);
        g.fillRect(bar.getX(), yFor(heldPeakDb) - 1.0f, bar.getWidth(), 2.0f);
    }

private:
    juce::String label;
    float peakDb = -100.0f, rmsDb = -100.0f, heldPeakDb = -100.0f;
};

namespace
{
void styleTab(juce::TextButton& b, bool on)
{
    b.setColour(juce::TextButton::buttonColourId, on ? colours::accent.withAlpha(0.35f) : colours::panelRaised);
    b.setColour(juce::TextButton::textColourOffId, on ? colours::text : colours::dim);
}
} // namespace

FxPluginEditor::FxPluginEditor(FxPluginProcessor& p)
    : juce::AudioProcessorEditor(p), processor(p)
{
    selection = std::make_unique<Selection>();
    analysis = std::make_unique<AnalysisModel>(processor.getCapture());
    waveform = std::make_unique<WaveformPanel>(*analysis, [this] { return processor.getCurrentSampleRate(); });
    workspace = std::make_unique<SpectrumWorkspace>(processor, *analysis, *selection);
    nodeList = std::make_unique<NodeList>(processor, *selection);
    inspector = std::make_unique<NodeInspector>(processor, *selection);
    inputMeter = std::make_unique<LevelMeter>("IN");
    outputMeter = std::make_unique<LevelMeter>("OUT");
    workspace->onSelectionChanged = [this] { selectionChanged(); };
    nodeList->onSelectionChanged = [this] { selectionChanged(); };
    for (auto* c : std::initializer_list<juce::Component*> { waveform.get(), workspace.get(), nodeList.get(), inspector.get(), inputMeter.get(), outputMeter.get() })
        addAndMakeVisible(c);

    title.setText("HPEQ", juce::dontSendNotification);
    title.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, colours::text);
    subtitle.setText("BETA  -  frequency-layer FX  (PRE-EQ > FX regions)", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, colours::accent);
    subtitle.setFont(juce::FontOptions(11.0f));
    unavailable.setText("Oversampling, quality modes, LUFS: not yet (M6/M8)", juce::dontSendNotification);
    unavailable.setColour(juce::Label::textColourId, colours::dim);
    unavailable.setJustificationType(juce::Justification::centredRight);
    unavailable.setFont(juce::FontOptions(11.0f));
    layerNote.setText("Layer 1 is processed. Layers 2-10, layer gain/mix/pan/width and Serial/Parallel routing arrive in M7.",
                      juce::dontSendNotification);
    layerNote.setColour(juce::Label::textColourId, colours::dim);
    layerNote.setFont(juce::FontOptions(11.0f));
    for (auto* l : { &title, &subtitle, &unavailable, &layerNote })
        addAndMakeVisible(*l);

    abA.setTooltip("Project A");
    abB.setTooltip("Project B (starts as a copy of A)");
    copyAB.setTooltip("Copy the current project into the other slot");
    abA.onClick = [this] { processor.selectAB(0); updateHeader(); selectionChanged(); };
    abB.onClick = [this] { processor.selectAB(1); updateHeader(); selectionChanged(); };
    copyAB.onClick = [this] { processor.copyToOtherAB(); };
    undo.onClick = [this] { processor.getUndoManager().undo(); };
    redo.onClick = [this] { processor.getUndoManager().redo(); };
    waveOverlay.onClick = [this] { waveform->mode = WaveformPanel::Mode::Overlay; updateHeader(); };
    waveDelta.onClick = [this] { waveform->mode = WaveformPanel::Mode::Delta; updateHeader(); };
    freeze.onClick = [this] { analysis->frozen = !analysis->frozen; updateHeader(); };
    tabSpectrum.onClick = [this] { workspace->mode = SpectrumWorkspace::Mode::Spectrum; updateHeader(); };
    tabDifference.onClick = [this] { workspace->mode = SpectrumWorkspace::Mode::Difference; updateHeader(); };
    view.addItemList({ "View: Mid", "View: Left", "View: Right", "View: Side" }, 1);
    view.setSelectedItemIndex(0, juce::dontSendNotification);
    view.onChange = [this] { analysis->view = static_cast<AnalysisModel::View>(view.getSelectedItemIndex()); };
    for (auto* b : { &abA, &abB, &copyAB, &undo, &redo, &waveOverlay, &waveDelta, &freeze, &tabSpectrum, &tabDifference })
        addAndMakeVisible(*b);
    addAndMakeVisible(view);

    for (int i = 0; i < static_cast<int>(layerButtons.size()); ++i)
    {
        auto& b = layerButtons[static_cast<size_t>(i)];
        b.setButtonText(juce::String(i + 1));
        styleTab(b, i == 0);
        b.setEnabled(i == 0);
        b.setTooltip(i == 0 ? "Layer 1 (processed)" : "Layer " + juce::String(i + 1) + ": not yet implemented (M7)");
        addAndMakeVisible(b);
    }

    outputGain.setSliderStyle(juce::Slider::LinearHorizontal);
    outputGain.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
    outputGain.setTextValueSuffix(" dB");
    outputGainLabel.setText("Output", juce::dontSendNotification);
    outputGainLabel.setColour(juce::Label::textColourId, colours::dim);
    addAndMakeVisible(outputGain);
    addAndMakeVisible(outputGainLabel);
    addAndMakeVisible(bypass);
    auto& apvts = processor.getParameters();
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, params::outputGain.getParamID(), outputGain);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, params::bypass.getParamID(), bypass);

    processor.addChangeListener(this);
    updateHeader();
    inspector->rebuild();
    setResizable(true, true);
    setResizeLimits(1000, 700, 2600, 1700);
    setSize(1280, 840);
    startTimerHz(30);
}

FxPluginEditor::~FxPluginEditor()
{
    stopTimer();
    processor.removeChangeListener(this);
}

void FxPluginEditor::updateHeader()
{
    styleTab(abA, processor.getActiveAB() == 0);
    styleTab(abB, processor.getActiveAB() == 1);
    styleTab(waveOverlay, waveform->mode == WaveformPanel::Mode::Overlay);
    styleTab(waveDelta, waveform->mode == WaveformPanel::Mode::Delta);
    styleTab(freeze, analysis->frozen);
    styleTab(tabSpectrum, workspace->mode == SpectrumWorkspace::Mode::Spectrum);
    styleTab(tabDifference, workspace->mode == SpectrumWorkspace::Mode::Difference);
    undo.setEnabled(processor.getUndoManager().canUndo());
    redo.setEnabled(processor.getUndoManager().canRedo());
    repaint();
}

void FxPluginEditor::selectionChanged()
{
    inspector->rebuild();
    nodeList->repaint();
    workspace->repaint();
}

void FxPluginEditor::changeListenerCallback(juce::ChangeBroadcaster*)
{
    // The project changed (edit, undo/redo, A/B, host state): refresh everything that shows it.
    inspector->refresh();
    nodeList->repaint();
    workspace->repaint();
    updateHeader();
}

void FxPluginEditor::updateAnalysisNow()
{
    timerCallback();
}

void FxPluginEditor::selectNode(bool fx, std::uint32_t id, int slot)
{
    selection->kind = fx ? Selection::Kind::Fx : Selection::Kind::Eq;
    selection->id = id;
    selection->slot = slot;
    selectionChanged();
}

void FxPluginEditor::showDifference(bool difference)
{
    workspace->mode = difference ? SpectrumWorkspace::Mode::Difference : SpectrumWorkspace::Mode::Spectrum;
    updateHeader();
}

void FxPluginEditor::timerCallback()
{
    analysis->update();
    const auto meters = processor.getMeters();
    inputMeter->setLevels(meters.inputPeak, meters.inputRms);
    outputMeter->setLevels(meters.outputPeak, meters.outputRms);
    waveform->repaint();
    workspace->repaint();
}

void FxPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(colours::background);
}

void FxPluginEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    auto header = area.removeFromTop(40);
    title.setBounds(header.removeFromLeft(90));
    subtitle.setBounds(header.removeFromLeft(330));
    header.removeFromLeft(10);
    abA.setBounds(header.removeFromLeft(36).reduced(0, 6));
    abB.setBounds(header.removeFromLeft(36).reduced(0, 6));
    copyAB.setBounds(header.removeFromLeft(46).reduced(2, 6));
    header.removeFromLeft(12);
    undo.setBounds(header.removeFromLeft(56).reduced(0, 6));
    redo.setBounds(header.removeFromLeft(56).reduced(2, 6));
    unavailable.setBounds(header);
    area.removeFromTop(6);

    auto strip = area.removeFromTop(150);
    auto meters = strip.removeFromRight(90);
    inputMeter->setBounds(meters.removeFromLeft(45));
    outputMeter->setBounds(meters);
    strip.removeFromRight(8);
    auto layers = strip.removeFromRight(5 * 52);
    const int rowHeight = (layers.getHeight() - 24) / 2;
    for (int i = 0; i < 10; ++i)
        layerButtons[static_cast<size_t>(i)].setBounds(layers.getX() + (i % 5) * 52, layers.getY() + (i / 5) * rowHeight, 48, rowHeight - 4);
    strip.removeFromRight(8);
    auto waveButtons = strip.removeFromTop(24);
    freeze.setBounds(waveButtons.removeFromRight(70).reduced(2, 0));
    waveDelta.setBounds(waveButtons.removeFromRight(70).reduced(2, 0));
    waveOverlay.setBounds(waveButtons.removeFromRight(70).reduced(2, 0));
    waveform->setBounds(strip.withTrimmedTop(2));
    area.removeFromTop(8);

    auto footer = area.removeFromBottom(28);
    layerNote.setBounds(footer.removeFromLeft(footer.getWidth() - 470));
    outputGainLabel.setBounds(footer.removeFromLeft(56));
    outputGain.setBounds(footer.removeFromLeft(300));
    bypass.setBounds(footer.removeFromLeft(100).reduced(8, 0));
    area.removeFromBottom(6);

    auto bottom = area.removeFromBottom(juce::jmax(240, area.getHeight() * 36 / 100));
    nodeList->setBounds(bottom.removeFromLeft(300));
    bottom.removeFromLeft(8);
    inspector->setBounds(bottom);
    area.removeFromBottom(8);

    auto tabs = area.removeFromTop(28);
    tabSpectrum.setBounds(tabs.removeFromLeft(100).reduced(0, 2));
    tabDifference.setBounds(tabs.removeFromLeft(100).reduced(4, 2));
    view.setBounds(tabs.removeFromRight(140).reduced(0, 2));
    area.removeFromTop(4);
    workspace->setBounds(area);
}
} // namespace hpeq
