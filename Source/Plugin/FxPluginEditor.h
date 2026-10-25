#pragma once

#include <array>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "UI/ModuleTile.h"
#include "UI/TubeButton.h"
#include "UI/SteelLookAndFeel.h"

namespace hpeq
{
class FxPluginProcessor;

namespace ui
{
class AnalysisModel;
class WaveformPanel;
class LayerControls;
class SpectrumWorkspace;
class NodeList;
class NodeInspector;
struct Selection;
} // namespace ui

// M5 editor, laid out after the owner's sketch: header (name, A/B, undo/redo), top strip (real
// input/processed waveform, layer buttons 1-10 in two rows, meters), the spectrum workspace, and the
// node list + inspector of the ONE selected processor. Things without DSP yet are shown disabled and
// say which milestone brings them; nothing pretends to work.
class FxPluginEditor final : public juce::AudioProcessorEditor, private juce::Timer, private juce::ChangeListener
{
public:
    explicit FxPluginEditor(FxPluginProcessor&);
    ~FxPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // For tests and screenshots: pull pending capture and repaint now.
    void updateAnalysisNow();
    // Select a node as a click would (fx: FX region, else EQ point), and pick the workspace mode.
    void selectNode(bool fx, std::uint32_t id, int slot = -1); // slot -1 = module grid
    // The layer's MIX chain (D7), as a click on its row would select it.
    void selectMix(int slot = -1);
    bool isMixSelected() const noexcept; // tests
    void showDifference(bool difference);

private:
    class LevelMeter;
    class LoudnessScreen;

    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;
    void selectSoftwareRenderer();
    void renderBackground();
    void selectionChanged();
    void updateHeader();
    void updateLayerButtons();
    void layerSelected();

    FxPluginProcessor& processor;
    std::unique_ptr<ui::Selection> selection;
    std::unique_ptr<ui::AnalysisModel> analysis;
    std::unique_ptr<ui::WaveformPanel> waveform;
    std::unique_ptr<ui::SpectrumWorkspace> workspace;
    std::unique_ptr<ui::NodeList> nodeList;
    std::unique_ptr<ui::NodeInspector> inspector;
    std::unique_ptr<LevelMeter> inputMeter, outputMeter;
    std::unique_ptr<ui::LayerControls> layerControls;
    std::unique_ptr<LoudnessScreen> loudnessScreen;
    std::unique_ptr<ui::TubeButton> leninButton, stalinButton; // BRING LENIN / STALIN BACK
    void updateTubeButtons();

    juce::Label title, subtitle, unavailable;
    juce::TextButton loudnessReset { "Reset" };
    juce::TextButton abA { "A" }, abB { "B" }, copyAB { "A>B" }, undo { "Undo" }, redo { "Redo" };
    juce::TextButton waveOverlay { "Overlay" }, waveDelta { "Delta" }, freeze { "Freeze" };
    juce::TextButton tabSpectrum { "Spectrum" }, tabDifference { "Difference" };
    juce::ComboBox view;
    std::array<juce::TextButton, 10> layerButtons;
    int lastShownLayer = -1;
    bool largeLayout = false;
    juce::Slider outputGain;
    juce::Label outputGainLabel;
    juce::ToggleButton bypass { "Bypass" };
    juce::TooltipWindow tooltips { this };
    juce::SharedResourcePointer<ui::SteelLookAndFeel> steelLook; // EQ pass: the Black Steel style
    juce::SharedResourcePointer<ui::ModuleThumbnails> thumbnails; // the module pages in miniature, while a window is open
    juce::Image background;                                      // the worn steel plate, rendered on resize
    juce::Rectangle<int> plateArea;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPluginEditor)
};
} // namespace hpeq
