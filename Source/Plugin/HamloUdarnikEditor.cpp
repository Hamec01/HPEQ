#include "Plugin/HamloUdarnikEditor.h"

#include "Plugin/HamloUdarnikProcessor.h"
#include "Plugin/Parameters.h"

namespace hamlo::udarnik
{
using Panel = ui::HamloUdarnikPanel;

HamloUdarnikEditor::HamloUdarnikEditor(HamloUdarnikProcessor& owner)
    : juce::AudioProcessorEditor(owner), processor(owner), panel(std::make_unique<Panel>())
{
    panel->onKnobGestureStart = [this](Panel::Param p) { beginGesture(Panel::keyFor(p)); };
    panel->onKnobChange = [this](Panel::Param p, double value) { processor.setParameterValue(Panel::keyFor(p), value); };
    panel->onKnobGestureEnd = [this] { endGesture(); };
    panel->onSwitchChange = [this](Panel::Switch sw, int value) { setImmediate(Panel::keyFor(sw), value); };
    panel->onPowerChange = [this](bool on) { setImmediate("bypass", on ? 0.0 : 1.0); };
    panel->readMeters = [this]
    {
        const auto& shaper = processor.shaper();
        if (!shaper.udarnikActive() && !shaper.v2Active())
            return Panel::Reading {};
        const auto m = shaper.v2Active() ? shaper.engine2().meters() : shaper.udarnik().meters();
        return Panel::Reading { m.impactDb, m.gainMinDb, m.gainMaxDb, m.events, m.confidence, m.strengthDb };
    };
    panel->readLastHit = [this](std::array<float, Panel::kHitPoints>& input, std::array<float, Panel::kHitPoints>& output)
    {
        const auto& shaper = processor.shaper();
        return shaper.v2Active() ? shaper.engine2().lastHit(input, output) : shaper.udarnikActive() ? shaper.udarnik().lastHit(input, output) : 0;
    };
    addAndMakeVisible(*panel);

    setResizable(true, true);
    setResizeLimits(kEditorWidth / 2, kEditorHeight / 2, kEditorWidth * 2, kEditorHeight * 2);
    setSize(kEditorWidth, kEditorHeight);
    refresh();
    startTimerHz(30);
}

HamloUdarnikEditor::~HamloUdarnikEditor()
{
    // FL Studio may close the editor while its audio thread is still running: stop the timer before the
    // panel and its callbacks go away.
    stopTimer();
    endGesture();
    panel.reset();
}

void HamloUdarnikEditor::refresh()
{
    Panel::Values values;
    for (size_t i = 0; i < values.knobs.size(); ++i)
        values.knobs[i] = processor.parameterValue(Panel::keyFor(static_cast<Panel::Param>(i)));
    for (size_t i = 0; i < values.switches.size(); ++i)
        values.switches[i] = juce::roundToInt(processor.parameterValue(Panel::keyFor(static_cast<Panel::Switch>(i))));
    values.on = processor.isOn();
    panel->setValues(values);
}

void HamloUdarnikEditor::beginGesture(const juce::String& id)
{
    endGesture();
    activeGesture = processor.parameter(id);
    if (activeGesture != nullptr)
        activeGesture->beginChangeGesture();
}

void HamloUdarnikEditor::endGesture()
{
    if (activeGesture != nullptr)
        activeGesture->endChangeGesture();
    activeGesture = nullptr;
}

void HamloUdarnikEditor::setImmediate(const juce::String& id, double value)
{
    if (auto* p = processor.parameter(id))
    {
        p->beginChangeGesture();
        processor.setParameterValue(id, value);
        p->endChangeGesture();
    }
}

void HamloUdarnikEditor::timerCallback()
{
    selectSoftwareRenderer();
    refresh();
}

void HamloUdarnikEditor::visibilityChanged()
{
    selectSoftwareRenderer();
}

void HamloUdarnikEditor::parentHierarchyChanged()
{
    selectSoftwareRenderer();
}

void HamloUdarnikEditor::selectSoftwareRenderer()
{
   #if JUCE_WINDOWS
    // JUCE 8 defaults every Windows peer to Direct2D; FL Studio can unload a VST3 while transport runs and
    // tearing down its D3D11 device has been seen to hang in the NVIDIA driver. The page is a cached bitmap /
    // vector UI, so the GDI software backend is used (as HPEQ's module plug-ins do).
    if (auto* peer = getPeer(); peer != nullptr && peer->getCurrentRenderingEngine() != 0)
        peer->setCurrentRenderingEngine(0);
   #endif
}

void HamloUdarnikEditor::resized()
{
    panel->setBounds(getLocalBounds());
}
} // namespace hamlo::udarnik
