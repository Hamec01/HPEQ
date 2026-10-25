#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "UI/BlackSteel.h"

namespace hpeq::ui
{
// The Gain page (module pass 9): HamloFader, the band level utility, Black Steel. A long vertical fader
// with an engraved dB scale (GAIN), INPUT and OUTPUT needle meters (band RMS), polarity levers for the
// left and right channel, MUTE and SOLO levers with lamps, a MATCH push button (sets the gain so the band
// lands on TARGET), DELAY / TARGET / MIX knobs, amber LCD, ON lamp. Design coordinates 1180 x 700.
class HamloFaderPanel final : public juce::Component, private juce::Timer
{
public:
    enum Param { Gain, Delay, Target, Mix, NumParams }; // Mix = the module's mix (FxSlotState::mix)
    enum Switch { Polarity, Mute, Solo, NumSwitches };   // polarity: bit 0 left, bit 1 right

    struct Values
    {
        std::array<double, NumParams> knobs { 0.0, 0.0, -18.0, 1.0 };
        std::array<int, NumSwitches> switches { 0, 0, 0 };
        bool on = true;
    };
    struct Meters
    {
        float inputDb = -120.0f, outputDb = -120.0f, inputLongDb = -120.0f;
    };

    HamloFaderPanel();
    ~HamloFaderPanel() override;

    void setValues(const Values&); // from the project (no callbacks)
    const Values& getValues() const noexcept { return values; }

    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    std::function<Meters()> readMeters; // polled at 30 Hz

    static double defaultFor(Param);
    static juce::String format(Param, double);
    static const char* keyFor(Param);  // "mix" for Mix
    static const char* keyFor(Switch);
    // The gain MATCH sets: target - long-term band input level, clamped to the fader range.
    static double matchGain(double targetDb, double inputLongDb) noexcept;
    void pressMatch(); // as a click on MATCH (tests)

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void settleMeters(const Meters&); // screenshots / tests

private:
    void timerCallback() override;
    juce::AffineTransform toScreen() const;
    juce::Point<float> toDesign(juce::Point<float>) const;
    double normFor(Param) const;
    double valueFor(Param, double norm) const;
    void setKnob(Param, double value, bool notify);
    void setSwitch(Switch, int value);
    int knobAt(juce::Point<float>) const;
    float faderY(double gainDb) const;
    double faderDb(float y) const;
    void renderStatic(juce::Graphics&) const;
    void drawFader(juce::Graphics&) const;
    void drawToggles(juce::Graphics&) const;
    void drawLcd(juce::Graphics&) const;
    void editReadout(Param);

    Values values;
    std::array<steel::Meter, 2> meters;
    Meters live;
    juce::Image staticLayer;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1;
    bool dragFader = false;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
    int matchFlash = 0;
    std::unique_ptr<juce::TextEditor> typing;
};
} // namespace hpeq::ui
