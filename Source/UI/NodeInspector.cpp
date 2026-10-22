#include "UI/NodeInspector.h"

namespace hpeq::ui
{
namespace
{
constexpr int kKnobWidth = 78, kKnobHeight = 82, kRowHeight = 26;

const std::array<EqShape, 9> kShapes { EqShape::Bell, EqShape::LowShelf, EqShape::HighShelf, EqShape::LowCut, EqShape::HighCut,
                                       EqShape::Notch, EqShape::BandPass, EqShape::Tilt, EqShape::AllPass };
const std::array<FxType, 10> kTypes { FxType::Compressor, FxType::Reverb, FxType::Gain, FxType::Limiter, FxType::Delay,
                                      FxType::TransientShaper, FxType::Saturator, FxType::MonstaFx, FxType::StereoProcessor,
                                      FxType::GateExpander };
const std::array<ChannelMode, 7> kModes { ChannelMode::Stereo, ChannelMode::Left, ChannelMode::Right, ChannelMode::Mid,
                                          ChannelMode::Side, ChannelMode::LinkedStereo, ChannelMode::DualMono };
const int kSlopes[] { 6, 12, 18, 24, 36, 48, 72, 96 };

template <typename T, size_t N>
int indexOf(const std::array<T, N>& a, T v)
{
    for (size_t i = 0; i < N; ++i)
        if (a[i] == v)
            return static_cast<int>(i);
    return 0;
}

double paramValue(const FxSlotState& s, const char* key, double fallback)
{
    const auto* v = s.parameters.getVarPointer(juce::Identifier(key));
    return v != nullptr ? static_cast<double>(*v) : fallback;
}
} // namespace

// Layout is stored on each control (properties): section 0 = node controls (top row),
// 1 = chain (left column), 2 = slot parameters.
NodeInspector::NodeInspector(FxPluginProcessor& p, Selection& s) : processor(p), selection(s)
{
    startTimerHz(15);
}

NodeInspector::~NodeInspector() = default;

void NodeInspector::rebuildAsync()
{
    // Never rebuild from inside the callback of a control that the rebuild deletes.
    juce::Component::SafePointer<NodeInspector> safe(this);
    juce::MessageManager::callAsync([safe] { if (safe != nullptr) safe->rebuild(); });
}

const std::vector<NodeInspector::ParamDef>& NodeInspector::paramsFor(FxType type)
{
    static const std::vector<ParamDef> compressor {
        { "thresholdDb", "Threshold", -60, 0, -18, -20, " dB", 1 }, { "ratio", "Ratio", 1, 20, 4, 4, ":1", 1 },
        { "kneeDb", "Knee", 0, 24, 6, 6, " dB", 1 }, { "attackMs", "Attack", 0.05, 500, 10, 10, " ms", 2 },
        { "releaseMs", "Release", 1, 5000, 120, 150, " ms", 0 }, { "makeupDb", "Makeup", -24, 24, 0, 0, " dB", 1 },
        { "detector", "Detector", 0, 1, 0, 0, "", 0, "Peak|RMS" }, { "character", "Character", 0, 2, 0, 0, "", 0, "Clean|Punch|Glue" },
        { "autoMakeup", "Auto makeup", 0, 1, 0, 0, "", 0, "Off|On" } };
    static const std::vector<ParamDef> reverb {
        { "predelayMs", "Pre-delay", 0, 250, 10, 30, " ms", 0 }, { "decaySeconds", "Decay", 0.1, 20, 1.8, 2, " s", 2 },
        { "hfDecayRatio", "HF decay", 0.05, 1, 0.5, 0.4, "", 2 }, { "size", "Size", 0.3, 1.5, 1, 0.9, "", 2 },
        { "width", "Width", 0, 1.5, 1, 0.75, "", 2 }, { "lowCutHz", "Wet low cut", 10, 1000, 80, 100, " Hz", 0 } };
    static const std::vector<ParamDef> gain { { "gainDb", "Gain", -60, 24, 0, 0, " dB", 1 } };
    static const std::vector<ParamDef> saturator {
        { "driveDb", "Drive", 0, 48, 12, 12, " dB", 1 }, { "curve", "Curve", 0, 2, 0, 0, "", 0, "Tanh|Tube|Hard" },
        { "bias", "Bias", -0.9, 0.9, 0, 0, "", 2 }, { "trimDb", "Trim", -24, 24, 0, 0, " dB", 1 },
        { "oversampling", "Oversampling", 0, 2, 1, 1, "", 0, "1x|2x|4x" }, { "autoGain", "Auto gain", 0, 1, 1, 1, "", 0, "Off|On" } };
    static const std::vector<ParamDef> transient {
        { "attackDb", "Attack", -24, 24, 0, 0, " dB", 1 }, { "sustainDb", "Sustain", -24, 24, 0, 0, " dB", 1 },
        { "speed", "Speed", 0, 1, 0.5, 0.5, "", 2 } };
    static const std::vector<ParamDef> delay {
        { "sync", "Sync", 0, 11, 8, 8, "", 0, "Free (ms)|1/32|1/16|1/16 D|1/8 T|1/8|1/8 D|1/4 T|1/4|1/4 D|1/2|1/1" },
        { "timeMs", "Time", 1, 2000, 250, 250, " ms", 0 }, { "feedback", "Feedback", 0, 0.95, 0.35, 0.35, "", 2 },
        { "pingPong", "Ping-pong", 0, 1, 0, 0, "", 0, "Off|On" }, { "dampingHz", "Damping", 500, 20000, 8000, 4000, " Hz", 0 },
        { "lowCutHz", "Low cut", 10, 2000, 120, 150, " Hz", 0 }, { "width", "Width", 0, 1.5, 1, 0.75, "", 2 } };
    static const std::vector<ParamDef> monsta {
        { "division", "Slice", 0, 3, 2, 2, "", 0, "1/4|1/8|1/16|1/32" }, { "chance", "Chance", 0, 1, 0.35, 0.35, "", 2 },
        { "seed", "Seed", 0, 9999, 1, 100, "", 0 }, { "crushBits", "Crush bits", 0, 16, 0, 8, "", 0 },
        { "gateDepth", "Gate depth", 0, 1, 0, 0.5, "", 2 }, { "reverseShare", "Reverse share", 0, 1, 0.3, 0.3, "", 2 } };
    static const std::vector<ParamDef> stereo {
        { "width", "Width", 0, 2, 1, 1, "", 2 }, { "balance", "Balance", -1, 1, 0, 0, "", 2 } };
    static const std::vector<ParamDef> gate {
        { "thresholdDb", "Threshold", -90, 0, -40, -40, " dB", 1 }, { "ratio", "Ratio", 1, 100, 4, 4, ":1", 1 },
        { "rangeDb", "Range", 0, 90, 40, 30, " dB", 1 }, { "attackMs", "Attack", 0.05, 100, 0.5, 2, " ms", 2 },
        { "holdMs", "Hold", 0, 500, 20, 30, " ms", 0 }, { "releaseMs", "Release", 1, 2000, 100, 150, " ms", 0 },
        { "hysteresisDb", "Hysteresis", 0, 24, 4, 4, " dB", 1 } };
    static const std::vector<ParamDef> limiter {
        { "ceilingDb", "Ceiling", -60, 0, -1, -6, " dB", 1 }, { "releaseMs", "Release", 1, 2000, 80, 100, " ms", 0 },
        { "lookaheadMs", "Look-ahead", 0.1, 10, 2, 2, " ms", 2 } };
    switch (type)
    {
        case FxType::Compressor: return compressor;
        case FxType::Reverb: return reverb;
        case FxType::Gain: return gain;
        case FxType::Saturator: return saturator;
        case FxType::TransientShaper: return transient;
        case FxType::Delay: return delay;
        case FxType::MonstaFx: return monsta;
        case FxType::StereoProcessor: return stereo;
        case FxType::GateExpander: return gate;
        case FxType::Limiter: return limiter;
    }
    return gain;
}

void NodeInspector::beginEdit(const juce::String& name)
{
    if (!dragging)
        processor.getUndoManager().beginNewTransaction(name);
}

void NodeInspector::editEq(const juce::String& name, const std::function<void(EqNodeState&)>& change)
{
    const auto id = selection.id;
    processor.editProject(name, [&](FxProjectState& p)
    {
        for (auto& n : selectedLayer(p).eqNodes)
            if (n.id == id)
                change(n);
    });
}

void NodeInspector::editFx(const juce::String& name, const std::function<void(FxNodeState&)>& change)
{
    const auto id = selection.id;
    processor.editProject(name, [&](FxProjectState& p)
    {
        for (auto& n : selectedLayer(p).fxNodes)
            if (n.id == id)
                change(n);
    });
}

juce::Slider& NodeInspector::addKnob(const juce::String& label, double min, double max, double mid, const juce::String& suffix, int decimals,
                                     std::function<void(double)> apply)
{
    auto* s = new juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    s->setRange(min, max, 0.0);
    if (mid > min && mid < max)
        s->setSkewFactorFromMidPoint(mid);
    s->setNumDecimalPlacesToDisplay(decimals);
    s->setTextValueSuffix(suffix);
    s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, kKnobWidth - 4, 18);
    s->setColour(juce::Slider::rotarySliderFillColourId, colours::accent);
    s->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s->onDragStart = [this, label] { processor.getUndoManager().beginNewTransaction(label); dragging = true; };
    s->onDragEnd = [this] { dragging = false; };
    s->onValueChange = [this, s, label, apply] { beginEdit(label); apply(s->getValue()); };
    controls.add(s);
    auto* l = labels.add(new juce::Label({}, label));
    l->setJustificationType(juce::Justification::centred);
    l->setColour(juce::Label::textColourId, colours::dim);
    l->setFont(juce::FontOptions(11.0f));
    l->attachToComponent(s, false);
    addAndMakeVisible(s);
    return *s;
}

