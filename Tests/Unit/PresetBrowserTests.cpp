#include "Rack/PresetCatalogue.h"
#include "UI/PresetBrowser.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/RackPresetBar.h"
#include <set>

namespace hpeq::test
{
class PresetBrowserTests final:public juce::UnitTest
{
public:
    PresetBrowserTests():UnitTest("Rack preset browser","HPEQ"){}
    void runTest() override
    {
        const auto folder=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("HamloRack-browser-tests",{},false);
        struct Cleanup {juce::File file;~Cleanup(){file.deleteRecursively();}} cleanup{folder};
        beginTest("The catalogue covers every recipe with editorial character labels and actual processors, without changing calibrated data");
        rack::PresetCatalogue catalogue(folder);expectEquals(static_cast<int>(catalogue.entries().size()),270);
        std::set<juce::String> ids;juce::StringArray allModules;
        for(const auto& row:catalogue.entries()){
            expect(ids.insert(row.id).second);expect(!row.characters.isEmpty(),row.id);expect(!row.modules.isEmpty(),row.id);
            allModules.addArray(row.modules);
        }
        allModules.removeDuplicates(false);expectEquals(allModules.size(),10);
        // The first assertion used the structural report's normalized design
        // hash. bankSha256 hashes the actual committed resource bytes instead.
        expect(rack::bankSha256()=="c72f1d993c1d99fb495053457a749150fdc0768dfb8925ac653247192aa9dfb9");

        beginTest("Search combines tokens across name, source, character, description and processor, with Cyrillic aliases and zero-result handling");
        rack::BrowserFilter filter;filter.query="bAsEmEnT COMP";
        auto found=catalogue.filter(filter);expect(!found.empty());
        for(int index:found)expect(catalogue.entries()[static_cast<size_t>(index)].modules.contains("HamloComp"));
        filter={};filter.query=juce::String::fromUTF8(u8"\u0432\u043e\u043a\u0430\u043b");expect(!catalogue.filter(filter).empty());
        filter={};filter.collection="Rack Power";filter.genre="Trap";expectEquals(static_cast<int>(catalogue.filter(filter).size()),4);
        filter.source="Bass";filter.character="Warm";filter.module="HamloMag";expectEquals(static_cast<int>(catalogue.filter(filter).size()),1);
        filter.query="does-not-exist-at-all";expect(catalogue.filter(filter).empty());
        filter={};filter.alphabetical=true;found=catalogue.filter(filter);
        for(size_t i=1;i<found.size();++i)expect(catalogue.entries()[static_cast<size_t>(found[i-1])].name.compareNatural(catalogue.entries()[static_cast<size_t>(found[i])].name)<=0);

        beginTest("Favourites and user tags survive reopening and preserve other writers; preset audio/state bytes are untouched");
        expect(catalogue.setFavourite("PW001",true));rack::PresetCatalogue other(folder);expect(other.entries()[0].favourite);
        expect(other.setFavourite("PW002",true));expect(catalogue.setFavourite("PW003",true));other.refresh();
        filter={};filter.favouritesOnly=true;expectEquals(static_cast<int>(other.filter(filter).size()),3);
        const auto userFile=folder.getChildFile("My Voice.hpeq");const juce::String original="opaque preset state";expect(userFile.replaceWithText(original));
        other.refresh();expect(other.setUserTags("user:My Voice.hpeq","Soul","Lead Vocal",{"Warm","Smooth"}));
        rack::PresetCatalogue reopened(folder);filter={};filter.collection="User";filter.character="Warm";filter.source="Vocals";
        expectEquals(static_cast<int>(reopened.filter(filter).size()),1);expect(userFile.loadFileAsString()==original);
        expect(!reopened.setUserTags("PW001","Changed","Changed",{}));

        beginTest("Selection and filtering never load audio; explicit load, Enter, filtered stepping, empty results and errors behave consistently");
        ui::PresetBrowser browser(folder);browser.setSize(1000,700);int loads=0;juce::String loaded;
        browser.onLoad=[&](const rack::BrowserEntry& entry){++loads;loaded=entry.id;return true;};
        filter={};filter.collection="Rack Power";filter.genre="Trap";browser.setFilter(filter);
        expectEquals(browser.resultCount(),4);expectEquals(loads,0);expect(browser.selectedEntry()&&browser.selectedEntry()->id=="PW005");
        browser.loadSelected();expectEquals(loads,1);expect(loaded=="PW005");
        expect(browser.stepAndLoad(1,loaded,{}));expect(loaded=="PW006");expectEquals(loads,2);
        browser.keyPressed(juce::KeyPress(juce::KeyPress::returnKey));expectEquals(loads,3);
        filter.query="nothing matches this query";browser.setFilter(filter);expectEquals(browser.resultCount(),0);expect(browser.selectedEntry()==nullptr);
        browser.loadSelected();expectEquals(loads,3);expect(browser.stepAndLoad(1,loaded,{}));expectEquals(loads,3);
        bool closed=false;browser.onClose=[&]{closed=true;};browser.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));expect(closed);
        filter={};browser.setFilter(filter);expectEquals(browser.resultCount(),271);
        expect(!browser.stepAndLoad(1,{},{}));
        browser.onLoad=[](const rack::BrowserEntry&){return false;};browser.loadSelected();
        bool reportedFailure=false;
        for(auto* child:browser.getChildren())if(auto* text=dynamic_cast<juce::Label*>(child))
            if(text->getText().startsWith("Could not load preset"))reportedFailure=true;
        expect(reportedFailure,"A failed load must show an error");
        const auto blocked=folder.getChildFile("not-a-folder");expect(blocked.replaceWithText("file"));
        rack::PresetCatalogue unwritable(blocked);expect(!unwritable.setFavourite("PW001",true));

        beginTest("The real editor opens the library, loads an original native recipe, keeps filters for its header arrows, and supports undo");
        auto processor=std::make_unique<FxPluginProcessor>();auto editor=std::make_unique<FxPluginEditor>(*processor);
        editor->showPresetBrowser();ui::PresetBrowser* embedded=nullptr;ui::RackPresetBar* bar=nullptr;
        for(auto* child:editor->getChildren()){
            if(auto* value=dynamic_cast<ui::PresetBrowser*>(child))embedded=value;
            if(auto* value=dynamic_cast<ui::RackPresetBar*>(child))bar=value;
        }
        expect(embedded&&embedded->isVisible());expect(bar!=nullptr);
        if(embedded&&bar){
            filter={};filter.collection="Rack Power";filter.genre="Trap";embedded->setFilter(filter);embedded->loadSelected();
            expect(processor->getProject().rackPresetId=="PW005");
            embedded->onClose();expect(!embedded->isVisible());
            for(auto* child:bar->getChildren())if(auto* button=dynamic_cast<juce::Button*>(child);button&&button->getButtonText()==">")button->onClick();
            expect(processor->getProject().rackPresetId=="PW006");expect(processor->getUndoManager().undo());
            expect(processor->getProject().rackPresetId=="PW005");
            editor->showPresetBrowser();expect(embedded->getFilter().genre=="Trap");
        }
    }
};
static PresetBrowserTests presetBrowserTests;
}
