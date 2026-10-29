#include "UI/PresetScreen.h"

#include <algorithm>

#include "UI/BlackSteel.h"

namespace hpeq::ui
{
namespace
{
// design coordinates inside kDesignBounds (336 x 116)
const juce::Rectangle<float> kBezel { 0.0f, 0.0f, 336.0f, 116.0f };
const juce::Rectangle<float> kScreen { 12.0f, 10.0f, 312.0f, 88.0f };
const juce::Rectangle<float> kGenre { 22.0f, 14.0f, 150.0f, 18.0f }, kGroup { 172.0f, 14.0f, 142.0f, 18.0f };
const juce::Rectangle<float> kPrev { 16.0f, 34.0f, 30.0f, 32.0f }, kNext { 290.0f, 34.0f, 30.0f, 32.0f };
const juce::Rectangle<float> kName { 46.0f, 34.0f, 244.0f, 32.0f };
const juce::Rectangle<float> kNote { 22.0f, 72.0f, 236.0f, 18.0f }, kCount { 258.0f, 72.0f, 56.0f, 18.0f };
constexpr const char* kAll = "All";

// the largest size from `size` down to 60 % of it at which `text` fits `width` (the CRT's monospace font)
float fitted(const juce::String& text, float width, float size)
{
    const float full = juce::GlyphArrangement::getStringWidth(juce::Font(steel::monoFont(size)), text);
    return full <= width ? size : juce::jmax(size * 0.6f, size * width / full);
}

void arrow(juce::Graphics& g, juce::Rectangle<float> r, bool left, bool lit)
{
    juce::Path p;
    const auto c = r.getCentre();
    const float w = 7.0f, h = 9.0f;
    if (left)
        p.addTriangle(c.x + w * 0.5f, c.y - h, c.x + w * 0.5f, c.y + h, c.x - w * 0.5f, c.y);
    else
        p.addTriangle(c.x - w * 0.5f, c.y - h, c.x - w * 0.5f, c.y + h, c.x + w * 0.5f, c.y);
    g.setColour(steel::phosphor.withAlpha(lit ? 0.9f : 0.3f));
    g.fillPath(p);
    steel::glow(g, p, steel::phosphor, 1.0f, lit ? 0.6f : 0.2f);
}
} // namespace

PresetScreen::PresetScreen()
{
    setRepaintsOnMouseActivity(false);
}

void PresetScreen::setEntries(std::vector<Entry> list)
{
    for (auto& e : list)
    {
        if (e.genre.isEmpty())
            e.genre = kFactory;
        if (e.group.isEmpty())
            e.group = "General";
    }
    entries = std::move(list);
    repaint();
}

const PresetScreen::Entry* PresetScreen::entryFor(int program) const
{
    for (const auto& e : entries)
        if (e.program == program)
            return &e;
    return nullptr;
}

void PresetScreen::setCurrent(int program, bool isModified)
{
    if (shown && program == current && isModified == modified)
        return;
    if (program != current || !shown)
        if (const auto* e = entryFor(program))
        {
            // a program chosen elsewhere (the host's list): show its genre and group unless the filter has it
            const bool genreOk = genreFilter == kAll || genreFilter == e->genre;
            const bool groupOk = groupFilter == kAll || groupFilter == e->group;
            if (!genreOk || !groupOk || !shown) // and at first: the screen opens on the program's genre and group
            {
                genreFilter = e->genre;
                groupFilter = e->group;
            }
        }
    current = program;
    modified = isModified;
    shown = true;
    repaint();
}

juce::StringArray PresetScreen::genres() const
{
    juce::StringArray list { kAll };
    for (const auto& e : entries)
        list.addIfNotAlreadyThere(e.genre);
    return list;
}

juce::StringArray PresetScreen::groupsOf(const juce::String& genre) const
{
    juce::StringArray list { kAll };
    for (const auto& e : entries)
        if (genre == kAll || e.genre == genre)
            list.addIfNotAlreadyThere(e.group);
    return list;
}

std::vector<int> PresetScreen::filtered() const
{
    std::vector<int> out;
    for (const auto& e : entries)
        if ((genreFilter == kAll || e.genre == genreFilter) && (groupFilter == kAll || e.group == groupFilter))
            out.push_back(e.program);
    return out;
}

void PresetScreen::setFilter(const juce::String& genre, const juce::String& group)
{
    genreFilter = genres().contains(genre) ? genre : juce::String(kAll);
    groupFilter = groupsOf(genreFilter).contains(group) ? group : juce::String(kAll);
    repaint();
}

void PresetScreen::step(int direction)
{
    const auto list = filtered();
    if (list.empty())
        return;
    const auto it = std::find(list.begin(), list.end(), current);
    int index = 0;
    if (it == list.end())
        index = direction > 0 ? 0 : static_cast<int>(list.size()) - 1;
    else
        index = (static_cast<int>(it - list.begin()) + direction + static_cast<int>(list.size())) % static_cast<int>(list.size());
    if (onSelect)
        onSelect(list[static_cast<size_t>(index)]);
}

void PresetScreen::placeOnStage(juce::Rectangle<float> stage)
{
    const float s = stage.getWidth() / 1180.0f;
    setBounds(juce::Rectangle<float>(stage.getX() + kDesignBounds.getX() * s, stage.getY() + kDesignBounds.getY() * s, kDesignBounds.getWidth() * s,
                                     kDesignBounds.getHeight() * s)
                  .toNearestInt());
}

juce::Point<float> PresetScreen::toDesign(juce::Point<float> p) const
{
    const float s = static_cast<float>(getWidth()) / kDesignBounds.getWidth();
    return p / juce::jmax(0.001f, s);
}

void PresetScreen::paint(juce::Graphics& g)
{
    g.addTransform(juce::AffineTransform::scale(static_cast<float>(getWidth()) / kDesignBounds.getWidth()));
    steel::crtBezel(g, kBezel, kScreen, "PRESETS", kBezel.getBottom() - 15.0f);
    steel::crtBegin(g, kScreen);
    const auto* e = entryFor(current);
    const auto list = filtered();
    steel::crtText(g, genreFilter.toUpperCase() + juce::String::fromUTF8(" \xe2\x96\xbe"), kGenre, 12.0f, juce::Justification::centredLeft, 0.75f);
    steel::crtText(g, groupFilter.toUpperCase() + juce::String::fromUTF8(" \xe2\x96\xbe"), kGroup, 12.0f, juce::Justification::centredRight, 0.75f);
    g.setColour(steel::phosphorDim.withAlpha(0.35f));
    g.drawHorizontalLine(juce::roundToInt(kGenre.getBottom() + 1.0f), kScreen.getX() + 6.0f, kScreen.getRight() - 6.0f);
    arrow(g, kPrev, true, !list.empty());
    arrow(g, kNext, false, !list.empty());
    const juce::String name = e != nullptr ? e->name + (modified ? " *" : "") : juce::String(current == 0 ? "Default" : "-");
    steel::crtText(g, name, kName, fitted(name, kName.getWidth(), 17.0f), juce::Justification::centred, 0.95f);
    if (e != nullptr)
        steel::crtText(g, e->note, kNote, fitted(e->note, kNote.getWidth(), 10.5f), juce::Justification::centredLeft, 0.6f);
    const auto at = std::find(list.begin(), list.end(), current);
    const juce::String count = at == list.end() ? "- / " + juce::String(static_cast<int>(list.size()))
                                                : juce::String(static_cast<int>(at - list.begin()) + 1) + " / " + juce::String(static_cast<int>(list.size()));
    steel::crtText(g, count, kCount, 11.0f, juce::Justification::centredRight, 0.7f);
    steel::crtEnd(g, kScreen);
}

void PresetScreen::mouseDown(const juce::MouseEvent& event)
{
    const auto d = toDesign(event.position);
    if (kPrev.contains(d))
        step(-1);
    else if (kNext.contains(d))
        step(1);
    else if (kGenre.contains(d))
        chooseGenre();
    else if (kGroup.contains(d))
        chooseGroup();
    else if (kScreen.contains(d))
        choosePreset();
}

void PresetScreen::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY != 0.0f)
        step(wheel.deltaY > 0.0f ? -1 : 1);
}