juce::ComboBox& NodeInspector::addCombo(const juce::String& label, const juce::StringArray& items, std::function<void(int)> apply)
{
    auto* c = new juce::ComboBox(label);
    c->addItemList(items, 1);
    c->onChange = [this, c, label, apply] { processor.getUndoManager().beginNewTransaction(label); apply(c->getSelectedItemIndex()); };
    controls.add(c);
    auto* l = labels.add(new juce::Label({}, label));
    l->setColour(juce::Label::textColourId, colours::dim);
    l->setFont(juce::FontOptions(11.0f));
    l->attachToComponent(c, false);
    addAndMakeVisible(c);
    return *c;
}

juce::Button& NodeInspector::addButton(const juce::String& text, std::function<void()> onClick, bool toggle)
{
    juce::Button* b = toggle ? static_cast<juce::Button*>(new juce::ToggleButton(text)) : new juce::TextButton(text);
    b->onClick = std::move(onClick);
    controls.add(b);
    addAndMakeVisible(b);
    return *b;
}

const std::vector<FxSlotState>* NodeInspector::selectedChain(const FxProjectState& p) const
{
    const auto& layer = selectedLayer(p);
    if (selection.kind == Selection::Kind::Fx)
        for (const auto& n : layer.fxNodes)
            if (n.id == selection.id)
                return &n.chain;
    if (selection.kind == Selection::Kind::Eq)
        for (const auto& n : layer.eqNodes)
            if (n.id == selection.id)
                return &n.chain;
    return nullptr;
}

