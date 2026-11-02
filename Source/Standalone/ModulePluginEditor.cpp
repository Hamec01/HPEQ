#include "Standalone/ModulePluginEditor.h"

#include "UI/HamloCompPanel.h"
#include "UI/HamloEchoPanel.h"
#include "UI/HamloMagPanel.h"
#include "UI/HamloRedWallPanel.h"
#include "UI/HamloRevolutionPanel.h"
#include "UI/HamloShirotaPanel.h"
#include "UI/HamloUdarnikPanel.h"
#include "UI/HamloVerbPanel.h"
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
    switch (processor.getKind())
    {
        case ModuleKind::Comp:
        {
            auto p = std::make_unique<HamloCompPanel>();
            p->readMeters = [this]
            {
                const auto& node = processor.moduleNode();
                return std::pair<float, float> { node.getCompressorInputDb(0), node.getGainReductionDb(0) };
            };
            installPanel(std::move(p));
            break;
        }
        case ModuleKind::Verb:
        {
            auto p = std::make_unique<HamloVerbPanel>();
            p->readMeters = [this]
            {
                const auto& node = processor.moduleNode();
                return std::pair<float, float> { node.getReverbOutputDb(0), node.getReverbDuckDb(0) };
            };
            installPanel(std::move(p));
            break;
        }
        case ModuleKind::Echo:
        {
            auto p = std::make_unique<HamloEchoPanel>();
            p->readMeters = [this]
            {
                const auto& node = processor.moduleNode();
                return std::pair<float, float> { node.getDelayOutputDb(0), node.getDelayDuckDb(0) };
            };
            p->readTempo = [this] { return processor.getHostBpm(); };
            installPanel(std::move(p));
            break;
        }
        case ModuleKind::Mag:
        {
            auto p = std::make_unique<HamloMagPanel>();
            p->readMeters = [this]
            {
                const auto& node = processor.moduleNode();
                return std::pair<float, float> { node.getSaturatorInputDb(0), node.getSaturatorOutputDb(0) };
            };
            installPanel(std::move(p));
            break;
        }
        case ModuleKind::RedWall:
        {
            auto p = std::make_unique<HamloRedWallPanel>();
            p->readMeters = [this]
            {
                const auto& node = processor.moduleNode();
                return std::pair<float, float> { node.getLimiterOutputDb(0), node.getGainReductionDb(0) };
            };
            installPanel(std::move(p));
            break;
        }
        case ModuleKind::Udarnik:
        {
            auto p = std::make_unique<HamloUdarnikPanel>();
            p->readMeters = [this]
            {
                const auto m = processor.moduleNode().getTransientMeters(0);
                return HamloUdarnikPanel::Reading { m.impactDb, m.gainMinDb, m.gainMaxDb, m.events, m.confidence, m.strengthDb };
            };
            p->readLastHit = [this](std::array<float, HamloUdarnikPanel::kHitPoints>& input,
                                    std::array<float, HamloUdarnikPanel::kHitPoints>& output)
            {
                return processor.moduleNode().getTransientLastHit(0, input, output);
            };
            installPanel(std::move(p));
            break;
        }
        case ModuleKind::Zastava:
        {
            auto p = std::make_unique<HamloZastavaPanel>();
            p->readMeters = [this]
            {
                const auto m = processor.moduleNode().getGateMeters(0);
                return HamloZastavaPanel::Reading { m.attenuationDb, m.maxAttenuationDb, m.keyDb, m.open, m.openings };
            };
            installPanel(std::move(p));
            break;
        }
        case ModuleKind::Revolution:
        {
            auto p = std::make_unique<HamloRevolutionPanel>();
            auto* const page = p.get();
            p->onPatternChange = [this](const revolution::Pattern& pattern, juce::int64 seed, const revolution::Feel& feel, const juce::String&)
            {
                auto state = processor.getRevolutionState(); // the scene shown; its lanes stay
                state.pattern = pattern;
                state.feel = feel;
                state.seed = seed;
                processor.setRevolutionState(state);
            };
            p->onLanesChange = [this](const revolution::Lanes& lanes, const juce::String&)
            {
                auto state = processor.getRevolutionState();
                state.lanes = lanes;
                processor.setRevolutionState(state);
            };
            p->onSceneSelect = [this](int scene) { setImmediate("scene", scene); };
            p->onSceneCopyTo = [this](int target) { processor.setScene(target, processor.getRevolutionState()); };
            p->onSceneClear = [this] { processor.clearScene(processor.currentScene()); };
            p->onUpgrade = [] {}; // the plug-in never runs the legacy M6 Monsta
            p->readStatus = [this]() -> HamloRevolutionPanel::Status
            {
                const auto& node = processor.moduleNode();
                HamloRevolutionPanel::Status s { node.getRevolutionStep(0), node.isRevolutionWarming(0), {} };
                node.getRevolutionFeel(0, s.feel);
                node.getRevolutionWave(0, s.waveIn, s.waveOut);
                return s;
            };
            installPanel(std::move(p));
            refreshPanel = [this, page]
            {
                HamloRevolutionPanel::Values values;
                for (size_t i = 0; i < values.knobs.size(); ++i)
                    values.knobs[i] = processor.parameterValue(HamloRevolutionPanel::keyFor(static_cast<HamloRevolutionPanel::Param>(i)));
                for (size_t i = 0; i < values.switches.size(); ++i)
                    values.switches[i] = juce::roundToInt(processor.parameterValue(HamloRevolutionPanel::keyFor(static_cast<HamloRevolutionPanel::Switch>(i))));
                const auto state = processor.getRevolutionState();
                values.pattern = state.pattern;
                values.feel = state.feel;
                values.seed = state.seed;
                values.lanes = state.lanes;
                values.scene = processor.currentScene();
                for (int s = 0; s < revolution::kScenes; ++s)
                    values.sceneUsed[static_cast<size_t>(s)] = processor.isSceneUsed(s);
                values.on = processor.isModuleOn();
                page->setValues(values);
            };
            page->onLevelLaneGenerate = [this](int lane) { processor.generateLane(lane); };
            page->onSimple = [this]
            {
                processor.setPage(ModulePluginProcessor::Page::Simple);
                applyPage();
            };
            installSimplePage();
            installTransitionPage();
            tooltips = std::make_unique<juce::TooltipWindow>(this, 600);
            break;
        }
        case ModuleKind::Shirota:
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
            break;
        }
    }

    // Hamlo line (2026-10-29): the preset screen top left, as on HamloComp / HamloUdarnik / HamloRedWall / HamloMag
    presets = std::make_unique<ui::PresetScreen>();
    {
        std::vector<ui::PresetScreen::Entry> entries;
        for (int i = 1; i < processor.getNumPrograms(); ++i)
            if (const auto* g = processor.genrePresetFor(i)) // R6: genre / group / name / what it is for
                entries.push_back({ i, g->genre, g->group, g->name, g->note });
            else
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
    presets->setCurrent(processor.getCurrentProgram(), false);

    applyPage();
    const auto& spec = processor.getSpec();
    setResizable(true, true);
    setResizeLimits(spec.editorWidth / 2, spec.editorHeight / 2, spec.editorWidth * 2, spec.editorHeight * 2);
    setSize(spec.editorWidth, spec.editorHeight);
    if (refreshPanel)
        refreshPanel();
    if (refreshSimple)
        refreshSimple();
    startTimerHz(30);
}