void PresetScreen::chooseGenre()
{
    juce::PopupMenu menu;
    const auto list = genres();
    for (int i = 0; i < list.size(); ++i)
        menu.addItem(i + 1, list[i], true, list[i] == genreFilter);
    juce::Component::SafePointer<PresetScreen> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [self, list](int r)
    {
        if (self != nullptr && r > 0)
            self->setFilter(list[r - 1], kAll);
    });
}

void PresetScreen::chooseGroup()
{
    juce::PopupMenu menu;
    const auto list = groupsOf(genreFilter);
    for (int i = 0; i < list.size(); ++i)
        menu.addItem(i + 1, list[i], true, list[i] == groupFilter);
    juce::Component::SafePointer<PresetScreen> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [self, list](int r)
    {
        if (self != nullptr && r > 0)
            self->setFilter(self->genreFilter, list[r - 1]);
    });
}

void PresetScreen::choosePreset()
{
    // the filter's presets; with GROUP "All" one sub-menu per group (and per genre with GENRE "All")
    juce::PopupMenu menu;
    const auto add = [this](juce::PopupMenu& m, const Entry& e) { m.addItem(e.program + 1, e.name, true, e.program == current); };
    for (const auto& genre : genres())
    {
        if (genre == kAll || (genreFilter != kAll && genre != genreFilter))
            continue;
        juce::PopupMenu genreMenu;
        for (const auto& group : groupsOf(genre))
        {
            if (group == kAll || (groupFilter != kAll && group != groupFilter))
                continue;
            juce::PopupMenu groupMenu;
            for (const auto& e : entries)
                if (e.genre == genre && e.group == group)
                    add(groupMenu, e);
            if (groupFilter != kAll)
                genreMenu = groupMenu;
            else
                genreMenu.addSubMenu(group, groupMenu);
        }
        if (genreFilter != kAll)
            menu = genreMenu;
        else
            menu.addSubMenu(genre, genreMenu);
    }
    juce::Component::SafePointer<PresetScreen> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [self](int r)
    {
        if (self != nullptr && r > 0 && self->onSelect)
            self->onSelect(r - 1);
    });
}
} // namespace hpeq::ui
