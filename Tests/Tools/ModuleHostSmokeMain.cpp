#include <atomic>
#include <cmath>
#include <iostream>
#include <set>
#include <thread>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace
{
bool allFinite(const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (!std::isfinite(buffer.getSample(channel, sample)))
                return false;
    return true;
}
}
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const juce::File folder(argc > 1 ? juce::String::fromUTF8(argv[1]) : juce::String());
    if (!folder.isDirectory())
    {
        std::cerr << "usage: HPEQ_ModuleHostSmoke <folder-containing-vst3-bundles>\n";
        return 2;
    }

    juce::Array<juce::File> bundles;
    folder.findChildFiles(bundles, juce::File::findDirectories, false, "*.vst3");
    if (bundles.size() != 8)
    {
        std::cerr << "[FAIL] expected 8 VST3 bundles, found " << bundles.size() << "\n";
        return 1;
    }

    juce::AudioPluginFormatManager formats;
    formats.addFormat(std::make_unique<juce::VST3PluginFormat>());
    auto* format = formats.getFormat(0);
    std::set<std::string> identifiers;
    int failures = 0;
    for (const auto& bundle : bundles)
    {
        juce::OwnedArray<juce::PluginDescription> descriptions;
        format->findAllTypesForFile(descriptions, bundle.getFullPathName());
        if (descriptions.size() != 1)
        {
            std::cerr << "[FAIL] " << bundle.getFileName() << " scan count " << descriptions.size() << "\n";
            ++failures;
            continue;
        }
        const auto& description = *descriptions[0];
        identifiers.insert(description.createIdentifierString().toStdString());
        juce::String error;
        auto plugin = formats.createPluginInstance(description, 48000.0, 512, error);
        if (plugin == nullptr)
        {
            std::cerr << "[FAIL] " << description.name << " instantiate: " << error << "\n";
            ++failures;
            continue;
        }

        plugin->setPlayConfigDetails(2, 2, 48000.0, 512);
        plugin->prepareToPlay(48000.0, 512);
        juce::AudioBuffer<float> audio(2, 512);
        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < audio.getNumSamples(); ++i)
                audio.setSample(channel, i, 0.1f * std::sin(0.02f * static_cast<float>(i + channel * 7)));
        juce::MidiBuffer midi;
        plugin->processBlock(audio, midi);

        juce::MemoryBlock state;
        plugin->getStateInformation(state);
        plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        bool editorsOk = true;
        int processedBlocks = 0;
        if (description.name == "HamloComp")
        {
            std::atomic<bool> processAudio { true };
            std::atomic<int> liveBlocks { 0 };
            std::thread audioThread([&]
            {
                juce::AudioBuffer<float> liveAudio(2, 512);
                juce::MidiBuffer liveMidi;
                while (processAudio.load(std::memory_order_acquire))
                {
                    liveAudio.clear();
                    plugin->processBlock(liveAudio, liveMidi);
                    liveBlocks.fetch_add(1, std::memory_order_relaxed);
                }
            });

            for (int cycle = 0; cycle < 12; ++cycle)
            {
                std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorIfNeeded());
                editorsOk = editorsOk && editor != nullptr;
                if (editor == nullptr)
                    break;
                editor->addToDesktop(0);
                editor->setVisible(true);
                juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
                editor.reset();
            }
            processAudio.store(false, std::memory_order_release);
            audioThread.join();
            processedBlocks = liveBlocks.load(std::memory_order_relaxed);
        }
        else
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorIfNeeded());
            editorsOk = editorsOk && editor != nullptr;
            processedBlocks = 1;
        }

        const bool ok = allFinite(audio) && editorsOk && processedBlocks > 0 && !state.isEmpty();
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << description.name << "  "
                  << plugin->getParameters().size() << " params  latency " << plugin->getLatencySamples() << "\n";
        failures += ok ? 0 : 1;
        plugin->releaseResources();
    }
    if (identifiers.size() != 8)
    {
        std::cerr << "[FAIL] VST3 identifiers are not unique\n";
        ++failures;
    }
    return failures == 0 ? 0 : 1;
}

