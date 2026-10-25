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
        { "predelayMs", "Pre-delay", 0, 500, 10, 30, " ms", 0 }, { "decaySeconds", "Decay", 0.1, 30, 1.8, 2, " s", 2 },
        { "hfDecayRatio", "HF decay", 0.05, 1, 0.5, 0.4, "", 2 }, { "size", "Size", 0.3, 1.5, 1, 0.9, "", 2 },
        { "width", "Width", 0, 1.5, 1, 0.75, "", 2 }, { "lowCutHz", "Wet low cut", 10, 1000, 80, 100, " Hz", 0 },
        // HamloVerb pass (defaults = the DSP's when a key is absent, LayerSnapshot.cpp)
        { "space", "Space", 0, 6, 0, 0, "", 0, "Classic|Room|Chamber|Hall|Plate|Cathedral|Ambience" },
        { "color", "Color", 0, 2, 2, 2, "", 0, "1970|1980|Now" },
        { "predelaySync", "Pre-delay sync", 0, 11, 0, 0, "", 0, "Free|1/64|1/32|1/16T|1/16|1/8T|1/16D|1/8|1/4T|1/8D|1/4|1/2" },
        { "attack", "Attack", 0, 1, 0.2, 0.5, "", 2 }, { "earlyDiffusion", "Early diffusion", 0, 1, 0.7, 0.5, "", 2 },
        { "lateDiffusion", "Late diffusion", 0, 1, 0.7, 0.5, "", 2 }, { "modRate", "Mod rate", 0.01, 10, 0.6, 1, " Hz", 2 },
        { "modDepth", "Mod depth", 0, 1, 0.4, 0.5, "", 2 }, { "bassMult", "Bass mult", 0.25, 4, 1.2, 1, "x", 2 },
        { "bassXoverHz", "Bass xover", 50, 2000, 300, 300, " Hz", 0 }, { "highXoverHz", "High xover", 1000, 16000, 4000, 4000, " Hz", 0 },
        { "highCutHz", "High cut", 1000, 20000, 12000, 6000, " Hz", 0 }, { "distance", "Distance", 0, 1, 0.4, 0.5, "", 2 },
        { "duck", "Duck", 0, 1, 0, 0.5, "", 2 }, { "freeze", "Freeze", 0, 1, 0, 0, "", 0, "Off|On" } };
    static const std::vector<ParamDef> gain { { "gainDb", "Gain", -60, 24, 0, 0, " dB", 1 } };
    static const std::vector<ParamDef> saturator {
        { "driveDb", "Drive", 0, 48, 12, 12, " dB", 1 }, { "curve", "Curve", 0, 2, 0, 0, "", 0, "Tanh|Tube|Hard" },
        { "bias", "Bias", -0.9, 0.9, 0, 0, "", 2 }, { "trimDb", "Trim", -24, 24, 0, 0, " dB", 1 },
        { "oversampling", "Oversampling", 0, 2, 1, 1, "", 0, "1x|2x|4x" }, { "autoGain", "Auto gain", 0, 1, 1, 1, "", 0, "Off|On" },
        // HamloMag pass (defaults = the DSP's when a key is absent, LayerSnapshot.cpp)
        { "model", "Model", 0, 5, 0, 0, "", 0, "Classic|Tape|Tube|Iron|Germanium|Vinyl" },
        { "punish", "Punish", 0, 1, 0, 0, "", 0, "Off|On" }, { "tone", "Tone", -1, 1, 0, 0, "", 2 },
        { "satLowCutHz", "Low cut", 10, 1000, 20, 100, " Hz", 0 }, { "satHighCutHz", "High cut", 1000, 20000, 20000, 6000, " Hz", 0 },
        { "thump", "Thump", 0, 1, 0, 0, "", 0, "Off|On" }, { "steep", "Steep", 0, 1, 0, 0, "", 0, "Off|On" },
        { "harmonics", "Harmonics", 0, 1, 0.5, 0.5, "", 2 }, { "evenOdd", "Even / odd", -1, 1, 0, 0, "", 2 },
        { "tapeBias", "Bias", 0, 1, 0.5, 0.5, "", 2 }, { "speed", "Speed", 0, 2, 0, 0, "", 0, "1|2|3" },
        { "wow", "Wow", 0, 1, 0.15, 0.5, "", 2 }, { "hiss", "Hiss", 0, 1, 0.15, 0.5, "", 2 }, { "hum", "Hum", 0, 1, 0, 0.5, "", 2 },
        { "crackle", "Crackle", 0, 1, 0, 0.5, "", 2 }, { "noiseFollow", "Noise", 0, 1, 1, 1, "", 0, "Always|Follow" } };
    static const std::vector<ParamDef> transient {
        { "attackDb", "Attack", -24, 24, 0, 0, " dB", 1 }, { "sustainDb", "Sustain", -24, 24, 0, 0, " dB", 1 },
        { "speed", "Speed", 0, 1, 0.5, 0.5, "", 2 } };
    static const std::vector<ParamDef> delay {
        { "sync", "Sync", 0, 11, 8, 8, "", 0, "Free (ms)|1/32|1/16|1/16 D|1/8 T|1/8|1/8 D|1/4 T|1/4|1/4 D|1/2|1/1" },
        { "timeMs", "Time", 1, 4000, 250, 250, " ms", 0 }, { "feedback", "Feedback", 0, 0.98, 0.35, 0.35, "", 2 },
        { "pingPong", "Ping-pong", 0, 1, 0, 0, "", 0, "Off|On" }, { "dampingHz", "Damping", 500, 20000, 8000, 4000, " Hz", 0 },
        { "lowCutHz", "Low cut", 10, 2000, 120, 150, " Hz", 0 }, { "width", "Width", 0, 1.5, 1, 0.75, "", 2 },
        // HamloEcho pass (defaults = the DSP's when a key is absent, LayerSnapshot.cpp)
        { "character", "Character", 0, 4, 0, 0, "", 0, "Classic|Clean|Tape|BBD|Lo-Fi" },
        { "timeRMs", "Time R", 1, 4000, 375, 375, " ms", 0 },
        { "syncR", "Sync R", 0, 11, 6, 6, "", 0, "Free (ms)|1/32|1/16|1/16 D|1/8 T|1/8|1/8 D|1/4 T|1/4|1/4 D|1/2|1/1" },
        { "link", "Link", 0, 1, 1, 1, "", 0, "Off|On" }, { "cross", "Cross", 0, 1, 0, 0.5, "", 2 },
        { "driveDb", "Drive", 0, 24, 3, 6, " dB", 1 }, { "modRate", "Mod rate", 0.05, 10, 0.5, 1, " Hz", 2 },
        { "modDepth", "Mod depth", 0, 1, 0.3, 0.5, "", 2 }, { "diffusion", "Diffusion", 0, 1, 0, 0.5, "", 2 },
        { "pitch", "Pitch", -12, 12, 0, 0, " st", 1 }, { "lowRes", "Low res", 0, 1, 0, 0.5, "", 2 },
        { "highRes", "High res", 0, 1, 0, 0.5, "", 2 }, { "bandHz", "Band freq", 100, 10000, 1000, 1000, " Hz", 0 },
        { "bandGainDb", "Band gain", -18, 12, 0, 0, " dB", 1 }, { "bandQ", "Band Q", 0.3, 6, 1, 1, "", 2 },
        { "duck", "Duck", 0, 1, 0, 0.5, "", 2 }, { "freeze", "Freeze", 0, 1, 0, 0, "", 0, "Off|On" },
        { "lofi", "Lo-Fi", 0, 1, 0, 0.5, "", 2 }, { "timeMode", "Time mode", 0, 1, 0, 0, "", 0, "Tape|Fade" } };
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
    if (selection.kind == Selection::Kind::Mix)
        return &layer.mixChain;
    return nullptr;
}

