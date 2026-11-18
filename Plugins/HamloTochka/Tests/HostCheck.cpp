#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Analysis/LoudnessMeter.h"
#include "DSP/PreEqProcessor.h"

namespace
{
using Signal = std::array<std::vector<float>, 2>;
constexpr double rate = 48000.0;
constexpr int length = 16384;
int checks = 0;

void check(bool condition, const char* label)
{
    if (!condition)
        throw std::runtime_error(label);
    ++checks;
}

juce::AudioProcessorParameter& parameter(juce::AudioPluginInstance& plugin, const juce::String& name)
{
    for (auto* p : plugin.getParameters())
        if (p->getName(128) == name)
            return *p;
    throw std::runtime_error("Missing host parameter: " + name.toStdString());
}

Signal signal()
{
    Signal result;
    for (size_t ch = 0; ch < result.size(); ++ch)
        for (int i = 0; i < length; ++i)
        {
            const auto t = 2.0 * juce::MathConstants<double>::pi * i / rate;
            result[ch].push_back(static_cast<float>(0.17 * std::sin((ch == 0 ? 1000.0 : 1170.0) * t)
                                                   + 0.03 * std::cos(86.0 * t)));
        }
    return result;
}

bool same(const Signal& a, const Signal& b, int channels)
{
    for (int ch = 0; ch < channels; ++ch)
    {
        const auto c = static_cast<size_t>(ch);
        if (a[c].size() != b[c].size())
            return false;
        if (std::memcmp(a[c].data(), b[c].data(), a[c].size() * sizeof(float)) != 0)
        {
            float peakError = 0.0f;
            size_t first = a[c].size();
            for (size_t i = 0; i < a[c].size(); ++i)
            {
                if (a[c][i] != b[c][i] && first == a[c].size())
                    first = i;
                peakError = std::max(peakError, std::abs(a[c][i] - b[c][i]));
            }
            std::cerr << "Channel " << c << " peak error " << peakError;
            if (first < a[c].size())
                std::cerr << ", first differing sample " << first << ": " << a[c][first] << " vs " << b[c][first];
            std::cerr << '\n';
            return false;
        }
    }
    return true;
}

void set(juce::AudioPluginInstance& plugin, const juce::String& name, float normalized)
{
    parameter(plugin, name).setValueNotifyingHost(normalized);
}

Signal render(juce::AudioPluginInstance& plugin, const Signal& input, int channels, int block, bool automate = false)
{
    auto layout = plugin.getBusesLayout();
    layout.inputBuses.set(0, channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo());
    layout.outputBuses.set(0, layout.inputBuses.getFirst());
    if (!plugin.setBusesLayout(layout))
        throw std::runtime_error("Unsupported test bus layout.");
    // The VST3 host must honour its negotiated maximum. Oversized native
    // callbacks are exercised separately in LocalCheck.
    plugin.prepareToPlay(rate, juce::jmax(256, block));
    auto result = input;
    juce::MidiBuffer midi;
    for (int pos = 0; pos < length;)
    {
        int end = juce::jmin(length, pos + block);
        if (automate)
        {
            if (pos == 8192)
                set(plugin, "Band 01 Gain", 0.375f);
            if (pos < 8192)
                end = juce::jmin(end, 8192);
        }
        std::array<float*, 2> ptr { result[0].data() + pos, result[1].data() + pos };
        juce::AudioBuffer<float> audio(ptr.data(), channels, end - pos);
        plugin.processBlock(audio, midi);
        pos = end;
    }
    plugin.releaseResources();
    return result;
}

Signal reference(const Signal& input, int channels, bool automate, bool dynamic)
{
    // A VST3 parameter crosses JUCE's normalized float range before reaching PRE-EQ.
    // Compare the same denormalized settings rather than the rounded engineering labels.
    const auto hostDefault = [](float value, float low, float high)
    {
        juce::NormalisableRange<float> range(low, high);
        range.setSkewForCentre(std::sqrt(low * high));
        return range.convertFrom0to1(range.convertTo0to1(value));
    };
    std::array<hpeq::dsp::EqBandParams, hpeq::kMaxEqNodesPerLayer> bands {};
    bands[0].active = true;
    bands[0].frequencyHz = hostDefault(1000.0f, 10.0f, 22000.0f);
    bands[0].q = hostDefault(0.707f, 0.1f, 18.0f);
    bands[0].ratio = hostDefault(3.0f, 1.0f, 20.0f);
    bands[0].attackMs = hostDefault(10.0f, 0.1f, 200.0f);
    bands[0].releaseMs = hostDefault(120.0f, 5.0f, 2000.0f);
    bands[0].gainDb = 6.0f;
    bands[0].dynamic = dynamic;
    hpeq::dsp::PreEqProcessor eq;
    eq.prepare(rate);
    eq.setTargets(bands);
    auto result = input;
    std::array<float*, 2> ptr { result[0].data(), result[1].data() };
    constexpr int half = length / 2;
    eq.process(ptr.data(), channels, half, 0);
    if (automate)
    {
        bands[0].gainDb = -6.0f;
        eq.setTargets(bands);
    }
    ptr = { result[0].data() + half, result[1].data() + half };
    eq.process(ptr.data(), channels, half, half % hpeq::dsp::kControlInterval);
    return result;
}
} // namespace

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    try
    {
        if (argc < 2)
            throw std::runtime_error("Usage: HamloTochka_HostCheck <VST3 DLL or bundle>");
        auto bundle = juce::File(juce::String::fromUTF8(argv[1]));
        if (!bundle.isDirectory() && bundle.getParentDirectory().getFileName() == "x86_64-win")
            bundle = bundle.getParentDirectory().getParentDirectory().getParentDirectory();

        juce::AudioPluginFormatManager formats;
        formats.addFormat(std::make_unique<juce::VST3PluginFormat>());
        juce::OwnedArray<juce::PluginDescription> descriptions;
        formats.getFormat(0)->findAllTypesForFile(descriptions, bundle.getFullPathName());
        check(descriptions.size() == 1 && descriptions[0]->name == "HamloTochka"
              && descriptions[0]->version == "0.1.0", "VST3 identity");
        const auto make = [&]()
        {
            juce::String error;
            auto plugin = formats.createPluginInstance(*descriptions[0], rate, 256, error);
            if (plugin == nullptr)
                throw std::runtime_error(error.toStdString());
            return plugin;
        };

        auto plugin = make();
        check(plugin->getLatencySamples() == 0, "zero reported latency");
        check(plugin->getNumPrograms() == 10 && plugin->getProgramName(8) == "Voice De-Ess", "factory programs");
        const auto input = signal();
        for (int channels : { 1, 2 })
            check(same(input, render(*plugin, input, channels, 127), channels), "bit-exact neutral pass-through");

        set(*plugin, "Band 01 Enabled", 1.0f);
        set(*plugin, "Band 01 Gain", 0.625f);
        check(same(reference(input, 1, false, false), render(*plugin, input, 1, 127), 1), "mono matches PRE-EQ");
        check(same(reference(input, 2, false, false), render(*plugin, input, 2, 127), 2), "stereo matches PRE-EQ");

        set(*plugin, "Band 01 Dynamic", 1.0f);
        const auto automated = reference(input, 2, true, true);
        for (int block : { 1, 64, 127, 512, 4096 })
        {
            set(*plugin, "Band 01 Gain", 0.625f);
            const auto actual = render(*plugin, input, 2, block, true);
            for (const auto& channel : actual)
                check(std::all_of(channel.begin(), channel.end(), [](float x) { return std::isfinite(x); }), "finite automation");
            check(same(automated, actual, 2), "dynamic EQ and automation match PRE-EQ at every block size");
        }

        set(*plugin, "Match Gain", 0.375f); // -3 dB
        set(*plugin, "Band 24 Enabled", 1.0f);
        set(*plugin, "Band 24 Shape", 0.25f); // high shelf
        set(*plugin, "Band 24 Gain", 0.625f);
        juce::MemoryBlock saved;
        plugin->getStateInformation(saved);
        auto restored = make();
        restored->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
        check(parameter(*restored, "Band 24 Enabled").getValue() == 1.0f
              && parameter(*restored, "Match Gain").getValue() == 0.375f, "restored state and output match");
        check(same(render(*plugin, input, 2, 256), render(*restored, input, 2, 256), 2), "restored audio");

        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(restored->createEditorIfNeeded());
        check(editor != nullptr, "VST3 editor created");
        std::cout << "HamloTochka: " << checks << " VST3 checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "HamloTochka VST3 check failed: " << error.what() << '\n';
        return 1;
    }
}
