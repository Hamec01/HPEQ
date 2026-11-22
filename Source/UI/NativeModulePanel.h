#pragma once
#include "Core/FxProjectState.h"
#include "Rack/NativeApi.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace hpeq::ui
{
// Native parameter surface. Reads/edit the project, never the audio processor's
// mutable parameter tree. All controls use the pinned public ranges and labels.
class NativeModulePanel final : public juce::Component, private juce::Timer
{
public:
    explicit NativeModulePanel(FxType);
    ~NativeModulePanel() override;
    std::function<FxSlotState()> readSlot;
    std::function<void(const FxSlotState&)> onChange;
    std::function<void()> onGesture;
    std::function<native::FaderView()> readFader;
    std::function<native::Meters()> readMeters;
    std::function<juce::String()> readError;
    std::function<bool(int)> faderAction;
    void refresh();
    juce::Component* control(const juce::String&) const;
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    struct Control;
    void timerCallback() override;
    void parameterChanged(const juce::String&,double);
    void editStep(int lane,int step);
    void layoutControls();
    bool owned(const juce::String&) const;
    FxType type;
    juce::Viewport viewport;
    juce::Component content;
    juce::TextEditor search;
    juce::ComboBox section;
    juce::ComboBox sceneSelect;
    juce::Label heading,status;
    juce::OwnedArray<Control> parameters;
    juce::OwnedArray<juce::TextButton> steps;
    juce::OwnedArray<juce::Label> laneLabels;
    juce::OwnedArray<juce::Label> stepLabels;
    std::array<juce::TextButton,4> actions;
    juce::TextButton clearScene{"Clear scene"};
    native::Scene scene;
    juce::MemoryBlock lastScene;
    int lastSceneIndex=-1;
    bool blockGlitch=true;
    bool refreshing=false;
    int sceneBottom=0;
    bool modern=true;int currentMode=0;
    juce::String actionMessage;int messageTicks=0;
};
}
