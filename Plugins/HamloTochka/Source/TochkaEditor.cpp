#include "TochkaEditor.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace hamlo::tochka
{
namespace
{
using hpeq::ChannelMode;
using hpeq::dsp::BandDesign;
using View = hpeq::ui::AnalysisModel::View;

constexpr float width = 1180.0f, height = 820.0f, minHz = 20.0f, dbLimit = 24.0f;
const juce::Colour ink { 0xffffb347 }, muted { 0xff817e69 }, grid { 0xff29372e };
const juce::Colour inputInk { 0xff4c936a }, outputInk { 0xff82e7a5 };

hpeq::EqNodeState panelNode(const Processor::Band& band)
{
    hpeq::EqNodeState node;
    node.enabled = band.active;
    node.shape = band.shape;
    node.frequencyHz = band.frequencyHz;
    node.gainDb = band.gainDb;
    node.q = band.q;
    node.slopeDbPerOct = band.slopeDbPerOct;
    node.channel = band.channel;
    node.solo = band.solo;
    node.dynamic = band.dynamic;
    node.detect = band.detect;
    node.thresholdDb = band.thresholdDb;
    node.ratio = band.ratio;
    node.rangeDb = band.rangeDb;
    node.attackMs = band.attackMs;
    node.releaseMs = band.releaseMs;
    return node;
}

Processor::Band bandFromPanel(const hpeq::EqNodeState& node)
{
    Processor::Band band;
    band.active = node.enabled;
    band.shape = node.shape;
    band.frequencyHz = static_cast<float>(node.frequencyHz);
    band.gainDb = static_cast<float>(node.gainDb);
    band.q = static_cast<float>(node.q);
    band.slopeDbPerOct = node.slopeDbPerOct;
    band.channel = node.channel;
    band.solo = node.solo;
    band.dynamic = node.dynamic;
    band.detect = node.detect;
    band.thresholdDb = static_cast<float>(node.thresholdDb);
    band.ratio = static_cast<float>(node.ratio);
    band.rangeDb = static_cast<float>(node.rangeDb);
    band.attackMs = static_cast<float>(node.attackMs);
    band.releaseMs = static_cast<float>(node.releaseMs);
    return band;
}

struct DisplayBand
{
    BandDesign design;
    ChannelMode mode;
};

double responseDb(const std::vector<DisplayBand>& designs, View view, double rate, double hz, bool mono)
{
    std::complex<double> left = view == View::Right ? 0.0 : 1.0;
    std::complex<double> right = view == View::Left ? 0.0 : (view == View::Side ? -1.0 : 1.0);
    for (const auto& band : designs)
    {
        const auto h = hpeq::dsp::bandResponse(band.design, rate, hz);
        if (mono)
        {
            if (band.mode != ChannelMode::Right && band.mode != ChannelMode::Side)
                left *= h;
            continue;
        }

        switch (band.mode)
        {
            case ChannelMode::Left: left *= h; break;
            case ChannelMode::Right: right *= h; break;
            case ChannelMode::Mid:
            case ChannelMode::Side:
            {
                auto mid = 0.5 * (left + right);
                auto side = 0.5 * (left - right);
                (band.mode == ChannelMode::Mid ? mid : side) *= h;
                left = mid + side;
                right = mid - side;
                break;
            }
            default: left *= h; right *= h; break;
        }
    }
    const auto result = mono || view == View::Left ? left
        : view == View::Right ? right : 0.5 * (left + (view == View::Side ? -right : right));
    return juce::Decibels::gainToDecibels(std::abs(result), -120.0);
}
} // namespace

Editor::Editor(Processor& processor)
    : AudioProcessorEditor(processor), audio(processor), analysis(processor.audioCapture())
{
    setLookAndFeel(&lookAndFeel);
    const std::array<juce::Component*, 9> children {
        &panel, &presetScreen, &bandSelector, &spectrumView, &add, &compare, &learn, &clear, &matchStatus
    };
    for (auto* component : children)
        addAndMakeVisible(component);

    std::vector<hamlo::ui::PresetScreen::Entry> entries;
    for (const auto& preset : Processor::presets())
        entries.push_back({ static_cast<int>(entries.size()), "Factory", preset.group, preset.name, preset.description });
    presetScreen.setEntries(std::move(entries));
    presetScreen.onSelect = [this](int program)
    {
        endGesture();
        audio.setCurrentProgram(program);
        selectBand(0);
        timerCallback();
    };

    for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
        bandSelector.addItem("Band " + juce::String(slot + 1), slot + 1);
    bandSelector.setTooltip("Select a band to edit.");
    bandSelector.onChange = [this] { selectBand(bandSelector.getSelectedId() - 1); };

    spectrumView.addItemList({ "Mid", "Left", "Right", "Side" }, 1);
    spectrumView.setSelectedId(1, juce::dontSendNotification);
    spectrumView.setTooltip("Spectrum view and corresponding channel response.");
    spectrumView.onChange = [this]
    {
        analysis.view = static_cast<View>(spectrumView.getSelectedId() - 1);
        repaint(graph.getSmallestIntegerContainer());
    };
    add.setTooltip("Add a bell band. You can also double-click the graph.");
    add.onClick = [this] { addBand(1000.0f, 0.0f); };
    compare.setTooltip("Compare the dry input with the processed output.");
    compare.onClick = [this] { audio.setDry(!audio.isDry()); timerCallback(); };
    learn.setTooltip("Play a passage, then press again to hold a loudness correction.");
    learn.onClick = [this]
    {
        insufficientSignal = false;
        if (audio.isLearning())
            insufficientSignal = !audio.stopLearn();
        else
            audio.startLearn();
        timerCallback();
    };
    clear.setTooltip("Clear the held loudness correction.");
    clear.onClick = [this] { insufficientSignal = false; audio.clearMatch(); timerCallback(); };
    matchStatus.setJustificationType(juce::Justification::centredRight);
    matchStatus.setColour(juce::Label::textColourId, ink);
    matchStatus.setFont(hpeq::ui::steel::monoFont(15.0f));

    panel.onChange = [this](const hpeq::EqNodeState& node, const juce::String&)
    {
        audio.editBand(selected, bandFromPanel(node), panelGesture >= 0);
        repaint(graph.getSmallestIntegerContainer());
    };
    panel.onSolo = [this](bool solo)
    {
        auto band = audio.band(selected);
        band.solo = solo;
        audio.editBand(selected, band);
        syncPanel();
    };
    panel.onDelete = [this]
    {
        audio.deleteBand(selected);
        syncPanel();
    };
    panel.readDynamicGain = [this] { return audio.dynamicGain(selected); };
    panel.onGestureStart = [this](const juce::String& key)
    {
        const juce::StringArray names { "freq", "gain", "q", "thresh", "ratio", "range", "attack", "release" };
        const auto field = names.indexOf(key);
        if (field >= 0)
        {
            panelGesture = Processor::Frequency + field;
            audio.beginBandGesture(selected, panelGesture);
        }
    };
    panel.onGestureEnd = [this] { endGesture(); };

    setResizable(true, true);
    setResizeLimits(800, 556, 1600, 1112);
    getConstrainer()->setFixedAspectRatio(width / height);
    setSize(static_cast<int>(width), static_cast<int>(height));
    selectBand(0);
    audio.enableCapture(true);
    timerCallback();
    startTimerHz(24);
}

Editor::~Editor()
{
    stopTimer();
    endGesture();
    audio.enableCapture(false);
    setLookAndFeel(nullptr);
}

void Editor::endGesture()
{
    if (panelGesture >= 0)
    {
        audio.endBandGesture(selected, panelGesture);
        panelGesture = -1;
    }
    if (dragging)
    {
        audio.endBandGesture(selected, Processor::Frequency);
        audio.endBandGesture(selected, Processor::Gain);
        dragging = false;
    }
}

void Editor::syncPanel()
{
    panel.setValues({ panelNode(audio.band(selected)), selected + 1 });
}

void Editor::selectBand(int index)
{
    endGesture();
    selected = juce::jlimit(0, hpeq::kMaxEqNodesPerLayer - 1, index);
    bandSelector.setSelectedId(selected + 1, juce::dontSendNotification);
    syncPanel();
    repaint(graph.getSmallestIntegerContainer());
}

void Editor::timerCallback()
{
    analysis.update();
    if (!dragging && panelGesture < 0)
        syncPanel();
    const auto dry = audio.isDry();
    compare.setButtonText(dry ? "A: DRY" : "B: EQ");
    compare.setToggleState(dry, juce::dontSendNotification);
    const auto learning = audio.isLearning();
    learn.setButtonText(learning ? "APPLY MATCH" : "LEARN MATCH");
    learn.setToggleState(learning, juce::dontSendNotification);
    matchStatus.setText(learning ? "MEASURING..." : insufficientSignal ? "PLAY MORE AUDIO"
        : "MATCH " + juce::String(audio.matchDb(), 1) + " dB", juce::dontSendNotification);
    presetScreen.setCurrent(audio.getCurrentProgram(), audio.presetEdited());
    repaint(graph.getSmallestIntegerContainer());
}

juce::Rectangle<int> Editor::at(float x, float y, float w, float h) const
{
    return juce::Rectangle<float>(x, y, w, h)
        .transformedBy(juce::AffineTransform::scale(static_cast<float>(getWidth()) / width,
                                                    static_cast<float>(getHeight()) / height)).toNearestInt();
}

void Editor::resized()
{
    presetScreen.placeOnStage(getLocalBounds().toFloat());
    compare.setBounds(at(898, 28, 106, 34));
    clear.setBounds(at(1012, 28, 112, 34));
    learn.setBounds(at(898, 70, 226, 36));
    matchStatus.setBounds(at(898, 108, 226, 22));
    graph = at(69, 152, 1042, 276).toFloat();
    bandSelector.setBounds(at(56, 457, 136, 30));
    add.setBounds(at(202, 457, 120, 30));
    spectrumView.setBounds(at(1008, 457, 116, 30));
    panel.setBounds(at(40, 495, 1100, 318));
}

void Editor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff212321));
    auto header = at(420, 24, 420, 77).toFloat();
    hpeq::ui::steel::engraved(g, "HAMLOTOCHKA", header, header.getHeight() * 0.43f);
    hpeq::ui::steel::engraved(g, "DYNAMIC EQUALIZER", at(435, 99, 390, 26).toFloat(), 12.0f, muted);
    g.setColour(juce::Colour(0xff131815));
    g.fillRoundedRectangle(at(56, 140, 1068, 309).toFloat(), 8.0f);
    drawGraph(g);
    g.setFont(hpeq::ui::steel::monoFont(12.0f));
    g.setColour(inputInk);
    g.drawText("INPUT", at(676, 459, 74, 28), juce::Justification::centred);
    g.setColour(outputInk);
    g.drawText("OUTPUT", at(752, 459, 74, 28), juce::Justification::centred);
    g.setColour(ink);
    g.drawText("EQ CURVE", at(831, 459, 100, 28), juce::Justification::centred);
}

