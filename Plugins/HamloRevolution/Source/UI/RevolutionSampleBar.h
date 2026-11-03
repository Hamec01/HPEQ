#pragma once
#include "Standalone/ModulePluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
namespace hpeq::ui
{
class RevolutionSampleBar final:public juce::Component,public juce::FileDragAndDropTarget
{
public:
 explicit RevolutionSampleBar(standalone::ModulePluginProcessor&);
 ~RevolutionSampleBar() override {chooser.reset();}
 bool isTransition() const noexcept;
 void renderTransition(bool capture);
 void refresh();void paint(juce::Graphics&) override;void resized() override;
 bool isInterestedInFileDrag(const juce::StringArray&) override;
 void filesDropped(const juce::StringArray&,int,int) override;
 void loadFile(const juce::File&);
 std::function<void()> onTransitionRendered;
 void openLoad();void openSave();
 juce::Rectangle<int> controlBounds(int index) const;
private:
 class Drag final:public juce::TextButton
 {
 public:Drag():juce::TextButton("DRAG WAV"){}std::function<void()> dragFile;
 void mouseDrag(const juce::MouseEvent& e)override{if(e.getDistanceFromDragStart()>5&&!dragged&&isEnabled()){dragged=true;if(dragFile)dragFile();}}
 void mouseDown(const juce::MouseEvent& e)override{dragged=false;juce::TextButton::mouseDown(e);}
 bool dragged=false;
 } drag;
 standalone::ModulePluginProcessor& processor;
 juce::TextButton load{"LOAD SAMPLE"},play{"PLAY"},loop{"LOOP"},input{"USE INPUT"},render{"RENDER WAV"},save{"EXPORT WAV"};
 juce::ComboBox tail;
 std::unique_ptr<juce::FileChooser> chooser;
 standalone::RevolutionSample::Status values;
 juce::String fileMessage;
 bool lastTransition=false;
};
}
