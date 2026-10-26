#include "Plugin/Presets.h"

#include "Core/Revolution.h"
#include <BinaryData.h>

namespace hpeq::presets
{
namespace
{
// "Vocals-Mastering-and-More" -> "Vocals, Mastering & More"; "DrumBus" -> "Drum Bus".
juce::String packName(juce::String folder)
{
    folder = folder.replace("-and-", " & ");
    const int dash = folder.indexOfChar('-');
    return dash < 0 ? folder : folder.substring(0, dash) + ", " + folder.substring(dash + 1).replace("-", " ");
}

juce::String categoryName(const juce::String& folder)
{
    juce::String out;
    for (int i = 0; i < folder.length(); ++i)
    {
        const auto c = folder[i];
        if (i > 0 && juce::CharacterFunctions::isUpperCase(c) && juce::CharacterFunctions::isLowerCase(folder[i - 1]))
            out << ' ';
        out << juce::String::charToString(c);
    }
    return out;
}

// The preset's name: the state's presetName, else the first layer's name, else the file's.
juce::String presetName(const char* data, int size, const juce::String& file)
{
    const juce::String text(juce::CharPointer_UTF8(data + std::min(size, 8)), static_cast<size_t>(std::max(0, size - 8)));
    if (const int root = text.indexOf("presetName=\""); root >= 0)
        if (const int end = text.indexOfChar(root + 12, '"'); end > root + 12)
            return text.substring(root + 12, end);
    const int layer = text.indexOf("<Layer index=\"0\"");
    if (layer >= 0)
    {
        const int at = text.indexOf(layer, "name=\"");
        if (at >= 0)
        {
            const int end = text.indexOfChar(at + 6, '"');
            if (end > at + 6)
                return text.substring(at + 6, end);
        }
    }
    return file.upToLastOccurrenceOf(".", false, false).fromFirstOccurrenceOf("-", false, false).replace("-", " ");
}
} // namespace

const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> list = []
    {
        // The index CMake writes: one line per preset, "<pack>/<category>[/<group>]/<file>.hpeq"; ordered by
        // pack, then the library's numbers.
        int indexSize = 0;
        const auto* index = HpeqAssets::getNamedResource("hpeq_factory_presets_txt", indexSize);
        juce::StringArray lines;
        if (index != nullptr)
            lines.addLines(juce::String::fromUTF8(index, indexSize));
        lines.trim();
        lines.removeEmptyStrings();
        std::vector<juce::StringArray> entries;
        for (const auto& line : lines)
            if (auto parts = juce::StringArray::fromTokens(line, "/", ""); parts.size() == 3 || parts.size() == 4)
                entries.push_back(parts);
        std::sort(entries.begin(), entries.end(), [](const juce::StringArray& a, const juce::StringArray& b)
        {
            return a[0] != b[0] ? a[0] < b[0] : a[a.size() - 1] < b[b.size() - 1];
        });
        std::vector<FactoryPreset> out;
        for (const auto& parts : entries)
        {
            const auto& file = parts[parts.size() - 1];
            for (int r = 0; r < HpeqAssets::namedResourceListSize; ++r)
                if (file == HpeqAssets::originalFilenames[r])
                {
                    FactoryPreset p;
                    p.data = HpeqAssets::getNamedResource(HpeqAssets::namedResourceList[r], p.size);
                    p.pack = packName(parts[0]);
                    p.category = categoryName(parts[1]);
                    p.group = parts.size() == 4 ? parts[2].replace("-", " / ") : juce::String();
                    p.name = presetName(p.data, p.size, file);
                    out.push_back(p);
                    break;
                }
        }
        return out;
    }();
    return list;
}

juce::File userRoot()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("HamloProd/HPEQ/Presets");
}

juce::File userPresetFolder(const juce::File& root)
{
    return root.getChildFile("User");
}

juce::File userModuleFolder(FxType type, const juce::File& root)
{
    return root.getChildFile("Modules").getChildFile(toString(type));
}

juce::Array<juce::File> listFiles(const juce::File& folder, const juce::String& pattern)
{
    auto files = folder.findChildFiles(juce::File::findFiles, false, pattern);
    files.sort();
    return files;
}

juce::String safeFileName(const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName(name.trim()).trim();
    return clean.isEmpty() ? juce::String("Preset") : clean;
}

FxSlotState makeModule(FxType type, const ModulePreset& preset)
{
    auto m = defaultModule(type);
    m.mix = preset.mix;
    for (const auto& [key, value] : preset.values)
        m.parameters.set(key, value);
    if (type == FxType::MonstaFx)
    {
        const auto seed = static_cast<std::uint64_t>(presetValue(preset, "revSeed", 1.0));
        const int character = juce::roundToInt(presetValue(preset, "character", static_cast<double>(revolution::Musical)));
        revolution::writePattern(m.parameters, revolution::generate({}, seed, character));
    }
    return m;
}

bool saveModulePreset(const FxSlotState& module, const juce::File& file)
{
    auto copy = module;
    copy.enabled = true;
    const auto xml = moduleToTree(copy).createXml();
    return xml != nullptr && file.getParentDirectory().createDirectory() && xml->writeTo(file);
}

std::optional<FxSlotState> loadModulePreset(const juce::File& file, FxType expected)
{
    const auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return std::nullopt;
    const auto tree = juce::ValueTree::fromXml(*xml);
    if (!tree.hasType("FxSlot") || tree["type"].toString() != toString(expected))
        return std::nullopt;
    auto module = moduleFromTree(tree);
    module.enabled = true;
    return module;
}
} // namespace hpeq::presets
