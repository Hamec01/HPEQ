#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/Revolution.h"

namespace hpeq::ui
{
// R4 (docs/REVOLUTION_V2.md section 9): the SIMPLE page of HamloRevolution. CONNECT TO HPDG, GENRE, five big knobs
// (INTENT, AMOUNT, VARIATION, GROOVE, MIX), FILL, TARGET, GENERATE, LOCK and a read-only preview of the seven lanes
// the generator wrote. Drawn in the ADVANCED page's 1180 x 1280 frame (scaled the same, so the preset screen sits at
// the same place); the editor owns both pages and shows one.
class HamloRevolutionSimplePage final : public juce::Component, public juce::TooltipClient
{
public:
    enum Knob { Intent, Amount, Variation, Groove, Mix, NumKnobs };
    enum class Link { Off, Searching, Linked, Lost, Update };

    struct Values
    {
        std::array<double, NumKnobs> knobs { 0.3, 0.5, 0.25, 1.0, 1.0 };
        int genre = 0, fill = 3, target = 2;
        bool lock = false;
        int shownGenre = 1, shownIntent = 0; // what the generator resolved (AUTO -> a genre)
        Link link = Link::Off;
        juce::String linkText, source;      // the status line; where the hits came from
        std::array<std::array<std::uint8_t, revolution::kSteps>, revolution::kLanes> preview {};
        std::array<int, revolution::kLanes> bands {};
        int step = -1;
        bool glitchBlock = false; // GLITCH 2: name the GLITCH steps as block actions
    };

    HamloRevolutionSimplePage();

    static const char* keyFor(Knob);
    static double defaultFor(Knob);
    void setValues(const Values&);
    const Values& getValues() const noexcept { return values; }

    std::function<void(Knob)> onKnobGestureStart;
    std::function<void(Knob, double)> onKnobChange;
    std::function<void()> onKnobGestureEnd;
    std::function<void(const char* id, int value)> onChoice; // simpleGenre, simpleFill, simpleTarget, simpleLock
    std::function<void()> onGenerate, onAdvanced, onConnect, onConnectMenu;

    // the controls, also for tests
    void pressGenerate();
    void pressConnect();
    void pressAdvanced();
    void chooseGenre(int);
    void chooseFill(int);
    void chooseTarget(int);
    void toggleLock();
    void setKnob(Knob, double);

    // R6: the help key of the control at a point (component coordinates), and its text as the tooltip
    juce::String helpKeyAt(juce::Point<float> local) const;
    juce::String getTooltip() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

private:
    juce::Point<float> toDesign(juce::Point<float>) const;
    void drawStatic(juce::Graphics&) const;

    Values values;
    float scale = 1.0f;
    juce::Point<float> offset;
    juce::Image staticLayer;
    int dragKnob = -1;
    double dragStart = 0.0;
    float dragStartY = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HamloRevolutionSimplePage)
};
} // namespace hpeq::ui
