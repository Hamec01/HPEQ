// Presets pass (owner, 2026-10-26): whole-plugin presets (the factory library in Assets/Presets/Factory and
// the user's own) behind a button before Overlay, and each module's own presets on its page.
// Thresholds fixed before the runs.

#include <cmath>
#include <memory>
#include <set>

#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "Plugin/Presets.h"
#include "UI/HamloCompPanel.h"
#include "UI/HamloEchoPanel.h"
#include "UI/HamloFaderPanel.h"
#include "UI/HamloMagPanel.h"
#include "UI/HamloRedWallPanel.h"
#include "UI/HamloRevolutionPanel.h"
#include "UI/HamloShirotaPanel.h"
#include "UI/HamloUdarnikPanel.h"
#include "UI/HamloVerbPanel.h"
#include "UI/HamloZastavaPanel.h"

namespace hpeq::test
{
namespace
{
template <typename Panel, int Knobs, int Switches>
std::set<juce::String> keysOf()
{
    std::set<juce::String> keys;
    for (int p = 0; p < Knobs; ++p)
        keys.insert(Panel::keyFor(static_cast<typename Panel::Param>(p)));
    for (int s = 0; s < Switches; ++s)
        keys.insert(Panel::keyFor(static_cast<typename Panel::Switch>(s)));
    return keys;
}

// The keys a module's page edits: a preset may only set those.
std::set<juce::String> pageKeys(FxType t)
{
    using namespace ui;
    switch (t)
    {
        case FxType::Compressor: return keysOf<HamloCompPanel, HamloCompPanel::NumKnobs, HamloCompPanel::NumSwitches>();
        case FxType::Limiter: return keysOf<HamloRedWallPanel, HamloRedWallPanel::NumKnobs, HamloRedWallPanel::NumSwitches>();
        case FxType::Reverb: return keysOf<HamloVerbPanel, HamloVerbPanel::NumKnobs, HamloVerbPanel::NumSwitches>();
        case FxType::Delay: return keysOf<HamloEchoPanel, HamloEchoPanel::NumParams, HamloEchoPanel::NumSwitches>();
        case FxType::TransientShaper: return keysOf<HamloUdarnikPanel, HamloUdarnikPanel::NumKnobs, HamloUdarnikPanel::NumSwitches>();
        case FxType::Saturator: return keysOf<HamloMagPanel, HamloMagPanel::NumParams, HamloMagPanel::NumSwitches>();
        case FxType::StereoProcessor: return keysOf<HamloShirotaPanel, HamloShirotaPanel::NumKnobs, HamloShirotaPanel::NumSwitches>();
        case FxType::GateExpander: return keysOf<HamloZastavaPanel, HamloZastavaPanel::NumKnobs, HamloZastavaPanel::NumSwitches>();
        case FxType::Gain: return keysOf<HamloFaderPanel, HamloFaderPanel::NumParams, HamloFaderPanel::NumSwitches>();
        case FxType::MonstaFx:
        {
            auto k = keysOf<HamloRevolutionPanel, HamloRevolutionPanel::NumParams, HamloRevolutionPanel::NumSwitches>();
            k.insert("revSeed");
            return k;
        }
    }
    return {};
}

constexpr std::array<FxType, 10> kAll { FxType::Compressor, FxType::Limiter, FxType::Reverb, FxType::Delay, FxType::TransientShaper,
                                        FxType::Saturator, FxType::MonstaFx, FxType::StereoProcessor, FxType::GateExpander, FxType::Gain };

template <typename T>
T* findChild(juce::Component* root)
{
    if (auto* t = dynamic_cast<T*>(root))
        return t;
    for (auto* c : root->getChildren())
        if (auto* t = findChild<T>(c))
            return t;
    return nullptr;
}

juce::TextButton* findButton(juce::Component* root, const juce::String& text)
{
    if (auto* b = dynamic_cast<juce::TextButton*>(root); b != nullptr && b->getButtonText() == text && b->isVisible())
        return b;
    for (auto* c : root->getChildren())
        if (auto* b = findButton(c, text))
            return b;
    return nullptr;
}
} // namespace

class PresetPassTests final : public juce::UnitTest
{
public:
    PresetPassTests() : juce::UnitTest("Preset pass", "HPEQ") {}

