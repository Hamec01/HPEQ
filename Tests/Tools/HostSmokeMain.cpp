// HPEQ_HostSmoke — M1-B / M1-C / STATE through the real built .vst3 binary, loaded with JUCE's VST3
// host the way a DAW scan does. Not a substitute for pluginval or a DAW, and reported as such.
//
//   HPEQ_HostSmoke --plugin <HPEQ.vst3> [--other <HPDG.vst3>] [--cycles 100] [--out <json>]

#include <cstring>
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

// Renders with a repeating sequence of block sizes at `fs` (T11 fixtures).
juce::AudioBuffer<float> processWith(juce::AudioPluginInstance& plugin, const juce::AudioBuffer<float>& input, double fs, const juce::Array<juce::var>& blocks)
{
    int maxBlock = 1;
    for (const auto& b : blocks)
        maxBlock = juce::jmax(maxBlock, static_cast<int>(b));
    plugin.setPlayConfigDetails(2, 2, fs, maxBlock);
    plugin.prepareToPlay(fs, maxBlock);
    auto audio = input;
    juce::MidiBuffer midi;
    for (int start = 0, k = 0; start < audio.getNumSamples(); ++k)
    {
        const int n = juce::jmin(static_cast<int>(blocks[k % blocks.size()]), audio.getNumSamples() - start);
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), 2, start, n);
        plugin.processBlock(view, midi);
        start += n;
    }
    plugin.releaseResources();
    return audio;
}

// JUCE's VST3 host keeps a plugin's state as <VST3PluginState><IComponent>base64</IComponent>...: the
// plugin's own getStateInformation bytes (plus optional JUCE private data appended by the wrapper).
juce::MemoryBlock wrapForHost(const juce::MemoryBlock& pluginState)
{
    juce::XmlElement state("VST3PluginState");
    state.createNewChildElement("IComponent")->addTextElement(pluginState.toBase64Encoding());
    juce::MemoryBlock out;
    juce::AudioProcessor::copyXmlToBinary(state, out);
    return out;
}

juce::MemoryBlock componentState(const juce::MemoryBlock& hostState)
{
    juce::MemoryBlock mem;
    if (auto xml = juce::AudioProcessor::getXmlFromBinary(hostState.getData(), static_cast<int>(hostState.getSize())))
        if (auto* component = xml->getChildByName("IComponent"))
            mem.fromBase64Encoding(component->getAllSubText());
    return mem;
}

bool startsWith(const juce::MemoryBlock& data, const juce::MemoryBlock& prefix)
{
    return data.getSize() >= prefix.getSize() && std::memcmp(data.getData(), prefix.getData(), prefix.getSize()) == 0;
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

    // T11: random full projects and the rate x block matrix from `HPEQ_Lab t11` fixtures.
    if (const juce::File t11(argValue(args, "--t11")); t11.isDirectory())
    {
        const auto manifest = juce::JSON::parse(t11.getChildFile("manifest.json"));
        if (manifest["items"].getArray() == nullptr)
        {
            check(false, "T11: manifest.json not readable in " + t11.getFullPathName(), checks);
            writeJson(outFile, report.set("checks", checks));
            return 1;
        }
        int states = 0, statesOk = 0, renders = 0, rendersOk = 0, reopened = 0, latencyOk = 0;
        juce::StringArray failed;
        for (const auto& item : *manifest["items"].getArray())
        {
            const juce::String name = item["name"];
            const double fs = item["fs"];
            if (args.contains("--verbose"))
                std::cout << "  t11 " << name << std::endl;
            juce::MemoryBlock state;
            t11.getChildFile(name + ".state").loadFileAsData(state);
            juce::String error;
            auto plugin = formats.createPluginInstance(description, fs, 512, error);
            if (plugin == nullptr)
            {
                failed.add(name + ": instantiate");
                continue;
            }
            plugin->setPlayConfigDetails(2, 2, fs, 512);
            plugin->prepareToPlay(fs, 512);
            const auto wrapped = wrapForHost(state);
            plugin->setStateInformation(wrapped.getData(), static_cast<int>(wrapped.getSize()));
            juce::MemoryBlock back;
            plugin->getStateInformation(back);
            ++states;
            const bool same = startsWith(componentState(back), state);
            statesOk += same ? 1 : 0;
            if (!same)
            {
                failed.add(name + ": state differs after load / save");
                const auto component = componentState(back);
                t11.getChildFile(name + ".state.back").replaceWithData(component.getData(), component.getSize());
            }
            const bool latencySame = plugin->getLatencySamples() == static_cast<int>(item["latency"]);
            plugin->releaseResources(); // a host deactivates before changing the block size (processWith re-prepares)
            latencyOk += latencySame ? 1 : 0;
            if (!latencySame)
                failed.add(name + ": latency " + juce::String(plugin->getLatencySamples()) + " vs " + item["latency"].toString());
            if (static_cast<bool>(item["reference"]))
            {
                juce::AudioBuffer<float> fixtureInput, reference;
                double rate = 0.0;
                readWav(t11.getChildFile(name + ".in.wav"), fixtureInput, rate);
                readWav(t11.getChildFile(name + ".ref.wav"), reference, rate);
                const auto& blocks = *item["blocks"].getArray();
                ++renders;
                const auto out = processWith(*plugin, fixtureInput, fs, blocks);
                const auto null = compareAligned(out, reference);
                rendersOk += null.bitExact ? 1 : 0;
                if (!null.bitExact)
                    failed.add(name + ": render " + juce::String(null.errorDb, 1) + " dB");
                // Re-open: a fresh instance from the saved state renders the same.
                auto again = formats.createPluginInstance(description, fs, 512, error);
                if (again != nullptr)
                {
                    again->setStateInformation(back.getData(), static_cast<int>(back.getSize()));
                    reopened += compareAligned(processWith(*again, fixtureInput, fs, blocks), reference).bitExact ? 1 : 0;
                }
            }
        }
        for (const auto& f : failed)
            std::cout << "  " << f << std::endl;
        check(states > 0 && statesOk == states, "T11: " + juce::String(statesOk) + "/" + juce::String(states) + " project states identical after load + save via the binary", checks);
        check(latencyOk == states, "T11: " + juce::String(latencyOk) + "/" + juce::String(states) + " reported latencies equal the in-process engine", checks);
        check(renders > 0 && rendersOk == renders, "T11: " + juce::String(rendersOk) + "/" + juce::String(renders) + " renders bit-exact to in-process (incl. 6 rates x 6 blocks, variable blocks)", checks);
        check(reopened == renders, "T11: " + juce::String(reopened) + "/" + juce::String(renders) + " re-opened instances render identically", checks);
        report.set("t11_failed", juce::var(failed));
    }

    report.set("checks", checks).set("failures", failures);
    writeJson(outFile, report);
    std::cout << (failures == 0 ? "host smoke: all checks passed" : "host smoke: FAILURES") << " -> " << outFile.getFullPathName() << "\n";
    return failures == 0 ? 0 : 1;
}