float Editor::xFor(float hz) const noexcept
{
    const float maxHz = static_cast<float>(juce::jmin(22000.0, audio.sampleRateHz() * 0.49));
    return graph.getX() + graph.getWidth() * std::log(juce::jlimit(minHz, maxHz, hz) / minHz) / std::log(maxHz / minHz);
}

float Editor::hzAt(float x) const noexcept
{
    const float maxHz = static_cast<float>(juce::jmin(22000.0, audio.sampleRateHz() * 0.49));
    const float normal = juce::jlimit(0.0f, 1.0f, (x - graph.getX()) / graph.getWidth());
    return minHz * std::pow(maxHz / minHz, normal);
}

float Editor::yFor(float db) const noexcept
{
    return juce::jmap(juce::jlimit(-dbLimit, dbLimit, db), -dbLimit, dbLimit, graph.getBottom(), graph.getY());
}

float Editor::dbAt(float y) const noexcept
{
    return juce::jmap(juce::jlimit(graph.getY(), graph.getBottom(), y), graph.getBottom(), graph.getY(), -dbLimit, dbLimit);
}

void Editor::drawGraph(juce::Graphics& g)
{
    g.setFont(hpeq::ui::steel::monoFont(10.0f));
    for (const auto hz : { 30.0f, 60.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        if (hz > hzAt(graph.getRight()))
            break;
        const auto x = xFor(hz);
        g.setColour(grid);
        g.drawVerticalLine(juce::roundToInt(x), graph.getY(), graph.getBottom());
        g.setColour(muted);
        const auto label = hz >= 1000.0f ? juce::String(hz * 0.001f, 0) + "k" : juce::String(hz, 0);
        g.drawText(label, juce::Rectangle<float>(x - 20.0f, graph.getBottom() + 1.0f, 40.0f, 13.0f), juce::Justification::centred);
    }
    for (const auto db : { -24.0f, -12.0f, 0.0f, 12.0f, 24.0f })
    {
        const auto y = yFor(db);
        g.setColour(db == 0.0f ? grid.brighter(0.6f) : grid);
        g.drawHorizontalLine(juce::roundToInt(y), graph.getX(), graph.getRight());
        g.setColour(muted);
        g.drawText(juce::String(db, 0), juce::Rectangle<float>(graph.getRight() - 27.0f, y - 6.0f, 24.0f, 12.0f), juce::Justification::centredRight);
    }
    g.setColour(inputInk);
    for (const auto level : { -96.0f, -72.0f, -48.0f, -24.0f, 0.0f })
    {
        const auto y = juce::jmap(level, -96.0f, 0.0f, graph.getBottom(), graph.getY());
        const auto label = level == 0.0f ? juce::String("0 dBFS") : juce::String(level, 0);
        g.drawText(label, juce::Rectangle<float>(graph.getX() + 3.0f, y - 6.0f, 56.0f, 12.0f), juce::Justification::centredLeft);
    }

    const auto rate = audio.sampleRateHz();
    const auto drawSpectrum = [&](const std::vector<float>& bins, juce::Colour colour)
    {
        if (bins.size() < 2)
            return;
        juce::Path path;
        constexpr int count = 320;
        for (int i = 0; i <= count; ++i)
        {
            const float x = graph.getX() + graph.getWidth() * static_cast<float>(i) / static_cast<float>(count);
            const float bin = hzAt(x) * static_cast<float>(analysis.fftSize()) / static_cast<float>(rate);
            const auto low = juce::jlimit(0, static_cast<int>(bins.size()) - 2, static_cast<int>(bin));
            const auto level = juce::jmap(bin - static_cast<float>(low), bins[static_cast<size_t>(low)], bins[static_cast<size_t>(low + 1)]);
            const auto y = juce::jmap(juce::jlimit(-96.0f, 0.0f, level), -96.0f, 0.0f, graph.getBottom(), graph.getY());
            if (i == 0) path.startNewSubPath(x, y);
            else path.lineTo(x, y);
        }
        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(1.15f));
    };
    drawSpectrum(analysis.inputSpectrum(), inputInk);
    drawSpectrum(analysis.outputSpectrum(), outputInk);

    std::vector<DisplayBand> designs;
    designs.reserve(hpeq::kMaxEqNodesPerLayer);
    for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
    {
        const auto band = audio.band(slot);
        if (band.active)
            designs.push_back({ hpeq::dsp::designBand(band.shape, band.slopeDbPerOct, rate, band.frequencyHz,
                                    band.gainDb + audio.dynamicGain(slot), band.q), band.channel });
    }

    juce::Path curve;
    constexpr int samples = 240;
    for (int i = 0; i <= samples; ++i)
    {
        const float x = graph.getX() + graph.getWidth() * static_cast<float>(i) / static_cast<float>(samples);
        const auto value = responseDb(designs, analysis.view, rate, hzAt(x), audio.getTotalNumInputChannels() == 1);
        const auto y = yFor(static_cast<float>(value));
        if (i == 0) curve.startNewSubPath(x, y);
        else curve.lineTo(x, y);
    }
    g.setColour(ink);
    g.strokePath(curve, juce::PathStrokeType(1.7f));
    for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
    {
        const auto band = audio.band(slot);
        if (!audio.hasBand(slot))
            continue;
        const auto x = xFor(band.frequencyHz), y = yFor(hpeq::dsp::isGainShape(band.shape) ? band.gainDb : 0.0f);
        g.setColour(band.active ? (slot == selected ? ink : muted.brighter(0.5f)) : muted.darker(0.45f));
        g.fillEllipse(x - 8.0f, y - 8.0f, 16.0f, 16.0f);
        g.setColour(juce::Colours::black);
        g.drawText(juce::String(slot + 1), juce::Rectangle<float>(x - 10.0f, y - 8.0f, 20.0f, 16.0f), juce::Justification::centred);
    }
}

