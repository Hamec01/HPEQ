#include "Standalone/ModulePluginEditor.h"

#include "UI/HamloEchoPanel.h"
#include "UI/ModuleHelp.h"
#include "UI/PresetScreen.h"
#include "Core/ModulePresets.h"
#include "UI/ScorePanel.h"

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
            auto p = std::make_unique<HamloEchoPanel>();
            p->readMeters = [this]
            {
                const auto& node = processor.moduleNode();
                return std::pair<float, float> { node.getDelayOutputDb(0), node.getDelayDuckDb(0) };
            };
            p->readTempo = [this] { return processor.getHostBpm(); };
            installPanel(std::move(p));
    }
    scorePanel=std::make_unique<ui::ScorePanel>(processor);addChildComponent(*scorePanel);
    scorePanel->onCategory=[this](juce::String group)
    {presets->setFilter(presets->genre()==ui::PresetScreen::kFactory?juce::String("All"):presets->genre(),group.isEmpty()?juce::String("All"):group);};
    scorePanel->onLoadStart=[this]
    {for(int program:presets->filtered())if(program>=8){processor.setCurrentProgram(program);processor.updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withProgramChanged(true));break;}};
    modeSelector.addItem("LEGACY ENGINE",1);modeSelector.addItem("NOTE SCORE",2);
    modeSelector.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff102016));
    modeSelector.setColour(juce::ComboBox::textColourId,juce::Colour(0xff76f6a0));
    modeLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.valueTreeState(),"echoMode",modeSelector);
    addAndMakeVisible(modeSelector);
    // Hamlo line (2026-10-29): the preset screen top left, as on HamloComp / HamloUdarnik / HamloRedWall / HamloMag
    presets = std::make_unique<ui::PresetScreen>();
    {
        std::vector<ui::PresetScreen::Entry> entries;
        entries.push_back({0,{},{},"Default","Clean start: Mix / feedback / space / glitch / pitch = 0"});
        const auto& bank=hpeq::presets::factoryModulePresets(processor.getSpec().type);
        for (int i = 1; i < processor.getNumPrograms(); ++i)
        {const auto& p=bank[static_cast<size_t>(i-1)];entries.push_back({i,p.genre,p.group,p.name,p.note});}
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
    presets->setCurrent(processor.getCurrentProgram(), false);

    const auto& spec = processor.getSpec();
    setResizable(true, true);
    setResizeLimits(spec.editorWidth / 2, spec.editorHeight / 2, spec.editorWidth * 2, spec.editorHeight * 2);
    setSize(spec.editorWidth, spec.editorHeight);
    if (refreshPanel)
        refreshPanel();
    timerCallback();
    startTimerHz(30);
}

ModulePluginEditor::~ModulePluginEditor()
{
    // FL Studio may close the editor while its audio thread is still running. Timer is a base
    // class, so relying on Timer::~Timer() would stop it only after refreshPanel and panel had
    // already been destroyed. Stop it before touching any callback-owned state.
    stopTimer();
    refreshPanel = {};
    modeLink.reset();scorePanel.reset();
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
    const int mode=processor.parameterValue("echoMode")>.5?1:0;
    if(mode!=shownMode)
    {
        shownMode=mode;panel->setVisible(mode==0);scorePanel->setVisible(mode==1);
        const int height=mode==1?980:760;
        setResizeLimits(885,juce::roundToInt(height*.75),2360,height*2);
        float scale=getWidth()/1180.f;
        if(mode==1)if(const auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())scale=juce::jmin(scale,(display->userArea.getHeight()-160)/980.f);
        scale=juce::jmax(.75f,scale);
        setSize(juce::roundToInt(1180*scale),juce::roundToInt(height*scale));resized();
    }
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
    if(scorePanel!=nullptr)scorePanel->setBounds(getLocalBounds());
    if (presets != nullptr && getPanel() != nullptr)
        if (auto* target = dynamic_cast<ui::HelpTarget*>(getPanel()))
        {
            presets->placeOnStage(target->contentBounds());
            const auto stage=target->contentBounds();const float scale=stage.getWidth()/1180.f;
            modeSelector.setBounds(juce::Rectangle<float>(stage.getX()+940*scale,stage.getY()+124*scale,185*scale,26*scale).toNearestInt());
        }
}

juce::Component* ModulePluginEditor::getPanel() noexcept{return shownMode==1?static_cast<juce::Component*>(scorePanel.get()):panel.get();}
void ModulePluginEditor::selectScorePage(int page){if(scorePanel!=nullptr)scorePanel->selectPage(page);}

} // namespace hpeq::standalone
