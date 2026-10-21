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
        { "releaseMs", "Release", 1, 5000, 120, 150, " ms", 0 }, { "makeupDb", "Makeup", -24, 24, 0, 0, " dB", 1 } };
    static const std::vector<ParamDef> reverb {
        { "predelayMs", "Pre-delay", 0, 250, 10, 30, " ms", 0 }, { "decaySeconds", "Decay", 0.1, 20, 1.8, 2, " s", 2 },
        { "hfDecayRatio", "HF decay", 0.05, 1, 0.5, 0.4, "", 2 }, { "size", "Size", 0.3, 1.5, 1, 0.9, "", 2 },
        { "width", "Width", 0, 1.5, 1, 0.75, "", 2 }, { "lowCutHz", "Wet low cut", 10, 1000, 80, 100, " Hz", 0 } };
    static const std::vector<ParamDef> gain { { "gainDb", "Gain", -60, 24, 0, 0, " dB", 1 } };
    static const std::vector<ParamDef> none;
    switch (type)
    {
        case FxType::Compressor: return compressor;
        case FxType::Reverb: return reverb;
        case FxType::Gain: return gain;
        case FxType::Limiter: case FxType::Delay: case FxType::TransientShaper: case FxType::Saturator:
        case FxType::MonstaFx: case FxType::StereoProcessor: case FxType::GateExpander: break;
    }
    return none;
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
        for (auto& n : p.layers[0].eqNodes)
            if (n.id == id)
                change(n);
    });
}

