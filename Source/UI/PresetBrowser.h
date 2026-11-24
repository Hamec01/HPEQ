#pragma once
#include "Rack/PresetCatalogue.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace hpeq::ui
{
class PresetBrowser final:public juce::Component,private juce::ListBoxModel
{
public:
    explicit PresetBrowser(juce::File userFolder);
    ~PresetBrowser() override;
    std::function<bool(const rack::BrowserEntry&)> onLoad;
    std::function<void()> onClose,onSave,onInit,onFolder,onFilterChanged;
    void open(const juce::String& currentId,const juce::String& currentName);
    void setFilter(const rack::BrowserFilter&);
    rack::BrowserFilter getFilter() const {return criteria;}
    int resultCount() const {return static_cast<int>(visible.size());}
    const rack::BrowserEntry* selectedEntry() const;
    void loadSelected();
    bool stepAndLoad(int direction,const juce::String& currentId,const juce::String& currentName);
    int positionOf(const juce::String& id,const juce::String& name) const;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    class Search final:public juce::TextEditor
    {
    public:
        std::function<void(int)> onNavigate;
        bool keyPressed(const juce::KeyPress& key) override;
    };
    void applyFilter();
    void fillFacets();
    void updateDetails();
    void editTags();
    void moveSelection(int);
    int getNumRows() override {return resultCount();}
    void paintListBoxItem(int,juce::Graphics&,int,int,bool) override;
    void selectedRowsChanged(int) override {updateDetails();}
    void listBoxItemDoubleClicked(int,const juce::MouseEvent&) override {loadSelected();}
    void returnKeyPressed(int) override {loadSelected();}
    void deleteKeyPressed(int) override {}
    rack::PresetCatalogue catalogue;
    rack::BrowserFilter criteria;
    std::vector<int> visible;
    juce::String loadedId,loadedName;
    juce::ListBox list {"Preset results",this};
    Search search;
    juce::ComboBox collection,genre,source,character,module,sort;
    juce::Label collectionLabel,genreLabel,sourceLabel,characterLabel,moduleLabel;
    juce::Label title,subtitle,count,detailName,detailInfo,detailDescription,status,emptyResults;
    juce::TextEditor detailModules;
    juce::ToggleButton favourites {"Favourites only"};
    juce::TextButton close {"Close  [Esc]"},reset {"Reset filters"},load {"Load preset  [Enter]"},favourite {"Add favourite"},tags {"Edit user tags..."};
    juce::TextButton save {"Save preset..."},init {"Init"},folder {"Preset folder"};
    std::unique_ptr<juce::AlertWindow> tagsDialog;
    juce::Rectangle<int> panel,sidebar,details,listBounds;
    bool updating=false;
};
}
