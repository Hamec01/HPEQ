#pragma once
#include "Standalone/ModulePluginProcessor.h"
#include "UI/ModuleHelp.h"
#include "UI/BlackSteel.h"

namespace hpeq::ui
{
class ScorePanel final:public juce::Component,public HelpTarget,private juce::Timer
{
public:
    explicit ScorePanel(standalone::ModulePluginProcessor&);
    ~ScorePanel() override;
    void resized() override;
    juce::Rectangle<float> contentBounds() const override;
    juce::String helpKeyAt(juce::Point<float>) const override{return {};}
    void selectPage(int p){showPage(juce::jlimit(0,4,p));}
    std::function<void(juce::String)> onCategory;
    std::function<void()> onLoadStart;
private:
    class HardwareLook final:public juce::LookAndFeel_V4
    {
    public:
        HardwareLook();
        void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
        juce::Font getComboBoxFont(juce::ComboBox&) override;
        juce::Label* createSliderTextBox(juce::Slider&) override;
    };
    class Canvas final:public juce::Component
    {
    public:
        explicit Canvas(ScorePanel& p):owner(p){}
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        ScorePanel& owner;juce::Image background;
    };
    struct Control
    {
        juce::String id;
        int page=0,tap=-1;
        juce::Rectangle<int> bounds;
        juce::Label caption;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderLink;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboLink;
    };
    void add(juce::String id,int page,juce::Rectangle<int>,juce::Slider::SliderStyle style=juce::Slider::RotaryHorizontalVerticalDrag,int tap=-1);
    void showPage(int);
    void draw(juce::Graphics&);
    void drawKeyboard(juce::Graphics&);
    void keyboardClick(juce::Point<float>);
    juce::String help(const juce::String&) const;
    void timerCallback() override;
    standalone::ModulePluginProcessor& processor;
    HardwareLook look;
    Canvas canvas;
    std::vector<std::unique_ptr<Control>> controls;
    std::array<juce::TextButton,5> tabs,categories;
    std::array<juce::TextButton,8> tapSelectors;
    juce::TextButton loadStart{"LOAD START"},power{"ON"};
    juce::TextButton fillNotes{"FILL NOTES"};
    juce::ComboBox fillMode;
    juce::Slider fillStep;
    juce::ComboBox detailTap;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerLink;
    juce::TooltipWindow tooltip;
    int page=0,selectedTap=0;
    bool lastFl=true;
    juce::String category;
};
}
