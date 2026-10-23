#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "DSP/LayerSnapshot.h"

namespace hpeq::ui
{
// The saturator page (module pass 4, docs/fx-addon/SATURATOR_DESIGN.md §4), in the owner's "Black Steel"
// style: worn steel plate, two aluminium tape reels at the top turning with the signal (at the SPEED
// set), the tape over guide rollers and a head block under the nameplate "HamloMag", SPEED and
// OVERSAMPLING levers, a six-position MODEL selector, a green phosphor CRT (CURVE: the transfer at the
// current drive, the tape loop drawn; HARMONICS: H1 … H9 for the live input level, computed by the
// engine), an OUTPUT needle meter, ON lamp, PUNISH / THUMP / STEEP / AUTO toggles, six large and seven
// small brass knobs and the NOISE ALWAYS / FOLLOW toggle. Design coordinates 1180 x 820, scaled to fit.
class HamloMagPanel final : public juce::Component, private juce::Timer
{
public:
    enum Param
    {
        Drive, Tone, LowCut, HighCut, Mix, Output,                 // large row
        Harmonics, EvenOdd, Bias, Wow, Hiss, Hum, Crackle,         // small row
        NumParams
    };
    static constexpr int kLarge = 6;
    enum Switch { Model, Speed, Oversampling, Punish, Thump, Steep, Auto, NoiseFollow, NumSwitches };
    enum DisplayMode { CurveView, HarmonicsView };

    struct Values
    {
        std::array<double, NumParams> knobs { 6.0, 0.0, 20.0, 20000.0, 1.0, 0.0, 0.5, 0.0, 0.5, 0.15, 0.15, 0.0, 0.0 };
        std::array<int, NumSwitches> switches { 1, 0, 1, 0, 0, 0, 1, 1 };
        bool on = true;
    };

    HamloMagPanel();
    ~HamloMagPanel() override;

    void setValues(const Values&); // from the project (no callbacks)
    const Values& getValues() const noexcept { return values; }

    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange; // Mix is the module's mix (FxSlotState::mix), not a parameter
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    // Live meters (polled at 30 Hz): input peak dB, output peak dB.
    std::function<std::pair<float, float>()> readMeters;

    static double defaultFor(Param);
    static juce::String format(Param, double);
    static const char* keyFor(Param);  // "mix" for Mix
    static const char* keyFor(Switch);
    static bool usedByClassic(Param) noexcept;
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

    // For tests and screenshots: set the meters to these readings at once and turn the reels a little.
    void settleMeters(float inputDb, float outputDb);

private:
    struct Knob
    {
        Param param;
        juce::Point<float> centre; // design coordinates
        float scale;
    };

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
    int modelLabelAt(juce::Point<float>) const;

    void renderStatic(juce::Graphics&) const;
    void drawReel(juce::Graphics&, juce::Point<float> centre, float packRadius, float angle) const;
    void drawTape(juce::Graphics&) const;
    void drawKnob(juce::Graphics&, const Knob&) const;
    void drawSelector(juce::Graphics&) const;
    void drawLevers(juce::Graphics&) const;
    void drawToggles(juce::Graphics&) const;
    void drawCrt(juce::Graphics&) const;
    void drawCurveView(juce::Graphics&, juce::Rectangle<float> screen) const;
    void drawHarmonicsView(juce::Graphics&, juce::Rectangle<float> screen) const;
    void drawLcd(juce::Graphics&) const;
    void editReadout(Param);

    Values values;
    DisplayMode mode = CurveView;
    std::array<Knob, NumParams> knobs;
    struct MeterState;
    std::unique_ptr<MeterState> meter;
    juce::Image staticLayer;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1;
    bool dragSelector = false;
    int dragSelectorStart = 0;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
    float inDb = -120.0f, outDb = -120.0f;
    double reelLeft = 0.0, reelRight = 0.0;
    // the harmonic profile is recomputed only when the settings or the (2 dB rounded) level change
    mutable std::array<double, 9> profile {};
    mutable dsp::FxSlotParams profileFor {};
    mutable int profileLevel = 1000;
    std::unique_ptr<juce::TextEditor> typing;
};
} // namespace hpeq::ui
