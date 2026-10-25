#pragma once

#include <functional>

#include "Plugin/FxPluginProcessor.h"
#include "UI/AnalysisModel.h"
#include "UI/Theme.h"

namespace hpeq::ui
{
// The central frequency editor (log 20 Hz - 20 kHz).
//  - Analyser: measured input (grey) / output (cyan) spectra, or Difference (output - input where
//    the input is above the mask). Scale: 0 ... -96 dBFS (right axis).
//  - PRE-EQ curve: exact response of the used coefficients (ResponseCurve), scale +-24 dB (left axis).
//  - FX nodes: translucent regions between their real edges, labelled with their chain.
// Gestures: double-click = add a Bell; right-click = add/convert/delete menu; drag an EQ point =
// frequency/gain; wheel on an EQ point = Q (cuts: slope); drag a region edge = low/high; drag a
// region = move it; Shift = fine. Every edit goes through FxPluginProcessor::editProject (undoable).
class SpectrumWorkspace final : public juce::Component
{
public:
    enum class Mode { Spectrum, Difference };

    SpectrumWorkspace(FxPluginProcessor&, const AnalysisModel&, Selection&);

    Mode mode = Mode::Spectrum;
    std::function<void()> onSelectionChanged;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove(const juce::MouseEvent&) override;

    void resized() override;

    juce::Rectangle<float> plotArea() const;
    float yForEqDb(double db, juce::Rectangle<float> plot) const;
    double eqDbForY(float y, juce::Rectangle<float> plot) const;
    // EQ pass: zoom. The view is a part of 20 Hz - 20 kHz (at least one octave) and an EQ range of +-3 ... 36 dB.
    float xFor(double hz, juce::Rectangle<float> plot) const;
    double hzFor(float x, juce::Rectangle<float> plot) const;
    void zoomFrequency(double factor, double aroundHz);
    void panOctaves(double octaves);
    void setEqRange(double db);
    void resetView();
    double viewLowHz() const noexcept { return viewMinHz; }
    double viewHighHz() const noexcept { return viewMaxHz; }
    double eqRangeDb() const noexcept { return eqRange; }

private:
    enum class Drag { None, EqPoint, FxLow, FxHigh, FxBody };
    struct Hit
    {
        Drag drag = Drag::None;
        std::uint32_t id = 0;
    };

    Hit hitTestNodes(juce::Point<float>) const;
    void showMenu(juce::Point<float>, Hit);
    void select(Selection::Kind, std::uint32_t id);
    double sampleRate() const;

    FxPluginProcessor& processor;
    const AnalysisModel& analysis;
    Selection& selection;

    Hit dragging;
    double dragStartHz = 0.0, dragStartLow = 0.0, dragStartHigh = 0.0, dragStartGain = 0.0;
    float dragStartX = 0.0f, dragStartY = 0.0f;
    bool panning = false;
    double viewMinHz = FrequencyAxis::kMin, viewMaxHz = FrequencyAxis::kMax, eqRange = 24.0;
    juce::TextButton zoomIn, zoomOut, zoomFull, rangeButton;
};
} // namespace hpeq::ui
