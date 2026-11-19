#pragma once

#include <array>
#include <functional>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "DSP/TransitionRender.h"

namespace hpeq::ui
{
// RISE / FILL (docs/RISE_FILL.md): the transition page. RISE / FILL tabs, BARS 1 / 2 / 4 (the last N bars heard),
// four knobs, GENERATE (capture now, a new seed), PLAY / STOP (the preview), the waveform (drag it into the DAW),
// SHOW FILE, BACK. The same 1180 x 1080 frame as SIMPLE and ADVANCED (the preset screen stays in its place).
class HamloRevolutionTransitionPage final : public juce::Component, public juce::TooltipClient, public juce::FileDragAndDropTarget
{
public:
    struct Values
    {
        int kind = 0, bars = 2, recipe = 1;
        int fillBeats = 4;
        double fillFx = 0.35;
        std::array<bool,hpeq::dsp::transition::kKitRoles> kitEnabled { true,true,true,true,true };
        std::array<int,hpeq::dsp::transition::kKitRoles> kitCounts {};
        std::array<std::uint32_t,hpeq::dsp::transition::kKitRoles> kitMask {};
        std::vector<hpeq::dsp::transition::Mark> marks;           // R15: the user's labels on the source (optional)
        std::vector<juce::Range<int>> hits;     // analysed hit regions of the source: a right-click snaps to one
        juce::String fillInfo; // R15 PHRASE FILL diagnostics line
        std::uint64_t seed = 1;
        std::array<double, 4> knobs { 0.6, 0.8, 0.5, 0.7 };
        bool valid = false, aligned = false, complete = false, previewing = false;
        double bpm = 120.0, playhead = -1.0; // 0 .. 1 while previewing
        juce::String fileName;
        juce::String sourceName; // a dropped sample's name; empty = the track's input
    };

    HamloRevolutionTransitionPage();

    void setValues(const Values&);
    const Values& getValues() const noexcept { return values; }
    void setClip(const juce::AudioBuffer<float>&); // the waveform (peaks per column)
    void setSource(const juce::AudioBuffer<float>&); // R15: the source under the fill, where hits are marked

    std::function<void(int kind)> onKind;
    std::function<void(int bars)> onBars;
    std::function<void(int recipe)> onRecipe;
    std::function<void(int knob, double value)> onKnob; // while turning (no render)
    std::function<void()> onKnobsDone;                  // the gesture ended: render again
    std::function<void()> onGenerate, onVariation, onFillOptions, onPlay, onShowFile, onBack;
    std::function<void()> onMarks; // R15: a mark was added / removed: render again (same capture and seed)
    std::function<juce::File()> fileToDrag;
    std::function<void(const juce::File&)> onSampleDropped; // a sample dropped on the page: the new source
    std::function<void()> onUseInput;                       // USE INPUT: the track's input again

    // the controls, also for tests
    void pressGenerate();
    void pressPlay();
    void pressBack();
    void pressShowFile();
    void pressUseInput();
    void chooseKind(int);
    void chooseBars(int);
    void chooseRecipe(int);
    void chooseFillBeats(int);
    void chooseFillFx(double);
    void pressVariation();
    void toggleKit(int role);
    void markRegion(int start, int end, int role); // R15: role 0..3; -1 removes marks overlapping the region
    void clearMarks();
    void undoMark();
    void forgetMarks() { values.marks.clear(); markHistory.clear(); repaint(); } // a new source: no marks, no undo                               // back to the marks before the last change
    int markAt(int sample) const;                  // index of the mark containing a source sample, -1 none
    juce::Range<int> hitAt(int sample) const;      // the analysed hit region containing / before a source sample
    void setKnob(int knob, double value, bool done);
    juce::String helpKeyAt(juce::Point<float> local) const;
    juce::String getTooltip() override;

    // a sample from the DAW or Explorer dropped anywhere on the page
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    juce::Point<float> toDesign(juce::Point<float>) const;

    Values values;
    std::vector<std::pair<float, float>> peaks; // min / max per waveform column
    std::vector<std::pair<float, float>> sourcePeaks;
    int sourceLength = 0;
    bool marking = false;      // a right-button gesture on the wave
    float markFrom = 0, markTo = 0; // design x of the gesture
    std::vector<std::vector<hpeq::dsp::transition::Mark>> markHistory; // UNDO LAST MARK
    float scale = 1.0f;
    juce::Point<float> offset;
    juce::Image staticLayer;
    int dragKnob = -1;
    double dragStart = 0.0;
    float dragStartY = 0.0f;
    bool waveDown = false, dragged = false, dropHover = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamloRevolutionTransitionPage)
};
} // namespace hpeq::ui
