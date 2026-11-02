#pragma once

#include <array>
#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "UI/BlackSteel.h"
#include "UI/ModuleHelp.h"

namespace hpeq::ui
{
// The stereo page (module pass 8, docs/fx-addon/STEREO_DESIGN.md §3), Black Steel like the other pages:
// nameplate "HamloShirota · MODEL SHR-2 · № 1957", amber LCDs (CORRELATION, WIDTH / SAFE), big brass knobs
// WIDTH, STEREOIZE, SPACE, MONO BASS, ROTATION and the MODE knob titled EDINSTVO / PROSTOR / KOSMOS with its
// character written small below, small knobs BALANCE, MIX, OUTPUT, levers SAFE and MONO, a green CRT
// vectorscope (OSHIBKA when the node has one channel), a CORRELATION needle meter, ON lamp and toggle,
// scratched pictures. Design coordinates 1180 x 780, scaled to fit.
class HamloShirotaPanel final : public juce::Component, public HelpTarget, private juce::Timer
{
public:
    enum Param { Width, Stereoize, Space, MonoBass, Rotation, Mode, Balance, Mix, Output, NumKnobs };
    enum Switch { Safe, Mono, NumSwitches };
    static constexpr int kScopePoints = 512;

    struct Values
    {
        std::array<double, NumKnobs> knobs { 1.0, 0.5, 0.5, 10.0, 0.0, 0.0, 0.0, 1.0, 0.0 };
        std::array<int, NumSwitches> switches { 1, 0 };
        bool on = true;
        bool stereoNode = true; // false: the node works on one channel (Mid / Side / Left / Right)
    };

    // What the DSP reports (ShirotaCore::Meters, kept free of DSP headers).
    struct Reading
    {
        float correlation = 1.0f, guardDb = 0.0f;
        bool oneChannel = false;
    };

    HamloShirotaPanel();
    juce::String helpKeyAt(juce::Point<float> local) const override; // HelpTarget: the control under a point
    juce::Rectangle<float> contentBounds() const override;
    ~HamloShirotaPanel() override;

    void setValues(const Values&);
    const Values& getValues() const noexcept { return values; }
    bool showsError() const noexcept { return !values.stereoNode || reading.oneChannel; }

    std::function<void(Param)> onKnobGestureStart;
    std::function<void(Param, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(Switch, int)> onSwitchChange;
    std::function<void(bool)> onPowerChange;
    std::function<Reading()> readMeters; // polled at 30 Hz
    std::function<bool(std::array<float, kScopePoints>&, std::array<float, kScopePoints>&)> readScope;

    static double defaultFor(Param);
    static juce::String format(Param, double);
    static const char* keyFor(Param); // "mix" is the slot's mix
    static const char* keyFor(Switch);
    static const juce::StringArray& modeNames();
    static const juce::StringArray& modeCharacters(); // the real character, written small under the name

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // For tests and screenshots: take this reading and the scope at once.
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
    steel::Meter correlationMeter;
    std::array<float, kScopePoints> scopeL {}, scopeR {};
    float scopeScale = 1.0f;
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
