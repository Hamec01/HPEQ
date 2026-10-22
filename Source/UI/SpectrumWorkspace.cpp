#include "UI/SpectrumWorkspace.h"

#include "Analysis/ResponseCurve.h"

namespace hpeq::ui
{
namespace
{
constexpr double kEqRangeDb = 24.0;
constexpr double kAnalyserFloorDb = -96.0;
constexpr float kPointRadius = 7.0f;
constexpr float kEdgeGrab = 6.0f;

EqNodeState* findEq(FxProjectState& p, std::uint32_t id)
{
    for (auto& n : selectedLayer(p).eqNodes)
        if (n.id == id)
            return &n;
    return nullptr;
}

FxNodeState* findFx(FxProjectState& p, std::uint32_t id)
{
    for (auto& n : selectedLayer(p).fxNodes)
        if (n.id == id)
            return &n;
    return nullptr;
}

const std::array<EqShape, 9> kShapes { EqShape::Bell, EqShape::LowShelf, EqShape::HighShelf, EqShape::LowCut, EqShape::HighCut,
                                       EqShape::Notch, EqShape::BandPass, EqShape::Tilt, EqShape::AllPass };
const std::array<FxType, 10> kFxTypes { FxType::Compressor, FxType::Reverb, FxType::Gain, FxType::Limiter, FxType::Delay,
                                        FxType::TransientShaper, FxType::Saturator, FxType::MonstaFx, FxType::StereoProcessor,
                                        FxType::GateExpander };
} // namespace

SpectrumWorkspace::SpectrumWorkspace(FxPluginProcessor& p, const AnalysisModel& a, Selection& s)
    : processor(p), analysis(a), selection(s)
{
}

double SpectrumWorkspace::sampleRate() const { return processor.getCurrentSampleRate(); }

juce::Rectangle<float> SpectrumWorkspace::plotArea() const
{
    return getLocalBounds().toFloat().reduced(36.0f, 10.0f).withTrimmedBottom(14.0f);
}

float SpectrumWorkspace::yForEqDb(double db, juce::Rectangle<float> plot)
{
    return plot.getCentreY() - static_cast<float>(juce::jlimit(-kEqRangeDb, kEqRangeDb, db) / kEqRangeDb) * plot.getHeight() * 0.5f;
}

double SpectrumWorkspace::eqDbForY(float y, juce::Rectangle<float> plot)
{
    return juce::jlimit(-kEqRangeDb, kEqRangeDb, static_cast<double>(plot.getCentreY() - y) / (plot.getHeight() * 0.5) * kEqRangeDb);
}

void SpectrumWorkspace::paint(juce::Graphics& g)
{
    const auto project = processor.getProject();
    const auto& layer = selectedLayer(project);
    const double fs = sampleRate();
    auto area = getLocalBounds().toFloat();
    const auto plot = plotArea();

    g.setColour(colours::panel);
    g.fillRoundedRectangle(area, 6.0f);

    // Grid and axes.
    g.setFont(10.0f);
    for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
    {
        const float x = FrequencyAxis::xFor(f, plot);
        g.setColour(colours::grid);
        g.drawVerticalLine(juce::roundToInt(x), plot.getY(), plot.getBottom());
        g.setColour(colours::dim);
        g.drawText(f >= 1000.0 ? juce::String(f / 1000.0) + "k" : juce::String(f), juce::Rectangle<float>(x - 20, plot.getBottom() + 2, 40, 12),
                   juce::Justification::centred);
    }
    for (double db = -kEqRangeDb; db <= kEqRangeDb; db += 6.0)
    {
        const float y = yForEqDb(db, plot);
        g.setColour(db == 0.0 ? colours::outline : colours::grid);
        g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
        g.setColour(colours::dim);
        g.drawText((db > 0 ? "+" : "") + juce::String(db, 0), juce::Rectangle<float>(area.getX() + 2, y - 6, 30, 12), juce::Justification::centredRight);
        const double analyserDb = (db + kEqRangeDb) / (2.0 * kEqRangeDb) * (-kAnalyserFloorDb) + kAnalyserFloorDb;
        if (mode == Mode::Spectrum)
            g.drawText(juce::String(analyserDb, 0), juce::Rectangle<float>(plot.getRight() + 2, y - 6, 32, 12), juce::Justification::centredLeft);
    }

    // FX regions (behind the curves).
    for (int i = 0; i < static_cast<int>(layer.fxNodes.size()); ++i)
    {
        const auto& n = layer.fxNodes[static_cast<size_t>(i)];
        const auto colour = colours::forNode(i);
        const float x0 = FrequencyAxis::xFor(n.lowHz, plot), x1 = FrequencyAxis::xFor(n.highHz, plot);
        const bool selected = selection.isFx(n.id);
        const float alpha = n.enabled ? (selected ? 0.22f : 0.13f) : 0.05f;
        g.setColour(colour.withAlpha(alpha));
        g.fillRect(juce::Rectangle<float>(x0, plot.getY(), x1 - x0, plot.getHeight()));
        g.setColour(colour.withAlpha(n.enabled ? 0.9f : 0.35f));
        g.drawVerticalLine(juce::roundToInt(x0), plot.getY(), plot.getBottom());
        g.drawVerticalLine(juce::roundToInt(x1), plot.getY(), plot.getBottom());
        const float handleY = plot.getY() + 30.0f + 18.0f * static_cast<float>(i % 3);
        g.fillRect(x0 - 3.0f, handleY - 3.0f, 6.0f, 6.0f);
        g.fillRect(x1 - 3.0f, handleY - 3.0f, 6.0f, 6.0f);
        g.drawLine(x0, handleY, x1, handleY, selected ? 2.0f : 1.0f);
        g.setFont(juce::FontOptions(11.5f, selected ? juce::Font::bold : juce::Font::plain));
        g.drawText(juce::String(i + 1) + "  " + nodeTitle(n) + (n.enabled ? "" : " (off)"),
                   juce::Rectangle<float>(x0 + 4, plot.getY() + 2 + 18.0f * static_cast<float>(i % 3), 260, 14), juce::Justification::centredLeft);
        g.setFont(10.0f);
        g.drawText(formatHz(n.lowHz) + " - " + formatHz(n.highHz), juce::Rectangle<float>(x0 + 4, plot.getY() + 14 + 18.0f * static_cast<float>(i % 3), 200, 12),
                   juce::Justification::centredLeft);
    }

    // Module bands of EQ points (D6): where the point's modules act, from its frequency and Q.
    for (const auto& n : layer.eqNodes)
    {
        if (n.chain.empty())
            continue;
        const auto [lowHz, highHz] = pointModuleBandHz(n);
        const float x0 = FrequencyAxis::xFor(lowHz, plot), x1 = FrequencyAxis::xFor(highHz, plot);
        const bool selected = selection.isEq(n.id);
        g.setColour(colours::eqNode.withAlpha(n.enabled ? (selected ? 0.16f : 0.08f) : 0.03f));
        g.fillRect(juce::Rectangle<float>(x0, plot.getY(), x1 - x0, plot.getHeight()));
        g.setColour(colours::eqNode.withAlpha(n.enabled ? 0.55f : 0.25f));
        const float dash[] { 4.0f, 4.0f };
        g.drawDashedLine(juce::Line<float>(x0, plot.getY(), x0, plot.getBottom()), dash, 2);
        g.drawDashedLine(juce::Line<float>(x1, plot.getY(), x1, plot.getBottom()), dash, 2);
        juce::StringArray names;
        for (const auto& m : n.chain)
            names.add(fxName(m.type));
        g.setFont(juce::FontOptions(10.5f, selected ? juce::Font::bold : juce::Font::plain));
        g.drawText(names.joinIntoString(" > "), juce::Rectangle<float>(x0 + 4, plot.getBottom() - 16, juce::jmax(120.0f, x1 - x0), 14),
                   juce::Justification::centredLeft);
    }

    // Analyser.
    const auto& inDb = analysis.inputSpectrum();
    const auto& outDb = analysis.outputSpectrum();
    const auto analyserY = [&](float db)
    {
        const double norm = (juce::jlimit(kAnalyserFloorDb, 0.0, static_cast<double>(db)) - kAnalyserFloorDb) / -kAnalyserFloorDb;
        return plot.getBottom() - static_cast<float>(norm) * plot.getHeight();
    };
    const auto spectrumPath = [&](const std::vector<float>& db, bool difference)
    {
        juce::Path path;
        bool started = false;
        const int columns = static_cast<int>(plot.getWidth());
        for (int c = 0; c <= columns; c += 2)
        {
            const double f0 = FrequencyAxis::hzFor(plot.getX() + static_cast<float>(c), plot);
            const double f1 = FrequencyAxis::hzFor(plot.getX() + static_cast<float>(c + 2), plot);
            const double bin0 = f0 * analysis.fftSize() / fs, bin1 = f1 * analysis.fftSize() / fs;
            float v = difference ? 0.0f : -1000.0f;
            int count = 0;
            if (bin1 - bin0 < 1.0)
            {
                // Column narrower than a bin (low frequencies): interpolate between the two nearest
                // bins instead of drawing a staircase.
                const double centre = 0.5 * (bin0 + bin1);
                const auto k = static_cast<size_t>(centre);
                if (k + 1 < db.size() && !std::isnan(db[k]) && !std::isnan(db[k + 1]))
                {
                    const auto frac = static_cast<float>(centre - static_cast<double>(k));
                    v = db[k] + frac * (db[k + 1] - db[k]);
                    count = 1;
                }
            }
            else
                for (int b = static_cast<int>(bin0); b < static_cast<int>(bin1) && b < static_cast<int>(db.size()); ++b)
                {
                    if (std::isnan(db[static_cast<size_t>(b)]))
                        continue;
                    v = difference ? v + db[static_cast<size_t>(b)] : juce::jmax(v, db[static_cast<size_t>(b)]);
                    ++count;
                }
            if (count == 0)
            {
                started = false; // masked: leave a gap instead of drawing invented values
                continue;
            }
            if (difference)
                v /= static_cast<float>(count);
            const float x = plot.getX() + static_cast<float>(c);
            const float y = difference ? yForEqDb(v, plot) : analyserY(v);
            if (!started)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
            started = true;
        }
        return path;
    };
    if (mode == Mode::Spectrum && !inDb.empty())
    {
        auto inPath = spectrumPath(inDb, false);
        g.setColour(colours::input.withAlpha(0.55f));
        g.strokePath(inPath, juce::PathStrokeType(1.0f));
        g.setColour(colours::accent.withAlpha(0.8f));
        g.strokePath(spectrumPath(outDb, false), juce::PathStrokeType(1.2f));
    }
    else if (mode == Mode::Difference && !analysis.differenceSpectrum().empty())
    {
        g.setColour(colours::warning.withAlpha(0.9f));
        g.strokePath(spectrumPath(analysis.differenceSpectrum(), true), juce::PathStrokeType(1.4f));
    }

    // Selected EQ band's own response, then the total PRE-EQ curve (exact coefficients).
    const int columns = static_cast<int>(plot.getWidth());
    for (const auto& n : layer.eqNodes)
        if (selection.isEq(n.id))
        {
            juce::Path band;
            band.startNewSubPath(plot.getX(), plot.getCentreY());
            for (int c = 0; c <= columns; c += 2)
                band.lineTo(plot.getX() + static_cast<float>(c), yForEqDb(bandResponseDb(n, fs, FrequencyAxis::hzFor(plot.getX() + static_cast<float>(c), plot)), plot));
            band.lineTo(plot.getRight(), plot.getCentreY());
            band.closeSubPath();
            g.setColour(colours::eqNode.withAlpha(0.18f));
            g.fillPath(band);
        }
    juce::Path curve;
    for (int c = 0; c <= columns; c += 2)
    {
        const float x = plot.getX() + static_cast<float>(c);
        const float y = yForEqDb(eqResponseDb(layer, fs, FrequencyAxis::hzFor(x, plot)), plot);
        if (c == 0)
            curve.startNewSubPath(x, y);
        else
            curve.lineTo(x, y);
    }
    g.setColour(colours::eqCurve.withAlpha(0.9f));
    g.strokePath(curve, juce::PathStrokeType(1.8f));

    // EQ points.
    for (int i = 0; i < static_cast<int>(layer.eqNodes.size()); ++i)
    {
        const auto& n = layer.eqNodes[static_cast<size_t>(i)];
        const float x = FrequencyAxis::xFor(n.frequencyHz, plot);
        const float y = isGainShapeUi(n.shape) ? yForEqDb(n.gainDb, plot) : yForEqDb(eqResponseDb(layer, fs, n.frequencyHz), plot);
        const bool selected = selection.isEq(n.id);
        g.setColour(colours::panel);
        g.fillEllipse(x - kPointRadius, y - kPointRadius, 2 * kPointRadius, 2 * kPointRadius);
        g.setColour(n.enabled ? colours::eqNode : colours::dim);
        if (!n.chain.empty())
            g.fillEllipse(x - kPointRadius, y - kPointRadius, 2 * kPointRadius, 2 * kPointRadius);
        g.drawEllipse(x - kPointRadius, y - kPointRadius, 2 * kPointRadius, 2 * kPointRadius, selected ? 2.5f : 1.5f);
        g.setColour(!n.chain.empty() ? colours::panel : (n.enabled ? colours::eqNode : colours::dim));
        g.setFont(10.0f);
        g.drawText(juce::String(i + 1), juce::Rectangle<float>(x - kPointRadius, y - kPointRadius, 2 * kPointRadius, 2 * kPointRadius), juce::Justification::centred);
        if (selected)
        {
            g.setFont(11.0f);
            juce::String label = shapeName(n.shape) + "  " + formatHz(n.frequencyHz);
            if (isGainShapeUi(n.shape))
                label << "  " << juce::String(n.gainDb, 1) << " dB";
            if (n.shape == EqShape::LowCut || n.shape == EqShape::HighCut)
                label << "  " << n.slopeDbPerOct << " dB/oct";
            g.drawText(label, juce::Rectangle<float>(x + 10, y - 22, 240, 14), juce::Justification::centredLeft);
        }
    }

    g.setColour(colours::dim);
    g.setFont(10.0f);
    const char* viewNames[] { "Mid", "Left", "Right", "Side" };
    g.drawText("FFT " + juce::String(analysis.fftSize()) + ", Hann, view " + viewNames[static_cast<int>(analysis.view)] + ", "
                   + juce::String(fs / 1000.0, 1) + " kHz" + (analysis.frozen ? ", frozen" : ""),
               plot.withTrimmedTop(plot.getHeight() - 14.0f).withTrimmedLeft(plot.getWidth() - 320.0f), juce::Justification::centredRight);
    if (layer.eqNodes.empty() && layer.fxNodes.empty())
        g.drawText("Double-click: add an EQ point.  Right-click: add an EQ shape or an FX region.", plot, juce::Justification::centred);
}

SpectrumWorkspace::Hit SpectrumWorkspace::hitTestNodes(juce::Point<float> p) const
{
    const auto project = processor.getProject();
    const auto& layer = selectedLayer(project);
    const auto plot = plotArea();
    const double fs = sampleRate();
    for (const auto& n : layer.eqNodes)
    {
        const float x = FrequencyAxis::xFor(n.frequencyHz, plot);
        const float y = isGainShapeUi(n.shape) ? yForEqDb(n.gainDb, plot) : yForEqDb(eqResponseDb(layer, fs, n.frequencyHz), plot);
        if (p.getDistanceFrom({ x, y }) <= kPointRadius + 3.0f)
            return { Drag::EqPoint, n.id };
    }
    for (auto it = layer.fxNodes.rbegin(); it != layer.fxNodes.rend(); ++it)
    {
        const float x0 = FrequencyAxis::xFor(it->lowHz, plot), x1 = FrequencyAxis::xFor(it->highHz, plot);
        if (std::abs(p.x - x0) <= kEdgeGrab)
            return { Drag::FxLow, it->id };
        if (std::abs(p.x - x1) <= kEdgeGrab)
            return { Drag::FxHigh, it->id };
    }
    for (auto it = layer.fxNodes.rbegin(); it != layer.fxNodes.rend(); ++it)
    {
        const float x0 = FrequencyAxis::xFor(it->lowHz, plot), x1 = FrequencyAxis::xFor(it->highHz, plot);
        if (p.x > x0 && p.x < x1 && plot.contains(p))
            return { Drag::FxBody, it->id };
    }
    return {};
}

void SpectrumWorkspace::select(Selection::Kind kind, std::uint32_t id)
{
    selection.kind = kind;
    selection.id = id;
    selection.slot = -1;
    if (onSelectionChanged)
        onSelectionChanged();
    repaint();
}

void SpectrumWorkspace::mouseMove(const juce::MouseEvent& e)
{
    const auto hit = hitTestNodes(e.position);
    setMouseCursor(hit.drag == Drag::FxLow || hit.drag == Drag::FxHigh ? juce::MouseCursor::LeftRightResizeCursor
                   : hit.drag != Drag::None                             ? juce::MouseCursor::DraggingHandCursor
                                                                         : juce::MouseCursor::NormalCursor);
}

void SpectrumWorkspace::mouseDown(const juce::MouseEvent& e)
{
    dragging = hitTestNodes(e.position);
    if (e.mods.isPopupMenu())
    {
        showMenu(e.position, dragging);
        dragging = {};
        return;
    }
    if (dragging.drag == Drag::None)
    {
        select(Selection::Kind::None, 0);
        return;
    }
    select(dragging.drag == Drag::EqPoint ? Selection::Kind::Eq : Selection::Kind::Fx, dragging.id);
    processor.getUndoManager().beginNewTransaction(dragging.drag == Drag::EqPoint ? "Move EQ point" : "Move FX region");
    auto project = processor.getProject();
    if (auto* n = findEq(project, dragging.id))
    {
        dragStartHz = n->frequencyHz;
        dragStartGain = n->gainDb;
    }
    if (auto* n = findFx(project, dragging.id))
    {
        dragStartLow = n->lowHz;
        dragStartHigh = n->highHz;
    }
    dragStartX = e.position.x;
    dragStartY = e.position.y;
}

void SpectrumWorkspace::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging.drag == Drag::None)
        return;
    const auto plot = plotArea();
    const double scale = e.mods.isShiftDown() ? 0.1 : 1.0; // Shift: fine
    const double octavesPerPixel = std::log2(FrequencyAxis::kMax / FrequencyAxis::kMin) / plot.getWidth();
    const double dOct = (e.position.x - dragStartX) * octavesPerPixel * scale;
    const double dDb = (eqDbForY(e.position.y, plot) - eqDbForY(dragStartY, plot)) * scale;
    const auto id = dragging.id;
    const auto drag = dragging.drag;

    processor.editProject("Drag", [&](FxProjectState& p)
    {
        if (auto* n = findEq(p, id))
        {
            n->frequencyHz = juce::jlimit(10.0, 22000.0, dragStartHz * std::exp2(dOct));
            if (isGainShapeUi(n->shape))
                n->gainDb = juce::jlimit(-24.0, 24.0, dragStartGain + dDb);
        }
        if (auto* n = findFx(p, id))
        {
            if (drag == Drag::FxLow)
                n->lowHz = juce::jlimit(10.0, n->highHz / 1.05, dragStartLow * std::exp2(dOct));
            else if (drag == Drag::FxHigh)
                n->highHz = juce::jlimit(n->lowHz * 1.05, 22000.0, dragStartHigh * std::exp2(dOct));
            else
            {
                const double k = std::exp2(dOct);
                const double lo = juce::jlimit(10.0, 22000.0 / (dragStartHigh / dragStartLow), dragStartLow * k);
                n->lowHz = lo;
                n->highHz = lo * (dragStartHigh / dragStartLow);
            }
        }
    });
    repaint();
}

