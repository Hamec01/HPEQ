#pragma once
#include "Rack/RackPresets.h"

namespace hpeq::rack
{
struct BrowserEntry
{
    juce::String id, name, collection, genre, source, description;
    juce::StringArray characters, modules;
    double bpm=0;
    int recipeIndex=-1;
    juce::File file;
    bool favourite=false;
};
struct BrowserFilter
{
    juce::String query, collection, genre, source, character, module;
    bool favouritesOnly=false, alphabetical=false;
    bool active() const;
};
class PresetCatalogue
{
public:
    explicit PresetCatalogue(juce::File userFolder);
    void refresh();
    const std::vector<BrowserEntry>& entries() const {return rows;}
    std::vector<int> filter(const BrowserFilter&) const;
    bool setFavourite(const juce::String& id,bool);
    bool setUserTags(const juce::String& id,const juce::String& genre,const juce::String& source,const juce::StringArray& characters);
    static bool matches(const BrowserEntry&,const BrowserFilter&);
private:
    juce::var preferences() const;
    bool writePreferences(const juce::var&);
    juce::File folder,settingsFile;
    std::vector<BrowserEntry> rows;
};
}