ModulePluginEditor::~ModulePluginEditor()
{
    // FL Studio may close the editor while its audio thread is still running. Timer is a base
    // class, so relying on Timer::~Timer() would stop it only after refreshPanel and panel had
    // already been destroyed. Stop it before touching any callback-owned state.
    stopTimer();
    refreshPanel = {};
    refreshSimple = {};
    refreshTransition = {};
    tooltips.reset();
    processor.setPreview(false);
    transitionPage.reset();
    endGesture();
    simplePage.reset();
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
    applyPage();
    if (simplePage != nullptr && simplePage->isVisible())
    {
        if (refreshSimple)
            refreshSimple();
    }
    else if (transitionPage != nullptr && transitionPage->isVisible())
    {
        if (refreshTransition)
            refreshTransition();
    }
    else if (refreshPanel)
        refreshPanel();
    if (presets != nullptr)
        presets->setCurrent(processor.getCurrentProgram(), false);
}

void ModulePluginEditor::visibilityChanged()
{
    selectSoftwareRenderer();
}

void ModulePluginEditor::parentHierarchyChanged()
{
    selectSoftwareRenderer();
}

void ModulePluginEditor::applyPage()
{
    if (simplePage == nullptr)
        return;
    const auto page = processor.getPage();
    const bool simple = page == ModulePluginProcessor::Page::Simple, transition = page == ModulePluginProcessor::Page::Transition && transitionPage != nullptr;
    const bool advanced = !simple && !transition;
    if (simplePage->isVisible() == simple && (panel == nullptr || panel->isVisible() == advanced)
        && (transitionPage == nullptr || transitionPage->isVisible() == transition))
        return;
    simplePage->setVisible(simple);
    if (panel != nullptr)
        panel->setVisible(advanced);
    if (transitionPage != nullptr)
        transitionPage->setVisible(transition);
    if (!transition)
        processor.setPreview(false);
    // the page coming up shows the processor's state at once (the hidden one is not refreshed)
    if (simple && refreshSimple)
        refreshSimple();
    else if (transition && refreshTransition)
        refreshTransition();
    else if (advanced && refreshPanel)
        refreshPanel();
}

