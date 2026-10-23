#pragma once

#include <array>
#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "UI/BlackSteel.h"

namespace hpeq::ui
{
// The gate page (module pass 7, docs/fx-addon/GATE_DESIGN.md §3), Black Steel like the other pages: nameplate
// "HamloZastava · MODEL KPP-1 · № 1945", amber LCDs (KEY, STATE), big brass knobs THRESHOLD, RANGE, ATTACK,
// HOLD, RELEASE and the MODE knob titled ZASTAVA / PATRUL / TREVOGA, small knobs RATIO, HYSTERESIS, KEY HP,
// KEY LP, MIX, OUTPUT, levers LOOK-AHEAD / KEY LISTEN / FLIP, a green CRT (key level of the last 4 s, the
// threshold, shut stretches shaded), an ATTENUATION needle meter, ON lamp and toggle, scratched pictures.
// Design coordinates 1180 x 780, scaled to fit.
class HamloZastavaPanel final : public juce::Component, private juce::Timer
{
public:
    enum Param { Threshold, Range, Attack, Hold, Release, Mode, Ratio, Hysteresis, KeyHp, KeyLp, Mix, Output, NumKnobs };
    enum Switch { Lookahead, KeyListen, Flip, NumSwitches };

    struct Values
    {
        std::array<double, NumKnobs> knobs { -40.0, 40.0, 0.5, 20.0, 100.0, 0.0, 4.0, 4.0, 20.0, 20000.0, 1.0, 0.0 };
        std::array<int, NumSwitches> switches { 1, 0, 0 };
        bool on = true;
    };

    // What the DSP reports (ZastavaCore::Meters, kept free of DSP headers).
    struct Reading
    {
        float attenuationDb = 0.0f, maxAttenuationDb = 0.0f, keyDb = -120.0f;
        bool open = false;
        int openings = 0;
    };

    HamloZastavaPanel();
    ~HamloZastavaPanel() override;

    void setValues(const Values&);
    const Values& getValues() const noexcept { return values; }

    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    std::function<Reading()> readMeters; // polled at 30 Hz

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

    // For tests and screenshots: take this reading at once (appended to the history).
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
    juce::Rectangle<float> readoutArea(int knob) const;
    bool hitSwitch(juce::Point<float>, int& which, int& option) const;
    bool hitPower(juce::Point<float>) const;
    void renderStatic(juce::Graphics&) const;
    void drawCrt(juce::Graphics&) const;
    void editReadout(Param);

    Values values;
    std::array<juce::Point<float>, NumKnobs> centres;
    steel::Meter attenuationMeter;
    std::vector<float> keyHistory, shutHistory; // key dB and attenuation dB, newest last, 30 Hz
    Reading reading;
    juce::Image staticLayer;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
    std::unique_ptr<juce::TextEditor> typing;
};
} // namespace hpeq::ui
