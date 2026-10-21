// HPEQ_HostSmoke — M1-B / M1-C / STATE through the real built .vst3 binary, loaded with JUCE's VST3
// host the way a DAW scan does. Not a substitute for pluginval or a DAW, and reported as such.
//
//   HPEQ_HostSmoke --plugin <HPEQ.vst3> [--other <HPDG.vst3>] [--cycles 100] [--out <json>]

#include <iostream>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Harness/AudioFileIO.h"
#include "Harness/AudioMetrics.h"
#include "Harness/Json.h"
#include "Harness/MachineInfo.h"
#include "Harness/TestSignalFactory.h"

using namespace hpeq::test;

namespace
{
int failures = 0;

void check(bool condition, const juce::String& what, juce::var& log)
{
    std::cout << (condition ? "[PASS] " : "[FAIL] ") << what << "\n";
    log.append(JsonObject().set("check", what).set("pass", condition));
    failures += condition ? 0 : 1;
}

juce::String argValue(const juce::StringArray& args, const juce::String& key, const juce::String& fallback = {})
{
    const int index = args.indexOf(key);
    return index >= 0 && index + 1 < args.size() ? args[index + 1] : fallback;
}

juce::var describe(const juce::PluginDescription& d)
{
    return JsonObject()
        .set("name", d.name)
        .set("manufacturer", d.manufacturerName)
        .set("category", d.category)
        .set("uid", d.uniqueId)
        .set("identifier", d.createIdentifierString())
        .set("is_instrument", d.isInstrument)
        .set("inputs", d.numInputChannels)
        .set("outputs", d.numOutputChannels)
        .set("version", d.version);
}

std::unique_ptr<juce::AudioPluginInstance> instantiate(juce::AudioPluginFormatManager& formats,
                                                       const juce::PluginDescription& d,
                                                       juce::String& error)
{
    return formats.createPluginInstance(d, 48000.0, 512, error);
}

juce::AudioBuffer<float> process(juce::AudioPluginInstance& plugin, const juce::AudioBuffer<float>& input, int block)
{
    plugin.setPlayConfigDetails(2, 2, 48000.0, block);
    plugin.prepareToPlay(48000.0, block);
    auto audio = input;
    juce::MidiBuffer midi;
    for (int start = 0; start < audio.getNumSamples(); start += block)
    {
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), 2, start, juce::jmin(block, audio.getNumSamples() - start));
        plugin.processBlock(view, midi);
    }
    plugin.releaseResources();
    return audio;
}