void ModulePluginEditor::installSimplePage()
{
    using Page = ui::HamloRevolutionSimplePage;
    namespace simple = revolution::simple;
    simplePage = std::make_unique<Page>();
    auto* const sp = simplePage.get();
    sp->onKnobGestureStart = [this](Page::Knob k) { beginGesture(Page::keyFor(k)); };
    sp->onKnobChange = [this](Page::Knob k, double v)
    {
        processor.setParameterValue(Page::keyFor(k), v);
        if (k != Page::Mix && k != Page::Groove) // GROOVE and MIX are read by the DSP, nothing to rewrite
            processor.simpleEdited();
    };
    sp->onKnobGestureEnd = [this] { endGesture(); };
    sp->onChoice = [this](const char* id, int value)
    {
        setImmediate(id, value);
        if (juce::String(id) != "simpleLock")
            processor.simpleEdited();
    };
    sp->onGenerate = [this] { processor.generateSimple(true); };
    sp->onTransition = [this](int kind) { openTransition(kind); };
    sp->onAdvanced = [this]
    {
        processor.setPage(ModulePluginProcessor::Page::Advanced);
        applyPage();
    };
    sp->onConnect = [this]
    {
        auto& link = processor.getHpdgLink();
        if (link.isConnected())
            link.disconnect();
        else
            link.connect(0); // the first live HPDG
    };
    sp->onConnectMenu = [this, sp]
    {
        const auto list = hamlo::link::listInstances(juce::Time::currentTimeMillis());
        juce::PopupMenu menu;
        menu.addSectionHeader("Running HPDG instances");
        std::vector<std::uint64_t> ids;
        for (const auto& l : list)
            if (l.status == hamlo::link::Status::Ok)
            {
                ids.push_back(l.instanceId);
                menu.addItem(static_cast<int>(ids.size()), l.name + "  -  " + juce::String(juce::roundToInt(l.bpm)) + " BPM", true,
                             processor.getHpdgLink().getInstanceId() == l.instanceId);
            }
        if (ids.empty())
            menu.addItem(-1, "(none running)", false);
        menu.addSeparator();
        menu.addItem(1000, "Disconnect", processor.getHpdgLink().isConnected());
        juce::Component::SafePointer<Page> safe(sp);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(sp), [this, safe, ids](int result)
        {
            if (safe == nullptr || result == 0)
                return;
            if (result == 1000)
                processor.getHpdgLink().disconnect();
            else if (result > 0 && result <= static_cast<int>(ids.size()))
                processor.getHpdgLink().connect(ids[static_cast<size_t>(result - 1)]);
        });
    };
    refreshSimple = [this, sp]
    {
        Page::Values v;
        for (int k = 0; k < Page::NumKnobs; ++k)
            v.knobs[static_cast<size_t>(k)] = processor.parameterValue(Page::keyFor(static_cast<Page::Knob>(k)));
        v.genre = juce::roundToInt(processor.parameterValue("simpleGenre"));
        v.fill = juce::roundToInt(processor.parameterValue("simpleFill"));
        v.target = juce::roundToInt(processor.parameterValue("simpleTarget"));
        v.lock = processor.parameterValue("simpleLock") >= 0.5;
        const auto last = processor.lastSimple();
        const bool generated = !last.parameters.empty();
        v.shownGenre = generated ? last.genre : simple::resolveGenre(v.genre, {}, processor.getHostBpm());
        v.shownIntent = simple::intentOf(v.knobs[Page::Intent]);
        auto& link = processor.getHpdgLink();
        auto snapshot = std::make_unique<hamlo::link::Snapshot>();
        const bool haveSnapshot = link.last(*snapshot);
        const auto hpdgName = haveSnapshot ? juce::String::fromUTF8(snapshot->instanceName, static_cast<int>(strnlen(snapshot->instanceName, hamlo::link::kNameChars)))
                                           : juce::String();
        static constexpr const char* hpdgGenres[] { "BOOM BAP", "RAP", "TRAP", "DRILL", "DNB", "TECHNO", "ATMOSPHERIC" };
        switch (link.getState())
        {
            case hamlo::link::Client::State::Off: v.link = Page::Link::Off; v.linkText = "NOT CONNECTED"; break;
            case hamlo::link::Client::State::Searching: v.link = Page::Link::Searching; v.linkText = "SEARCHING FOR HPDG..."; break;
            case hamlo::link::Client::State::Linked:
                v.link = Page::Link::Linked;
                v.linkText = haveSnapshot ? (hpdgName.isEmpty() ? juce::String("HPDG") : hpdgName.toUpperCase()) + "  " + juce::String(juce::roundToInt(snapshot->bpm))
                                                + " BPM  " + hpdgGenres[juce::jlimit(0, 6, static_cast<int>(snapshot->genre))]
                                          : juce::String("LINKED");
                break;
            case hamlo::link::Client::State::Lost: v.link = Page::Link::Lost; v.linkText = "HPDG GONE: LAST PATTERN KEPT"; break;
            case hamlo::link::Client::State::Update: v.link = Page::Link::Update; v.linkText = "HPDG IS NEWER: UPDATE REVOLUTION"; break;
        }
        v.source = !generated     ? juce::String("PRESS GENERATE OR TURN A KNOB")
                   : last.linked  ? "FROM HPDG: ITS HITS AND GROOVE"
                   : last.listened ? juce::String("FROM LISTEN: THE INPUT'S HITS")
                                   : juce::String("FROM THE GENRE'S TEMPLATE");
        const auto state = processor.getRevolutionState();
        for (int l = 0; l < revolution::kLanes; ++l)
        {
            v.preview[static_cast<size_t>(l)] = l < revolution::kEngines ? state.pattern.steps[static_cast<size_t>(l)] : state.lanes[static_cast<size_t>(l)].first;
            v.bands[static_cast<size_t>(l)] = state.lanes[static_cast<size_t>(l)].band;
        }
        v.step = processor.moduleNode().getRevolutionStep(0);
        v.glitchBlock = processor.parameterValue("glitchEngine") >= 0.5;
        sp->setValues(v);
    };
    addAndMakeVisible(*sp);
}

