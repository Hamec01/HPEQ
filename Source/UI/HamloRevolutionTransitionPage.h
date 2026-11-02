#pragma once

#include <array>
#include <functional>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace hpeq::ui
{
// RISE / FILL (docs/RISE_FILL.md): the transition page. RISE / FILL tabs, BARS 1 / 2 / 4 (the last N bars heard),
// four knobs, GENERATE (capture now, a new seed), PLAY / STOP (the preview), the waveform (drag it into the DAW),
// SHOW FILE, BACK. The same 1180 x 1080 frame as SIMPLE and ADVANCED (the preset screen stays in its place).
class HamloRevolutionTransitionPage final : public juce::Component, public juce::TooltipClient
{
public:
    struct Values
    {
        int kind = 0, bars = 2;
        std::array<double, 4> knobs { 0.6, 0.8, 0.5, 0.7 };
        bool valid = false, aligned = false, complete = false, previewing = false;
        double bpm = 120.0, playhead = -1.0; // 0 .. 1 while previewing
        juce::String fileName;
    };

    HamloRevolutionTransitionPage();

    void setValues(const Values&);
    const Values& getValues() const noexcept { return values; }
    void setClip(const juce::AudioBuffer<float>&); // the waveform (peaks per column)

    std::function<void(int kind)> onKind;
    std::function<void(int bars)> onBars;
    std::function<void(int knob, double value)> onKnob; // while turning (no render)
    std::function<void()> onKnobsDone;                  // the gesture ended: render again
    std::function<void()> onGenerate, onPlay, onShowFile, onBack;
    std::function<juce::File()> fileToDrag;

    // the controls, also for tests
    void pressGenerate();
    void pressPlay();
    void pressBack();
    void pressShowFile();
    void chooseKind(int);
    void chooseBars(int);
    void setKnob(int knob, double value, bool done);
    juce::String helpKeyAt(juce::Point<float> local) const;
    juce::String getTooltip() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    juce::Point<float> toDesign(juce::Point<float>) const;

    Values values;
    std::vector<std::pair<float, float>> peaks; // min / max per waveform column
    float scale = 1.0f;
    juce::Point<float> offset;
    juce::Image staticLayer;
    int dragKnob = -1;
    double dragStart = 0.0;
    float dragStartY = 0.0f;
    bool waveDown = false, dragged = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamloRevolutionTransitionPage)
};
} // namespace hpeq::ui