int NodeInspector::selectedEngineSlot(const FxProjectState& p) const
{
    const auto& layer = selectedLayer(p);
    if (selection.kind == Selection::Kind::Fx)
        for (const auto& n : layer.fxNodes)
            if (n.id == selection.id)
                return n.engineSlot;
    if (selection.kind == Selection::Kind::Eq)
        for (const auto& n : layer.eqNodes)
            if (n.id == selection.id)
                return n.engineSlot;
    return -1;
}

void NodeInspector::editChain(const juce::String& name, const std::function<void(std::vector<FxSlotState>&)>& change)
{
    const auto id = selection.id;
    const bool fx = selection.kind == Selection::Kind::Fx;
    processor.editProject(name, [&](FxProjectState& p)
    {
        auto& layer = selectedLayer(p);
        if (fx)
        {
            for (auto& n : layer.fxNodes)
                if (n.id == id)
                    change(n.chain);
        }
        else
            for (auto& n : layer.eqNodes)
                if (n.id == id)
                    change(n.chain);
    });
}

void NodeInspector::buildModules(const std::vector<FxSlotState>& chain, bool region)
{
    const auto setInfo = [](juce::Component& c, int section, int width, int height, bool newRow = false, bool tile = false)
    {
        auto& props = c.getProperties();
        props.set("section", section);
        props.set("width", width);
        props.set("height", height);
        props.set("newRow", newRow);
        props.set("tile", tile);
    };
    const int count = static_cast<int>(chain.size());
    if (selection.slot >= count)
        selection.slot = -1;

    if (selection.slot < 0)
    {
        // The grid: modules in processing order, then "+" tiles (D6).
        showingGrid = true;
        for (int i = 0; i < kMaxFxPerNode; ++i)
        {
            if (i < count)
            {
                const auto& m = chain[static_cast<size_t>(i)];
                auto& tile = addButton(juce::String(i + 1) + "   " + fxName(m.type) + (m.enabled ? "" : "   (off)"), [this, i]
                {
                    selection.slot = i;
                    rebuildAsync();
                });
                tile.setColour(juce::TextButton::buttonColourId, m.enabled ? colours::accent.withAlpha(0.28f) : colours::panelRaised);
                tile.setColour(juce::TextButton::textColourOffId, m.enabled ? colours::text : colours::dim);
                tile.setTooltip("Open " + fxName(m.type) + " (module " + juce::String(i + 1) + ")");
                setInfo(tile, 1, 0, 0, false, true);
            }
            else
            {
                auto& tile = addButton("+", [this, region]
                {
                    const auto project = processor.getProject();
                    const auto* current = selectedChain(project);
                    const bool needsBand = !region && current != nullptr && current->empty();
                    juce::PopupMenu menu;
                    if (needsBand && freeEngineSlots(selectedLayer(project)) <= 0)
                        menu.addItem(1, "All 8 module bands of this layer are in use", false);
                    else
                        for (size_t t = 0; t < kTypes.size(); ++t)
                            menu.addItem(static_cast<int>(t) + 10, fxName(kTypes[t]));
                    juce::Component::SafePointer<NodeInspector> safe(this);
                    menu.showMenuAsync(juce::PopupMenu::Options(), [safe](int result)
                    {
                        if (safe == nullptr || result < 10)
                            return;
                        safe->addModule(kTypes[static_cast<size_t>(result - 10)]);
                    });
                });
                tile.setTooltip("Add a module (it runs after the ones before it)");
                setInfo(tile, 1, 0, 0, false, true);
            }
        }
        return;
    }

    // One module's page, with a way back to the grid.
    showingGrid = false;
    const int k = selection.slot;
    const auto& module = chain[static_cast<size_t>(k)];
    auto& back = addButton(juce::String::fromUTF8("\xe2\x86\x90  Modules"), [this] { selection.slot = -1; rebuildAsync(); });
    back.setTooltip("Back to all modules");
    setInfo(back, 1, 110, kRowHeight - 2, true);
    juce::StringArray typeNames;
    for (auto t : kTypes)
        typeNames.add(fxName(t));
    auto& type = addCombo({}, typeNames, [this, k](int i)
    {
        editChain("Module type", [k, i](std::vector<FxSlotState>& c)
        {
            const auto keepOn = c[static_cast<size_t>(k)].enabled;
            const auto keepMix = c[static_cast<size_t>(k)].mix;
            c[static_cast<size_t>(k)] = defaultModule(kTypes[static_cast<size_t>(i)]);
            c[static_cast<size_t>(k)].enabled = keepOn;
            c[static_cast<size_t>(k)].mix = keepMix;
        });
        rebuildAsync();
    });
    setInfo(type, 1, 140, kRowHeight - 2);
    auto& moduleOn = addButton("On", [this, k]
    {
        beginEdit("Module on");
        editChain("Module on", [k](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = !c[static_cast<size_t>(k)].enabled; });
    }, true);
    setInfo(moduleOn, 1, 50, kRowHeight - 2, true);
    auto* moduleMix = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    moduleMix->setRange(0, 1, 0.01);
    moduleMix->setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 18);
    moduleMix->setTooltip("Module mix");
    moduleMix->onDragStart = [this] { processor.getUndoManager().beginNewTransaction("Module mix"); dragging = true; };
    moduleMix->onDragEnd = [this] { dragging = false; };
    moduleMix->onValueChange = [this, moduleMix, k]
    {
        beginEdit("Module mix");
        const auto v = static_cast<float>(moduleMix->getValue());
        editChain("Module mix", [k, v](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = v; });
    };
    controls.add(moduleMix);
    addAndMakeVisible(moduleMix);
    setInfo(*moduleMix, 1, 200, kRowHeight - 2);
    auto& earlier = addButton("Earlier", [this, k] { moveModule(k, -1); });
    earlier.setEnabled(k > 0);
    earlier.setTooltip("Process this module before the previous one");
    setInfo(earlier, 1, 70, kRowHeight - 2, true);
    auto& later = addButton("Later", [this, k] { moveModule(k, 1); });
    later.setEnabled(k + 1 < count);
    later.setTooltip("Process this module after the next one");
    setInfo(later, 1, 70, kRowHeight - 2);
    auto& remove = addButton("Remove", [this, k]
    {
        processor.getUndoManager().beginNewTransaction("Remove module");
        editChain("Remove module", [k](std::vector<FxSlotState>& c) { if (k < static_cast<int>(c.size())) c.erase(c.begin() + k); });
        selection.slot = -1;
        rebuildAsync();
    });
    remove.setEnabled(!region || count > 1);
    if (region && count <= 1)
        remove.setTooltip("An FX region keeps at least one module; delete the region instead");
    setInfo(remove, 1, 80, kRowHeight - 2);
    refreshers.push_back([this, &type, &moduleOn, moduleMix, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        if (const auto* c = selectedChain(project); c != nullptr && k < static_cast<int>(c->size()))
        {
            const auto& m = (*c)[static_cast<size_t>(k)];
            type.setSelectedItemIndex(indexOf(kTypes, m.type), juce::dontSendNotification);
            moduleOn.setToggleState(m.enabled, juce::dontSendNotification);
            moduleMix->setValue(m.mix, juce::dontSendNotification);
        }
    });

    if (module.type == FxType::Compressor)
    {
        // The owner's HamloComp design: one drawn panel instead of generic knobs (it fills the editor body).
        largePage = true;
        auto* panel = new HamloCompPanel();
        controls.add(panel);
        addAndMakeVisible(panel);
        panel->getProperties().set("section", 4);
        hamloPanel = panel;
        panel->onKnobGestureStart = [this](HamloCompPanel::Param p)
        {
            processor.getUndoManager().beginNewTransaction("Compressor " + juce::String(HamloCompPanel::keyFor(p)));
            dragging = true;
        };
        panel->onKnobGestureEnd = [this] { dragging = false; };
        panel->onKnobChange = [this, k](HamloCompPanel::Param p, double v)
        {
            const juce::Identifier key(HamloCompPanel::keyFor(p));
            editChain("Compressor", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
        };
        panel->onSwitchChange = [this, k](HamloCompPanel::Switch sw, int option)
        {
            processor.getUndoManager().beginNewTransaction("Compressor switch");
            const juce::Identifier key(HamloCompPanel::keyFor(sw));
            editChain("Compressor switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
        };
        panel->onPowerChange = [this, k](bool on)
        {
            processor.getUndoManager().beginNewTransaction("Module on");
            editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
        };
        panel->readMeters = [this, k]() -> std::pair<float, float>
        {
            const auto project = processor.getProject();
            const int engineSlot = selectedEngineSlot(project);
            if (engineSlot < 0)
                return { -60.0f, 0.0f };
            const auto& node = processor.getEngine().getLayer(project.selectedLayer).getNode(engineSlot);
            return { node.getCompressorInputDb(k), node.getGainReductionDb(k) };
        };
        refreshers.push_back([this, panel, k]
        {
            const auto project = processor.getProject(); // keep alive: selectedChain points into it
            const auto* c = selectedChain(project);
            if (c == nullptr || k >= static_cast<int>(c->size()))
                return;
            const auto& m = (*c)[static_cast<size_t>(k)];
            HamloCompPanel::Values v;
            const auto& defs = paramsFor(FxType::Compressor); // the DSP defaults when a key is absent
            const auto fallback = [&defs](const char* key)
            {
                for (const auto& d : defs)
                    if (juce::String(d.key) == key)
                        return d.defaultValue;
                return 0.0;
            };
            for (int p = 0; p < HamloCompPanel::NumKnobs; ++p)
            {
                const auto* key = HamloCompPanel::keyFor(static_cast<HamloCompPanel::Param>(p));
                v.knobs[static_cast<size_t>(p)] = paramValue(m, key, fallback(key));
            }
            for (int sw = 0; sw < HamloCompPanel::NumSwitches; ++sw)
            {
                const auto* key = HamloCompPanel::keyFor(static_cast<HamloCompPanel::Switch>(sw));
                v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, key, fallback(key)));
            }
            v.on = m.enabled;
            panel->setValues(v);
        });
        return;
    }

    for (const auto& def : paramsFor(module.type))
    {
        const auto key = juce::String(def.key);
        const double fallback = def.defaultValue;
        if (def.choices != nullptr)
        {
            auto& combo = addCombo(def.label, juce::StringArray::fromTokens(def.choices, "|", ""), [this, key, k](int index)
            {
                editChain(key, [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(juce::Identifier(key), index); });
            });
            setInfo(combo, 2, kKnobWidth, kRowHeight - 2);
            refreshers.push_back([this, &combo, key, k, fallback]
            {
                const auto project = processor.getProject(); // keep alive: selectedChain points into it
                if (const auto* c = selectedChain(project); c != nullptr && k < static_cast<int>(c->size()))
                    combo.setSelectedItemIndex(juce::roundToInt(paramValue((*c)[static_cast<size_t>(k)], key.toRawUTF8(), fallback)), juce::dontSendNotification);
            });
            continue;
        }
        auto& knob = addKnob(def.label, def.min, def.max, def.midPoint, def.suffix, def.decimals, [this, key, k](double v)
        {
            editChain(key, [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(juce::Identifier(key), v); });
        });
        setInfo(knob, 2, kKnobWidth, kKnobHeight);
        refreshers.push_back([this, &knob, key, k, fallback]
        {
            const auto project = processor.getProject(); // keep alive: selectedChain points into it
            if (const auto* c = selectedChain(project); c != nullptr && k < static_cast<int>(c->size()))
                knob.setValue(paramValue((*c)[static_cast<size_t>(k)], key.toRawUTF8(), fallback), juce::dontSendNotification);
        });
    }
}

