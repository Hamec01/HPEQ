#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Standalone/ModuleVariant.h"
#include "UI/ModuleHelp.h"

namespace hpeq::ui
{
// The standalone Verb surface. Browser actions are separate from sound edits.
// Native sliders/buttons retain keyboard and screen-reader interaction.
class VerbV2Panel final : public juce::Component, public HelpTarget
{
public:
    explicit VerbV2Panel(const standalone::ModuleSpec&);
    ~VerbV2Panel() override;

    std::function<double(const juce::String&)> readValue;
    std::function<double()> readSampleRate;
    std::function<void(const juce::String&, double)> onChange;
    std::function<void(const juce::String&)> onGestureStart;
    std::function<void()> onGestureEnd;
    std::function<void(const juce::String&)> onMaterial;
    std::function<void(const juce::String&)> onLoadStart;
    std::function<void(bool)> onMixLock;

    void refresh(bool mixLocked, float outputDb, float duckDb);
    void showAdvanced(bool);
    bool isAdvanced() const noexcept;
    void showAdvancedSection(int);
    void selectMaterial(const juce::String&);
    const juce::String& selectedMaterial() const noexcept;
    juce::Rectangle<float> contentBounds() const override;
    juce::String helpKeyAt(juce::Point<float>) const override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerbV2Panel)
};
}
