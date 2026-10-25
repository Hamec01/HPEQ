#pragma once

#include <map>
#include <memory>
#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/FxProjectState.h"

namespace hpeq::ui
{
// Owner, 2026-10-25: a module is shown as a small picture of its own page, a little darkened, with its
// name written clearly over it. The pictures are the real Black Steel pages rendered small, once per
// module type, kept while an HPEQ window holds this cache (FxPluginEditor).
struct ModuleThumbnails
{
    std::map<FxType, juce::Image> images;
};
juce::Image moduleThumbnail(FxType);
// The picture for menus (PopupMenu::addItem's image).
std::unique_ptr<juce::Drawable> moduleIcon(FxType);

// A module place in the grid: the module's page in miniature (darkened), its name over it, the
// function under the name and the number in the corner, an amber stripe when it runs; an empty place
// is an engraved "+". The button text stays what it was ("1   HamloComp · Compressor", "+").
class ModuleTile final : public juce::TextButton
{
public:
    ModuleTile(const juce::String& text, int number, std::optional<FxType> type, bool enabled);
    void paintButton(juce::Graphics&, bool over, bool down) override;

private:
    int number;
    std::optional<FxType> type;
    bool enabled;
    juce::SharedResourcePointer<ModuleThumbnails> cache;
};
} // namespace hpeq::ui
