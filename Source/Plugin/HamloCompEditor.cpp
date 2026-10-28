#include "Plugin/HamloCompEditor.h"

#include "Plugin/HamloCompProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"

namespace hamlo::comp
{
using Panel = ui::HamloCompPanel;

HamloCompEditor::HamloCompEditor(HamloCompProcessor& owner)
    : juce::AudioProcessorEditor(owner), processor(owner), panel(std::make_unique<Panel>()), presets(std::make_unique<ui::PresetScreen>())
{
    panel->onKnobGestureStart = [this](Panel::Param p) { beginGesture(Panel::keyFor(p)); };
    panel->onKnobChange = [this](Panel::Param p, double value) { processor.setParameterValue(Panel::keyFor(p), value); };
    panel->onKnobGestureEnd = [this] { endGesture(); };
    panel->onSwitchChange = [this](Panel::Switch sw, int value) { setImmediate(Panel::keyFor(sw), value); };
    panel->onPowerChange = [this](bool on) { setImmediate("bypass", on ? 0.0 : 1.0); };
    panel->readMeters = [this] { return std::pair<float, float> { processor.inputLevelDb(), processor.gainReductionDb() }; };
    addAndMakeVisible(*panel);
    {
        std::vector<ui::PresetScreen::Entry> entries;
        const auto& list = factoryPresets();
        for (size_t i = 0; i < list.size(); ++i)
            entries.push_back({ static_cast<int>(i) + 1, list[i].genre, list[i].group, list[i].name, list[i].note });
        presets->setEntries(std::move(entries));
    }
    presets->onSelect = [this](int program)
    {
        processor.setCurrentProgram(program);
        processor.updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withProgramChanged(true));
        refresh();
    };
    addAndMakeVisible(*presets);

    setResizable(true, true);
    setResizeLimits(kEditorWidth / 2, kEditorHeight / 2, kEditorWidth * 2, kEditorHeight * 2);
    setSize(kEditorWidth, kEditorHeight);
    refresh();
    startTimerHz(30);
}

HamloCompEditor::~HamloCompEditor()
{
    // FL Studio may close the editor while its audio thread is still running: stop the timer before the
    // panel and its callbacks go away.
    stopTimer();
    endGesture();
    panel.reset();
}

void HamloCompEditor::refresh()
{
    Panel::Values values;
    for (size_t i = 0; i < values.knobs.size(); ++i)
        values.knobs[i] = processor.parameterValue(Panel::keyFor(static_cast<Panel::Param>(i)));
    for (size_t i = 0; i < values.switches.size(); ++i)
        values.switches[i] = juce::roundToInt(processor.parameterValue(Panel::keyFor(static_cast<Panel::Switch>(i))));
    values.on = processor.isOn();
    panel->setValues(values);
    presets->setCurrent(processor.getCurrentProgram(), processor.programModified());
}

void HamloCompEditor::beginGesture(const juce::String& id)
{
    endGesture();
    activeGesture = processor.parameter(id);
    if (activeGesture != nullptr)
        activeGesture->beginChangeGesture();
}

void HamloCompEditor::endGesture()
{
    if (activeGesture != nullptr)
        activeGesture->endChangeGesture();
    activeGesture = nullptr;
}

void HamloCompEditor::setImmediate(const juce::String& id, double value)
{
    if (auto* p = processor.parameter(id))
    {
        p->beginChangeGesture();
        processor.setParameterValue(id, value);
        p->endChangeGesture();
    }
}

void HamloCompEditor::timerCallback()
{
    selectSoftwareRenderer();
    refresh();
}

void HamloCompEditor::visibilityChanged()
{
    selectSoftwareRenderer();
}

void HamloCompEditor::parentHierarchyChanged()
{
    selectSoftwareRenderer();
}

void HamloCompEditor::selectSoftwareRenderer()
{
   #if JUCE_WINDOWS
    // JUCE 8 defaults every Windows peer to Direct2D. FL Studio can unload a VST3 while transport is
    // running, and tearing down its D3D11 device has been seen to hang inside the NVIDIA driver; the page is
    // a cached bitmap / vector UI, so the GDI software backend is used (as HPEQ's module plug-ins do).
    if (auto* peer = getPeer(); peer != nullptr && peer->getCurrentRenderingEngine() != 0)
        peer->setCurrentRenderingEngine(0);
   #endif
}

void HamloCompEditor::resized()
{
    panel->setBounds(getLocalBounds());
    presets->placeOnStage(panel->contentBounds());
}
} // namespace hamlo::comp
