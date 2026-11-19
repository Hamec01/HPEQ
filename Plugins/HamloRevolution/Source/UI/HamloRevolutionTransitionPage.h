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
        // R16: the wave's GRID (0 off: beat lines only, no snap; 1 1/4, 2 1/8, 3 1/16, 4 1/32, 5 1/8T, 6 1/16T)
        int grid = 0;
        // R16: the dropped sample's tempo (0 = the host's), the host's tempo and the sample's length (BPM menu)
        double sampleBpm = 0.0, hostBpm = 120.0, sampleSeconds = 0.0;
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
    std::function<void()> onClear;                          // R16 CLEAR: start over on the same source
    std::function<void(double bpm)> onBpm;                  // R16: the dropped sample's tempo (0 = the host's)

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
    void pressClear();
    void chooseGrid(int grid);
    void chooseBpm(double bpm); // 0 = the host's tempo; else 40 .. 250
    void selectRegion(int start, int end); // R16: the pending selection (source samples); labelled by a left-click
    juce::Range<int> getSelection() const noexcept { return selection; }
    void setView(double start, double end); // R16 zoom: the shown part of the wave, 0 .. 1
    juce::Range<double> getView() const noexcept { return { viewStart, viewEnd }; }
    void zoomAround(double fraction, double factor); // factor < 1 zooms in, keeping `fraction` under the mouse
    int snapToGrid(int sample) const;                // the nearest grid line (GRID off: unchanged)
    static double bpmFromName(const juce::String& fileName); // "fill_88bpm.wav" -> 88, none -> 0
    void markRegion(int start, int end, int role); // R15: role 0..4 (R16: 5 SAMPLE); -1 removes marks overlapping the region
    void clearMarks();
    void undoMark();
    void forgetMarks() { values.marks.clear(); markHistory.clear(); selection = {}; repaint(); } // a new source: no marks, no undo
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
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

private:
    juce::Point<float> toDesign(juce::Point<float>) const;
    float fractionToX(double fraction) const;           // design x of a position 0 .. 1 in the shown view
    double xToFraction(float x) const;
    float sampleToX(int sample, int length) const { return fractionToX(static_cast<double>(sample) / std::max(1, length)); }
    int xToSample(float x, int length) const;
    void updatePeaks();                                 // peaks of the shown view (zoom)
    void showRoleMenu(juce::Range<int> region, int current);
    void showBpmMenu();

    Values values;
    std::vector<std::pair<float, float>> peaks; // min / max per waveform column
    std::vector<std::pair<float, float>> sourcePeaks;
    juce::AudioBuffer<float> clipBuffer, sourceBuffer; // kept to draw the zoomed view
    double viewStart = 0.0, viewEnd = 1.0;
    juce::Range<int> selection;   // R16: right-drag selection waiting for a role (left-click it)
    bool roleClick = false;       // a left press on the selection / a mark: its role menu on release (no drag)
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