juce::AudioProcessorParameter* findParameter(juce::AudioPluginInstance& plugin, const juce::String& name)
{
    for (auto* p : plugin.getParameters())
        if (p->getName(64) == name)
            return p;
    return nullptr;
}
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    juce::StringArray args;
    for (int i = 1; i < argc; ++i)
        args.add(argv[i]);

    const juce::File pluginPath(argValue(args, "--plugin"));
    const juce::File otherPath(argValue(args, "--other"));
    const int cycles = argValue(args, "--cycles", "100").getIntValue();
    const juce::File outFile(argValue(args, "--out", "host_smoke.json"));

    juce::AudioPluginFormatManager formats;
    formats.addFormat(std::make_unique<juce::VST3PluginFormat>());
    auto* vst3 = formats.getFormat(0);

    auto checks = jsonArray();
    auto report = JsonObject().set("machine", describeMachine()).set("plugin_path", pluginPath.getFullPathName());

    juce::OwnedArray<juce::PluginDescription> found;
    vst3->findAllTypesForFile(found, pluginPath.getFullPathName());
    check(found.size() == 1, "HPEQ.vst3 scan finds exactly one plugin", checks);
    if (found.isEmpty())
    {
        writeJson(outFile, report.set("checks", checks));
        return 1;
    }
    const auto description = *found[0];
    report.set("hpeq", describe(description));
    check(!description.isInstrument, "scanned as an effect, not an instrument", checks);
    // Channel counts are not filled in by a VST3 factory scan (0/0 for every plugin), so the bus
    // layout is checked on an instance below.

    if (otherPath.exists())
    {
        juce::OwnedArray<juce::PluginDescription> other;
        vst3->findAllTypesForFile(other, otherPath.getFullPathName());
        check(!other.isEmpty(), "HPDG.vst3 still scans", checks);
        if (!other.isEmpty())
        {
            report.set("other", describe(*other[0]));
            check(other[0]->uniqueId != description.uniqueId, "HPEQ and HPDG UIDs differ", checks);
            check(other[0]->createIdentifierString() != description.createIdentifierString(), "HPEQ and HPDG identifiers differ", checks);
        }
    }

    // Unity through the real binary.
    const auto input = makeSyntheticDrums({ 48000.0, 2, 1.0, kFixtureSeed });
    {
        juce::String error;
        auto plugin = instantiate(formats, description, error);
        check(plugin != nullptr, "instantiate: " + (error.isEmpty() ? juce::String("ok") : error), checks);
        if (plugin != nullptr)
        {
            const auto layout = plugin->getBusesLayout();
            check(layout.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
                      && layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(),
                  "instance default bus: stereo in / stereo out", checks);
            auto mono = layout;
            mono.inputBuses.getReference(0) = juce::AudioChannelSet::mono();
            mono.outputBuses.getReference(0) = juce::AudioChannelSet::mono();
            check(plugin->checkBusesLayoutSupported(mono), "instance accepts mono in / mono out", checks);
        }
        if (plugin != nullptr)
            for (int block : { 64, 512, 2048 })
            {
                const auto out = process(*plugin, input, block);
                const auto null = compareAligned(out, input);
                check(null.bitExact, "M1-A via VST3 binary: bit-exact pass-through, block " + juce::String(block), checks);
            }
    }

    // State through the binary: set Output Gain, save, load into a fresh instance, compare sound.
    {
        juce::String error;
        auto a = instantiate(formats, description, error);
        auto b = instantiate(formats, description, error);
        auto* gain = a != nullptr ? findParameter(*a, "Output Gain") : nullptr;
        check(gain != nullptr, "Output Gain parameter exposed to the host", checks);
        if (gain != nullptr && b != nullptr)
        {
            gain->setValueNotifyingHost(gain->getValueForText("-6"));
            juce::MemoryBlock state;
            a->getStateInformation(state);
            b->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            const auto outA = process(*a, input, 512);
            const auto outB = process(*b, input, 512);
            check(compareAligned(outB, outA).bitExact, "STATE via VST3 binary: restored instance renders identically", checks);
            const double gainDb = measureAll(outA).rmsDbfs - measureAll(input).rmsDbfs;
            check(std::abs(gainDb + 6.0) < 0.05, "Output Gain -6 dB measured " + juce::String(gainDb, 3) + " dB", checks);
        }
    }

    // M1-C: repeated instance + editor life cycles with the editor on screen.
    {
        int created = 0, editors = 0;
        for (int cycle = 0; cycle < cycles; ++cycle)
        {
            juce::String error;
            auto plugin = instantiate(formats, description, error);
            if (plugin == nullptr)
                break;
            ++created;
            process(*plugin, input, 512);
            if (auto* editor = plugin->createEditorIfNeeded())
            {
                ++editors;
                editor->setOpaque(true);
                editor->addToDesktop(juce::ComponentPeer::windowHasTitleBar);
                editor->setVisible(true);
                juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
                delete editor;
            }
            plugin.reset();
        }
        check(created == cycles, "M1-C: " + juce::String(created) + "/" + juce::String(cycles) + " instance cycles", checks);
        check(editors == cycles, "M1-C: " + juce::String(editors) + "/" + juce::String(cycles) + " editor open/close on desktop", checks);
    }

    report.set("checks", checks).set("failures", failures);
    writeJson(outFile, report);
    std::cout << (failures == 0 ? "host smoke: all checks passed" : "host smoke: FAILURES") << " -> " << outFile.getFullPathName() << "\n";
    return failures == 0 ? 0 : 1;
}
