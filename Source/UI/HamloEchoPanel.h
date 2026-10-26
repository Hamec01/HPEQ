#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "UI/ModuleHelp.h"

#include "DSP/LayerSnapshot.h"

namespace hpeq::ui
{
// The delay page (module pass 3, docs/fx-addon/DELAY_DESIGN.md §3), in the owner's "Black Steel" style
// of HamloComp / HamloVerb: worn steel plate, nameplate "HamloEcho", a five-position CHARACTER selector,
// a TIME MODE lever, a green phosphor CRT (ECHO: the echo train, left up and right down; FILTER: the loop
// response with LOW / BAND / HIGH handles and the response after 2..4 repeats), an OUTPUT needle meter,
// ON lamp, PING-PONG and FREEZE levers, six large and eleven small brass knobs. Drawn in its own design
// coordinates (1180 x 760) and scaled to fit; the static plate is cached per size.
class HamloEchoPanel final : public juce::Component, public HelpTarget, private juce::Timer
{
public:
    enum Param
    {
        TimeL, TimeR, Feedback, Cross, Drive, Width, Mix,                                              // large row (Mix = the module's mix)
        ModRate, ModDepth, Diffusion, Pitch, Lofi, LowCut, HighCut, BandFreq, BandGain, BandQ, Duck,   // small row
        LowRes, HighRes,                                                                               // CRT handles only
        NumParams
    };
    static constexpr int kNumKnobs = Duck + 1;
    enum Switch { Character, TimeMode, SyncL, SyncR, Link, PingPong, Freeze, NumSwitches };
    enum DisplayMode { EchoView, FilterView };

    struct Values
    {
        std::array<double, NumParams> knobs { 250.0, 375.0, 0.35, 0.0, 3.0, 1.0, 1.0, 0.5, 0.3, 0.0, 0.0, 0.0, 120.0, 8000.0, 1000.0, 0.0, 1.0, 0.0, 0.0, 0.0 };
        std::array<int, NumSwitches> switches { 2, 0, 8, 6, 1, 0, 0 };
        bool on = true;
    };

    HamloEchoPanel();
    juce::String helpKeyAt(juce::Point<float> local) const override; // HelpTarget: the control under a point
    juce::Rectangle<float> contentBounds() const override;
    ~HamloEchoPanel() override;

    void setValues(const Values&); // from the project (no callbacks)
    const Values& getValues() const noexcept { return values; }

    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    // Live meters (polled at 30 Hz): wet output level dB, ducking dB (>= 0).
    std::function<std::pair<float, float>()> readMeters;
    // Host tempo for the synced times on the display (BPM).
    std::function<double()> readTempo;

    static double defaultFor(Param);
    static juce::String format(Param, double);
    static const char* keyFor(Param);
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

    // For tests and screenshots: set the meter and the live readouts to these readings at once.
    void settleMeters(float outputDb, float duckDb);

private:
    struct Knob
    {
        Param param;
        juce::Point<float> centre; // design coordinates
        float scale;
    };
    enum Handle { NoHandle = -1, LowHandle, BandHandle, HighHandle };
    struct Echo
    {
        double seconds, db;
        int side;      // 0 left, 1 right
        int repeat;    // 1 = first echo
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
    bool synced(Param) const noexcept;
    int knobAt(juce::Point<float>) const;
    int characterLabelAt(juce::Point<float>) const;
    Handle handleAt(juce::Point<float>) const;
    std::array<juce::Point<float>, 3> handlePositions() const;
    double tempo() const;
    std::vector<Echo> echoTrain(double& span) const;

    void renderStatic(juce::Graphics&) const;
    void drawKnob(juce::Graphics&, const Knob&) const;
    void drawSelector(juce::Graphics&) const;
    void drawTimeMode(juce::Graphics&) const;
    void drawLevers(juce::Graphics&) const;
    void drawCrt(juce::Graphics&) const;
    void drawEchoView(juce::Graphics&, juce::Rectangle<float> screen) const;
    void drawFilterView(juce::Graphics&, juce::Rectangle<float> screen) const;
    void drawLcd(juce::Graphics&) const;
    void editReadout(Param);

    float xForHz(double hz) const;
    double hzForX(float x) const;
    float yForDb(double db) const;
    double dbForY(float y) const;

    Values values;
    DisplayMode mode = EchoView;
    std::array<Knob, kNumKnobs> knobs;
    struct MeterState;
    std::unique_ptr<MeterState> meter;
    juce::Image staticLayer, phosphorImage;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1;
    Handle dragHandle = NoHandle;
    bool dragSelector = false;
    int dragSelectorStart = 0;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
    float outDb = -120.0f, duckDb = 0.0f;
    double playhead = 0.0;
    std::unique_ptr<juce::TextEditor> typing;
};
} // namespace hpeq::ui