const dsp::FxNodeProcessor* NodeInspector::selectedNodeProcessor(const FxProjectState& p) const
{
    const auto& layerEngine = processor.getEngine().getLayer(p.selectedLayer);
    if (selection.kind == Selection::Kind::Mix)
        return &layerEngine.getMixNode();
    const int engineSlot = selectedEngineSlot(p);
    return engineSlot >= 0 ? &layerEngine.getNode(engineSlot) : nullptr;
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
    const bool mix = selection.kind == Selection::Kind::Mix;
    processor.editProject(name, [&](FxProjectState& p)
    {
        auto& layer = selectedLayer(p);
        if (mix)
            change(layer.mixChain);
        else if (fx)
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

void NodeInspector::buildModules(const std::vector<FxSlotState>& chain, ChainKind kind)
{
    const bool region = kind == ChainKind::Region; // a region keeps at least one module
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
                auto& tile = addButton(juce::String(i + 1) + "   " + fxFullName(m.type) + (m.enabled ? "" : "   (off)"), [this, i]
                {
                    selection.slot = i;
                    rebuildAsync();
                });
                tile.setColour(juce::TextButton::buttonColourId, m.enabled ? colours::accent.withAlpha(0.28f) : colours::panelRaised);
                tile.setColour(juce::TextButton::textColourOffId, m.enabled ? colours::text : colours::dim);
                tile.setTooltip("Open " + fxFullName(m.type) + " (module " + juce::String(i + 1) + ")");
                setInfo(tile, 1, 0, 0, false, true);
            }
            else
            {
                auto& tile = addButton("+", [this, kind]
                {
                    const auto project = processor.getProject();
                    const auto* current = selectedChain(project);
                    const bool needsBand = kind == ChainKind::Point && current != nullptr && current->empty();
                    juce::PopupMenu menu;
                    if (needsBand && freeEngineSlots(selectedLayer(project)) <= 0)
                        menu.addItem(1, "All 8 module bands of this layer are in use", false);
                    else
                        for (size_t t = 0; t < kTypes.size(); ++t)
                            menu.addItem(static_cast<int>(t) + 10, fxFullName(kTypes[t]));
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
        typeNames.add(fxFullName(t));
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
            if (p == HamloCompPanel::Mix) // the page's Mix is the module's mix
            {
                editChain("Module mix", [k, v](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
                return;
            }
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
            const auto* nodePtr = selectedNodeProcessor(project);
            if (nodePtr == nullptr)
                return { -60.0f, 0.0f };
            const auto& node = *nodePtr;
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
                v.knobs[static_cast<size_t>(p)] = p == HamloCompPanel::Mix ? static_cast<double>(m.mix) : paramValue(m, key, fallback(key));
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

    if (module.type == FxType::Reverb)
    {
        buildReverbPage(k);
        return;
    }
    if (module.type == FxType::Delay)
    {
        buildDelayPage(k);
        return;
    }
    if (module.type == FxType::Saturator)
    {
        buildSaturatorPage(k);
        return;
    }
    if (module.type == FxType::Gain)
    {
        buildFaderPage(k);
        return;
    }
    if (module.type == FxType::Limiter)
    {
        buildLimiterPage(k);
        return;
    }
    if (module.type == FxType::TransientShaper)
    {
        buildTransientPage(k);
        return;
    }
    if (module.type == FxType::GateExpander)
    {
        buildGatePage(k);
        return;
    }
    if (module.type == FxType::StereoProcessor)
    {
        buildStereoPage(k);
        return;
    }
    if (module.type == FxType::MonstaFx)
    {
        buildRevolutionPage(k);
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
    verbPanel = nullptr;
    echoPanel = nullptr;
    magPanel = nullptr;
    faderPanel = nullptr;
    wallPanel = nullptr;
    udarnikPanel = nullptr;
    zastavaPanel = nullptr;
    shirotaPanel = nullptr;
    revolutionPanel = nullptr;
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
        buildModules(layer.eqNodes[static_cast<size_t>(nodeIndex)].chain, ChainKind::Point);
    }
    else if (selection.kind == Selection::Kind::Mix)
    {
        title = "MIX  /  whole layer " + juce::String(project.selectedLayer + 1) + " (after the regions and points), "
              + juce::String(static_cast<int>(layer.mixChain.size())) + " of 6 modules";
        status = {};
        nodeIndex = 0;
        buildModules(layer.mixChain, ChainKind::Mix);
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

        buildModules(node.chain, ChainKind::Region);

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

void NodeInspector::buildReverbPage(int k)
{
    // HamloVerb (module pass 2): one drawn panel in the HamloComp style; it fills the editor body.
    largePage = true;
    auto* panel = new HamloVerbPanel();
    controls.add(panel);
    addAndMakeVisible(panel);
    panel->getProperties().set("section", 4);
    verbPanel = panel;
    panel->onKnobGestureStart = [this](HamloVerbPanel::Param p)
    {
        processor.getUndoManager().beginNewTransaction("Reverb " + juce::String(HamloVerbPanel::keyFor(p)));
        dragging = true;
    };
    panel->onKnobGestureEnd = [this] { dragging = false; };
    panel->onKnobChange = [this, k](HamloVerbPanel::Param p, double v)
    {
        if (p == HamloVerbPanel::Mix) // the page's Mix is the module's mix
        {
            editChain("Module mix", [k, v](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
            return;
        }
        const juce::Identifier key(HamloVerbPanel::keyFor(p));
        editChain("Reverb", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
    };
    panel->onSwitchChange = [this, k](HamloVerbPanel::Switch sw, int option)
    {
        processor.getUndoManager().beginNewTransaction("Reverb " + juce::String(HamloVerbPanel::keyFor(sw)));
        const juce::Identifier key(HamloVerbPanel::keyFor(sw));
        editChain("Reverb switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
    };
    panel->onPowerChange = [this, k](bool on)
    {
        processor.getUndoManager().beginNewTransaction("Module on");
        editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
    };
    panel->readMeters = [this, k]() -> std::pair<float, float>
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        if (nodePtr == nullptr)
            return { -120.0f, 0.0f };
        const auto& node = *nodePtr;
        return { node.getReverbOutputDb(k), node.getReverbDuckDb(k) };
    };
    refreshers.push_back([this, panel, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        const auto* c = selectedChain(project);
        if (c == nullptr || k >= static_cast<int>(c->size()))
            return;
        const auto& m = (*c)[static_cast<size_t>(k)];
        HamloVerbPanel::Values v;
        const auto& defs = paramsFor(FxType::Reverb); // the DSP defaults when a key is absent
        const auto fallback = [&defs](const char* key)
        {
            for (const auto& d : defs)
                if (juce::String(d.key) == key)
                    return d.defaultValue;
            return 0.0;
        };
        for (int p = 0; p < HamloVerbPanel::NumKnobs; ++p)
        {
            const auto* key = HamloVerbPanel::keyFor(static_cast<HamloVerbPanel::Param>(p));
            v.knobs[static_cast<size_t>(p)] = p == HamloVerbPanel::Mix ? static_cast<double>(m.mix) : paramValue(m, key, fallback(key));
        }
        for (int sw = 0; sw < HamloVerbPanel::NumSwitches; ++sw)
        {
            const auto* key = HamloVerbPanel::keyFor(static_cast<HamloVerbPanel::Switch>(sw));
            v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, key, fallback(key)));
        }
        v.on = m.enabled;
        panel->setValues(v);
    });
}

void NodeInspector::buildDelayPage(int k)
{
    // HamloEcho (module pass 3): one drawn panel in the HamloComp style; it fills the editor body.
    largePage = true;
    auto* panel = new HamloEchoPanel();
    controls.add(panel);
    addAndMakeVisible(panel);
    panel->getProperties().set("section", 4);
    echoPanel = panel;
    panel->onKnobGestureStart = [this](HamloEchoPanel::Param p)
    {
        processor.getUndoManager().beginNewTransaction("Delay " + juce::String(HamloEchoPanel::keyFor(p)));
        dragging = true;
    };
    panel->onKnobGestureEnd = [this] { dragging = false; };
    panel->onKnobChange = [this, k](HamloEchoPanel::Param p, double v)
    {
        if (p == HamloEchoPanel::Mix) // the page's Mix is the module's mix
        {
            editChain("Module mix", [k, v](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
            return;
        }
        const juce::Identifier key(HamloEchoPanel::keyFor(p));
        editChain("Delay", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
    };
    panel->onSwitchChange = [this, k](HamloEchoPanel::Switch sw, int option)
    {
        processor.getUndoManager().beginNewTransaction("Delay " + juce::String(HamloEchoPanel::keyFor(sw)));
        const juce::Identifier key(HamloEchoPanel::keyFor(sw));
        editChain("Delay switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
    };
    panel->onPowerChange = [this, k](bool on)
    {
        processor.getUndoManager().beginNewTransaction("Module on");
        editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
    };
    panel->readMeters = [this, k]() -> std::pair<float, float>
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        if (nodePtr == nullptr)
            return { -120.0f, 0.0f };
        const auto& node = *nodePtr;
        return { node.getDelayOutputDb(k), node.getDelayDuckDb(k) };
    };
    panel->readTempo = [this] { return processor.getHostBpm(); };
    refreshers.push_back([this, panel, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        const auto* c = selectedChain(project);
        if (c == nullptr || k >= static_cast<int>(c->size()))
            return;
        const auto& m = (*c)[static_cast<size_t>(k)];
        HamloEchoPanel::Values v;
        const auto& defs = paramsFor(FxType::Delay); // the DSP defaults when a key is absent
        const auto fallback = [&defs](const char* key)
        {
            for (const auto& d : defs)
                if (juce::String(d.key) == key)
                    return d.defaultValue;
            return 0.0;
        };
        for (int p = 0; p < HamloEchoPanel::NumParams; ++p)
        {
            const auto* key = HamloEchoPanel::keyFor(static_cast<HamloEchoPanel::Param>(p));
            v.knobs[static_cast<size_t>(p)] = p == HamloEchoPanel::Mix ? static_cast<double>(m.mix) : paramValue(m, key, fallback(key));
        }
        for (int sw = 0; sw < HamloEchoPanel::NumSwitches; ++sw)
        {
            const auto* key = HamloEchoPanel::keyFor(static_cast<HamloEchoPanel::Switch>(sw));
            v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, key, fallback(key)));
        }
        v.on = m.enabled;
        panel->setValues(v);
    });
}

void NodeInspector::buildLimiterPage(int k)
{
    // HamloRedWall (module pass 5): one drawn panel in the Black Steel style; it fills the editor body.
    largePage = true;
    auto* panel = new HamloRedWallPanel();
    controls.add(panel);
    addAndMakeVisible(panel);
    panel->getProperties().set("section", 4);
    wallPanel = panel;
    panel->onKnobGestureStart = [this](HamloRedWallPanel::Param p)
    {
        processor.getUndoManager().beginNewTransaction("Limiter " + juce::String(HamloRedWallPanel::keyFor(p)));
        dragging = true;
    };
    panel->onKnobGestureEnd = [this] { dragging = false; };
    panel->onKnobChange = [this, k](HamloRedWallPanel::Param p, double v)
    {
        if (p == HamloRedWallPanel::Mix) // the page's Mix is the module's mix
        {
            editChain("Module mix", [k, v](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
            return;
        }
        const juce::Identifier key(HamloRedWallPanel::keyFor(p));
        editChain("Limiter", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
    };
    panel->onSwitchChange = [this, k](HamloRedWallPanel::Switch sw, int option)
    {
        processor.getUndoManager().beginNewTransaction("Limiter " + juce::String(HamloRedWallPanel::keyFor(sw)));
        const juce::Identifier key(HamloRedWallPanel::keyFor(sw));
        editChain("Limiter switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
    };
    panel->onPowerChange = [this, k](bool on)
    {
        processor.getUndoManager().beginNewTransaction("Module on");
        editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
    };
    panel->readMeters = [this, k]() -> std::pair<float, float>
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        if (nodePtr == nullptr)
            return { -120.0f, 0.0f };
        return { nodePtr->getLimiterOutputDb(k), nodePtr->getGainReductionDb(k) };
    };
    refreshers.push_back([this, panel, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        const auto* c = selectedChain(project);
        if (c == nullptr || k >= static_cast<int>(c->size()))
            return;
        const auto& m = (*c)[static_cast<size_t>(k)];
        // the DSP's own defaults when a key is absent (LayerSnapshot): older limiters keep the M6 law
        const auto fallback = [](const char* key)
        {
            const juce::String name(key);
            if (name == "ceilingDb") return -1.0;
            if (name == "releaseMs") return 80.0;
            if (name == "lookaheadMs") return 2.0;
            if (name == "link") return 1.0;
            return 0.0;
        };
        HamloRedWallPanel::Values v;
        for (int p = 0; p < HamloRedWallPanel::NumKnobs; ++p)
        {
            const auto* key = HamloRedWallPanel::keyFor(static_cast<HamloRedWallPanel::Param>(p));
            v.knobs[static_cast<size_t>(p)] = p == HamloRedWallPanel::Mix ? static_cast<double>(m.mix) : paramValue(m, key, fallback(key));
        }
        for (int sw = 0; sw < HamloRedWallPanel::NumSwitches; ++sw)
        {
            const auto* key = HamloRedWallPanel::keyFor(static_cast<HamloRedWallPanel::Switch>(sw));
            v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, key, fallback(key)));
        }
        v.on = m.enabled;
        panel->setValues(v);
    });
}

void NodeInspector::buildTransientPage(int k)
{
    // HamloUdarnik (module pass 6): one drawn panel in the Black Steel style; it fills the editor body.
    largePage = true;
    auto* panel = new HamloUdarnikPanel();
    controls.add(panel);
    addAndMakeVisible(panel);
    panel->getProperties().set("section", 4);
    udarnikPanel = panel;
    panel->onKnobGestureStart = [this](HamloUdarnikPanel::Param p)
    {
        processor.getUndoManager().beginNewTransaction("Transient " + juce::String(HamloUdarnikPanel::keyFor(p)));
        dragging = true;
    };
    panel->onKnobGestureEnd = [this] { dragging = false; };
    panel->onKnobChange = [this, k](HamloUdarnikPanel::Param p, double v)
    {
        if (p == HamloUdarnikPanel::Mix) // MIX is the slot's own mix
        {
            editChain("Transient mix", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
            return;
        }
        const juce::Identifier key(HamloUdarnikPanel::keyFor(p));
        editChain("Transient", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
    };
    panel->onSwitchChange = [this, k](HamloUdarnikPanel::Switch sw, int option)
    {
        processor.getUndoManager().beginNewTransaction("Transient " + juce::String(HamloUdarnikPanel::keyFor(sw)));
        const juce::Identifier key(HamloUdarnikPanel::keyFor(sw));
        editChain("Transient switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
    };
    panel->onPowerChange = [this, k](bool on)
    {
        processor.getUndoManager().beginNewTransaction("Module on");
        editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
    };
    panel->readMeters = [this, k]() -> HamloUdarnikPanel::Reading
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        if (nodePtr == nullptr)
            return {};
        const auto m = nodePtr->getTransientMeters(k);
        return { m.impactDb, m.gainMinDb, m.gainMaxDb, m.events, m.confidence, m.strengthDb };
    };
    panel->readLastHit = [this, k](std::array<float, HamloUdarnikPanel::kHitPoints>& in, std::array<float, HamloUdarnikPanel::kHitPoints>& out)
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        return nodePtr != nullptr ? nodePtr->getTransientLastHit(k, in, out) : 0;
    };
    refreshers.push_back([this, panel, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        const auto* c = selectedChain(project);
        if (c == nullptr || k >= static_cast<int>(c->size()))
            return;
        const auto& m = (*c)[static_cast<size_t>(k)];
        // the DSP's own defaults when a key is absent (LayerSnapshot): older transients keep the M6 law
        const auto fallback = [](const char* key)
        {
            const juce::String name(key);
            if (name == "speed") return 0.5;
            if (name == "rise" || name == "decay" || name == "sensitivity") return 1.0;
            return 0.0;
        };
        HamloUdarnikPanel::Values v;
        for (int p = 0; p < HamloUdarnikPanel::NumKnobs; ++p)
        {
            const auto* key = HamloUdarnikPanel::keyFor(static_cast<HamloUdarnikPanel::Param>(p));
            v.knobs[static_cast<size_t>(p)] = p == HamloUdarnikPanel::Mix ? static_cast<double>(m.mix) : paramValue(m, key, fallback(key));
        }
        for (int sw = 0; sw < HamloUdarnikPanel::NumSwitches; ++sw)
            v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, HamloUdarnikPanel::keyFor(static_cast<HamloUdarnikPanel::Switch>(sw)), 0.0));
        v.on = m.enabled;
        panel->setValues(v);
    });
}

void NodeInspector::buildStereoPage(int k)
{
    // HamloShirota (module pass 8): one drawn panel in the Black Steel style; it fills the editor body.
    largePage = true;
    auto* panel = new HamloShirotaPanel();
    controls.add(panel);
    addAndMakeVisible(panel);
    panel->getProperties().set("section", 4);
    shirotaPanel = panel;
    panel->onKnobGestureStart = [this](HamloShirotaPanel::Param p)
    {
        processor.getUndoManager().beginNewTransaction("Stereo " + juce::String(HamloShirotaPanel::keyFor(p)));
        dragging = true;
    };
    panel->onKnobGestureEnd = [this] { dragging = false; };
    panel->onKnobChange = [this, k](HamloShirotaPanel::Param p, double v)
    {
        if (p == HamloShirotaPanel::Mix) // MIX is the slot's own mix
        {
            editChain("Stereo mix", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
            return;
        }
        const juce::Identifier key(HamloShirotaPanel::keyFor(p));
        editChain("Stereo", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
    };
    panel->onSwitchChange = [this, k](HamloShirotaPanel::Switch sw, int option)
    {
        processor.getUndoManager().beginNewTransaction("Stereo " + juce::String(HamloShirotaPanel::keyFor(sw)));
        const juce::Identifier key(HamloShirotaPanel::keyFor(sw));
        editChain("Stereo switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
    };
    panel->onPowerChange = [this, k](bool on)
    {
        processor.getUndoManager().beginNewTransaction("Module on");
        editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
    };
    panel->readMeters = [this, k]() -> HamloShirotaPanel::Reading
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        if (nodePtr == nullptr)
            return {};
        const auto m = nodePtr->getStereoMeters(k);
        return { m.correlation, m.guardDb, m.oneChannel };
    };
    panel->readScope = [this, k](std::array<float, HamloShirotaPanel::kScopePoints>& left, std::array<float, HamloShirotaPanel::kScopePoints>& right)
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        return nodePtr != nullptr && nodePtr->getStereoScope(k, left, right);
    };
    refreshers.push_back([this, panel, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        const auto* c = selectedChain(project);
        if (c == nullptr || k >= static_cast<int>(c->size()))
            return;
        const auto& m = (*c)[static_cast<size_t>(k)];
        HamloShirotaPanel::Values v;
        for (int p = 0; p < HamloShirotaPanel::NumKnobs; ++p)
        {
            const auto param = static_cast<HamloShirotaPanel::Param>(p);
            // the DSP's own defaults when a key is absent (LayerSnapshot): older stereo modules keep the M6 law
            double fallback = param == HamloShirotaPanel::Width ? 1.0 : param == HamloShirotaPanel::MonoBass ? 10.0 : 0.0;
            v.knobs[static_cast<size_t>(p)] = param == HamloShirotaPanel::Mix ? static_cast<double>(m.mix) : paramValue(m, HamloShirotaPanel::keyFor(param), fallback);
        }
        for (int sw = 0; sw < HamloShirotaPanel::NumSwitches; ++sw)
            v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, HamloShirotaPanel::keyFor(static_cast<HamloShirotaPanel::Switch>(sw)), 0.0));
        v.on = m.enabled;
        // a node on one channel (Mid / Side / Left / Right) has no stereo to widen: the page shows OSHIBKA
        auto channel = ChannelMode::Stereo;
        const auto& layer = selectedLayer(project);
        if (selection.kind == Selection::Kind::Fx)
            for (const auto& n : layer.fxNodes)
                if (n.id == selection.id)
                    channel = n.channel;
        if (selection.kind == Selection::Kind::Eq)
            for (const auto& n : layer.eqNodes)
                if (n.id == selection.id)
                    channel = n.channel;
        v.stereoNode = channel == ChannelMode::Stereo || channel == ChannelMode::LinkedStereo || channel == ChannelMode::DualMono;
        panel->setValues(v);
    });
}

void NodeInspector::buildGatePage(int k)
{
    // HamloZastava (module pass 7): one drawn panel in the Black Steel style; it fills the editor body.
    largePage = true;
    auto* panel = new HamloZastavaPanel();
    controls.add(panel);
    addAndMakeVisible(panel);
    panel->getProperties().set("section", 4);
    zastavaPanel = panel;
    panel->onKnobGestureStart = [this](HamloZastavaPanel::Param p)
    {
        processor.getUndoManager().beginNewTransaction("Gate " + juce::String(HamloZastavaPanel::keyFor(p)));
        dragging = true;
    };
    panel->onKnobGestureEnd = [this] { dragging = false; };
    panel->onKnobChange = [this, k](HamloZastavaPanel::Param p, double v)
    {
        if (p == HamloZastavaPanel::Mix) // MIX is the slot's own mix
        {
            editChain("Gate mix", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
            return;
        }
        const juce::Identifier key(HamloZastavaPanel::keyFor(p));
        editChain("Gate", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
    };
    panel->onSwitchChange = [this, k](HamloZastavaPanel::Switch sw, int option)
    {
        processor.getUndoManager().beginNewTransaction("Gate " + juce::String(HamloZastavaPanel::keyFor(sw)));
        const juce::Identifier key(HamloZastavaPanel::keyFor(sw));
        editChain("Gate switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
    };
    panel->onPowerChange = [this, k](bool on)
    {
        processor.getUndoManager().beginNewTransaction("Module on");
        editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
    };
    panel->readMeters = [this, k]() -> HamloZastavaPanel::Reading
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        if (nodePtr == nullptr)
            return {};
        const auto m = nodePtr->getGateMeters(k);
        return { m.attenuationDb, m.maxAttenuationDb, m.keyDb, m.open, m.openings };
    };
    refreshers.push_back([this, panel, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        const auto* c = selectedChain(project);
        if (c == nullptr || k >= static_cast<int>(c->size()))
            return;
        const auto& m = (*c)[static_cast<size_t>(k)];
        HamloZastavaPanel::Values v;
        for (int p = 0; p < HamloZastavaPanel::NumKnobs; ++p)
        {
            const auto param = static_cast<HamloZastavaPanel::Param>(p);
            // the DSP's own defaults when a key is absent (LayerSnapshot): older gates keep the M6 law
            v.knobs[static_cast<size_t>(p)] = param == HamloZastavaPanel::Mix ? static_cast<double>(m.mix)
                                                                              : paramValue(m, HamloZastavaPanel::keyFor(param), HamloZastavaPanel::defaultFor(param));
        }
        for (int sw = 0; sw < HamloZastavaPanel::NumSwitches; ++sw)
            v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, HamloZastavaPanel::keyFor(static_cast<HamloZastavaPanel::Switch>(sw)), 0.0));
        v.on = m.enabled;
        panel->setValues(v);
    });
}

void NodeInspector::buildSaturatorPage(int k)
{
    // HamloMag (module pass 4): one drawn panel in the HamloComp style; it fills the editor body.
    largePage = true;
    auto* panel = new HamloMagPanel();
    controls.add(panel);
    addAndMakeVisible(panel);
    panel->getProperties().set("section", 4);
    magPanel = panel;
    panel->onKnobGestureStart = [this](HamloMagPanel::Param p)
    {
        processor.getUndoManager().beginNewTransaction("Saturator " + juce::String(HamloMagPanel::keyFor(p)));
        dragging = true;
    };
    panel->onKnobGestureEnd = [this] { dragging = false; };
    panel->onKnobChange = [this, k](HamloMagPanel::Param p, double v)
    {
        if (p == HamloMagPanel::Mix) // the page's Mix is the module's mix
        {
            editChain("Module mix", [k, v](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
            return;
        }
        const juce::Identifier key(HamloMagPanel::keyFor(p));
        editChain("Saturator", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
    };
    panel->onSwitchChange = [this, k](HamloMagPanel::Switch sw, int option)
    {
        processor.getUndoManager().beginNewTransaction("Saturator " + juce::String(HamloMagPanel::keyFor(sw)));
        const juce::Identifier key(HamloMagPanel::keyFor(sw));
        editChain("Saturator switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
    };
    panel->onPowerChange = [this, k](bool on)
    {
        processor.getUndoManager().beginNewTransaction("Module on");
        editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
    };
    panel->readMeters = [this, k]() -> std::pair<float, float>
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        if (nodePtr == nullptr)
            return { -120.0f, -120.0f };
        const auto& node = *nodePtr;
        return { node.getSaturatorInputDb(k), node.getSaturatorOutputDb(k) };
    };
    refreshers.push_back([this, panel, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        const auto* c = selectedChain(project);
        if (c == nullptr || k >= static_cast<int>(c->size()))
            return;
        const auto& m = (*c)[static_cast<size_t>(k)];
        HamloMagPanel::Values v;
        const auto& defs = paramsFor(FxType::Saturator); // the DSP defaults when a key is absent
        const auto fallback = [&defs](const char* key)
        {
            for (const auto& d : defs)
                if (juce::String(d.key) == key)
                    return d.defaultValue;
            return 0.0;
        };
        for (int p = 0; p < HamloMagPanel::NumParams; ++p)
        {
            const auto* key = HamloMagPanel::keyFor(static_cast<HamloMagPanel::Param>(p));
            v.knobs[static_cast<size_t>(p)] = p == HamloMagPanel::Mix ? static_cast<double>(m.mix) : paramValue(m, key, fallback(key));
        }
        for (int sw = 0; sw < HamloMagPanel::NumSwitches; ++sw)
        {
            const auto* key = HamloMagPanel::keyFor(static_cast<HamloMagPanel::Switch>(sw));
            v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, key, fallback(key)));
        }
        v.on = m.enabled;
        panel->setValues(v);
    });
}

void NodeInspector::buildFaderPage(int k)
{
    // HamloFader (module pass 9): the band level utility; one drawn panel that fills the editor body.
    largePage = true;
    auto* panel = new HamloFaderPanel();
    controls.add(panel);
    addAndMakeVisible(panel);
    panel->getProperties().set("section", 4);
    faderPanel = panel;
    panel->onKnobGestureStart = [this](HamloFaderPanel::Param p)
    {
        processor.getUndoManager().beginNewTransaction("Fader " + juce::String(HamloFaderPanel::keyFor(p)));
        dragging = true;
    };
    panel->onKnobGestureEnd = [this] { dragging = false; };
    panel->onKnobChange = [this, k](HamloFaderPanel::Param p, double v)
    {
        if (p == HamloFaderPanel::Mix) // the page's Mix is the module's mix
        {
            editChain("Module mix", [k, v](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
            return;
        }
        const juce::Identifier key(HamloFaderPanel::keyFor(p));
        editChain("Fader", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
    };
    panel->onSwitchChange = [this, k](HamloFaderPanel::Switch sw, int option)
    {
        processor.getUndoManager().beginNewTransaction("Fader " + juce::String(HamloFaderPanel::keyFor(sw)));
        const juce::Identifier key(HamloFaderPanel::keyFor(sw));
        editChain("Fader switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
    };
    panel->onPowerChange = [this, k](bool on)
    {
        processor.getUndoManager().beginNewTransaction("Module on");
        editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
    };
    panel->readMeters = [this, k]() -> HamloFaderPanel::Meters
    {
        const auto project = processor.getProject();
        const int engineSlot = selectedEngineSlot(project);
        if (engineSlot < 0)
            return {};
        const auto& node = processor.getEngine().getLayer(project.selectedLayer).getNode(engineSlot);
        return { node.getFaderInputDb(k), node.getFaderOutputDb(k), node.getFaderInputLongDb(k) };
    };
    refreshers.push_back([this, panel, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        const auto* c = selectedChain(project);
        if (c == nullptr || k >= static_cast<int>(c->size()))
            return;
        const auto& m = (*c)[static_cast<size_t>(k)];
        HamloFaderPanel::Values v;
        for (int p = 0; p < HamloFaderPanel::NumParams; ++p)
        {
            const auto param = static_cast<HamloFaderPanel::Param>(p);
            v.knobs[static_cast<size_t>(p)] = param == HamloFaderPanel::Mix ? static_cast<double>(m.mix)
                                                                           : paramValue(m, HamloFaderPanel::keyFor(param), HamloFaderPanel::defaultFor(param));
        }
        for (int sw = 0; sw < HamloFaderPanel::NumSwitches; ++sw)
            v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, HamloFaderPanel::keyFor(static_cast<HamloFaderPanel::Switch>(sw)), 0.0));
        v.on = m.enabled;
        panel->setValues(v);
    });
}

void NodeInspector::buildRevolutionPage(int k)
{
    // HamloRevolution (module pass 10): the Monsta FX slot; one drawn panel that fills the editor body.
    largePage = true;
    auto* panel = new HamloRevolutionPanel();
    controls.add(panel);
    addAndMakeVisible(panel);
    panel->getProperties().set("section", 4);
    revolutionPanel = panel;
    panel->onKnobGestureStart = [this](HamloRevolutionPanel::Param p)
    {
        processor.getUndoManager().beginNewTransaction("Revolution " + juce::String(HamloRevolutionPanel::keyFor(p)));
        dragging = true;
    };
    panel->onKnobGestureEnd = [this] { dragging = false; };
    panel->onKnobChange = [this, k](HamloRevolutionPanel::Param p, double v)
    {
        if (p == HamloRevolutionPanel::Mix) // the page's MIX is the module's mix
        {
            editChain("Module mix", [k, v](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].mix = static_cast<float>(v); });
            return;
        }
        const juce::Identifier key(HamloRevolutionPanel::keyFor(p));
        editChain("Revolution", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, v); });
    };
    panel->onSwitchChange = [this, k](HamloRevolutionPanel::Switch sw, int option)
    {
        processor.getUndoManager().beginNewTransaction("Revolution " + juce::String(HamloRevolutionPanel::keyFor(sw)));
        const juce::Identifier key(HamloRevolutionPanel::keyFor(sw));
        editChain("Revolution switch", [&](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].parameters.set(key, option); });
    };
    panel->onPatternChange = [this, k](const revolution::Pattern& pattern, juce::int64 seed, const juce::String& what)
    {
        processor.getUndoManager().beginNewTransaction("Revolution " + what);
        editChain("Revolution pattern", [&](std::vector<FxSlotState>& c)
        {
            auto& params = c[static_cast<size_t>(k)].parameters;
            revolution::writePattern(params, pattern);
            params.set("revSeed", seed);
        });
    };
    panel->onUpgrade = [this, k]
    {
        processor.getUndoManager().beginNewTransaction("Upgrade to HamloRevolution");
        editChain("Upgrade to HamloRevolution", [k](std::vector<FxSlotState>& c)
        {
            c[static_cast<size_t>(k)].parameters = defaultModule(FxType::MonstaFx).parameters;
        });
    };
    panel->onPowerChange = [this, k](bool on)
    {
        processor.getUndoManager().beginNewTransaction("Module on");
        editChain("Module on", [k, on](std::vector<FxSlotState>& c) { c[static_cast<size_t>(k)].enabled = on; });
    };
    panel->readStatus = [this, k]() -> HamloRevolutionPanel::Status
    {
        const auto project = processor.getProject();
        const int engineSlot = selectedEngineSlot(project);
        if (engineSlot < 0)
            return {};
        const auto& node = processor.getEngine().getLayer(project.selectedLayer).getNode(engineSlot);
        return { node.getRevolutionStep(k), node.isRevolutionWarming(k) };
    };
    refreshers.push_back([this, panel, k]
    {
        const auto project = processor.getProject(); // keep alive: selectedChain points into it
        const auto* c = selectedChain(project);
        if (c == nullptr || k >= static_cast<int>(c->size()))
            return;
        const auto& m = (*c)[static_cast<size_t>(k)];
        HamloRevolutionPanel::Values v;
        for (int p = 0; p < HamloRevolutionPanel::NumParams; ++p)
        {
            const auto param = static_cast<HamloRevolutionPanel::Param>(p);
            v.knobs[static_cast<size_t>(p)] = param == HamloRevolutionPanel::Mix
                                                  ? static_cast<double>(m.mix)
                                                  : paramValue(m, HamloRevolutionPanel::keyFor(param), HamloRevolutionPanel::defaultFor(param));
        }
        const HamloRevolutionPanel::Values defaults;
        for (int sw = 0; sw < HamloRevolutionPanel::NumSwitches; ++sw)
            v.switches[static_cast<size_t>(sw)] = juce::roundToInt(paramValue(m, HamloRevolutionPanel::keyFor(static_cast<HamloRevolutionPanel::Switch>(sw)),
                                                                              defaults.switches[static_cast<size_t>(sw)]));
        v.pattern = revolution::readPattern(m.parameters);
        v.seed = static_cast<juce::int64>(paramValue(m, "revSeed", 1.0));
        v.legacy = paramValue(m, "mode", 0.0) < 0.5;
        v.on = m.enabled;
        panel->setValues(v);
    });
}

void NodeInspector::updateLiveReadouts(bool settle)
{
    if (settle && hamloPanel != nullptr && hamloPanel->readMeters)
    {
        const auto [inputDb, reductionDb] = hamloPanel->readMeters();
        hamloPanel->settleMeters(inputDb, reductionDb);
    }
    if (settle && verbPanel != nullptr && verbPanel->readMeters)
    {
        const auto [outputDb, duckDb] = verbPanel->readMeters();
        verbPanel->settleMeters(outputDb, duckDb);
    }
    if (settle && echoPanel != nullptr && echoPanel->readMeters)
    {
        const auto [outputDb, duckDb] = echoPanel->readMeters();
        echoPanel->settleMeters(outputDb, duckDb);
    }
    if (settle && wallPanel != nullptr && wallPanel->readMeters)
    {
        const auto [outputDb, reductionDb] = wallPanel->readMeters();
        wallPanel->settleMeters(outputDb, reductionDb);
    }
    if (settle && shirotaPanel != nullptr && shirotaPanel->readMeters)
        shirotaPanel->settleMeters(shirotaPanel->readMeters());
    if (settle && zastavaPanel != nullptr && zastavaPanel->readMeters)
        zastavaPanel->settleMeters(zastavaPanel->readMeters());
    if (settle && udarnikPanel != nullptr && udarnikPanel->readMeters)
        udarnikPanel->settleMeters(udarnikPanel->readMeters());
    if (settle && magPanel != nullptr && magPanel->readMeters)
    {
        const auto [inputDb, outputDb] = magPanel->readMeters();
        magPanel->settleMeters(inputDb, outputDb);
    }
    if (settle && faderPanel != nullptr && faderPanel->readMeters)
        faderPanel->settleMeters(faderPanel->readMeters());
    if (settle && revolutionPanel != nullptr && revolutionPanel->readStatus)
        revolutionPanel->settleStatus(revolutionPanel->readStatus());
    // Live readouts from the audio engine (atomics, no locks).
    juce::String newStatus;
    if (selection.kind != Selection::Kind::None && selection.slot >= 0)
    {
        const auto project = processor.getProject();
        const auto* nodePtr = selectedNodeProcessor(project);
        const auto* chain = selectedChain(project);
        if (nodePtr != nullptr && chain != nullptr && selection.slot < static_cast<int>(chain->size()))
        {
            const auto& node = *nodePtr;
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
