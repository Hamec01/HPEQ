#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hpeq::ui
{
// BRING LENIN BACK / BRING STALIN BACK (owner, 2026-10-26): a portrait button for the tube colour of the
// whole output. Off: the portrait dark; on: it turns red and lights up. A click switches it; dragging up /
// down on it or the wheel sets its MIX (a thin amber gauge on its edge, "MIX 70 %" while it changes);
// a double click sets the MIX back to 100 %.
class TubeButton final : public juce::Component, public juce::SettableTooltipClient
{
public:
    // `portrait`: "lenin" or "stalin" (Assets/tubes/<portrait>_off.png / _on.png); `caption` under it.
    TubeButton(const juce::String& portrait, const juce::String& caption);

    void setState(bool on, double mix); // from the project (no callbacks)
    bool isOn() const noexcept { return on; }
    double getMix() const noexcept { return mix; }

    std::function<void(bool)> onToggle;
    std::function<void()> onMixGestureStart, onMixGestureEnd;
    std::function<void(double)> onMix;

    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void setMix(double);

    juce::Image imageOff, imageOn;
    juce::String caption;
    bool on = false, hover = false, dragging = false, moved = false;
    double mix = 1.0, dragStartMix = 1.0;
    float dragStartY = 0.0f;
};
} // namespace hpeq::ui
