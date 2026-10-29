#pragma once

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hpeq::ui
{
// The preset screen of the Hamlo line (owner 2026-10-28): a small green CRT at the same place on every plug-in's
// page - the top left, design rectangle kDesignBounds of the 1180 x 820 stage. Three lines:
//   GENRE v                     GROUP v      (click: choose the genre / the group)
//   <        the preset's name          >    (arrows: previous / next within the genre and group; click the
//                                             name: every preset of the filter in a menu)
//   what the preset is for            12 / 50
// The plug-in gives it the list (Entry: host program number, genre, group, name, note) and the current program;
// onSelect asks for a program. "Factory" is the genre of the presets that have none (HPEQ's and the v2 ones).
class PresetScreen final : public juce::Component
{
public:
    struct Entry
    {
        int program = 0; // the host program it loads
        juce::String genre, group, name, note;
    };

    static inline const juce::Rectangle<float> kDesignBounds { 56.0f, 20.0f, 336.0f, 116.0f };
    static constexpr const char* kFactory = "Factory";

    PresetScreen();

    void setEntries(std::vector<Entry>);
    // the program now loaded (0 = none / Default), and whether a knob moved since
    void setCurrent(int program, bool modified);
    std::function<void(int program)> onSelect;

    // place it on the page: `stage` = where the 1180 x 820 design stage is drawn, in the parent's coordinates
    void placeOnStage(juce::Rectangle<float> stage);

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // for tests: the presets of the current filter, the filter, and the step the arrows take
    std::vector<int> filtered() const;
    const juce::String& genre() const noexcept { return genreFilter; }
    const juce::String& group() const noexcept { return groupFilter; }
    void setFilter(const juce::String& genre, const juce::String& group);
    void step(int direction);

private:
    juce::StringArray genres() const;
    juce::StringArray groupsOf(const juce::String& genre) const;
    const Entry* entryFor(int program) const;
    juce::Point<float> toDesign(juce::Point<float>) const;
    void chooseGenre();
    void chooseGroup();
    void choosePreset();

    std::vector<Entry> entries;
    juce::String genreFilter { "All" }, groupFilter { "All" };
    int current = 0;
    bool modified = false, shown = false;
};
} // namespace hpeq::ui
