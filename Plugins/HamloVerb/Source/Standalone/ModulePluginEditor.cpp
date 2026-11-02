#include "Standalone/ModulePluginEditor.h"
#include "UI/VerbV2Panel.h"
#include "Core/ModulePresets.h"
#include <algorithm>

namespace hpeq::standalone
{
ModulePluginEditor::ModulePluginEditor(ModulePluginProcessor& owner)
    : juce::AudioProcessorEditor(owner), processor(owner)
{
    presets = std::make_unique<ui::PresetScreen>();
    std::vector<ui::PresetScreen::Entry> entries;
    const auto& bank = presets::factoryModulePresets(processor.getSpec().type);
    for (size_t i = 0; i < bank.size(); ++i)
        entries.push_back({ static_cast<int>(i+1), bank[i].genre, bank[i].group, bank[i].name, bank[i].note });
    presets->setEntries(std::move(entries));
    auto surface = std::make_unique<ui::VerbV2Panel>(processor.getSpec());
    auto* view = surface.get();
    view->readValue = [this](const juce::String& id) { return processor.parameterValue(id); };
    view->readSampleRate = [this] { return processor.getSampleRate() > 0 ? processor.getSampleRate() : 48000.; };
    view->onChange = [this](const juce::String& id, double value) { processor.setParameterValue(id, value); };
    view->onGestureStart = [this](const juce::String& id) { beginGesture(id); };
    view->onGestureEnd = [this] { endGesture(); };
    view->onMixLock = [this](bool locked) { processor.setMixLock(locked); };
    view->onMaterial = [this](const juce::String& group) { presets->setFilter(presets->genre(), group); };
    view->onLoadStart = [this](const juce::String& group)
    {
        const auto& recipes = presets::factoryModulePresets(processor.getSpec().type);
        const auto genre = presets->genre();
        for (size_t i = 7; i < recipes.size(); ++i)
            if ((group == "All" || group == recipes[i].group) && (genre == "All" || genre == "Factory" || genre == recipes[i].genre))
            {
                processor.setCurrentProgram(static_cast<int>(i+1));
                processor.updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withProgramChanged(true));
                refreshPanel(); return;
            }
    };
    refreshPanel = [this, view] { view->refresh(processor.getMixLock(), processor.getOutputDb(), processor.getDuckDb()); };
    panel = std::move(surface); addAndMakeVisible(*panel);
    presets->onSelect = [this](int program)
    {
        processor.setCurrentProgram(program);
        processor.updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withProgramChanged(true));
        refreshPanel();
    };
    addAndMakeVisible(*presets);
    presets->setCurrent(processor.getCurrentProgram(), processor.isProgramModified());
    setResizable(true, true);
    setResizeLimits(885, 705, 2360, 1880);
    setSize(processor.getSpec().editorWidth, processor.getSpec().editorHeight);
    refreshPanel(); startTimerHz(20);
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
    if (auto* parameter = processor.parameter(id))
        if (std::find(activeGestures.begin(), activeGestures.end(), parameter) == activeGestures.end())
        { activeGestures.push_back(parameter); parameter->beginChangeGesture(); }
}

void ModulePluginEditor::endGesture()
{
    for (auto* parameter : activeGestures) parameter->endChangeGesture();
    activeGestures.clear();
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
        presets->setCurrent(processor.getCurrentProgram(), processor.isProgramModified());
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
