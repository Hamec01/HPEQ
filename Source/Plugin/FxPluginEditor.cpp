#include "Plugin/FxPluginEditor.h"
#include "UI/BlackSteel.h"

#include "Plugin/FxParameterContract.h"
#include "Plugin/FxPluginProcessor.h"
#include "Plugin/Presets.h"
#include "UI/AnalysisModel.h"
#include "UI/LayerControls.h"
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

// Output loudness on a narrow green CRT in a gunmetal bezel (owner: as the module pages' scopes).
class FxPluginEditor::LoudnessScreen final : public juce::Component, public juce::SettableTooltipClient
{
public:
    void set(const juce::String& newText, bool over)
    {
        if (newText == text && over == hot)
            return;
        text = newText;
        hot = over;
        repaint();
    }
    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const auto screen = b.withTrimmedLeft(20.0f).withTrimmedRight(20.0f).reduced(0.0f, 7.0f);
        ui::steel::crtBezel(g, b, screen, {}, 0.0f);
        ui::steel::crtBegin(g, screen);
        g.setFont(juce::FontOptions("Consolas", juce::jlimit(10.0f, 13.0f, screen.getHeight() * 0.5f), juce::Font::bold));
        const auto colour = hot ? colours::danger : colours::phosphor;
        g.setColour(colour.withAlpha(0.25f));
        g.drawFittedText(text, screen.reduced(8.0f, 0.0f).translated(0.0f, 0.6f).toNearestInt(), juce::Justification::centred, 1, 0.7f);
        g.setColour(colour);
        g.drawFittedText(text, screen.reduced(8.0f, 0.0f).toNearestInt(), juce::Justification::centred, 1, 0.7f);
        ui::steel::crtEnd(g, screen);
    }

private:
    juce::String text;
    bool hot = false;
};

namespace
{
int openEditors = 0; // message thread only: the steel look stays the default while any HPEQ window is open

void styleTab(juce::TextButton& b, bool on)
{
    b.setColour(juce::TextButton::buttonColourId, on ? colours::accent.withAlpha(0.35f) : colours::panelRaised);
    b.setColour(juce::TextButton::textColourOffId, on ? colours::text : colours::dim);
}
} // namespace

