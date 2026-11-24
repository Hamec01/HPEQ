#include "Rack/PresetCatalogue.h"
#include "Plugin/Presets.h"
#include <BinaryData.h>
#include <algorithm>

namespace hpeq::rack
{
namespace
{
juce::String normal(juce::String text)
{
    return text.toLowerCase().replaceCharacters("-_/,", "    ");
}
void modules(const juce::var& data,juce::StringArray& names)
{
    if(auto* chain=data["chain"].getArray())for(const auto& m:*chain)names.addIfNotAlreadyThere(m["plugin"].toString());
    if(auto* regions=data["regions"].getArray())for(const auto& r:*regions)modules(r,names);
    if(auto* layers=data["layers"].getArray())for(const auto& l:*layers)modules(l,names);
}
juce::String sourceGroup(const juce::String& source)
{
    if(source=="Vocal"||source=="Lead Vocal"||source=="Backing Vocal"||source=="Vocals")return "Vocals";
    if(source=="Master"||source=="Mix Bus"||source=="Mix / Master")return "Mix / Master";
    return source;
}
juce::String aliases(const BrowserEntry& entry)
{
    juce::String result;
    const auto text=normal(entry.source+" "+entry.characters.joinIntoString(" ")+" "+entry.genre);
    for(const auto& pair:std::initializer_list<std::pair<const char*,const char*>>{
        {"vocal",u8"\u0432\u043e\u043a\u0430\u043b \u0433\u043e\u043b\u043e\u0441"},
        {"bass",u8"\u0431\u0430\u0441"},
        {"drum",u8"\u0431\u0430\u0440\u0430\u0431\u0430\u043d\u044b \u0443\u0434\u0430\u0440\u043d\u044b\u0435"},
        {"kick",u8"\u043a\u0438\u043a \u0431\u043e\u0447\u043a\u0430"},
        {"snare",u8"\u0441\u043d\u0435\u0439\u0440 \u043c\u0430\u043b\u044b\u0439"},
        {"hat",u8"\u0445\u044d\u0442\u044b \u0445\u0430\u0439\u0445\u044d\u0442"},
        {"guitar",u8"\u0433\u0438\u0442\u0430\u0440\u0430"},
        {"keys",u8"\u043a\u043b\u0430\u0432\u0438\u0448\u0438 \u043f\u0438\u0430\u043d\u0438\u043d\u043e"},
        {"master",u8"\u043c\u0430\u0441\u0442\u0435\u0440 \u043c\u0430\u0441\u0442\u0435\u0440\u0438\u043d\u0433"},
        {"warm",u8"\u0442\u0435\u043f\u043b\u044b\u0439 \u0442\u0451\u043f\u043b\u044b\u0439 \u0442\u0435\u043f\u043b\u043e"},
        {"dark",u8"\u0442\u0435\u043c\u043d\u044b\u0439 \u0442\u0451\u043c\u043d\u044b\u0439"},
        {"bright",u8"\u044f\u0440\u043a\u0438\u0439"},
        {"punchy",u8"\u043f\u0430\u043d\u0447 \u0443\u0434\u0430\u0440\u043d\u044b\u0439"},
        {"clean",u8"\u0447\u0438\u0441\u0442\u044b\u0439 \u043f\u0440\u043e\u0437\u0440\u0430\u0447\u043d\u044b\u0439"},
        {"lo fi",u8"\u043b\u043e\u0444\u0430\u0439 \u0432\u0438\u043d\u0442\u0430\u0436"},
        {"wide",u8"\u0448\u0438\u0440\u043e\u043a\u0438\u0439 \u0448\u0438\u0440\u0438\u043d\u0430"},
        {"spacious",u8"\u043f\u0440\u043e\u0441\u0442\u0440\u0430\u043d\u0441\u0442\u0432\u043e \u043e\u0431\u044a\u0435\u043c \u043e\u0431\u044a\u0451\u043c"},
        {"rhythmic",u8"\u0440\u0438\u0442\u043c \u0440\u0438\u0442\u043c\u0438\u0447\u043d\u044b\u0439"},
        {"aggressive",u8"\u0430\u0433\u0440\u0435\u0441\u0441\u0438\u0432\u043d\u044b\u0439"},
        {"smooth",u8"\u043c\u044f\u0433\u043a\u0438\u0439"},
        {"echo",u8"\u044d\u0445\u043e \u0434\u0435\u043b\u0435\u0439"},
        {"spacious",u8"\u0440\u0435\u0432\u0435\u0440\u0431 \u0440\u0435\u0432\u0435\u0440\u0431\u0435\u0440\u0430\u0446\u0438\u044f"},
        {"sample",u8"\u0441\u044d\u043c\u043f\u043b \u0441\u0435\u043c\u043f\u043b"},
        {"synth",u8"\u0441\u0438\u043d\u0442 \u0441\u0438\u043d\u0442\u0435\u0437\u0430\u0442\u043e\u0440"},
        {"percussion",u8"\u043f\u0435\u0440\u043a\u0443\u0441\u0441\u0438\u044f"}})
        if(text.contains(pair.first))result+=" "+juce::String::fromUTF8(pair.second);
    return result;
}
}
bool BrowserFilter::active() const
{
    return query.isNotEmpty()||collection.isNotEmpty()||genre.isNotEmpty()||source.isNotEmpty()
        ||character.isNotEmpty()||module.isNotEmpty()||favouritesOnly||alphabetical;
}
PresetCatalogue::PresetCatalogue(juce::File userFolder):folder(std::move(userFolder)),settingsFile(folder.getChildFile("browser.json")) {refresh();}
juce::var PresetCatalogue::preferences() const
{
    auto data=juce::JSON::parse(settingsFile);return data.isObject()?data:juce::var(new juce::DynamicObject());
}
bool PresetCatalogue::writePreferences(const juce::var& data)
{
    if(!folder.createDirectory())return false;
    juce::TemporaryFile temp(settingsFile);
    return temp.getFile().replaceWithText(juce::JSON::toString(data))&&temp.overwriteTargetFileWithTemporary();
}
void PresetCatalogue::refresh()
{
    rows.clear();int size=0;const auto* bytes=HpeqAssets::getNamedResource("browsertags_json",size);
    const auto tags=juce::JSON::parse(juce::String::fromUTF8(bytes,size));
    const auto prefs=preferences();
    const auto& bank=recipes();
    for(size_t i=0;i<bank.size();++i)
    {
        const auto& recipe=bank[i];BrowserEntry row;
        row.id=recipe.id;row.name=recipe.name;row.collection=row.id.startsWith("PW")?"Rack Power":"Task presets";
        row.genre=recipe.data["bank"].toString();row.source=sourceGroup(recipe.category);
        row.description=recipe.intent;row.bpm=recipe.bpm;row.recipeIndex=static_cast<int>(i);
        row.characters=juce::StringArray::fromTokens(tags[juce::Identifier(recipe.id)].toString(),",",{});row.characters.trim();row.characters.removeEmptyStrings();
        modules(recipe.data,row.modules);rows.push_back(std::move(row));
    }
    for(const auto& file:presets::listFiles(folder,"*.hpeq"))
    {
        BrowserEntry row;row.id="user:"+file.getFileName();row.name=file.getFileNameWithoutExtension();row.file=file;
        row.collection="User";row.source="Uncategorized";row.description="Your saved rack preset.";
        const auto tagsForUser=prefs["userTags"][juce::Identifier(row.id)];
        row.genre=tagsForUser["genre"].toString();
        if(tagsForUser["source"].toString().isNotEmpty())row.source=tagsForUser["source"].toString();
        row.characters=juce::StringArray::fromTokens(tagsForUser["characters"].toString(),",",{});row.characters.trim();row.characters.removeEmptyStrings();
        rows.push_back(std::move(row));
    }
    for(auto& row:rows)row.favourite=static_cast<bool>(prefs["favourites"][juce::Identifier(row.id)]);
}
bool PresetCatalogue::matches(const BrowserEntry& row,const BrowserFilter& f)
{
    if(f.favouritesOnly&&!row.favourite)return false;
    if(f.collection.isNotEmpty()&&row.collection!=f.collection)return false;
    if(f.genre.isNotEmpty()&&row.genre!=f.genre)return false;
    if(f.source.isNotEmpty()&&row.source!=f.source)return false;
    if(f.character.isNotEmpty()&&!row.characters.contains(f.character,true))return false;
    if(f.module.isNotEmpty()&&!row.modules.contains(f.module,true))return false;
    const auto text=normal(row.name+" "+row.id+" "+row.genre+" "+row.source+" "+row.collection+" "+row.description+" "+row.characters.joinIntoString(" ")+" "+row.modules.joinIntoString(" ")+aliases(row));
    for(const auto& token:juce::StringArray::fromTokens(normal(f.query)," \t\r\n",{}))
        if(token.isNotEmpty()&&!text.contains(token))return false;
    return true;
}
std::vector<int> PresetCatalogue::filter(const BrowserFilter& f) const
{
    std::vector<int> result;
    for(size_t i=0;i<rows.size();++i)if(matches(rows[i],f))result.push_back(static_cast<int>(i));
    if(f.alphabetical)std::stable_sort(result.begin(),result.end(),[&](int a,int b){return rows[static_cast<size_t>(a)].name.compareNatural(rows[static_cast<size_t>(b)].name)<0;});
    return result;
}
bool PresetCatalogue::setFavourite(const juce::String& id,bool value)
{
    auto data=preferences();auto favourites=data["favourites"];
    if(!favourites.isObject())favourites=juce::var(new juce::DynamicObject());
    favourites.getDynamicObject()->setProperty(id,value);data.getDynamicObject()->setProperty("favourites",favourites);
    if(!writePreferences(data))return false;
    for(auto& row:rows)if(row.id==id)row.favourite=value;
    return true;
}
bool PresetCatalogue::setUserTags(const juce::String& id,const juce::String& genre,const juce::String& source,const juce::StringArray& characters)
{
    if(!id.startsWith("user:"))return false;
    auto data=preferences();auto tags=data["userTags"];
    if(!tags.isObject())tags=juce::var(new juce::DynamicObject());
    auto* metadata=new juce::DynamicObject();metadata->setProperty("genre",genre.trim());metadata->setProperty("source",sourceGroup(source.trim()));metadata->setProperty("characters",characters.joinIntoString(", "));
    tags.getDynamicObject()->setProperty(id,juce::var(metadata));data.getDynamicObject()->setProperty("userTags",tags);
    if(!writePreferences(data))return false;
    refresh();return true;
}
}