void NodeInspector::addModule(FxType type)
{
    processor.getUndoManager().beginNewTransaction("Add module");
    editChain("Add module", [type](std::vector<FxSlotState>& c)
    {
        if (c.size() < static_cast<size_t>(kMaxFxPerNode))
            c.push_back(defaultModule(type));
    });
    const auto project = processor.getProject(); // keep it alive: selectedChain points into it
    const auto* after = selectedChain(project);
    selection.slot = after != nullptr && !after->empty() ? static_cast<int>(after->size()) - 1 : -1;
    rebuildAsync();
}

void NodeInspector::moveModule(int index, int delta)
{
    processor.getUndoManager().beginNewTransaction("Move module");
    editChain("Move module", [index, delta](std::vector<FxSlotState>& c)
    {
        const int to = index + delta;
        if (to >= 0 && to < static_cast<int>(c.size()))
            std::swap(c[static_cast<size_t>(index)], c[static_cast<size_t>(to)]);
    });
    selection.slot = juce::jlimit(0, kMaxFxPerNode - 1, index + delta);
    rebuildAsync();
}

void NodeInspector::rebuild()
{
    labels.clear();
    controls.clear();
    refreshers.clear();
    nodeIndex = -1;
    showingGrid = true;
    largePage = false;
    hamloPanel = nullptr;
    title = "Nothing selected";
    status = "Select an EQ point or FX region in the workspace or the node list.";

    const auto project = processor.getProject();
    const auto& layer = selectedLayer(project);
    const auto setInfo = [](juce::Component& c, int section, int width, int height, bool newRow = false)
    {
        auto& props = c.getProperties();
        props.set("section", section);
        props.set("width", width);
        props.set("height", height);
        props.set("newRow", newRow);
    };

    if (selection.kind == Selection::Kind::Eq)
    {
        for (int i = 0; i < static_cast<int>(layer.eqNodes.size()); ++i)
            if (layer.eqNodes[static_cast<size_t>(i)].id == selection.id)
                nodeIndex = i;
        if (nodeIndex < 0)
        {
            selection = {};
            repaint();
            return;
        }
        const auto band = pointModuleBandHz(layer.eqNodes[static_cast<size_t>(nodeIndex)]);
        title = "EQ " + juce::String(nodeIndex + 1) + "  /  PRE-EQ; modules act on " + juce::String(band.first, 0) + "-"
              + juce::String(band.second, 0) + " Hz (from frequency and Q)";
        status = {};

        auto& on = addButton("On", [this] { beginEdit("Enable"); editEq("Enable", [](EqNodeState& n) { n.enabled = !n.enabled; }); }, true);
        setInfo(on, 0, 50, kRowHeight);
        juce::StringArray shapeNames;
        for (auto s : kShapes)
            shapeNames.add(shapeName(s));
        auto& shape = addCombo("Shape", shapeNames, [this](int i) { editEq("Shape", [i](EqNodeState& n)
        {
            n.shape = kShapes[static_cast<size_t>(i)];
            if (!isGainShapeUi(n.shape)) n.gainDb = 0.0;
        }); });
        setInfo(shape, 0, 120, kRowHeight);
        auto& freq = addKnob("Frequency", 10, 22000, 1000, " Hz", 1, [this](double v) { editEq("Frequency", [v](EqNodeState& n) { n.frequencyHz = v; }); });
        setInfo(freq, 0, kKnobWidth, kKnobHeight);
        auto& gainKnob = addKnob("Gain", -24, 24, 0, " dB", 1, [this](double v) { editEq("Gain", [v](EqNodeState& n) { n.gainDb = v; }); });
        setInfo(gainKnob, 0, kKnobWidth, kKnobHeight);
        auto& q = addKnob("Q", 0.1, 18, 1, "", 2, [this](double v) { editEq("Q", [v](EqNodeState& n) { n.q = v; }); });
        setInfo(q, 0, kKnobWidth, kKnobHeight);
        juce::StringArray slopeNames;
        for (int s : kSlopes)
            slopeNames.add(juce::String(s) + " dB/oct");
        auto& slope = addCombo("Slope", slopeNames, [this](int i) { editEq("Slope", [i](EqNodeState& n) { n.slopeDbPerOct = kSlopes[i]; }); });
        setInfo(slope, 0, 110, kRowHeight);
        juce::StringArray modeNames { "Stereo", "Left", "Right", "Mid", "Side" };
        auto& mode = addCombo("Channel", modeNames, [this](int i) { editEq("Channel", [i](EqNodeState& n) { n.channel = kModes[static_cast<size_t>(i)]; }); });
        setInfo(mode, 0, 100, kRowHeight);
        auto& del = addButton("Delete", [this]
        {
            processor.getUndoManager().beginNewTransaction("Delete EQ");
            const auto id = selection.id;
            processor.editProject("Delete EQ", [id](FxProjectState& p)
            {
                auto& v = selectedLayer(p).eqNodes;
                v.erase(std::remove_if(v.begin(), v.end(), [id](const EqNodeState& n) { return n.id == id; }), v.end());
            });
            selection = {};
            rebuildAsync();
        });
        setInfo(del, 0, 70, kRowHeight);

        refreshers.push_back([this, &on, &shape, &freq, &gainKnob, &q, &slope, &mode]
        {
            const auto p = processor.getProject();
            for (const auto& n : selectedLayer(p).eqNodes)
                if (n.id == selection.id)
                {
                    on.setToggleState(n.enabled, juce::dontSendNotification);
                    shape.setSelectedItemIndex(indexOf(kShapes, n.shape), juce::dontSendNotification);
                    freq.setValue(n.frequencyHz, juce::dontSendNotification);
                    gainKnob.setValue(n.gainDb, juce::dontSendNotification);
                    gainKnob.setEnabled(isGainShapeUi(n.shape));
                    q.setValue(n.q, juce::dontSendNotification);
                    q.setEnabled(n.shape != EqShape::LowCut && n.shape != EqShape::HighCut && n.shape != EqShape::Tilt);
                    const auto slopeIndex = std::find(std::begin(kSlopes), std::end(kSlopes), n.slopeDbPerOct) - std::begin(kSlopes);
                    slope.setSelectedItemIndex(static_cast<int>(slopeIndex), juce::dontSendNotification);
                    slope.setEnabled(n.shape == EqShape::LowCut || n.shape == EqShape::HighCut);
                    mode.setSelectedItemIndex(juce::jmin(4, indexOf(kModes, n.channel)), juce::dontSendNotification);
                }
        });
        buildModules(layer.eqNodes[static_cast<size_t>(nodeIndex)].chain, false);
    }
    else if (selection.kind == Selection::Kind::Fx)
    {
        for (int i = 0; i < static_cast<int>(layer.fxNodes.size()); ++i)
            if (layer.fxNodes[static_cast<size_t>(i)].id == selection.id)
                nodeIndex = i;
        if (nodeIndex < 0)
        {
            selection = {};
            repaint();
            return;
        }
        const auto node = layer.fxNodes[static_cast<size_t>(nodeIndex)];
        title = "FX " + juce::String(nodeIndex + 1) + "  /  region after PRE-EQ, " + juce::String(node.chain.size()) + " of 6 modules";
        status = {};

        auto& on = addButton("On", [this] { beginEdit("Enable"); editFx("Enable", [](FxNodeState& n) { n.enabled = !n.enabled; }); }, true);
        setInfo(on, 0, 50, kRowHeight);
        auto& low = addKnob("Low edge", 10, 22000, 500, " Hz", 1, [this](double v) { editFx("Low edge", [v](FxNodeState& n) { n.lowHz = juce::jmin(v, n.highHz / 1.05); }); });
        setInfo(low, 0, kKnobWidth, kKnobHeight);
        auto& high = addKnob("High edge", 10, 22000, 2000, " Hz", 1, [this](double v) { editFx("High edge", [v](FxNodeState& n) { n.highHz = juce::jmax(v, n.lowHz * 1.05); }); });
        setInfo(high, 0, kKnobWidth, kKnobHeight);
        auto& mix = addKnob("Node mix", 0, 1, 0.5, "", 2, [this](double v) { editFx("Node mix", [v](FxNodeState& n) { n.mix = static_cast<float>(v); }); });
        setInfo(mix, 0, kKnobWidth, kKnobHeight);
        juce::StringArray modeNames;
        for (auto m : kModes)
            modeNames.add(channelName(m));
        auto& mode = addCombo("Channel", modeNames, [this](int i) { editFx("Channel", [i](FxNodeState& n) { n.channel = kModes[static_cast<size_t>(i)]; }); });
        setInfo(mode, 0, 110, kRowHeight);
        auto& del = addButton("Delete", [this]
        {
            processor.getUndoManager().beginNewTransaction("Delete FX");
            const auto id = selection.id;
            processor.editProject("Delete FX", [id](FxProjectState& p)
            {
                auto& v = selectedLayer(p).fxNodes;
                v.erase(std::remove_if(v.begin(), v.end(), [id](const FxNodeState& n) { return n.id == id; }), v.end());
            });
            selection = {};
            rebuildAsync();
        });
        setInfo(del, 0, 70, kRowHeight);

        buildModules(node.chain, true);

        refreshers.push_back([this, &on, &low, &high, &mix, &mode]
        {
            const auto p = processor.getProject();
            for (const auto& n : selectedLayer(p).fxNodes)
                if (n.id == selection.id)
                {
                    on.setToggleState(n.enabled, juce::dontSendNotification);
                    low.setValue(n.lowHz, juce::dontSendNotification);
                    high.setValue(n.highHz, juce::dontSendNotification);
                    mix.setValue(n.mix, juce::dontSendNotification);
                    mode.setSelectedItemIndex(indexOf(kModes, n.channel), juce::dontSendNotification);
                }
        });
    }

    refresh();
    resized();
    repaint();
    if (onLayoutChange)
        onLayoutChange();
}