FxPluginEditor::FxPluginEditor(FxPluginProcessor& p)
    : juce::AudioProcessorEditor(p), processor(p)
{
    // EQ pass: every child (and every menu, which uses the default look) in the Black Steel style
    setLookAndFeel(&steelLook.get());
    juce::LookAndFeel::setDefaultLookAndFeel(&steelLook.get());
    ++openEditors;
    selection = std::make_unique<Selection>();
    analysis = std::make_unique<AnalysisModel>(processor.getCapture());
    waveform = std::make_unique<WaveformPanel>(*analysis, [this] { return processor.getCurrentSampleRate(); });
    workspace = std::make_unique<SpectrumWorkspace>(processor, *analysis, *selection);
    nodeList = std::make_unique<NodeList>(processor, *selection);
    inspector = std::make_unique<NodeInspector>(processor, *selection);
    inputMeter = std::make_unique<LevelMeter>("IN");
    outputMeter = std::make_unique<LevelMeter>("OUT");
    layerControls = std::make_unique<LayerControls>(processor);
    leninButton = std::make_unique<ui::TubeButton>("lenin", "BRING LENIN BACK");
    stalinButton = std::make_unique<ui::TubeButton>("stalin", "BRING STALIN BACK");
    leninButton->setTooltip("BRING LENIN BACK: the LOMO UP-25 tube preamp (6Zh32P, 6N3P) on the whole mix. Click: on / off; drag up / down or wheel: MIX; double click: MIX 100 %.");
    stalinButton->setTooltip("BRING STALIN BACK: the LOMO UP-27 tube preamp (input transformer, 6Zh32P, 6N3P) on the whole mix. Click: on / off; drag up / down or wheel: MIX; double click: MIX 100 %.");
    for (auto* b : { leninButton.get(), stalinButton.get() })
    {
        const bool isLenin = b == leninButton.get();
        b->onToggle = [this, isLenin](bool on)
        {
            processor.getUndoManager().beginNewTransaction(isLenin ? "Bring Lenin back" : "Bring Stalin back");
            processor.editProject(isLenin ? "Bring Lenin back" : "Bring Stalin back", [isLenin, on](FxProjectState& p) { (isLenin ? p.lenin : p.stalin) = on; });
        };
        b->onMixGestureStart = [this, isLenin] { processor.getUndoManager().beginNewTransaction(isLenin ? "Lenin mix" : "Stalin mix"); };
        b->onMix = [this, isLenin](double m)
        {
            processor.editProject(isLenin ? "Lenin mix" : "Stalin mix", [isLenin, m](FxProjectState& p) { (isLenin ? p.leninMix : p.stalinMix) = m; });
        };
        addAndMakeVisible(*b);
    }
    loudnessScreen = std::make_unique<LoudnessScreen>();
    loudnessScreen->setTooltip("Output loudness (ITU-R BS.1770-4): momentary / short-term / integrated LUFS, and true peak (dBTP, max since reset). In: input integrated LUFS.");
    addAndMakeVisible(*loudnessScreen);
    layerControls->onLayerChanged = [this] { layerSelected(); };
    inspector->onLayoutChange = [this]
    {
        if (inspector->wantsLargeView() != largeLayout)
        {
            largeLayout = inspector->wantsLargeView();
            resized();
        }
    };
    workspace->onSelectionChanged = [this] { selectionChanged(); };
    nodeList->onSelectionChanged = [this] { selectionChanged(); };
    for (auto* c : std::initializer_list<juce::Component*> { waveform.get(), workspace.get(), nodeList.get(), inspector.get(), inputMeter.get(), outputMeter.get(), layerControls.get() })
        addAndMakeVisible(c);

    title.setText("HPEQ", juce::dontSendNotification);
    title.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, colours::text);
    subtitle.setText("BETA  -  frequency-layer FX  (PRE-EQ > FX regions)", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, ui::steel::stencil);
    subtitle.setFont(juce::FontOptions(13.0f));
    subtitle.setText(juce::String::fromUTF8("HAMLOPROD \xc2\xb7 FREQUENCY-LAYER FX"), juce::dontSendNotification);
    unavailable.setTooltip("Output loudness (ITU-R BS.1770-4): momentary / short-term / integrated LUFS, and true peak (dBTP, max since reset). In: input integrated LUFS.");
    unavailable.setColour(juce::Label::textColourId, colours::dim);
    unavailable.setJustificationType(juce::Justification::centredRight);
    unavailable.setFont(juce::FontOptions(11.0f));
    for (auto* l : { &title, &subtitle, &unavailable })
        addAndMakeVisible(*l);

    abA.setTooltip("Project A");
    abB.setTooltip("Project B (starts as a copy of A)");
    copyAB.setTooltip("Copy the current project into the other slot");
    abA.onClick = [this] { processor.selectAB(0); updateHeader(); selectionChanged(); };
    abB.onClick = [this] { processor.selectAB(1); updateHeader(); selectionChanged(); };
    copyAB.onClick = [this] { processor.copyToOtherAB(); };
    loudnessReset.setTooltip("Reset integrated loudness and true-peak hold");
    loudnessReset.onClick = [this] { processor.resetLoudness(); };
    addAndMakeVisible(loudnessReset);
    undo.onClick = [this] { processor.getUndoManager().undo(); };
    redo.onClick = [this] { processor.getUndoManager().redo(); };
    waveOverlay.onClick = [this] { waveform->setMode(WaveformPanel::Mode::Overlay); updateHeader(); };
    waveDelta.onClick = [this] { waveform->setMode(WaveformPanel::Mode::Delta); updateHeader(); };
    freeze.onClick = [this] { analysis->frozen = !analysis->frozen; updateHeader(); };
    presetButton.setTooltip("Presets of the whole HPEQ: the factory library and your own (save, load)");
    presetButton.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible(presetButton);
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
        b.onClick = [this, i] { processor.selectLayer(i); layerSelected(); };
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
    // The layer shown now: the first project change (e.g. a module added to the MIX chain) is not a
    // layer change and must not drop the selection.
    lastShownLayer = processor.getProject().selectedLayer;
    updateLayerButtons();
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
    setLookAndFeel(nullptr);
    if (--openEditors == 0 && &juce::LookAndFeel::getDefaultLookAndFeel() == &steelLook.get())
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr); // the last HPEQ window: hand the default back before it goes
}

