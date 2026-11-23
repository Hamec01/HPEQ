#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace hpeq::ui
{
class RackPresetBar final : public juce::Component
{
public:
    RackPresetBar();
    void setCurrent(const juce::String& name,const juce::String& group,int index,int count);
    void setTooltip(const juce::String& text) {nameButton.setTooltip(text);}
    std::function<void(int)> onStep;
    std::function<void()> onChoose;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
    juce::Component& menuTarget() noexcept {return nameButton;}
private:
    class Key final : public juce::TextButton
    {
    public:
        explicit Key(const juce::String& text):juce::TextButton(text){}
        void paintButton(juce::Graphics&,bool,bool) override;
    };
    Key previous {"<"},next {">"},nameButton {"Init (empty project)"};
    juce::String groupText {"HAMLO RACK"};
    double wheel=0;
};
}
