#include "Plugin/HamloRedWallEditor.h"

#include "Plugin/HamloRedWallProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"

namespace hamlo::redwall
{
using Panel = ui::HamloRedWallPanel;

HamloRedWallEditor::HamloRedWallEditor(HamloRedWallProcessor& owner)
    : juce::AudioProcessorEditor(owner), processor(owner), panel(std::make_unique<Panel>()), presets(std::make_unique<ui::PresetScreen>())
{
    panel->onKnobGestureStart = [this](Panel::Param p) { beginGesture(Panel::keyFor(p)); };
    panel->onKnobChange = [this](Panel::Param p, double value) { processor.setParameterValue(Panel::keyFor(p), value); };
    panel->onKnobGestureEnd = [this] { endGesture(); };
    panel->onSwitchChange = [this](Panel::Switch sw, int value) { setImmediate(Panel::keyFor(sw), value); };
    panel->onPowerChange = [this](bool on) { setImmediate("bypass", on ? 0.0 : 1.0); };
    panel->readMeters = [this]
    {
        const auto& l = processor.limiter();
        return std::pair<float, float> { l.lastOutputPeakDb(), l.lastGainReductionDb() };
    };
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

HamloRedWallEditor::~HamloRedWallEditor()
{
    // FL Studio may close the editor while its audio thread is still running: stop the timer before the
    // panel and its callbacks go away.
    stopTimer();
    endGesture();
    panel.reset();
}

void HamloRedWallEditor::refresh()
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

void HamloRedWallEditor::beginGesture(const juce::String& id)
{
    endGesture();
    activeGesture = processor.parameter(id);
    if (activeGesture != nullptr)
        activeGesture->beginChangeGesture();
}

void HamloRedWallEditor::endGesture()
{
    if (activeGesture != nullptr)
        activeGesture->endChangeGesture();
    activeGesture = nullptr;
}

void HamloRedWallEditor::setImmediate(const juce::String& id, double value)
{
    if (auto* p = processor.parameter(id))
    {
        p->beginChangeGesture();
        processor.setParameterValue(id, value);
        p->endChangeGesture();
    }
}

void HamloRedWallEditor::timerCallback()
{
    selectSoftwareRenderer();
    refresh();
}

void HamloRedWallEditor::visibilityChanged()
{
    selectSoftwareRenderer();
}

void HamloRedWallEditor::parentHierarchyChanged()
{
    selectSoftwareRenderer();
}

void HamloRedWallEditor::selectSoftwareRenderer()
{
   #if JUCE_WINDOWS
    // JUCE 8 defaults every Windows peer to Direct2D; FL Studio can unload a VST3 while transport runs and
    // tearing down its D3D11 device has been seen to hang in the NVIDIA driver. The page is a cached bitmap /
    // vector UI, so the GDI software backend is used (as HPEQ's module plug-ins do).
    if (auto* peer = getPeer(); peer != nullptr && peer->getCurrentRenderingEngine() != 0)
        peer->setCurrentRenderingEngine(0);
   #endif
}

void HamloRedWallEditor::resized()
{
    panel->setBounds(getLocalBounds());
    presets->placeOnStage(panel->contentBounds());
}
} // namespace hamlo::redwall