void FxPluginEditor::renderBackground()
{
    background = {};
    if (getWidth() <= 0 || getHeight() <= 0)
        return;
    background = juce::Image(juce::Image::RGB, getWidth(), getHeight(), true);
    juce::Graphics g(background);
    ui::steel::panel(g, static_cast<float>(getWidth()), static_cast<float>(getHeight()), 1961);
    ui::steel::scratches(g, { { 7, { static_cast<float>(getWidth()) - 260.0f, 8.0f, 70.0f, 46.0f }, 3.0f },
                              { 16, { 18.0f, static_cast<float>(getHeight()) - 64.0f, 60.0f, 52.0f }, -5.0f } }, 0.75f);
    if (!plateArea.isEmpty())
    {
        // the nameplate at its design size (330 x 92), scaled into the header
        const auto plate = plateArea.toFloat();
        const float s = juce::jmin(plate.getWidth() / 330.0f, plate.getHeight() / 92.0f);
        juce::Graphics::ScopedSaveState state(g);
        g.addTransform(juce::AffineTransform::scale(s).translated(plate.getX(), plate.getY()));
        ui::steel::nameplate(g, { 0.0f, 0.0f, 330.0f, 92.0f }, "HPEQ", juce::String::fromUTF8("MODEL \xd0\xad\xd0\x9a-1"),
                             juce::String::fromUTF8("\xe2\x84\x96 0001"));
    }
}

void FxPluginEditor::visibilityChanged()
{
    selectSoftwareRenderer();
}

void FxPluginEditor::parentHierarchyChanged()
{
    selectSoftwareRenderer();
}

void FxPluginEditor::selectSoftwareRenderer()
{
   #if JUCE_WINDOWS
    // JUCE 8 gives every Windows peer Direct2D. When FL Studio closed or unloaded HPEQ, tearing down that
    // DLL's D3D11 device could wait forever inside the GPU driver (the DAW hung). The pages are cached
    // images and vector drawing: the GDI software renderer is enough (as for the module plug-ins).
    if (auto* peer = getPeer(); peer != nullptr && peer->getCurrentRenderingEngine() != 0)
        peer->setCurrentRenderingEngine(0);
   #endif
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
    const auto presetName = processor.getPresetName();
    presetButton.setButtonText(presetName.isEmpty() ? juce::String("Presets") : presetName);
    undo.setEnabled(processor.getUndoManager().canUndo());
    redo.setEnabled(processor.getUndoManager().canRedo());
    repaint();
}

void FxPluginEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Init (empty project)");
    menu.addSeparator();

    // Factory: pack > category [> group] > preset; item ids 1000 + index in the library.
    const auto& factory = presets::factoryPresets();
    juce::PopupMenu factoryMenu, packMenu, categoryMenu, groupMenu;
    juce::String pack, category, group;
    const auto closeGroup = [&] { if (group.isNotEmpty()) categoryMenu.addSubMenu(group, groupMenu); groupMenu = {}; group = {}; };
    const auto closeCategory = [&] { closeGroup(); if (category.isNotEmpty()) packMenu.addSubMenu(category, categoryMenu); categoryMenu = {}; };
    const auto closePack = [&] { closeCategory(); if (pack.isNotEmpty()) factoryMenu.addSubMenu(pack, packMenu); packMenu = {}; };
    const auto current = processor.getPresetName();
    for (size_t i = 0; i < factory.size(); ++i)
    {
        const auto& p = factory[i];
        if (p.pack != pack)
        {
            closePack();
            pack = p.pack;
            category = {};
        }
        if (p.category != category)
        {
            closeCategory();
            category = p.category;
        }
        if (p.group != group)
        {
            closeGroup();
            group = p.group;
        }
        (group.isEmpty() ? categoryMenu : groupMenu).addItem(1000 + static_cast<int>(i), p.name, true, p.name == current);
    }
    closePack();
    menu.addSubMenu("Factory", factoryMenu, !factory.empty());

    // User: item ids 5000 + index.
    const auto userFiles = presets::listFiles(presets::userPresetFolder(), "*.hpeq");
    juce::PopupMenu userMenu;
    for (int i = 0; i < userFiles.size(); ++i)
        userMenu.addItem(5000 + i, userFiles[i].getFileNameWithoutExtension(), true, userFiles[i].getFileNameWithoutExtension() == current);
    if (userFiles.isEmpty())
        userMenu.addItem(2, "(no presets saved yet)", false);
    menu.addSubMenu("User", userMenu);
    menu.addSeparator();
    menu.addItem(3, "Save preset...");
    menu.addItem(4, "Open preset folder");

    juce::Component::SafePointer<FxPluginEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(presetButton), [safe, userFiles](int result)
    {
        if (safe == nullptr || result == 0)
            return;
        auto& proc = safe->processor;
        if (result == 1)
        {
            proc.getUndoManager().beginNewTransaction("Init");
            proc.editProject("Init", [](FxProjectState& p) { p = FxProjectState {}; });
            proc.setPresetName({});
        }
        else if (result == 3)
            safe->savePresetAs();
        else if (result == 4)
        {
            const auto folder = presets::userPresetFolder();
            folder.createDirectory();
            folder.startAsProcess();
        }
        else if (result >= 5000 && result - 5000 < userFiles.size())
        {
            juce::MemoryBlock data;
            const auto& file = userFiles[result - 5000];
            if (file.loadFileAsData(data))
                proc.loadPreset(data.getData(), static_cast<int>(data.getSize()), file.getFileNameWithoutExtension());
        }
        else if (result >= 1000 && static_cast<size_t>(result - 1000) < presets::factoryPresets().size())
        {
            const auto& p = presets::factoryPresets()[static_cast<size_t>(result - 1000)];
            proc.loadPreset(p.data, p.size, p.name);
        }
    });
}

void FxPluginEditor::savePresetAs()
{
    saveDialog = std::make_unique<juce::AlertWindow>("Save preset", "Name of the preset (saved to your HPEQ preset folder):", juce::MessageBoxIconType::NoIcon, this);
    saveDialog->addTextEditor("name", processor.getPresetName().isEmpty() ? juce::String("My preset") : processor.getPresetName());
    saveDialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    saveDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<FxPluginEditor> safe(this);
    saveDialog->enterModalState(true, juce::ModalCallbackFunction::create([safe](int result)
    {
        if (safe == nullptr || safe->saveDialog == nullptr)
            return;
        const auto name = presets::safeFileName(safe->saveDialog->getTextEditorContents("name"));
        safe->saveDialog.reset();
        if (result != 1)
            return;
        const auto file = presets::userPresetFolder().getChildFile(name + ".hpeq");
        safe->processor.setPresetName(name); // saved inside the preset too
        juce::MemoryBlock data;
        safe->processor.getStateInformation(data);
        if (!file.getParentDirectory().createDirectory() || !file.replaceWithData(data.getData(), data.getSize()))
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Save preset", "Could not write " + file.getFullPathName());
    }), false);
}

void FxPluginEditor::updateTubeButtons()
{
    const auto project = processor.getProject();
    leninButton->setState(project.lenin, project.leninMix);
    stalinButton->setState(project.stalin, project.stalinMix);
}

