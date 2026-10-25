#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/Revolution.h"
#include "UI/BlackSteel.h"

namespace hpeq::ui
{
// The Monsta FX slot as HamloRevolution (module pass 10), Black Steel, design coordinates 1180 x 700.
// Top: nameplate, SEED counter with GENERATE (red) and NEW SEED, CHARACTER selector, module MIX, ON lamp.
// Second row: ORDER (six serial orders) and GRID selectors, TRIPLET and SYNC / FREE levers, RATE (free
// step length), STEPS, SWING, HUMAN, FADE knobs, status LCD. Then one row per engine (TIME, GLITCH,
// MOTION): plate with ON lever, MIX and DEPTH knobs (MOTION: SMOOTH and the TARGET window too) and 16
// step cells. A click on a TIME / GLITCH cell steps through its actions, right click picks one; MOTION
// cells take a level from where they are clicked or dragged; shift-click locks a step (Generate keeps
// it); right click on a plate: which actions Generate may use, clear, lock / unlock all. Projects saved
// before this pass show the legacy M6 Monsta with an UPGRADE button (their sound is kept until then).
class HamloRevolutionPanel final : public juce::Component, private juce::Timer
{
public:
    enum Param { Mix, Rate, Steps, Swing, Humanize, Fade, TimeMix, TimeDepth, GlitchMix, GlitchDepth, MotionMix, MotionDepth, MotionSmooth, NumParams };
    enum Switch { Order, Character, Grid, Triplet, Free, TimeOn, GlitchOn, MotionOn, MotionTarget, NumSwitches };

    struct Values
    {
        std::array<double, NumParams> knobs { 1.0, 125.0, 16.0, 0.0, 0.0, 2.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.6, 0.3 };
        std::array<int, NumSwitches> switches { 0, 1, 2, 0, 0, 1, 1, 1, 1 };
        revolution::Pattern pattern;
        juce::int64 seed = 1;
        bool on = true;
        bool legacy = false; // the slot still runs the M6 Monsta (no "mode")
    };
    struct Status
    {
        int step = -1;
        bool warming = false;
    };

    HamloRevolutionPanel();
    ~HamloRevolutionPanel() override;

    void setValues(const Values&); // from the project (no callbacks)
    const Values& getValues() const noexcept { return values; }

    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    // A new pattern (and seed), one undo step named `what`.
    std::function<void(const revolution::Pattern&, juce::int64 seed, const juce::String& what)> onPatternChange;
    std::function<void()> onUpgrade;
    std::function<Status()> readStatus; // polled at 30 Hz

    static double defaultFor(Param);
    static juce::String format(Param, double);
    static const char* keyFor(Param);  // "mix" for Mix (the module's mix)
    static const char* keyFor(Switch);

    // As the clicks on the page (tests, screenshots).
    void pressGenerate();
    void pressNewSeed();
    void clickStep(int engine, int step, bool shift = false);
    void setMotionLevel(int step, int level);
    void settleStatus(const Status&);

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
    void setKnob(Param, double value, bool notify);
    void setSwitch(Switch, int value);
    int knobAt(juce::Point<float>) const;
    int selectorAt(juce::Point<float>) const;
    bool stepAt(juce::Point<float>, int& engine, int& step) const;
    void changePattern(const revolution::Pattern&, juce::int64 seed, const juce::String& what);
    void showStepMenu(int engine, int step);
    void showPlateMenu(int engine);
    void renderStatic(juce::Graphics&) const;
    void drawSelectors(juce::Graphics&) const;
    void drawKnobs(juce::Graphics&) const;
    void drawRows(juce::Graphics&) const;
    void drawTop(juce::Graphics&) const;
    void drawLegacy(juce::Graphics&) const;

    Values values;
    Status live;
    juce::Image staticLayer;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1, dragMotionStep = -1;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
    int generateFlash = 0;
};
} // namespace hpeq::ui
