#pragma once
#include "Standalone/ModulePluginProcessor.h"
#include "UI/BlackSteel.h"
#include "UI/ModuleHelp.h"
namespace hpeq::ui
{
class ShirotaV2Panel final:public juce::Component,public HelpTarget
{
public:
    ShirotaV2Panel(standalone::ModulePluginProcessor&,std::unique_ptr<juce::Component>);
    ~ShirotaV2Panel() override;
    void refresh();
    void paint(juce::Graphics&) override;
    void resized() override;
    juce::Rectangle<float> contentBounds() const override;
    juce::String helpKeyAt(juce::Point<float>) const override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    struct Look;struct Cell;
    void showPage(int);
    void layoutCells();
    void set(const juce::String&,double);
    juce::Point<float> design(juce::Point<float>) const;
    void setAngle(juce::Point<float>);
    standalone::ModulePluginProcessor& processor;
    std::unique_ptr<Look> look;
    juce::Component canvas;
    std::unique_ptr<juce::Component> legacy;
    juce::ComboBox engine,node;
    juce::TextButton restart{"RESTART PATH"};
    juce::TextButton power{"ON"};
    std::array<juce::TextButton,5> tabs;
    std::vector<std::unique_ptr<Cell>> cells;
    juce::Label help;
    juce::TooltipWindow tooltip{this,600};
    juce::Image background;
    int page=-1,selectedNode=0;
    float scale=1;
    juce::Point<float> offset;
    juce::RangedAudioParameter* dragging=nullptr;
};
}