void FxPluginEditor::updateLayerButtons()
{
    // Number, then what the layer is doing: "*" has content, "off" bypassed, "M" muted, "S" soloed.
    const auto project = processor.getProject();
    for (int i = 0; i < kNumLayers; ++i)
    {
        const auto& layer = project.layers[static_cast<size_t>(i)];
        auto& b = layerButtons[static_cast<size_t>(i)];
        juce::String text(i + 1);
        juce::StringArray tip { "Layer " + juce::String(i + 1) };
        if (layerHasContent(layer))
        {
            text << "*";
            tip.add(juce::String(static_cast<int>(layer.eqNodes.size())) + " EQ, " + juce::String(static_cast<int>(layer.fxNodes.size())) + " FX");
        }
        if (!layer.enabled) { text << " off"; tip.add("bypassed"); }
        if (layer.mute) { text << " M"; tip.add("muted"); }
        if (layer.solo) { text << " S"; tip.add("solo"); }
        b.setButtonText(text);
        b.setTooltip(tip.joinIntoString(", "));
        styleTab(b, i == project.selectedLayer);
        b.setAlpha(layer.enabled ? 1.0f : 0.55f);
    }
}

void FxPluginEditor::layerSelected()
{
    // Node ids belong to one layer: drop the selection when the shown layer changes.
    selection->kind = Selection::Kind::None;
    selection->id = 0;
    lastShownLayer = processor.getProject().selectedLayer;
    updateLayerButtons();
    layerControls->refresh();
    selectionChanged();
}

void FxPluginEditor::selectionChanged()
{
    inspector->rebuild();
    nodeList->repaint();
    workspace->repaint();
}

void FxPluginEditor::changeListenerCallback(juce::ChangeBroadcaster*)
{
    // The project changed (edit, undo/redo, A/B, host state, layer selection): refresh what shows it.
    const int shown = processor.getProject().selectedLayer;
    if (shown != lastShownLayer)
        layerSelected();
    updateLayerButtons();
    updateTubeButtons();
    layerControls->refresh();
    inspector->refresh();
    nodeList->repaint();
    workspace->repaint();
    updateHeader();
}

void FxPluginEditor::updateAnalysisNow()
{
    timerCallback();
    waveform->tick(true); // tests and screenshots: the current audio even when frozen
    inspector->updateLiveReadouts(true);
}

void FxPluginEditor::selectNode(bool fx, std::uint32_t id, int slot)
{
    selection->kind = fx ? Selection::Kind::Fx : Selection::Kind::Eq;
    selection->id = id;
    selection->slot = slot;
    selectionChanged();
}

bool FxPluginEditor::isMixSelected() const noexcept
{
    return selection->kind == Selection::Kind::Mix;
}

void FxPluginEditor::selectMix(int slot)
{
    selection->kind = Selection::Kind::Mix;
    selection->id = 0;
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
    selectSoftwareRenderer();
    analysis->update();
    {
        const auto& out = processor.getOutputLoudness();
        const auto value = [](double v) { return v <= LoudnessMeter::kSilence + 1.0 ? juce::String("-inf") : juce::String(v, 1); };
        unavailable.setText("M " + value(out.momentaryLufs()) + "   S " + value(out.shortTermLufs()) + "   I " + value(out.integratedLufs())
                                + " LUFS    TP " + value(out.truePeakDbtp()) + " dBTP    (in I " + value(processor.getInputLoudness().integratedLufs()) + ")",
                            juce::dontSendNotification);
        unavailable.setColour(juce::Label::textColourId, out.truePeakDbtp() > 0.0 ? colours::danger : colours::dim);
        loudnessScreen->set(unavailable.getText().replace("    ", "  ").replace("   ", "  "), out.truePeakDbtp() > 0.0);
    }
    const auto meters = processor.getMeters();
    inputMeter->setLevels(meters.inputPeak, meters.inputRms);
    outputMeter->setLevels(meters.outputPeak, meters.outputRms);
    waveform->tick(); // live: a new picture every tick (FREEZE holds it)
    workspace->repaint();
}

