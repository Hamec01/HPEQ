#include "Standalone/ModulePluginEditor.h"

#include "UI/HamloZastavaPanel.h"
#include "UI/ModuleHelp.h"
#include "UI/PresetScreen.h"

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
            auto p = std::make_unique<HamloZastavaPanel>();
            p->readMeters = [this]
            {
                const auto m = processor.moduleNode().getGateMeters(0);
                return HamloZastavaPanel::Reading { m.attenuationDb, m.maxAttenuationDb, m.keyDb, m.open, m.openings };
            };
            installPanel(std::move(p));
        }
    modernPanel=std::make_unique<ModernZastavaPanel>(processor);addAndMakeVisible(*modernPanel);

    // Hamlo line (2026-10-29): the preset screen top left, as on HamloComp / HamloUdarnik / HamloRedWall / HamloMag
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
    presets->setCurrent(processor.getCurrentProgram(), processor.isProgramModified());
    const char* titles[]{"QUICK","ADVANCED","LEGACY"};
    for(int n=0;n<3;++n)
    {
        auto& b=navigation[static_cast<size_t>(n)];b.setButtonText(titles[n]);b.setComponentID("view_"+juce::String(titles[n]).toLowerCase());
        b.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff202923));b.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff245b37));b.setColour(juce::TextButton::textColourOffId,steel::stencil);b.setColour(juce::TextButton::textColourOnId,steel::phosphor);
        b.onClick=[this,n]{showView(n);};b.setTooltip(n==2?"Select the original Legacy engine and page. Modern settings are retained.":"Open modern controls. If Legacy is selected, return to the last modern role.");addAndMakeVisible(b);
    }
    tooltip=std::make_unique<juce::TooltipWindow>(this,600);

    const auto& spec = processor.getSpec();
    setResizable(true, true);
    setResizeLimits(spec.editorWidth / 2, spec.editorHeight / 2, spec.editorWidth * 2, spec.editorHeight * 2);
    setSize(spec.editorWidth, spec.editorHeight);
    if (refreshPanel)
        refreshPanel();
    refreshEditor();
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
    tooltip.reset();modernPanel.reset();
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
{refreshEditor();}

void ModulePluginEditor::showView(int view)
{
    if(view==2){setImmediate("v2_listen",0);setImmediate("v2_engine",0);}
    else if(processor.parameterValue("v2_engine")==0)setImmediate("v2_engine",lastEngine);
    processor.setEditorView(view);refreshEditor();
}

void ModulePluginEditor::refreshEditor()
{
    selectSoftwareRenderer();
    if (refreshPanel)
        refreshPanel();
    const int engine=juce::roundToInt(processor.parameterValue("v2_engine"));
    if(engine!=lastSeenEngine)
    {
        if(engine==0)processor.setEditorView(2);
        else {lastEngine=engine;if(processor.editorView()==2)processor.setEditorView(0);}
        lastSeenEngine=engine;
    }
    const bool legacy=processor.editorView()==2;
    panel->setVisible(legacy);modernPanel->setVisible(!legacy);modernPanel->refresh();
    for(int n=0;n<3;++n)navigation[static_cast<size_t>(n)].setToggleState(processor.editorView()==n,juce::dontSendNotification);
    if (presets != nullptr)presets->setCurrent(processor.getCurrentProgram(), processor.isProgramModified());
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
    if(modernPanel!=nullptr)modernPanel->setBounds(getLocalBounds());
    if (presets != nullptr && panel != nullptr)
        if (auto* target = dynamic_cast<ui::HelpTarget*>(panel.get()))
            presets->placeOnStage(target->contentBounds());
    const float s=juce::jmin(getWidth()/1180.f,getHeight()/780.f),x=(getWidth()-1180*s)*.5f,y=(getHeight()-780*s)*.5f;
    for(int n=0;n<3;++n)navigation[static_cast<size_t>(n)].setBounds(juce::Rectangle<float>(812.f+n*108.f,20,96,30).transformedBy(juce::AffineTransform::scale(s).translated(x,y)).toNearestInt());
}

} // namespace hpeq::standalone
