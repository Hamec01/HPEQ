#pragma once
#include <memory>
#include <juce_gui_basics/juce_gui_basics.h>
#include "UI/ModuleHelp.h"
#include "Standalone/ModulePluginProcessor.h"
namespace hpeq::ui
{
class FaderV2Panel final : public juce::Component, public HelpTarget
{
public:
    explicit FaderV2Panel(standalone::ModulePluginProcessor&);
    ~FaderV2Panel() override;
    void refresh();
    void selectPage(int);
    void editPoint(int,double);
    bool action(standalone::FaderAction);
    bool apply();
    juce::Component* control(const juce::String&) const;
    void resized() override;
    juce::Rectangle<float> contentBounds() const override;
    juce::String helpKeyAt(juce::Point<float>) const override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