void SpectrumWorkspace::mouseUp(const juce::MouseEvent&)
{
    dragging = {};
}

void SpectrumWorkspace::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (hitTestNodes(e.position).drag != Drag::None || !plotArea().contains(e.position))
        return;
    const auto plot = plotArea();
    std::uint32_t newId = 0;
    processor.getUndoManager().beginNewTransaction("Add EQ point");
    processor.editProject("Add EQ point", [&](FxProjectState& p)
    {
        if (selectedLayer(p).eqNodes.size() >= static_cast<size_t>(kMaxEqNodesPerLayer))
            return;
        EqNodeState n;
        n.id = newId = p.nextNodeId++;
        n.shape = EqShape::Bell;
        n.frequencyHz = FrequencyAxis::hzFor(e.position.x, plot);
        n.gainDb = std::round(eqDbForY(e.position.y, plot) * 10.0) / 10.0;
        n.q = 1.0;
        selectedLayer(p).eqNodes.push_back(n);
    });
    if (newId != 0)
        select(Selection::Kind::Eq, newId);
}

void SpectrumWorkspace::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto hit = hitTestNodes(e.position);
    if (hit.drag != Drag::EqPoint)
        return;
    processor.getUndoManager().beginNewTransaction("Q");
    processor.editProject("Q", [&](FxProjectState& p)
    {
        if (auto* n = findEq(p, hit.id))
        {
            if (n->shape == EqShape::LowCut || n->shape == EqShape::HighCut)
            {
                const int slopes[] { 6, 12, 18, 24, 36, 48, 72, 96 };
                int index = 0;
                while (index < 7 && slopes[index] < n->slopeDbPerOct)
                    ++index;
                index = juce::jlimit(0, 7, index + (wheel.deltaY > 0 ? 1 : -1));
                n->slopeDbPerOct = slopes[index];
            }
            else
                n->q = juce::jlimit(0.1, 18.0, n->q * std::exp2(wheel.deltaY * (e.mods.isShiftDown() ? 0.25 : 1.5)));
        }
    });
    repaint();
}