void FxPluginEditor::paint(juce::Graphics& g)
{
    if (background.isValid())
        g.drawImageAt(background, 0, 0);
    else
        g.fillAll(colours::background);
}

void FxPluginEditor::resized()
{
    struct Background
    {
        FxPluginEditor& e;
        ~Background() { e.renderBackground(); } // every return below still renders the plate
    } plateGuard { *this };
    auto area = getLocalBounds().reduced(10);

    // header: nameplate, A / B, undo / redo, OUTPUT and BYPASS (owner: up here), the loudness CRT, Reset
    auto header = area.removeFromTop(48);
    plateArea = header.removeFromLeft(210).withTrimmedBottom(-6).translated(0, -4);
    title.setVisible(false);    // the nameplate says HPEQ
    subtitle.setVisible(false); // ... and the room goes to the controls
    unavailable.setVisible(false);
    header.removeFromLeft(14);
    abA.setBounds(header.removeFromLeft(36).reduced(0, 6));
    abB.setBounds(header.removeFromLeft(36).reduced(0, 6));
    copyAB.setBounds(header.removeFromLeft(46).reduced(2, 6));
    header.removeFromLeft(12);
    undo.setBounds(header.removeFromLeft(56).reduced(0, 6));
    redo.setBounds(header.removeFromLeft(56).reduced(2, 6));
    header.removeFromLeft(14);
    outputGainLabel.setBounds(header.removeFromLeft(52).reduced(0, 10));
    outputGain.setBounds(header.removeFromLeft(juce::jlimit(150, 240, header.getWidth() / 4)).reduced(0, 10));
    bypass.setBounds(header.removeFromLeft(92).reduced(6, 10));
    loudnessReset.setBounds(header.removeFromRight(56).reduced(2, 8));
    header.removeFromRight(6);
    loudnessScreen->setBounds(header.removeFromRight(juce::jmin(header.getWidth() - 10, 380)).reduced(0, 2));
    area.removeFromTop(6);

    auto strip = area.removeFromTop(104);
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
    presetButton.setBounds(waveButtons.removeFromRight(190).reduced(2, 0));
    waveform->setBounds(strip.withTrimmedTop(2));
    area.removeFromTop(8);
    const int layersX = layers.getX();

    layerControls->setBounds(area.removeFromBottom(32));
    area.removeFromBottom(6);

    const bool large = inspector->wantsLargeView();
    for (auto* c : std::initializer_list<juce::Component*> { workspace.get(), nodeList.get(), &tabSpectrum, &tabDifference, &view })
        c->setVisible(!large);
    if (large)
    {
        inspector->setBounds(area); // the module page takes the workspace and the bottom panel
        return;
    }
    auto bottom = area.removeFromBottom(juce::jmax(240, area.getHeight() * 36 / 100));
    nodeList->setBounds(bottom.removeFromLeft(300));
    bottom.removeFromLeft(8);
    inspector->setBounds(bottom);
    area.removeFromBottom(8);

    // the spectrum's tab row, taller: the two portrait buttons stand in it, under the layer buttons
    auto tabs = area.removeFromTop(56);
    const int tubesTop = tabs.getY() - 30, tubesHeight = tabs.getBottom() - tubesTop - 2;
    leninButton->setBounds(layersX, tubesTop, 2 * 52 - 4, tubesHeight);
    stalinButton->setBounds(layersX + 2 * 52, tubesTop, 2 * 52 - 4, tubesHeight);
    auto tabLine = tabs.removeFromBottom(28);
    tabSpectrum.setBounds(tabLine.removeFromLeft(100).reduced(0, 2));
    tabDifference.setBounds(tabLine.removeFromLeft(100).reduced(4, 2));
    view.setBounds(tabLine.removeFromRight(140).reduced(0, 2));
    area.removeFromTop(4);
    workspace->setBounds(area);
}
} // namespace hpeq
