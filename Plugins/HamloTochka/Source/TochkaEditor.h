#pragma once

#include "TochkaProcessor.h"

#include "UI/AnalysisModel.h"
#include "UI/HamloTochkaPanel.h"
#include "UI/PresetScreen.h"
#include "UI/SteelLookAndFeel.h"

namespace hamlo::tochka
{
class Editor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor(Processor&);
    ~Editor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    void syncPanel();
    void selectBand(int);
    void addBand(float hz, float gain, bool dynamic = false);
    void endGesture();
    int findPoint(juce::Point<float>) const;
    int findRangeHandle(juce::Point<float>) const;
    void showBandMenu(int);
    float xFor(float hz) const noexcept;
    float yFor(float db) const noexcept;
    float hzAt(float x) const noexcept;
    float dbAt(float y) const noexcept;
    void drawGraph(juce::Graphics&);
    void drawMeasuredDifference(juce::Graphics&) const;
    juce::Rectangle<int> at(float x, float y, float width, float height) const;

    Processor& audio;
    hpeq::ui::AnalysisModel analysis, keyAnalysis;
    hpeq::ui::SteelLookAndFeel lookAndFeel;
    hpeq::ui::HamloTochkaPanel panel;
    hamlo::ui::PresetScreen presetScreen;
    juce::ComboBox bandSelector, analysisMode, spectrumView;
    juce::TextButton add { "ADD BAND" }, compare { "B: EQ" }, learn { "LEARN MATCH" }, clear { "CLEAR" };
    juce::TextButton keySource { "KEY: SELF" };
    juce::Label matchStatus, keyStatus;
    juce::Rectangle<float> graph;
    int selected = 0;
    int panelGesture = -1;
    enum class Drag { None, Point, Q, Range };
    Drag dragging = Drag::None;
    Processor::Band dragStart;
    int wheelSlopeBand = -1;
    float wheelSlopeRemainder = 0.0f;
    bool insufficientSignal = false;
};
} // namespace hamlo::tochka
