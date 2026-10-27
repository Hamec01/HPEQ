#pragma once

#include <array>
#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "UI/BlackSteel.h"
#include "UI/ModuleHelp.h"

namespace hamlo::ui
{
// The transient page (module pass 6, docs/fx-addon/TRANSIENT_DESIGN.md §4), Black Steel: nameplate
// "HamloUdarnik · MODEL UD-1 · № 1961", amber LCDs (IMPACT, last HIT), ten brass knobs (SNAP, BODY, SPEED,
// RISE, DECAY, SENS, PRESERVE, MIX, OUTPUT and the MODE knob titled PRAVDA / UDARNIK / PERESTROIKA),
// LOOK-AHEAD and LISTEN levers, a green CRT (gain of the last 4 s with hit marks; the last hit's input and
// output envelopes), an IMPACT needle meter, ON lamp and toggle. Design coordinates 1180 x 820 (HamloUdarnik v2: a
// bottom strip with the ENGINE CLASSIC / V2 and PROTECT levers), scaled to fit.
class HamloUdarnikPanel final : public juce::Component, public HelpTarget, private juce::Timer
{
public:
    enum Param { Snap, Body, Speed, Rise, Decay, Sensitivity, Preserve, Mix, Output, Mode, NumKnobs };
    enum Switch { Lookahead, Listen, Engine, Protect, NumSwitches }; // ENGINE and PROTECT: the bottom strip (v2)
    static constexpr int kHitPoints = 64; // 2.5 ms each

    struct Values
    {
        std::array<double, NumKnobs> knobs { 0.0, 0.0, 0.5, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0 };
        std::array<int, NumSwitches> switches { 1, 0, 1, 1 };
        bool on = true;
    };

    // What the DSP reports (UdarnikCore::Meters, kept free of DSP headers).
    struct Reading
    {
        float impactDb = 0.0f, gainMinDb = 0.0f, gainMaxDb = 0.0f;
        int events = 0;
        float confidence = 0.0f, strengthDb = -120.0f;
    };

    HamloUdarnikPanel();
    juce::String helpKeyAt(juce::Point<float> local) const override; // HelpTarget: the control under a point
    juce::Rectangle<float> contentBounds() const override;
    ~HamloUdarnikPanel() override;

    void setValues(const Values&);
    const Values& getValues() const noexcept { return values; }

    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    // Live readings (polled at 30 Hz); the last hit's envelopes (returns its sequence, 0 = none).
    std::function<Reading()> readMeters;
    std::function<int(std::array<float, kHitPoints>&, std::array<float, kHitPoints>&)> readLastHit;

    static double defaultFor(Param);
    static juce::String format(Param, double);
    static const char* keyFor(Param); // "mix" is the slot's mix
    static const char* keyFor(Switch);
    static const juce::StringArray& modeNames();

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // For tests and screenshots: take this reading at once (appended to the history) and the last hit.
    void settleMeters(const Reading&);

private:
    void timerCallback() override;
    void take(const Reading&);
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
    steel::Meter impactMeter;
    std::vector<float> gainHistory, markHistory; // dB and hit confidence (-1 = none), newest last, 30 Hz
    std::array<float, kHitPoints> hitIn {}, hitOut {};
    int hitSequence = 0, lastEvents = 0;
    Reading reading;
    float impactText = 0.0f;
    juce::Image staticLayer;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
    std::unique_ptr<juce::TextEditor> typing;
};
} // namespace hamlo::ui
