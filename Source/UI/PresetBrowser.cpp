#include "UI/PresetBrowser.h"

namespace hpeq::ui
{
namespace
{
const juce::Colour ink{0xffd8e3d9},dim{0xff8b9d92},green{0xffa6efb5},edge{0xff35463d};
void label(juce::Label& component,const juce::String& text,float size,juce::Colour colour)
{
    component.setText(text,juce::dontSendNotification);component.setFont(juce::FontOptions(size));
    component.setColour(juce::Label::textColourId,colour);component.setMinimumHorizontalScale(1.f);
}
void options(juce::ComboBox& box,juce::StringArray names,const juce::String& selected)
{
    if(selected.isNotEmpty())names.addIfNotAlreadyThere(selected);
    names.removeEmptyStrings();names.removeDuplicates(false);names.sort(true);
    box.clear(juce::dontSendNotification);box.addItem("All",1);
    box.addItemList(names,2);box.setSelectedId(selected.isEmpty()?1:names.indexOf(selected)+2,juce::dontSendNotification);
    if(box.getSelectedId()==0)box.setSelectedId(1,juce::dontSendNotification);
}
juce::String selected(const juce::ComboBox& box) {return box.getSelectedId()<=1?juce::String():box.getText();}
}
bool PresetBrowser::Search::keyPressed(const juce::KeyPress& key)
{
    if(key==juce::KeyPress::upKey||key==juce::KeyPress::downKey)
    {if(onNavigate)onNavigate(key==juce::KeyPress::upKey?-1:1);return true;}
    return TextEditor::keyPressed(key);
}
PresetBrowser::PresetBrowser(juce::File userFolder):catalogue(std::move(userFolder))
{
    setWantsKeyboardFocus(true);setAlwaysOnTop(true);
    for(juce::Component* component:std::initializer_list<juce::Component*>{&list,&search,&collection,&genre,&source,&character,&module,&sort,
        &collectionLabel,&genreLabel,&sourceLabel,&characterLabel,&moduleLabel,&title,&subtitle,&count,&detailName,&detailInfo,&detailDescription,&detailModules,&status,&emptyResults,
        &favourites,&close,&reset,&load,&favourite,&tags,&save,&init,&folder})addAndMakeVisible(component);
    label(title,"PRESET LIBRARY",23,green);label(subtitle,"Find the right starting point. Shape it in your rack.",12,dim);
    label(collectionLabel,"COLLECTION",11,dim);label(genreLabel,"GENRE",11,dim);label(sourceLabel,"SOURCE / APPLICATION",11,dim);
    label(characterLabel,"CHARACTER",11,dim);label(moduleLabel,"PROCESSOR",11,dim);
    label(count,{},12,dim);label(detailName,{},19,green);label(detailInfo,{},12,dim);
    label(detailDescription,{},14,ink);label(status,"Select a preset. Enter or double-click to load.",12,dim);
    detailModules.setMultiLine(true);detailModules.setReadOnly(true);detailModules.setScrollbarsShown(true);detailModules.setFont(juce::FontOptions(12));
    detailModules.setColour(juce::TextEditor::textColourId,dim);detailModules.setColour(juce::TextEditor::backgroundColourId,juce::Colours::transparentBlack);
    detailModules.setColour(juce::TextEditor::outlineColourId,juce::Colours::transparentBlack);detailModules.setColour(juce::TextEditor::focusedOutlineColourId,juce::Colours::transparentBlack);
    label(emptyResults,"No presets match these filters.\nReset filters to see the full library.",14,dim);emptyResults.setJustificationType(juce::Justification::centred);
    emptyResults.setInterceptsMouseClicks(false,false);
    for(auto* component:{&detailName,&detailInfo,&detailDescription})component->setJustificationType(juce::Justification::topLeft);
    search.setTextToShowWhenEmpty("Search names, descriptions, tags or processors...",dim);search.setTitle("Search rack presets");
    search.setFont(juce::FontOptions(15));search.setSelectAllWhenFocused(true);
    search.onTextChange=[this]{if(!updating){criteria.query=search.getText();applyFilter();}};
    search.onNavigate=[this](int direction){moveSelection(direction);};
    search.onReturnKey=[this]{loadSelected();};search.onEscapeKey=[this]{if(onClose)onClose();};
    list.setRowHeight(66);list.setColour(juce::ListBox::backgroundColourId,juce::Colour(0xff0c1410));list.setOutlineThickness(0);
    sort.addItem("Library order",1);sort.addItem("Name A-Z",2);sort.setSelectedId(1,juce::dontSendNotification);
    sort.onChange=[this]{if(!updating){criteria.alphabetical=sort.getSelectedId()==2;applyFilter();}};
    const auto changed=[this]{if(!updating){criteria.collection=selected(collection);criteria.genre=selected(genre);criteria.source=selected(source);
        criteria.character=selected(character);criteria.module=selected(module);criteria.favouritesOnly=favourites.getToggleState();applyFilter();}};
    for(auto* box:{&collection,&genre,&source,&character,&module})box->onChange=changed;
    favourites.onClick=changed;
    reset.onClick=[this]{setFilter({});search.grabKeyboardFocus();};
    close.onClick=[this]{if(onClose)onClose();};load.onClick=[this]{loadSelected();};
    favourite.onClick=[this]{if(const auto* entry=selectedEntry()){
        if(catalogue.setFavourite(entry->id,!entry->favourite))applyFilter();
        else label(status,"Could not save favourites. Check your preset folder permissions.",12,juce::Colours::orange);}};
    tags.onClick=[this]{editTags();};
    save.onClick=[this]{if(onSave)onSave();};init.onClick=[this]{if(onInit)onInit();};folder.onClick=[this]{if(onFolder)onFolder();};
    fillFacets();applyFilter();
}
PresetBrowser::~PresetBrowser() {list.setModel(nullptr);}
void PresetBrowser::fillFacets()
{
    juce::StringArray collections,genres,sources,characters,processors;
    for(const auto& row:catalogue.entries()) {collections.add(row.collection);genres.add(row.genre);sources.add(row.source);characters.addArray(row.characters);processors.addArray(row.modules);}
    options(collection,collections,criteria.collection);options(genre,genres,criteria.genre);options(source,sources,criteria.source);
    options(character,characters,criteria.character);options(module,processors,criteria.module);
}
void PresetBrowser::setFilter(const rack::BrowserFilter& filter)
{
    const juce::ScopedValueSetter<bool> guard(updating,true);criteria=filter;
    search.setText(criteria.query,false);favourites.setToggleState(criteria.favouritesOnly,juce::dontSendNotification);
    sort.setSelectedId(criteria.alphabetical?2:1,juce::dontSendNotification);fillFacets();applyFilter();
}
const rack::BrowserEntry* PresetBrowser::selectedEntry() const
{
    const int row=list.getSelectedRow();if(row<0||row>=resultCount())return nullptr;
    const auto index=static_cast<size_t>(visible[static_cast<size_t>(row)]);
    return index<catalogue.entries().size()?&catalogue.entries()[index]:nullptr;
}
int PresetBrowser::positionOf(const juce::String& id,const juce::String& name) const
{
    for(size_t i=0;i<visible.size();++i){const auto& row=catalogue.entries()[static_cast<size_t>(visible[i])];
        if(row.recipeIndex<0&&row.name==name&&name.isNotEmpty())return static_cast<int>(i);}
    for(size_t i=0;i<visible.size();++i){const auto& row=catalogue.entries()[static_cast<size_t>(visible[i])];
        if((id.isNotEmpty()&&row.id==id)||(id.isEmpty()&&row.name==name))return static_cast<int>(i);}
    return -1;
}
void PresetBrowser::open(const juce::String& id,const juce::String& name)
{
    loadedId=id;loadedName=name;visible.clear();catalogue.refresh();fillFacets();applyFilter();
    if(const int position=positionOf(id,name);position>=0){list.selectRow(position);loadedId=catalogue.entries()[static_cast<size_t>(visible[static_cast<size_t>(position)])].id;}
    label(status,name.isNotEmpty()?"Current: "+name:"Select a preset. Enter or double-click to load.",12,dim);
    setVisible(true);toFront(false);
    juce::Component::SafePointer<PresetBrowser> safe(this);
    juce::Timer::callAfterDelay(30,[safe]{if(safe&&safe->isShowing())safe->search.grabKeyboardFocus();});
}
void PresetBrowser::applyFilter()
{
    const auto* old=selectedEntry();const juce::String id=old?old->id:juce::String();
    visible=catalogue.filter(criteria);list.updateContent();list.deselectAllRows();
    if(!visible.empty()){const int previous=positionOf(id,{});list.selectRow(previous>=0?previous:0);}
    label(count,juce::String(resultCount())+" / "+juce::String(static_cast<int>(catalogue.entries().size()))+" presets",12,dim);
    emptyResults.setVisible(visible.empty());emptyResults.toFront(false);
    updateDetails();list.repaint();repaint();if(onFilterChanged)onFilterChanged();
}
void PresetBrowser::moveSelection(int direction)
{
    if(visible.empty())return;
    const int next=juce::jlimit(0,resultCount()-1,list.getSelectedRow()+direction);list.selectRow(next);list.scrollToEnsureRowIsOnscreen(next);
}
bool PresetBrowser::stepAndLoad(int direction,const juce::String& id,const juce::String& name)
{
    if(!criteria.active())return false;
    if(visible.empty())return true;
    const int position=positionOf(id,name),n=resultCount();
    const int next=position<0?(direction<0?n-1:0):(position+(direction<0?-1:1)+n)%n;
    list.selectRow(next);loadSelected();return true;
}
void PresetBrowser::updateDetails()
{
    const auto* entry=selectedEntry();load.setEnabled(entry!=nullptr);favourite.setEnabled(entry!=nullptr);tags.setEnabled(entry&&entry->recipeIndex<0);
    label(detailName,entry?entry->name:"No matching presets",19,green);
    label(detailInfo,entry?entry->collection+" / "+entry->genre+"\n"+entry->source+"\n"+entry->characters.joinIntoString(" / ")
        +(entry->bpm>0?"\nReference tempo: "+juce::String(entry->bpm,0)+" BPM":""):"Try fewer filters or reset the search.",12,dim);
    label(detailDescription,entry?entry->description:juce::String(),14,ink);
    detailModules.setText(entry?(entry->modules.isEmpty()?"User preset. Edit its tags to organize your library.":"PROCESSORS\n"+entry->modules.joinIntoString("\n")):juce::String(),false);
    favourite.setButtonText(entry&&entry->favourite?"Remove favourite":"Add favourite");
}
void PresetBrowser::loadSelected()
{
    const auto* entry=selectedEntry();if(!entry||!onLoad)return;
    const auto copy=*entry;
    if(onLoad(copy)){loadedId=copy.id;loadedName=copy.name;label(status,"Loaded: "+copy.name,12,green);list.repaint();}
    else label(status,"Could not load preset. The file may be missing, unreadable or unsupported.",12,juce::Colours::orange);
}
void PresetBrowser::editTags()
{
    const auto* row=selectedEntry();if(!row||row->recipeIndex>=0)return;
    const auto id=row->id;
    tagsDialog=std::make_unique<juce::AlertWindow>("Organize user preset",row->name,juce::MessageBoxIconType::NoIcon,this);
    tagsDialog->addTextEditor("genre",row->genre,"Genre");tagsDialog->addTextEditor("source",row->source=="Uncategorized"?juce::String():row->source,"Source / application");
    tagsDialog->addTextEditor("characters",row->characters.joinIntoString(", "),"Character tags (comma separated)");
    tagsDialog->addButton("Save",1,juce::KeyPress(juce::KeyPress::returnKey));tagsDialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PresetBrowser> safe(this);
    tagsDialog->enterModalState(true,juce::ModalCallbackFunction::create([safe,id](int result){
        if(!safe||!safe->tagsDialog)return;
        if(result==1){auto characters=juce::StringArray::fromTokens(safe->tagsDialog->getTextEditorContents("characters"),",",{});characters.trim();characters.removeEmptyStrings();
            if(safe->catalogue.setUserTags(id,safe->tagsDialog->getTextEditorContents("genre"),safe->tagsDialog->getTextEditorContents("source"),characters))
            {safe->fillFacets();safe->applyFilter();}
            else label(safe->status,"Could not save tags. Check your preset folder permissions.",12,juce::Colours::orange);}
        safe->tagsDialog.reset();}),false);
}
bool PresetBrowser::keyPressed(const juce::KeyPress& key)
{
    if(key.getModifiers().isCommandDown()&&(key.getKeyCode()=='F'||key.getKeyCode()=='f')){search.grabKeyboardFocus();return true;}
    if(key==juce::KeyPress::escapeKey){if(onClose)onClose();return true;}
    if(key==juce::KeyPress::returnKey){loadSelected();return true;}
    return false;
}
void PresetBrowser::paintListBoxItem(int row,juce::Graphics& g,int width,int height,bool isSelected)
{
    if(row<0||row>=resultCount())return;
    const auto& entry=catalogue.entries()[static_cast<size_t>(visible[static_cast<size_t>(row)])];
    g.fillAll(isSelected?juce::Colour(0xff213b2e):(row%2?juce::Colour(0xff101a14):juce::Colour(0xff0c1410)));
    if(isSelected){g.setColour(green);g.fillRect(0,0,3,height);}
    const bool loaded=entry.id==loadedId||(loadedId.isEmpty()&&entry.name==loadedName);
    g.setColour(loaded?green:ink);g.setFont(juce::FontOptions(15.f));
    g.drawText((entry.favourite?juce::String::fromUTF8(u8"\u2605 "):juce::String())+entry.name,12,5,width-38,23,juce::Justification::centredLeft);
    if(loaded){g.setColour(green);g.fillEllipse(static_cast<float>(width-20),13.f,6.f,6.f);}
    g.setColour(dim);g.setFont(juce::FontOptions(11.f));
    g.drawText(entry.genre+" / "+entry.source+"   "+entry.characters.joinIntoString(" / "),12,30,width-22,18,juce::Justification::centredLeft);
    g.setColour(edge);g.drawHorizontalLine(height-1,10.f,static_cast<float>(width-10));
}
void PresetBrowser::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(.76f));g.setColour(juce::Colour(0xff141d17));g.fillRoundedRectangle(panel.toFloat(),7);
    g.setColour(edge);g.drawRoundedRectangle(panel.toFloat(),7,1);
    g.setColour(juce::Colour(0xff101712));g.fillRect(sidebar);g.fillRect(details);
    if(visible.empty()){g.setColour(dim);g.setFont(juce::FontOptions(15));g.drawFittedText("No presets match these filters.\nReset filters to see the full library.",listBounds.reduced(15),juce::Justification::centred,3);}
}
void PresetBrowser::resized()
{
    panel=getLocalBounds().reduced(16);auto body=panel.reduced(18);
    auto heading=body.removeFromTop(55);close.setBounds(heading.removeFromRight(118).withHeight(28));title.setBounds(heading.removeFromTop(28));subtitle.setBounds(heading);
    auto searchRow=body.removeFromTop(38);sort.setBounds(searchRow.removeFromRight(170).reduced(0,3));searchRow.removeFromRight(12);search.setBounds(searchRow.reduced(0,3));body.removeFromTop(12);
    auto actions=body.removeFromBottom(36);load.setBounds(actions.removeFromRight(190));actions.removeFromRight(12);status.setBounds(actions);
    body.removeFromBottom(10);sidebar=body.removeFromLeft(182);body.removeFromLeft(12);details=body.removeFromRight(juce::jlimit(224,280,getWidth()/4));body.removeFromRight(12);
    auto left=sidebar.reduced(8);
    for(auto pair:std::initializer_list<std::pair<juce::Label*,juce::ComboBox*>>{{&collectionLabel,&collection},{&genreLabel,&genre},{&sourceLabel,&source},{&characterLabel,&character},{&moduleLabel,&module}})
    {pair.first->setBounds(left.removeFromTop(21));pair.second->setBounds(left.removeFromTop(28));left.removeFromTop(7);}
    favourites.setBounds(left.removeFromTop(29));left.removeFromTop(5);reset.setBounds(left.removeFromTop(27));
    folder.setBounds(left.removeFromBottom(26));left.removeFromBottom(5);save.setBounds(left.removeFromBottom(26));left.removeFromBottom(5);init.setBounds(left.removeFromBottom(26));
    auto right=details.reduced(12);detailName.setBounds(right.removeFromTop(58));right.removeFromTop(7);detailInfo.setBounds(right.removeFromTop(100));right.removeFromTop(10);
    tags.setBounds(right.removeFromBottom(27));right.removeFromBottom(6);favourite.setBounds(right.removeFromBottom(28));right.removeFromBottom(10);
    detailDescription.setBounds(right.removeFromTop(juce::jmin(100,right.getHeight()/2)));right.removeFromTop(12);detailModules.setBounds(right);
    count.setBounds(body.removeFromTop(24));listBounds=body;list.setBounds(body);emptyResults.setBounds(body.reduced(15));
}
}
