#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "DSP/LayerSnapshot.h"

namespace hpeq::ui
{
// The reverb page (module pass 2, docs/fx-addon/REVERB_DESIGN.md §3), in the owner's "Black Steel"
// style of HamloComp: worn steel plate, nameplate "HamloVerb", a seven-position SPACE selector, a green
// phosphor CRT (DECAY: decay time by frequency with draggable Bass / Mid / High handles and the output
// filters; TIME: early reflections and the late envelopes per band), an OUTPUT needle meter, COLOR and
// FREEZE levers, a red ON lamp, six large and eleven small brass knobs. Drawn in its own design
// coordinates (1180 x 760) and scaled to fit; the static plate is cached per size, the CRT keeps a
// phosphor persistence image of its live trace.
class HamloVerbPanel final : public juce::Component, private juce::Timer
{
public:
    enum Param
    {
        PreDelay, Size, Decay, Attack, Distance, Width, Mix,                                             // large row (Mix = the module's mix)
        EarlyDiff, LateDiff, ModRate, ModDepth, BassMult, BassXover, HighMult, HighXover, LowCut, HighCut, Duck, // small row
        NumKnobs
    };
    enum Switch { Space, Color, Sync, Freeze, NumSwitches };
    enum DisplayMode { DecayView, TimeView };

    struct Values
    {
        std::array<double, NumKnobs> knobs { 20.0, 1.0, 2.4, 0.2, 0.4, 1.0, 1.0, 0.7, 0.7, 0.6, 0.4, 1.2, 300.0, 0.5, 4000.0, 80.0, 12000.0, 0.0 };
        std::array<int, NumSwitches> switches { 3, 2, 0, 0 };
        bool on = true;
    };

    HamloVerbPanel();
    ~HamloVerbPanel() override;

    void setValues(const Values&); // from the project (no callbacks)
    const Values& getValues() const noexcept { return values; }

    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    // Live meters (polled at 30 Hz): wet output level dB, ducking dB (>= 0).
    std::function<std::pair<float, float>()> readMeters;

    static double defaultFor(Param);
    static juce::String format(Param, double);
    static const char* keyFor(Param);
    static const char* keyFor(Switch);
    // The knobs that act on the Classic (M4) engine.
    static bool usedByClassic(Param) noexcept;
    // The DSP parameter block these values describe (for the display's analytic curves).
    dsp::FxSlotParams slotParams() const;

    void setDisplayMode(DisplayMode);
    DisplayMode getDisplayMode() const noexcept { return mode; }

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // For tests and screenshots: set the meter and the live readouts to these readings at once.
    void settleMeters(float outputDb, float duckDb);

private:
    struct Knob
    {
        Param param;
        juce::Point<float> centre; // design coordinates
        float scale;
    };
    enum Handle { NoHandle = -1, BassHandle, MidHandle, HighHandle };

    void timerCallback() override;
    juce::AffineTransform toScreen() const;
    juce::Point<float> toDesign(juce::Point<float> screen) const;
    double normFor(Param) const;
    double valueFor(Param, double norm) const;
    void setNorm(Param, double norm, bool notify);
    void setKnob(Param, double value, bool notify);
    void setSwitch(Switch, int value);
    bool isActive(Param) const noexcept;
    int knobAt(juce::Point<float>) const;
    int spaceLabelAt(juce::Point<float>) const;
    Handle handleAt(juce::Point<float>) const;
    std::array<juce::Point<float>, 3> handlePositions() const;

    void renderStatic(juce::Graphics&) const;
    void drawKnob(juce::Graphics&, const Knob&) const;
    void drawSpaceSelector(juce::Graphics&) const;
    void drawColorSwitch(juce::Graphics&) const;
    void drawFreeze(juce::Graphics&) const;
    void drawCrt(juce::Graphics&) const;
    void drawDecayView(juce::Graphics&, juce::Rectangle<float> screen) const;
    void drawTimeView(juce::Graphics&, juce::Rectangle<float> screen) const;
    void drawLcd(juce::Graphics&) const;
    void editReadout(Param);

    // CRT mapping (design coordinates inside the screen)
    float xForHz(double hz) const;
    double hzForX(float x) const;
    float yForSeconds(double s) const;
    double secondsForY(float y) const;
    double decayViewMax() const;

    Values values;
    DisplayMode mode = DecayView;
    std::array<Knob, NumKnobs> knobs;
    struct MeterState;
    std::unique_ptr<MeterState> meter;
    juce::Image staticLayer, phosphor;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1;
    Handle dragHandle = NoHandle;
    bool dragSpace = false;
    int dragSpaceStart = 0;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
    float outDb = -120.0f, duckDb = 0.0f;
    double sweep = 0.0;
    std::unique_ptr<juce::TextEditor> typing;
};
} // namespace hpeq::ui