int Editor::findPoint(juce::Point<float> point) const
{
    int best = -1;
    float distance = 13.0f;
    for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
    {
        const auto band = audio.band(slot);
        if (!audio.hasBand(slot))
            continue;
        const auto p = juce::Point<float>(xFor(band.frequencyHz), yFor(hpeq::dsp::isGainShape(band.shape) ? band.gainDb : 0.0f));
        if (const auto d = point.getDistanceFrom(p); d < distance)
        {
            best = slot;
            distance = d;
        }
    }
    return best;
}

void Editor::addBand(float hz, float gain)
{
    for (int slot = 0; slot < hpeq::kMaxEqNodesPerLayer; ++slot)
        if (!audio.hasBand(slot))
        {
            Processor::Band band;
            band.active = true;
            band.frequencyHz = hz;
            band.gainDb = gain;
            audio.editBand(slot, band);
            selectBand(slot);
            return;
        }
}

void Editor::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown() && graph.contains(e.position))
        if (const auto slot = findPoint(e.position); slot >= 0)
        {
            selectBand(slot);
            dragging = true;
            audio.beginBandGesture(slot, Processor::Frequency);
            audio.beginBandGesture(slot, Processor::Gain);
        }
}

void Editor::mouseDrag(const juce::MouseEvent& e)
{
    if (!dragging)
        return;
    auto band = audio.band(selected);
    band.frequencyHz = hzAt(e.position.x);
    if (hpeq::dsp::isGainShape(band.shape))
        band.gainDb = dbAt(e.position.y);
    audio.editBand(selected, band, true);
    syncPanel();
    repaint(graph.getSmallestIntegerContainer());
}

void Editor::mouseUp(const juce::MouseEvent&) { endGesture(); }

void Editor::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (graph.contains(e.position) && findPoint(e.position) < 0)
        addBand(hzAt(e.position.x), dbAt(e.position.y));
}
} // namespace hamlo::tochka