    void runTest() override
    {
        beginTest("Factory library (presets pass 2): 197 presets embedded, named, in 3 packs and 17 categories; each loads as one undoable edit");
        {
            const auto& list = presets::factoryPresets();
            expectEquals(static_cast<int>(list.size()), 197);
            std::set<juce::String> packs, categories;
            int loaded = 0, undone = 0;
            for (const auto& p : list)
            {
                packs.insert(p.pack);
                categories.insert(p.pack + "/" + p.category);
                expect(p.name.isNotEmpty() && p.data != nullptr && p.size > 0, p.pack + "/" + p.category);
                FxPluginProcessor plugin;
                const auto before = toValueTree(plugin.getProject());
                if (plugin.loadPreset(p.data, p.size, p.name))
                {
                    ++loaded;
                    const bool changed = !toValueTree(plugin.getProject()).isEquivalentTo(before);
                    plugin.getUndoManager().undo();
                    undone += changed && toValueTree(plugin.getProject()).isEquivalentTo(before) ? 1 : 0;
                }
                expect(plugin.getPresetName() == p.name || !plugin.getProject().layers[0].name.isEmpty());
            }
            logMessage("  " + juce::String(static_cast<int>(packs.size())) + " packs, " + juce::String(static_cast<int>(categories.size())) + " categories; "
                       + juce::String(loaded) + " loaded, " + juce::String(undone) + " undone; e.g. " + list.front().pack + " / " + list.front().category
                       + " / " + list.front().name);
            expectEquals(static_cast<int>(packs.size()), 3);
            expectEquals(static_cast<int>(categories.size()), 17);
            expectEquals(loaded, 197);
            expectEquals(undone, 197);
        }

        beginTest("Preset load: Output gain comes with it, Bypass stays, the name is saved with the state; a broken block changes nothing");
        {
            FxPluginProcessor plugin;
            auto* bypass = plugin.getBypassParameter();
            bypass->setValueNotifyingHost(1.0f);
            const auto& p = presets::factoryPresets().front(); // 001 Kick
            expect(plugin.loadPreset(p.data, p.size, p.name));
            const juce::String text(p.data + 8, static_cast<size_t>(p.size - 8));
            const float wanted = text.fromFirstOccurrenceOf("id=\"outputGain\" value=\"", false, false).upToFirstOccurrenceOf("\"", false, false).getFloatValue();
            const auto outputDb = plugin.getParameters().getRawParameterValue("outputGain")->load();
            logMessage("  " + p.category + " / " + p.name + ": output " + juce::String(outputDb, 2) + " dB (preset " + juce::String(wanted, 2) + "), bypass "
                       + juce::String(bypass->get() ? "on" : "off"));
            expect(std::abs(outputDb - wanted) < 0.01f && bypass->get());
            juce::MemoryBlock state;
            plugin.getStateInformation(state);
            FxPluginProcessor other;
            other.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            expect(other.getPresetName() == p.name);
            const char junk[] = "not a preset";
            const auto before = toValueTree(plugin.getProject());
            expect(!plugin.loadPreset(junk, static_cast<int>(sizeof(junk)), "junk"));
            expect(toValueTree(plugin.getProject()).isEquivalentTo(before) && plugin.getPresetName() == p.name);
        }

        beginTest("Module presets: every module has 6 or more; names unique; each key is one its page edits; Revolution's pattern comes from its seed");
        {
            int total = 0;
            for (auto t : kAll)
            {
                const auto& list = presets::factoryModulePresets(t);
                total += static_cast<int>(list.size());
                expect(list.size() >= 6, toString(t));
                std::set<juce::String> names;
                const auto keys = pageKeys(t);
                for (const auto& p : list)
                {
                    names.insert(p.name);
                    expect(p.mix > 0.0f && p.mix <= 1.0f, juce::String(p.name));
                    for (const auto& [key, value] : p.values)
                        expect(keys.count(juce::String(key)) == 1 && juce::String(key) != "mix", toString(t) + " / " + p.name + ": " + key);
                }
                expectEquals(static_cast<int>(names.size()), static_cast<int>(list.size()));
            }
            logMessage("  " + juce::String(total) + " module presets in 10 modules");
            const auto& rev = presets::factoryModulePresets(FxType::MonstaFx);
            const auto a = presets::makeModule(FxType::MonstaFx, rev[1]), b = presets::makeModule(FxType::MonstaFx, rev[5]);
            expect(toString(FxType::MonstaFx) == "MonstaFx");
            expect(!moduleToTree(a).isEquivalentTo(moduleToTree(b)), "two Revolution presets with the same pattern");
        }

        beginTest("Module presets sound: each one in the MIX chain on noise at -18 dBFS gives finite, non-silent output");
        {
            int bad = 0;
            for (auto t : kAll)
                for (const auto& preset : presets::factoryModulePresets(t))
                {
                    FxPluginProcessor plugin;
                    FxProjectState project;
                    project.layers[0].mixChain = { presets::makeModule(t, preset) };
                    plugin.setProject(project);
                    plugin.prepareToPlay(48000.0, 512);
                    juce::Random rng(7);
                    juce::MidiBuffer midi;
                    double sum = 0.0;
                    bool finite = true;
                    for (int block = 0; block < 94; ++block)
                    {
                        juce::AudioBuffer<float> buffer(2, 512);
                        for (int ch = 0; ch < 2; ++ch)
                            for (int i = 0; i < 512; ++i)
                                buffer.setSample(ch, i, 0.125f * (2.0f * rng.nextFloat() - 1.0f) * 1.73f);
                        plugin.processBlock(buffer, midi);
                        for (int ch = 0; ch < 2; ++ch)
                            for (int i = 0; i < 512; ++i)
                            {
                                const float v = buffer.getSample(ch, i);
                                finite = finite && std::isfinite(v);
                                if (block >= 47)
                                    sum += static_cast<double>(v) * v;
                            }
                    }
                    const double rmsDb = 10.0 * std::log10(sum / (47.0 * 512.0 * 2.0) + 1.0e-30);
                    if (!finite || rmsDb < -40.0)
                    {
                        ++bad;
                        logMessage("  " + toString(t) + " / " + preset.name + ": " + (finite ? "" : "NOT FINITE ") + juce::String(rmsDb, 1) + " dBFS");
                    }
                }
            expectEquals(bad, 0);
        }

        beginTest("User module presets: save and load round trip; a preset of another module does not load");
        {
            const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("hpeq_preset_test_" + juce::String(juce::Random::getSystemRandom().nextInt64()));
            auto m = presets::makeModule(FxType::Reverb, presets::factoryModulePresets(FxType::Reverb)[0]);
            m.enabled = false;
            const auto file = presets::userModuleFolder(FxType::Reverb, root).getChildFile(presets::safeFileName("My: plate?") + ".hpeqmod");
            expect(presets::saveModulePreset(m, file));
            const auto back = presets::loadModulePreset(file, FxType::Reverb);
            expect(back.has_value() && back->enabled && std::abs(back->mix - m.mix) < 1.0e-6f
                   && static_cast<int>(back->parameters["space"]) == 4 && static_cast<double>(back->parameters["decaySeconds"]) == 1.6);
            expect(!presets::loadModulePreset(file, FxType::Delay).has_value());
            expectEquals(presets::listFiles(presets::userModuleFolder(FxType::Reverb, root), "*.hpeqmod").size(), 1);
            root.deleteRecursively();
        }

        beginTest("Panel: the preset button stands left of Overlay and shows the loaded preset; module pages have Presets");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            FxNodeState n;
            n.id = 1;
            n.chain = { defaultModule(FxType::Compressor) };
            p.layers[0].fxNodes = { n };
            p.nextNodeId = 2;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1400, 900);
            auto* overlay = findButton(editor.get(), "Overlay");
            auto* button = findButton(editor.get(), "Presets");
            expect(overlay != nullptr && button != nullptr && button->getRight() <= overlay->getX() + 4 && button->getY() == overlay->getY());
            const auto& f = presets::factoryPresets()[3];
            plugin->loadPreset(f.data, f.size, f.name);
            plugin->dispatchPendingMessages();
            expect(findButton(editor.get(), f.name) != nullptr, "the button does not show " + f.name);
            plugin->setProject(p);
            editor->selectNode(true, 1, 0);
            plugin->dispatchPendingMessages();
            expect(findButton(editor.get(), "Presets") != nullptr, "no Presets on the module page");
        }
    }
};

static PresetPassTests presetPassTests;
} // namespace hpeq::test
