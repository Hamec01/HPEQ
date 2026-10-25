#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/FxProjectState.h"
#include "UI/BlackSteel.h"

namespace hpeq::ui
{
// EQ pass: the page of one EQ point, HamloTochka ("point"), Black Steel, design coordinates 1100 x 340.
// Left: nameplate, SHAPE window with the shape's picture (a click opens all nine as pictures), SLOPE and
// CHANNEL windows, ON lamp, SOLO (headphones: hear only this point's region, as FabFilter's listen),
// DELETE. Middle: FREQ, GAIN, Q; the GAIN knob carries a ring for the dynamic range and the gain the
// dynamics add right now. Right: DYNAMIC lever and lamp, THRESH, RATIO, RANGE, ATTACK, RELEASE, and an
// LCD with the current dynamic gain, DETECT (BAND / VOICE: the region against the voice body) and DE-ESS
// (one press makes the point a de-esser). The point's modules stay in the grid next to the page.
class HamloTochkaPanel final : public juce::Component, private juce::Timer
{
public:
    enum Param { Freq, Gain, Q, Thresh, Ratio, Range, Attack, Release, NumParams };

    struct Values
    {
        EqNodeState node;
        int number = 1; // EQ 1, 2 ... as in the node list
    };

    HamloTochkaPanel();
    ~HamloTochkaPanel() override;

    void setValues(const Values&); // from the project (no callbacks)
    const Values& getValues() const noexcept { return values; }

    std::function<void(const juce::String& what)> onGestureStart;
    std::function<void()> onGestureEnd;
    // The edited point (everything but id and modules); `what` names the undo step.
    std::function<void(const EqNodeState&, const juce::String& what)> onChange;
    std::function<void(bool)> onSolo; // the inspector makes it the only soloed point of the layer
    std::function<void()> onDelete;
    std::function<float()> readDynamicGain; // dB the dynamics add now, polled at 30 Hz

    static double valueOf(const EqNodeState&, Param);
    static void setValueOf(EqNodeState&, Param, double);
    static juce::String format(Param, double);
    static bool enabledFor(const EqNodeState&, Param);

    // As clicks on the page (tests, screenshots).
    void chooseShape(EqShape);
    void pressSolo();
    void setDynamic(bool);
    void setDetect(int);  // 0 BAND, 1 VOICE
    void pressDeEss();    // the point as a de-esser: bell 7 kHz, Q 0.8, VOICE, range -9, 0.5 / 40 ms
    void openShapes(bool open);
    void settleDynamicGain(float db);

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    juce::AffineTransform toScreen() const;
    juce::Point<float> toDesign(juce::Point<float>) const;
    double normFor(Param) const;
    double valueFor(Param, double norm) const;
    void setParam(Param, double value);
    void change(const juce::String& what);
    int knobAt(juce::Point<float>) const;
    void renderStatic(juce::Graphics&) const;
    void drawShapeChooser(juce::Graphics&) const;

    Values values;
    float dynamicGain = 0.0f;
    bool shapesOpen = false;
    juce::Image staticLayer;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
};
} // namespace hpeq::ui