void NodeInspector::refresh()
{
    for (auto& r : refreshers)
        r();
}

void NodeInspector::timerCallback()
{
    updateLiveReadouts();
}

void NodeInspector::updateLiveReadouts(bool settle)
{
    if (settle && hamloPanel != nullptr && hamloPanel->readMeters)
    {
        const auto [inputDb, reductionDb] = hamloPanel->readMeters();
        hamloPanel->settleMeters(inputDb, reductionDb);
    }
    // Live readouts from the audio engine (atomics, no locks).
    juce::String newStatus;
    if (selection.kind != Selection::Kind::None && selection.slot >= 0)
    {
        const auto project = processor.getProject();
        const int engineSlot = selectedEngineSlot(project);
        const auto* chain = selectedChain(project);
        if (engineSlot >= 0 && chain != nullptr && selection.slot < static_cast<int>(chain->size()))
        {
            const auto& node = processor.getEngine().getLayer(project.selectedLayer).getNode(engineSlot);
            const int k = selection.slot;
            if ((*chain)[static_cast<size_t>(k)].type == FxType::Compressor)
            {
                newStatus = "Gain reduction  " + juce::String(node.getGainReductionDb(k), 1) + " dB";
                const float autoDb = node.getCompressorAutoMakeupDb(k);
                if (autoDb > 0.05f)
                    newStatus << "    auto makeup +" << juce::String(autoDb, 1) << " dB";
            }
            if (node.slotStarved(k))
                newStatus = fxName((*chain)[static_cast<size_t>(k)].type) + " pool full (shared by all layers): this module is silent.";
        }
    }
    if (newStatus != status && selection.kind != Selection::Kind::None)
    {
        status = newStatus;
        repaint();
    }
}

