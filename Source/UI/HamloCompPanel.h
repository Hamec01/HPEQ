#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hpeq::ui
{
// The compressor page, after the owner's HamloComp design ("Black Steel", hamlocomp.zip: index.html /
// style.css / app.js): worn black steel panel with bolts, brass nameplate, amber GR display, six brass
// knobs with engraved scales, three chrome toggle switches, two brass-bezel needle meters with aged dial
// paper and scratched glass, a red ON lamp with a toggle. Drawn in the design's own coordinates
// (1180 x 700) and scaled to fit; textures are procedural with the design's seeds. Static layers are
// cached per size.
class HamloCompPanel final : public juce::Component, private juce::Timer
{
public:
    enum Param { Threshold, Ratio, Knee, Attack, Release, Makeup, Mix, NumKnobs }; // Mix = the module's mix (FxSlotState::mix)
    enum Switch { Detector, Character, AutoMakeup, NumSwitches };

    struct Values
    {
        std::array<double, NumKnobs> knobs { -30.0, 4.0, 6.0, 1.0, 80.0, 0.0, 1.0 };
        std::array<int, NumSwitches> switches { 0, 1, 1 };
        bool on = true;
    };

    HamloCompPanel();
    ~HamloCompPanel() override;

    void setValues(const Values&); // from the project (no callbacks)
    const Values& getValues() const noexcept { return values; }

    // Edits: gesture start (undo grouping), value, gesture end.
    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    // Live meters (polled at 30 Hz): input level dB, gain reduction dB (>= 0).
    std::function<std::pair<float, float>()> readMeters;

    static double defaultFor(Param);
    static juce::String format(Param, double);
    static const char* keyFor(Param);
    static const char* keyFor(Switch);

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // For tests and screenshots: advance the needles to the given readings at once.
    void settleMeters(float inputDb, float reductionDb);

private:
    struct Knob
    {
        Param param;
        juce::Point<float> centre; // design coordinates
    };
    struct Meter
    {
        juce::Rectangle<float> bezel; // design coordinates
        float min, max, redFrom, rest;
        juce::String title;
        float titleSize;
        std::vector<std::pair<float, bool>> ticks; // value, labelled
        int seed;
        float shown;
    };

    void timerCallback() override;
    juce::AffineTransform toScreen() const;
    juce::Point<float> toDesign(juce::Point<float> screen) const;
    double normFor(Param) const;
    double valueFor(Param, double norm) const;
    void setNorm(Param, double norm, bool notify);
    int knobAt(juce::Point<float> design) const;
    bool hitSwitch(juce::Point<float> design, int& which, int& option) const;
    bool hitPower(juce::Point<float> design) const;

    void renderStatic(juce::Graphics&) const; // panel, wear, bolts, nameplate, labels, scales, meter faces
    void drawKnob(juce::Graphics&, const Knob&) const;
    void drawSwitch(juce::Graphics&, int which) const;
    void drawPower(juce::Graphics&) const;
    void drawMeterDynamic(juce::Graphics&, const Meter&) const;
    void editReadout(Param);

    Values values;
    std::array<Knob, NumKnobs> knobs;
    std::array<Meter, 2> meters;
    juce::Image staticLayer;
    float scale = 1.0f;
    juce::Point<float> offset;
    int dragKnob = -1;
    double dragStartNorm = 0.0;
    float dragStartY = 0.0f;
    float grText = 0.0f;
    std::unique_ptr<juce::TextEditor> typing;
};
} // namespace hpeq::ui
