#pragma once
#include "Standalone/ModulePluginProcessor.h"
#include "UI/BlackSteel.h"
#include "UI/ModuleHelp.h"

namespace hpeq::ui
{
class ModernZastavaPanel final:public juce::Component,public HelpTarget
{
public:
    explicit ModernZastavaPanel(standalone::ModulePluginProcessor&);
    ~ModernZastavaPanel() override;
    void refresh();
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::Rectangle<float> contentBounds() const override;
    juce::String helpKeyAt(juce::Point<float>) const override;
    juce::Component* control(const juce::String&) const;
    bool layoutValid() const;
private:
    struct Look;struct Control;
    struct Canvas:juce::Component
    {explicit Canvas(ModernZastavaPanel& p):owner(p){}void paint(juce::Graphics& g) override{owner.paintStage(g);}ModernZastavaPanel& owner;};
    void paintStage(juce::Graphics&);
    void arrange();
    void immediate(const juce::String&,double);
    void mouseMove(const juce::MouseEvent&) override;
    void mouseEnter(const juce::MouseEvent&) override;
    standalone::ModulePluginProcessor& processor;
    std::unique_ptr<Look> look;
    Canvas canvas;
    std::vector<std::unique_ptr<Control>> controls;
    std::array<juce::TextButton,4> modes;
    std::array<juce::TextButton,2> profiles;
    std::array<juce::TextButton,3> tabs;
    juce::TextButton power,listen,init,all,off,alternate;
    juce::Label help;
    juce::Image background;
    int shownMode=-1,shownView=-1,shownTab=-1,shownSteps=-1,shownSync=-1;
    juce::String context;
};
}
