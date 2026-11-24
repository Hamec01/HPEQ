#pragma once
#include "Plugin/FxPluginProcessor.h"
namespace hpeq::ui {
class MacroRoutingPanel final:public juce::Component,private juce::Timer {
public:
    explicit MacroRoutingPanel(FxPluginProcessor&);
    ~MacroRoutingPanel() override;
    void resized() override;void paint(juce::Graphics&) override;
    std::function<void()> onClose;
private:
    struct Row;
    void timerCallback() override;
    void assign(int);void ranges(int);
    void editRange(int,FxPluginProcessor::MacroDestination);
    FxPluginProcessor& processor;
    juce::Label heading,hint;
    juce::TextButton close{"Close"};
    std::array<std::unique_ptr<Row>,control::kMacros> rows;
    std::unique_ptr<juce::AlertWindow> dialog;
};
}
