#include "Standalone/ModulePluginEditor.h"

#include "UI/HamloShirotaPanel.h"
#include "UI/ModuleHelp.h"
#include "UI/PresetScreen.h"
#include "UI/ShirotaV2Panel.h"

namespace hpeq::standalone
{
template <typename Panel>
void ModulePluginEditor::installPanel(std::unique_ptr<Panel> owned)
{
    auto* const p = owned.get();
    p->onKnobGestureStart = [this](typename Panel::Param parameter)
    {
        beginGesture(Panel::keyFor(parameter));
    };
    p->onKnobChange = [this](typename Panel::Param parameter, double value)
    {
        processor.setParameterValue(Panel::keyFor(parameter), value);
    };
    p->onKnobGestureEnd = [this] { endGesture(); };
    p->onSwitchChange = [this](typename Panel::Switch sw, int value)
    {
        setImmediate(Panel::keyFor(sw), value);
    };
    p->onPowerChange = [this](bool on) { setImmediate("bypass", on ? 0.0 : 1.0); };

    refreshPanel = [this, p]
    {
        typename Panel::Values values;
        for (size_t i = 0; i < values.knobs.size(); ++i)
            values.knobs[i] = processor.parameterValue(Panel::keyFor(static_cast<typename Panel::Param>(i)));
        for (size_t i = 0; i < values.switches.size(); ++i)
            values.switches[i] = juce::roundToInt(processor.parameterValue(Panel::keyFor(static_cast<typename Panel::Switch>(i))));
        values.on = processor.isModuleOn();
        p->setValues(values);
    };

    panel = std::move(owned);
    addAndMakeVisible(*panel);
}

ModulePluginEditor::ModulePluginEditor(ModulePluginProcessor& owner)
    : juce::AudioProcessorEditor(owner), processor(owner)
{
    using namespace hpeq::ui;
    {
            auto p = std::make_unique<HamloShirotaPanel>();
            p->readMeters = [this]
            {
                const auto m = processor.moduleNode().getStereoMeters(0);
                return HamloShirotaPanel::Reading { m.correlation, m.guardDb, m.oneChannel };
            };
            p->readScope = [this](std::array<float, HamloShirotaPanel::kScopePoints>& left,
                                  std::array<float, HamloShirotaPanel::kScopePoints>& right)
            {
                return processor.moduleNode().getStereoScope(0, left, right);
            };
            installPanel(std::move(p));
    }

    {auto original=std::move(panel);removeChildComponent(original.get());auto wrapped=std::make_unique<ui::ShirotaV2Panel>(processor,std::move(original));auto* p=wrapped.get();auto oldRefresh=std::move(refreshPanel);refreshPanel=[p,oldRefresh]{if(oldRefresh)oldRefresh();p->refresh();};panel=std::move(wrapped);addAndMakeVisible(*panel);}
    // Hamlo line: shared preset screen remains unchanged and top-left.
    presets = std::make_unique<ui::PresetScreen>();
    {
        std::vector<ui::PresetScreen::Entry> entries;
        for (int i = 1; i < processor.getNumPrograms(); ++i)
            entries.push_back({ i, {}, {}, processor.getProgramName(i), {} });
        presets->setEntries(std::move(entries));
    }
    presets->onSelect = [this](int program)
    {
        processor.setCurrentProgram(program);
        processor.updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withProgramChanged(true));
        if (refreshPanel)
            refreshPanel();
    };
    addAndMakeVisible(*presets);
    presets->setCurrent(processor.getCurrentProgram(), processor.presetModified());

    const auto& spec = processor.getSpec();
    setResizable(true, true);
    setResizeLimits(spec.editorWidth / 2, spec.editorHeight / 2, spec.editorWidth * 2, spec.editorHeight * 2);
    setSize(spec.editorWidth, spec.editorHeight);
    if (refreshPanel)
        refreshPanel();
    startTimerHz(30);
}

ModulePluginEditor::~ModulePluginEditor()
{
    // FL Studio may close the editor while its audio thread is still running. Timer is a base
    // class, so relying on Timer::~Timer() would stop it only after refreshPanel and panel had
    // already been destroyed. Stop it before touching any callback-owned state.
    stopTimer();
    refreshPanel = {};
    endGesture();
    panel.reset();
}

void ModulePluginEditor::beginGesture(const juce::String& id)
{
    endGesture();
    activeGesture = processor.parameter(id);
    if (activeGesture != nullptr)
        activeGesture->beginChangeGesture();
}

void ModulePluginEditor::endGesture()
{
    if (activeGesture != nullptr)
        activeGesture->endChangeGesture();
    activeGesture = nullptr;
}

void ModulePluginEditor::setImmediate(const juce::String& id, double value)
{
    if (auto* p = processor.parameter(id))
    {
        p->beginChangeGesture();
        processor.setParameterValue(id, value);
        p->endChangeGesture();
    }
}

void ModulePluginEditor::timerCallback()
{
    selectSoftwareRenderer();
    if (refreshPanel)
        refreshPanel();
    if (presets != nullptr)
        presets->setCurrent(processor.getCurrentProgram(), processor.presetModified());
}

void ModulePluginEditor::visibilityChanged()
{
    selectSoftwareRenderer();
}

void ModulePluginEditor::parentHierarchyChanged()
{
    selectSoftwareRenderer();
}

void ModulePluginEditor::selectSoftwareRenderer()
{
   #if JUCE_WINDOWS
    // JUCE 8 defaults every Windows peer to Direct2D. FL Studio can unload an individual VST3
    // while transport is running; tearing down that per-DLL D3D11 device has been observed to
    // wait indefinitely inside the NVIDIA driver. These panels are cached bitmap/vector UIs and
    // do not need a GPU renderer, so select the GDI software backend before the first paint.
    if (auto* peer = getPeer(); peer != nullptr && peer->getCurrentRenderingEngine() != 0)
        peer->setCurrentRenderingEngine(0);
   #endif
}

void ModulePluginEditor::resized()
{
    if (panel != nullptr)
        panel->setBounds(getLocalBounds());
    if (presets != nullptr && panel != nullptr)
        if (auto* target = dynamic_cast<ui::HelpTarget*>(panel.get()))
            presets->placeOnStage(target->contentBounds());
}

} // namespace hpeq::standalone