void SpectrumWorkspace::showMenu(juce::Point<float> position, Hit hit)
{
    const auto plot = plotArea();
    const double hz = FrequencyAxis::hzFor(position.x, plot);
    juce::PopupMenu menu;
    juce::PopupMenu eqMenu, fxMenu;
    for (size_t i = 0; i < kShapes.size(); ++i)
        eqMenu.addItem(static_cast<int>(100 + i), shapeName(kShapes[i]));
    for (size_t i = 0; i < kFxTypes.size(); ++i)
        fxMenu.addItem(static_cast<int>(200 + i), fxName(kFxTypes[i]));
    const auto project = processor.getProject();
    const auto& shown = selectedLayer(project);
    const bool bandFree = freeEngineSlots(shown) > 0;
    bool pointHasModules = false, pointFull = false;
    for (const auto& n : shown.eqNodes)
        if (n.id == hit.id)
        {
            pointHasModules = !n.chain.empty();
            pointFull = n.chain.size() >= static_cast<size_t>(kMaxFxPerNode);
        }
    juce::PopupMenu moduleMenu;
    for (size_t i = 0; i < kFxTypes.size(); ++i)
        moduleMenu.addItem(static_cast<int>(300 + i), fxName(kFxTypes[i]));

    if (hit.drag == Drag::EqPoint)
    {
        menu.addSectionHeader("EQ point");
        menu.addSubMenu("Change shape", eqMenu);
        menu.addSubMenu("Add module on this point", moduleMenu, !pointFull && (pointHasModules || bandFree));
        menu.addItem(1, "Enable / disable");
        menu.addItem(2, "Delete");
    }
    else if (hit.drag != Drag::None)
    {
        menu.addSectionHeader("FX region");
        menu.addItem(1, "Enable / disable");
        menu.addItem(2, "Delete");
    }
    else
    {
        menu.addSectionHeader("At " + formatHz(hz));
        menu.addSubMenu("Add EQ point", eqMenu);
        menu.addSubMenu("Add FX region (one octave)", fxMenu, bandFree);
    }

    juce::Component::SafePointer<SpectrumWorkspace> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(
                           juce::Rectangle<int>(1, 1).withPosition(localPointToGlobal(position.toInt()))),
                       [this, safe, hit, hz](int result)
    {
        if (result == 0 || safe == nullptr)
            return;
        std::uint32_t newId = 0;
        Selection::Kind newKind = Selection::Kind::None;
        processor.getUndoManager().beginNewTransaction("Edit");
        processor.editProject("Edit", [&](FxProjectState& p)
        {
            auto& layer = selectedLayer(p);
            if (hit.drag == Drag::EqPoint)
            {
                auto it = std::find_if(layer.eqNodes.begin(), layer.eqNodes.end(), [&](const EqNodeState& n) { return n.id == hit.id; });
                if (it == layer.eqNodes.end())
                    return;
                if (result == 1) it->enabled = !it->enabled;
                else if (result == 2) layer.eqNodes.erase(it);
                else if (result >= 100 && result < 200)
                {
                    it->shape = kShapes[static_cast<size_t>(result - 100)];
                    if (!isGainShapeUi(it->shape))
                        it->gainDb = 0.0;
                }
                else if (result >= 300 && it->chain.size() < static_cast<size_t>(kMaxFxPerNode))
                {
                    FxSlotState m;
                    m.type = kFxTypes[static_cast<size_t>(result - 300)];
                    it->chain.push_back(m);
                    newId = it->id;
                    newKind = Selection::Kind::Eq;
                }
            }
            else if (hit.drag != Drag::None)
            {
                auto it = std::find_if(layer.fxNodes.begin(), layer.fxNodes.end(), [&](const FxNodeState& n) { return n.id == hit.id; });
                if (it == layer.fxNodes.end())
                    return;
                if (result == 1) it->enabled = !it->enabled;
                else if (result == 2) layer.fxNodes.erase(it);
            }
            else if (result >= 100 && result < 200 && layer.eqNodes.size() < static_cast<size_t>(kMaxEqNodesPerLayer))
            {
                EqNodeState n;
                n.id = newId = p.nextNodeId++;
                n.shape = kShapes[static_cast<size_t>(result - 100)];
                n.frequencyHz = hz;
                n.gainDb = isGainShapeUi(n.shape) ? 3.0 : 0.0;
                n.slopeDbPerOct = 24;
                layer.eqNodes.push_back(n);
                newKind = Selection::Kind::Eq;
            }
            else if (result >= 200 && result < 300 && freeEngineSlots(layer) > 0)
            {
                FxNodeState n;
                n.id = newId = p.nextNodeId++;
                n.lowHz = hz / std::sqrt(2.0);
                n.highHz = hz * std::sqrt(2.0);
                n.chain.front().type = kFxTypes[static_cast<size_t>(result - 200)];
                layer.fxNodes.push_back(n);
                newKind = Selection::Kind::Fx;
            }
        });
        if (hit.drag != Drag::None && (result == 2))
            select(Selection::Kind::None, 0);
        else if (newId != 0)
            select(newKind, newId);
        repaint();
    });
}
} // namespace hpeq::ui