void NodeInspector::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour(colours::panel);
    g.fillRoundedRectangle(area, 6.0f);
    const auto accent = selection.kind == Selection::Kind::Fx ? colours::forNode(nodeIndex) : colours::eqNode;
    if (selection.kind != Selection::Kind::None)
    {
        g.setColour(accent.withAlpha(0.6f));
        g.drawRoundedRectangle(area.reduced(0.5f), 6.0f, 1.0f);
    }
    g.setColour(colours::text);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText(title, area.reduced(12.0f, 6.0f).removeFromTop(18.0f), juce::Justification::centredLeft);
    g.setFont(12.0f);
    g.setColour(status.contains("pool full") ? colours::warning : colours::accent);
    g.drawText(status, area.reduced(12.0f, 6.0f).removeFromTop(18.0f), juce::Justification::centredRight);
    if (!chainArea.isEmpty())
    {
        g.setColour(colours::dim);
        g.setFont(11.0f);
        if (showingGrid)
            g.drawText("MODULES (processed left to right, then the next row)", chainArea.toFloat().removeFromTop(14.0f), juce::Justification::centredLeft);
    }
}

void NodeInspector::resized()
{
    auto area = getLocalBounds().reduced(12, 6);
    area.removeFromTop(22);
    const bool any = selection.kind != Selection::Kind::None;

    if (any && !showingGrid)
    {
        // One module's page fills the panel (the region / point controls come back with the arrow).
        auto header = area.removeFromTop(kRowHeight);
        int x = header.getX();
        area.removeFromTop(8);
        chainArea = header;
        paramArea = area;
        auto inner = area;
        int knobX = inner.getX(), comboX = inner.getX();
        for (auto* c : controls)
        {
            const auto& props = c->getProperties();
            const int section = props["section"], width = props["width"], height = props["height"];
            c->setVisible(section != 0);
            if (section == 1)
            {
                c->setBounds(x, header.getY(), width, height);
                x += width + 6;
            }
            else if (section == 4)
                c->setBounds(area);
            else if (section == 2 && height == kKnobHeight)
            {
                c->setBounds(knobX, inner.getY() + 14, width, height);
                knobX += width + 8;
            }
            else if (section == 2)
            {
                c->setBounds(comboX, inner.getY() + 14 + kKnobHeight + 22, juce::jmax(width, 96), height);
                comboX += juce::jmax(width, 96) + 10;
            }
        }
        return;
    }

    auto top = area.removeFromTop(kKnobHeight + 16);
    chainArea = any ? area : juce::Rectangle<int>();
    paramArea = {};
    int tileIndex = 0;
    const int tileGap = 8, tileWidth = juce::jmin(220, (chainArea.getWidth() - 2 * tileGap) / 3);
    const int tileHeight = juce::jlimit(28, 56, (chainArea.getHeight() - 18 - tileGap) / 2);
    int topX = top.getX();
    for (auto* c : controls)
    {
        const auto& props = c->getProperties();
        c->setVisible(true);
        if (static_cast<bool>(props["tile"]))
        {
            const int col = tileIndex % 3, row = tileIndex / 3;
            c->setBounds(chainArea.getX() + col * (tileWidth + tileGap), chainArea.getY() + 18 + row * (tileHeight + tileGap), tileWidth, tileHeight);
            ++tileIndex;
            continue;
        }
        const int width = props["width"], height = props["height"];
        const int y = height == kKnobHeight ? top.getY() + 16 : top.getY() + 32;
        c->setBounds(topX, y, width, height);
        topX += width + 10;
    }
}
} // namespace hpeq::ui
