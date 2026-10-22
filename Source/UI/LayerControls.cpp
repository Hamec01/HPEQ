#include "UI/LayerControls.h"

namespace hpeq::ui
{
namespace
{
constexpr std::array<ChannelMode, 5> kModes { ChannelMode::Stereo, ChannelMode::Left, ChannelMode::Right, ChannelMode::Mid, ChannelMode::Side };

void styleToggle(juce::TextButton& b, bool on, juce::Colour colour)
{
    b.setColour(juce::TextButton::buttonColourId, on ? colour.withAlpha(0.45f) : colours::panelRaised);
    b.setColour(juce::TextButton::textColourOffId, on ? colours::text : colours::dim);
}
} // namespace

LayerControls::LayerControls(FxPluginProcessor& p) : processor(p)
{
    title.setColour(juce::Label::textColourId, colours::text);
    title.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    addAndMakeVisible(title);

    on.setTooltip("Layer on / bypassed (a bypassed layer is not processed)");
    mute.setTooltip("Mute: the layer keeps running but is not heard");
    solo.setTooltip("Solo: hear only soloed layers (diagnostic; mutes are kept)");
    on.onClick = [this] { editLayer("Layer on", [](LayerState& l) { l.enabled = !l.enabled; }); };
    mute.onClick = [this] { editLayer("Layer mute", [](LayerState& l) { l.mute = !l.mute; }); };
    solo.onClick = [this] { editLayer("Layer solo", [](LayerState& l) { l.solo = !l.solo; }); };
    for (auto* b : { &on, &mute, &solo })
        addAndMakeVisible(*b);

    setupSlider(gain, gainLabel, "Gain", -60.0, 24.0, 0.0, " dB", 1, [](LayerState& l, double v) { l.gainDb = static_cast<float>(v); });
    setupSlider(mix, mixLabel, "Mix", 0.0, 100.0, 50.0, " %", 0, [](LayerState& l, double v) { l.mix = static_cast<float>(v / 100.0); });
    setupSlider(pan, panLabel, "Pan", -1.0, 1.0, 0.0, "", 2, [](LayerState& l, double v) { l.pan = static_cast<float>(v); });
    setupSlider(width, widthLabel, "Width", 0.0, 200.0, 100.0, " %", 0, [](LayerState& l, double v) { l.width = static_cast<float>(v / 100.0); });
    gain.setSkewFactorFromMidPoint(-12.0);

    mode.addItemList({ "Stereo", "Left only", "Right only", "Mid only", "Side only" }, 1);
    mode.setTooltip("Which part of the signal this layer processes; the rest passes unchanged");
    mode.onChange = [this]
    {
        const auto m = kModes[static_cast<size_t>(juce::jmax(0, mode.getSelectedItemIndex()))];
        processor.getUndoManager().beginNewTransaction("Layer channel");
        editLayer("Layer channel", [m](LayerState& l) { l.channel = m; });
    };
    routing.addItemList({ "Parallel layers", "Serial layers" }, 1);
    routing.setTooltip("Parallel: every layer processes the input and adds its difference.\nSerial: layer 1 > 2 > ... > 10.");
    routing.onChange = [this]
    {
        const auto r = routing.getSelectedItemIndex() == 1 ? LayerRouting::Serial : LayerRouting::Parallel;
        processor.getUndoManager().beginNewTransaction("Routing");
        processor.editProject("Routing", [r](FxProjectState& p) { p.routing = r; });
    };
    addAndMakeVisible(mode);
    addAndMakeVisible(routing);

    copy.setTooltip("Copy this layer");
    paste.setTooltip("Replace this layer with the copied one");
    duplicate.setTooltip("Copy this layer into the next empty layer and select it");
    clear.setTooltip("Remove every node of this layer and reset its controls");
    copy.onClick = [this] { processor.copyLayer(processor.getProject().selectedLayer); refresh(); };
    paste.onClick = [this]
    {
        processor.getUndoManager().beginNewTransaction("Paste layer");
        processor.pasteLayer(processor.getProject().selectedLayer);
    };
    duplicate.onClick = [this]
    {
        const auto project = processor.getProject();
        for (int j = 1; j < kNumLayers; ++j)
        {
            const int target = (project.selectedLayer + j) % kNumLayers;
            if (layerHasContent(project.layers[static_cast<size_t>(target)]))
                continue;
            const auto source = selectedLayer(project);
            processor.getUndoManager().beginNewTransaction("Duplicate layer");
            processor.editProject("Duplicate layer", [&](FxProjectState& p)
            {
                copyLayerInto(p, source, target);
                p.selectedLayer = target;
            });
            if (onLayerChanged)
                onLayerChanged();
            return;
        }
    };
    clear.onClick = [this]
    {
        processor.getUndoManager().beginNewTransaction("Clear layer");
        editLayer("Clear layer", [](LayerState& l) { l = LayerState {}; });
    };
    for (auto* b : { &copy, &paste, &duplicate, &clear })
        addAndMakeVisible(*b);
    refresh();
}

void LayerControls::setupSlider(juce::Slider& s, juce::Label& label, const juce::String& name, double min, double max, double mid,
                                const juce::String& suffix, int decimals, std::function<void(LayerState&, double)> apply)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 20);
    s.setRange(min, max, 0.0);
    if (mid > min && mid < max)
        s.setSkewFactorFromMidPoint(mid);
    s.setNumDecimalPlacesToDisplay(decimals);
    s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::trackColourId, colours::accent.withAlpha(0.6f));
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.onDragStart = [this, name] { processor.getUndoManager().beginNewTransaction("Layer " + name); dragging = true; };
    s.onDragEnd = [this] { dragging = false; };
    s.onValueChange = [this, &s, name, apply]
    {
        if (!dragging)
            processor.getUndoManager().beginNewTransaction("Layer " + name);
        const double v = s.getValue();
        editLayer("Layer " + name, [apply, v](LayerState& l) { apply(l, v); });
    };
    label.setText(name, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, colours::dim);
    label.setFont(juce::FontOptions(11.0f));
    label.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(s);
    addAndMakeVisible(label);
}