void ModulePluginEditor::installTransitionPage()
{
    using Page = ui::HamloRevolutionTransitionPage;
    namespace tr = dsp::transition;
    transitionPage = std::make_unique<Page>();
    auto* const tp = transitionPage.get();
    const auto render = [this, tp](bool capture, bool seed)
    {
        const auto& v = tp->getValues();
        processor.makeTransition(v.kind, v.bars, v.knobs, capture, seed);
        tp->setClip(processor.transitionClip());
        if (refreshTransition)
            refreshTransition();
    };
    tp->onKind = [this, tp, render](int kind)
    {
        auto v = tp->getValues();
        v.kind = kind;
        v.knobs = tr::defaultKnobs(kind);
        tp->setValues(v);
        render(false, false); // the same capture and seed, the other recipe
    };
    tp->onBars = [render](int) { render(true, false); };
    tp->onKnobsDone = [render] { render(false, false); };
    tp->onGenerate = [render] { render(true, true); };
    tp->onPlay = [this] { processor.setPreview(!processor.isPreviewing()); };
    tp->onShowFile = [this]
    {
        const auto t = processor.lastTransition();
        if (t.valid && t.file.existsAsFile())
            t.file.revealToUser();
    };
    tp->onBack = [this]
    {
        processor.setPreview(false);
        processor.setPage(ModulePluginProcessor::Page::Simple);
        applyPage();
    };
    tp->fileToDrag = [this] { return processor.lastTransition().file; };
    refreshTransition = [this, tp]
    {
        auto v = tp->getValues();
        const auto t = processor.lastTransition();
        v.valid = t.valid;
        v.aligned = t.aligned;
        v.complete = t.complete;
        v.bpm = t.valid ? t.settings.bpm : processor.getHostBpm();
        v.fileName = t.valid ? t.file.getFileName() : juce::String();
        v.previewing = processor.isPreviewing();
        const int length = t.valid ? tr::lengthSamples(t.settings) : 0;
        v.playhead = v.previewing && length > 0 ? static_cast<double>(processor.previewPosition()) / length : -1.0;
        tp->setValues(v);
    };
    addChildComponent(*tp);
    if (processor.lastTransition().valid) // a clip made before the editor opened
        tp->setClip(processor.transitionClip());
}

void ModulePluginEditor::openTransition(int kind)
{
    if (transitionPage == nullptr)
        return;
    auto v = transitionPage->getValues();
    v.kind = kind;
    v.knobs = dsp::transition::defaultKnobs(kind);
    transitionPage->setValues(v);
    processor.setPage(ModulePluginProcessor::Page::Transition);
    applyPage();
    transitionPage->pressGenerate(); // capture the last bars now, a new seed
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
    if (simplePage != nullptr)
        simplePage->setBounds(getLocalBounds());
    if (transitionPage != nullptr)
        transitionPage->setBounds(getLocalBounds());
    if (presets != nullptr && panel != nullptr)
        if (auto* target = dynamic_cast<ui::HelpTarget*>(panel.get()))
            presets->placeOnStage(target->contentBounds());
}

} // namespace hpeq::standalone
