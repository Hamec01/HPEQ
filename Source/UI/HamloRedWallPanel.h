#pragma once

#include <array>
#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "UI/BlackSteel.h"
#include "UI/ModuleHelp.h"

namespace hamlo::ui
{
// The limiter page (module pass 5, docs/fx-addon/LIMITER_DESIGN.md §4), Black Steel: nameplate
// "HamloRedWall · MODEL OGR-1 · № 1917", amber LCDs (gain reduction, output peak), six brass knobs
// (THRESHOLD, CEILING, RELEASE, LOOKAHEAD, LINK, OUTPUT, MIX, STYLE with three detents GLASNOST / STAKHANOV / OCTOBER),
// TRUE PEAK / RELEASE / MONITOR levers, a green CRT with the scrolling gain-reduction history, a GAIN
// REDUCTION needle meter, ON lamp and toggle. Design coordinates 1180 x 700, scaled to fit.
class HamloRedWallPanel final : public juce::Component, public HelpTarget, private juce::Timer
{
public:
    // Threshold is labelled LOUDNESS (push), Ceiling LIMIT; Output is the gain after the limiter.
    enum Param { Threshold, Ceiling, Release, Lookahead, Link, Output, Mix, Style, NumKnobs }; // Mix = the module's mix
    enum Switch { TruePeak, AutoRelease, Monitor, NumSwitches };

    struct Values
    {
        std::array<double, NumKnobs> knobs { 0.0, -1.0, 60.0, 2.0, 1.0, 0.0, 1.0, 0.0 };
        std::array<int, NumSwitches> switches { 1, 1, 0 };
        bool on = true;
    };

    HamloRedWallPanel();
    juce::String helpKeyAt(juce::Point<float> local) const override; // HelpTarget: the control under a point
    juce::Rectangle<float> contentBounds() const override;
    ~HamloRedWallPanel() override;

    void setValues(const Values&);
    const Values& getValues() const noexcept { return values; }

    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    // Live meters (polled at 30 Hz): output sample peak dB, gain reduction dB (>= 0).
    std::function<std::pair<float, float>()> readMeters;

    static double defaultFor(Param);
    static juce::String format(Param, double);
    static const char* keyFor(Param);
    static const char* keyFor(Switch);
    static const juce::StringArray& styleNames();

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // For tests and screenshots: meters to these readings at once; the reading is appended to the history.
    void settleMeters(float outputDb, float reductionDb);

private:
    void timerCallback() override;
    juce::AffineTransform toScreen() const;
    juce::Point<float> toDesign(juce::Point<float>) const;
    double normFor(Param) const;
    double valueFor(Param, double norm) const;
    void setNorm(Param, double norm, bool notify);
    int knobAt(juce::Point<float>) const;
    bool hitSwitch(juce::Point<float>, int& which, int& option) const;
    bool hitPower(juce::Point<float>) const;
    void renderStatic(juce::Graphics&) const;
    void drawCrt(juce::Graphics&) const;
    void editReadout(Param);

    Values values;
    std::array<juce::Point<float>, NumKnobs> centres;
    steel::Meter grMeter;
    std::vector<float> history; // GR dB, newest last, 30 Hz
    float outputDb = -120.0f, grText = 0.0f;
    juce::Image staticLayer;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
    std::unique_ptr<juce::TextEditor> typing;
};
} // namespace hamlo::ui