void LayerControls::editLayer(const juce::String& name, const std::function<void(LayerState&)>& change)
{
    processor.editProject(name, [&](FxProjectState& p) { change(selectedLayer(p)); });
}

void LayerControls::refresh()
{
    const auto project = processor.getProject();
    const auto& layer = selectedLayer(project);
    title.setText("Layer " + juce::String(project.selectedLayer + 1), juce::dontSendNotification);
    styleToggle(on, layer.enabled, colours::accent);
    styleToggle(mute, layer.mute, colours::warning);
    styleToggle(solo, layer.solo, colours::warning);
    gain.setValue(layer.gainDb, juce::dontSendNotification);
    mix.setValue(layer.mix * 100.0, juce::dontSendNotification);
    pan.setValue(layer.pan, juce::dontSendNotification);
    width.setValue(layer.width * 100.0, juce::dontSendNotification);
    int modeIndex = 0;
    for (size_t i = 0; i < kModes.size(); ++i)
        if (kModes[i] == layer.channel)
            modeIndex = static_cast<int>(i);
    mode.setSelectedItemIndex(modeIndex, juce::dontSendNotification);
    routing.setSelectedItemIndex(project.routing == LayerRouting::Serial ? 1 : 0, juce::dontSendNotification);
    paste.setEnabled(processor.canPasteLayer());
}

void LayerControls::paint(juce::Graphics& g)
{
    g.setColour(colours::panel);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void LayerControls::resized()
{
    auto r = getLocalBounds().reduced(6, 3);
    title.setBounds(r.removeFromLeft(70));
    on.setBounds(r.removeFromLeft(40).reduced(1, 0));
    mute.setBounds(r.removeFromLeft(30).reduced(1, 0));
    solo.setBounds(r.removeFromLeft(30).reduced(1, 0));
    r.removeFromLeft(6);
    routing.setBounds(r.removeFromRight(130).reduced(2, 0));
    r.removeFromRight(8);
    for (auto* b : { &clear, &duplicate, &paste, &copy })
        b->setBounds(r.removeFromRight(46).reduced(1, 0));
    r.removeFromRight(6);
    mode.setBounds(r.removeFromRight(100).reduced(2, 0));
    const int cell = r.getWidth() / 4;
    for (auto [slider, label] : { std::pair { &gain, &gainLabel }, { &mix, &mixLabel }, { &pan, &panLabel }, { &width, &widthLabel } })
    {
        auto c = r.removeFromLeft(cell);
        label->setBounds(c.removeFromLeft(40));
        slider->setBounds(c);
    }
}
} // namespace hpeq::ui