void NodeInspector::editFx(const juce::String& name, const std::function<void(FxNodeState&)>& change)
{
    const auto id = selection.id;
    processor.editProject(name, [&](FxProjectState& p)
    {
        for (auto& n : p.layers[0].fxNodes)
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

void NodeInspector::rebuild()
{
    labels.clear();
    controls.clear();
    refreshers.clear();
    nodeIndex = -1;
    title = "Nothing selected";
    status = "Select an EQ point or FX region in the workspace or the node list.";

    const auto project = processor.getProject();
    const auto& layer = project.layers[0];
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
        title = "EQ " + juce::String(nodeIndex + 1) + "  /  PRE-EQ (before every FX region)";
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
                auto& v = p.layers[0].eqNodes;
                v.erase(std::remove_if(v.begin(), v.end(), [id](const EqNodeState& n) { return n.id == id; }), v.end());
            });
            selection = {};
            rebuildAsync();
        });
        setInfo(del, 0, 70, kRowHeight);

        refreshers.push_back([this, &on, &shape, &freq, &gainKnob, &q, &slope, &mode]
        {
            const auto p = processor.getProject();
            for (const auto& n : p.layers[0].eqNodes)
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
        selection.slot = juce::jlimit(0, static_cast<int>(node.chain.size()) - 1, selection.slot);
        title = "FX " + juce::String(nodeIndex + 1) + "  /  region after PRE-EQ, chain of " + juce::String(node.chain.size()) + " (max 6)";
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
                auto& v = p.layers[0].fxNodes;
                v.erase(std::remove_if(v.begin(), v.end(), [id](const FxNodeState& n) { return n.id == id; }), v.end());
            });
            selection = {};
            rebuildAsync();
        });
        setInfo(del, 0, 70, kRowHeight);

        // Chain: one row per slot.
        juce::StringArray typeNames;
        for (auto t : kTypes)
            typeNames.add(fxName(t) + (isFxAvailable(t) ? "" : " (M6)"));
        for (int k = 0; k < static_cast<int>(node.chain.size()); ++k)
        {
            auto& pick = addButton(juce::String(k + 1) + (k == selection.slot ? "  >" : ""), [this, k] { selection.slot = k; rebuildAsync(); });
            setInfo(pick, 1, 44, kRowHeight - 2, true);
            auto& type = addCombo({}, typeNames, [this, k](int i)
            {
                if (!isFxAvailable(kTypes[static_cast<size_t>(i)]))
                    return;
                editFx("Slot type", [k, i](FxNodeState& n) { n.chain[static_cast<size_t>(k)].type = kTypes[static_cast<size_t>(i)]; n.chain[static_cast<size_t>(k)].parameters.clear(); });
                rebuildAsync();
            });
            for (size_t t = 0; t < kTypes.size(); ++t)
                type.setItemEnabled(static_cast<int>(t) + 1, isFxAvailable(kTypes[t]));
            setInfo(type, 1, 130, kRowHeight - 2);
            auto& slotOn = addButton("On", [this, k] { beginEdit("Slot on"); editFx("Slot on", [k](FxNodeState& n) { n.chain[static_cast<size_t>(k)].enabled = !n.chain[static_cast<size_t>(k)].enabled; }); }, true);
            setInfo(slotOn, 1, 46, kRowHeight - 2);
            auto* slotMix = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
            slotMix->setRange(0, 1, 0.01);
            slotMix->setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
            slotMix->onDragStart = [this] { processor.getUndoManager().beginNewTransaction("Slot mix"); dragging = true; };
            slotMix->onDragEnd = [this] { dragging = false; };
            slotMix->onValueChange = [this, slotMix, k] { beginEdit("Slot mix"); const auto v = static_cast<float>(slotMix->getValue());
                                                          editFx("Slot mix", [k, v](FxNodeState& n) { n.chain[static_cast<size_t>(k)].mix = v; }); };
            controls.add(slotMix);
            addAndMakeVisible(slotMix);
            setInfo(*slotMix, 1, 130, kRowHeight - 2);
            refreshers.push_back([this, &type, &slotOn, slotMix, k]
            {
                const auto p = processor.getProject();
                for (const auto& n : p.layers[0].fxNodes)
                    if (n.id == selection.id && k < static_cast<int>(n.chain.size()))
                    {
                        const auto& s = n.chain[static_cast<size_t>(k)];
                        type.setSelectedItemIndex(indexOf(kTypes, s.type), juce::dontSendNotification);
                        slotOn.setToggleState(s.enabled, juce::dontSendNotification);
                        slotMix->setValue(s.mix, juce::dontSendNotification);
                    }
            });
        }
        if (node.chain.size() < static_cast<size_t>(kMaxFxPerNode))
        {
            auto& add = addButton("+ slot", [this]
            {
                processor.getUndoManager().beginNewTransaction("Add slot");
                editFx("Add slot", [](FxNodeState& n) { FxSlotState s; s.type = FxType::Gain; n.chain.push_back(s); });
                rebuildAsync();
            });
            setInfo(add, 1, 70, kRowHeight - 2, true);
        }
        if (node.chain.size() > 1)
        {
            auto& remove = addButton("- slot " + juce::String(selection.slot + 1), [this]
            {
                processor.getUndoManager().beginNewTransaction("Remove slot");
                const int k = selection.slot;
                editFx("Remove slot", [k](FxNodeState& n) { n.chain.erase(n.chain.begin() + k); });
                selection.slot = juce::jmax(0, selection.slot - 1);
                rebuildAsync();
            });
            setInfo(remove, 1, 80, kRowHeight - 2);
        }

        // Parameters of the selected slot.
        const auto& slotState = node.chain[static_cast<size_t>(selection.slot)];
        for (const auto& def : paramsFor(slotState.type))
        {
            const auto key = juce::String(def.key);
            const int k = selection.slot;
            auto& knob = addKnob(def.label, def.min, def.max, def.midPoint, def.suffix, def.decimals, [this, key, k](double v)
            {
                editFx(key, [&](FxNodeState& n) { n.chain[static_cast<size_t>(k)].parameters.set(juce::Identifier(key), v); });
            });
            setInfo(knob, 2, kKnobWidth, kKnobHeight);
            const double fallback = def.defaultValue;
            refreshers.push_back([this, &knob, key, k, fallback]
            {
                const auto p = processor.getProject();
                for (const auto& n : p.layers[0].fxNodes)
                    if (n.id == selection.id && k < static_cast<int>(n.chain.size()))
                        knob.setValue(paramValue(n.chain[static_cast<size_t>(k)], key.toRawUTF8(), fallback), juce::dontSendNotification);
            });
        }

        refreshers.push_back([this, &on, &low, &high, &mix, &mode]
        {
            const auto p = processor.getProject();
            for (const auto& n : p.layers[0].fxNodes)
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
}

void NodeInspector::refresh()
{
    for (auto& r : refreshers)
        r();
}

void NodeInspector::timerCallback()
{
    // Live readouts from the audio engine (atomics, no locks).
    juce::String newStatus;
    if (selection.kind == Selection::Kind::Fx && nodeIndex >= 0)
    {
        const auto& node = processor.getEngine().getNode(nodeIndex);
        const auto project = processor.getProject();
        if (nodeIndex < static_cast<int>(project.layers[0].fxNodes.size()))
        {
            const auto& chain = project.layers[0].fxNodes[static_cast<size_t>(nodeIndex)].chain;
            const int k = juce::jlimit(0, static_cast<int>(chain.size()) - 1, selection.slot);
            if (chain[static_cast<size_t>(k)].type == FxType::Compressor)
                newStatus = "Gain reduction  " + juce::String(node.getGainReductionDb(k), 1) + " dB";
            if (chain[static_cast<size_t>(k)].type == FxType::Reverb && node.slotStarved(k))
                newStatus = "Reverb pool full (8 per layer): this slot is silent.";
        }
    }
    if (newStatus != status && selection.kind == Selection::Kind::Fx)
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
    g.setColour(status.startsWith("Reverb pool") ? colours::warning : colours::accent);
    g.drawText(status, area.reduced(12.0f, 6.0f).removeFromTop(18.0f), juce::Justification::centredRight);
    if (!chainArea.isEmpty())
    {
        g.setColour(colours::dim);
        g.setFont(11.0f);
        g.drawText("CHAIN (processed top to bottom)", chainArea.toFloat().removeFromTop(14.0f), juce::Justification::centredLeft);
        g.drawText("SLOT " + juce::String(selection.slot + 1) + " PARAMETERS", paramArea.toFloat().removeFromTop(14.0f), juce::Justification::centredLeft);
    }
}

void NodeInspector::resized()
{
    auto area = getLocalBounds().reduced(12, 6);
    area.removeFromTop(22);
    const bool fx = selection.kind == Selection::Kind::Fx;
    auto top = area.removeFromTop(kKnobHeight + 16);
    chainArea = fx ? area.removeFromLeft(juce::jmin(380, area.getWidth() / 2)) : juce::Rectangle<int>();
    paramArea = fx ? area.withTrimmedLeft(16) : juce::Rectangle<int>();

    int topX = top.getX();
    int chainY = chainArea.getY() + 16, chainX = chainArea.getX();
    int paramX = paramArea.getX();
    for (auto* c : controls)
    {
        const auto& props = c->getProperties();
        const int section = props["section"], width = props["width"], height = props["height"];
        const bool newRow = props["newRow"];
        if (section == 0)
        {
            const int y = height == kKnobHeight ? top.getY() + 16 : top.getY() + 32;
            c->setBounds(topX, y, width, height);
            topX += width + 10;
        }
        else if (section == 1)
        {
            if (newRow && chainX != chainArea.getX())
            {
                chainX = chainArea.getX();
                chainY += kRowHeight;
            }
            c->setBounds(chainX, chainY, width, height);
            chainX += width + 4;
        }
        else
        {
            c->setBounds(paramX, paramArea.getY() + 32, width, height);
            paramX += width + 8;
        }
    }
}
} // namespace hpeq::ui
