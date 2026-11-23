#include "UI/RackPresetBar.h"

namespace hpeq::ui
{
void RackPresetBar::Key::paintButton(juce::Graphics& g,bool over,bool down)
{
    if(over||down){g.setColour(juce::Colour(down?0xff244b36:0xff162f24));g.fillRoundedRectangle(getLocalBounds().toFloat(),2.f);}
    g.setColour(juce::Colour(0xffa6efb5));g.setFont(juce::FontOptions(13.f));
    g.drawFittedText(getButtonText(),getLocalBounds().reduced(3,0),juce::Justification::centred,1);
}
RackPresetBar::RackPresetBar()
{
    for(auto* key:{&previous,&next,&nameButton})addAndMakeVisible(*key);
    previous.setTooltip("Previous preset in this bank");
    next.setTooltip("Next preset in this bank");
    nameButton.setTooltip("Choose a rack preset, or save your own");
    previous.setTitle("Previous rack preset");next.setTitle("Next rack preset");
    previous.onClick=[this]{if(onStep)onStep(-1);};
    next.onClick=[this]{if(onStep)onStep(1);};
    nameButton.onClick=[this]{if(onChoose)onChoose();};
}
void RackPresetBar::setCurrent(const juce::String& name,const juce::String& group,int index,int count)
{
    nameButton.setButtonText(name.isEmpty()?"Init (empty project)":name);
    groupText=group.toUpperCase()+(count>0?"   "+juce::String(index+1)+" / "+juce::String(count):juce::String());
    repaint();
}
void RackPresetBar::paint(juce::Graphics& g)
{
    const auto bounds=getLocalBounds().toFloat().reduced(.5f);
    g.setColour(juce::Colour(0xff080f0c));g.fillRoundedRectangle(bounds,4.f);
    g.setColour(juce::Colour(0xff425348));g.drawRoundedRectangle(bounds,4.f,1.f);
    g.setColour(juce::Colour(0xff6c9d7e));g.setFont(juce::FontOptions(9.f));
    g.drawFittedText(groupText,getLocalBounds().reduced(8,1).withHeight(13),juce::Justification::centred,1);
}
void RackPresetBar::resized()
{
    auto row=getLocalBounds().reduced(4,2).withTrimmedTop(12);
    previous.setBounds(row.removeFromLeft(25));next.setBounds(row.removeFromRight(25));nameButton.setBounds(row);
}
void RackPresetBar::mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails& details)
{
    if(details.isInertial)return;
    wheel+=details.deltaY;
    if(std::abs(wheel)>=.1){if(onStep)onStep(wheel>0?-1:1);wheel=0;}
}
}
