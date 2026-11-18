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

float scalableParameter(float value, float low, float high)
{
    juce::NormalisableRange<float> range(low, high);
    range.setSkewForCentre(std::sqrt(low * high));
    return range.convertTo0to1(value);
}

struct KeyFrequencies
{
    double target = 125.0;
    double probe = 2000.0;
    double offBand = 4000.0;
    double body = 0.0;
};

std::vector<float> renderKey(juce::AudioPluginInstance& plugin, int keyChannels,
                             bool audibleMain = true, int mainChannels = 2, KeyFrequencies frequencies = {})
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    auto layout = plugin.getBusesLayout();
    layout.inputBuses.set(0, mainChannels == 1 ? mono : stereo);
    layout.inputBuses.set(1, keyChannels == 2 ? stereo : keyChannels == 1 ? mono : juce::AudioChannelSet::disabled());
    layout.outputBuses.set(0, layout.inputBuses.getFirst());
    check(plugin.setBusesLayout(layout), "host accepted the requested sidechain layout");
    plugin.prepareToPlay(rate, 512);

    constexpr auto total = static_cast<int>(2.0 * rate);
    constexpr int block = 512;
    std::vector<float> result;
    result.reserve(total);
    juce::AudioBuffer<float> buffer(mainChannels + keyChannels, block);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total; pos += block)
    {
        const auto frames = juce::jmin(block, total - pos);
        buffer.setSize(mainChannels + keyChannels, frames, false, false, true);
        for (int frame = 0; frame < frames; ++frame)
        {
            const auto t = (pos + frame) / rate;
            const auto phase = 2.0 * juce::MathConstants<double>::pi * t;
            const auto program = audibleMain ? static_cast<float>(0.15 * std::sin(frequencies.target * phase)
                                                                  + 0.06 * std::sin(frequencies.probe * phase)) : 0.0f;
            const auto key = t >= 0.5 && t < 1.0
                ? static_cast<float>(0.35 * std::sin(frequencies.target * phase) + 0.12 * std::sin(frequencies.body * phase))
                : t >= 1.55 ? static_cast<float>(0.35 * std::sin(frequencies.offBand * phase)) : 0.0f;
            for (int ch = 0; ch < mainChannels; ++ch)
                buffer.setSample(ch, frame, program);
            if (keyChannels == 2)
                buffer.setSample(mainChannels, frame, 0.0f);
            if (keyChannels > 0)
                buffer.setSample(mainChannels + keyChannels - 1, frame, key);
        }
        plugin.processBlock(buffer, midi);
        result.insert(result.end(), buffer.getReadPointer(0), buffer.getReadPointer(0) + frames);
    }
    plugin.releaseResources();
    return result;
}

double toneLevel(const std::vector<float>& signal, double hz, double startSeconds, double endSeconds)
{
    const auto start = static_cast<int>(startSeconds * rate);
    const auto end = static_cast<int>(endSeconds * rate);
    std::complex<double> projection {};
    for (int i = start; i < end; ++i)
        projection += static_cast<double>(signal[static_cast<size_t>(i)])
                     * std::polar(1.0, -2.0 * juce::MathConstants<double>::pi * hz * i / rate);
    return juce::Decibels::gainToDecibels(2.0 * std::abs(projection) / (end - start), -200.0);
}

void checkSidechain(juce::AudioPluginInstance& plugin)
{
    set(plugin, "Band 01 Enabled", 1.0f);
    set(plugin, "Band 01 Dynamic", 1.0f);
    set(plugin, "Band 01 External Key", 1.0f);
    set(plugin, "Band 01 Frequency", scalableParameter(125.0f, 10.0f, 22000.0f));
    set(plugin, "Band 01 Q", scalableParameter(4.0f, 0.1f, 18.0f));
    set(plugin, "Band 01 Threshold", 0.6f); // -24 dB

    check(plugin.getBusCount(true) == 2 && plugin.getLatencySamples() == 0, "external bus and zero latency");
    for (int channels : { 2, 1 })
    {
        const auto signal = renderKey(plugin, channels, true, channels);
        const auto dry125 = toneLevel(signal, 125.0, 0.2, 0.4);
        const auto cut = toneLevel(signal, 125.0, 0.8, 1.0) - dry125;
        const auto other = toneLevel(signal, 2000.0, 0.8, 1.0) - toneLevel(signal, 2000.0, 0.2, 0.4);
        const auto recovered = toneLevel(signal, 125.0, 1.4, 1.5) - dry125;
        const auto offBand = toneLevel(signal, 125.0, 1.8, 2.0) - dry125;
        std::cout << "Key " << channels << " ch: target " << cut << ", outside " << other
                  << ", released " << recovered << ", off-band trigger " << offBand << " dB\n";
        check(std::abs(cut + 6.0) <= 0.2, "keyed bell reaches its requested depth");
        check(std::abs(other) <= 0.1, "outside frequency remains within tolerance");
        check(std::abs(recovered) <= 0.1, "gain returns after the key releases");
        check(std::abs(offBand) <= 0.1, "out-of-band key does not trigger the bell");
    }

    const auto withoutKey = renderKey(plugin, 0);
    check(std::abs(toneLevel(withoutKey, 125.0, 0.8, 1.0) - juce::Decibels::gainToDecibels(0.15)) <= 0.1,
          "disabled key bus does not fall back to the main signal");
    const auto withoutProgram = renderKey(plugin, 2, false);
    check(std::all_of(withoutProgram.begin(), withoutProgram.end(), [](float x) { return x == 0.0f; }),
          "key audio stays out of the output");
}

void checkVocalPocket(juce::AudioPluginInstance& plugin)
{
    constexpr KeyFrequencies vocal { 2500.0, 250.0, 100.0, 700.0 };
    set(plugin, "Band 01 Enabled", 1.0f);
    set(plugin, "Band 01 Dynamic", 1.0f);
    set(plugin, "Band 01 External Key", 1.0f);
    set(plugin, "Band 01 Frequency", scalableParameter(static_cast<float>(vocal.target), 10.0f, 22000.0f));
    set(plugin, "Band 01 Q", scalableParameter(4.0f, 0.1f, 18.0f));
    set(plugin, "Band 01 Threshold", 0.6f);
    const auto signal = renderKey(plugin, 1, true, 2, vocal);
    const auto targetBefore = toneLevel(signal, vocal.target, 0.2, 0.4);
    const auto cut = toneLevel(signal, vocal.target, 0.8, 1.0) - targetBefore;
    const auto low = toneLevel(signal, vocal.probe, 0.8, 1.0) - toneLevel(signal, vocal.probe, 0.2, 0.4);
    const auto recovered = toneLevel(signal, vocal.target, 1.4, 1.5) - targetBefore;
    std::cout << "Vocal key: presence " << cut << ", low probe " << low << ", released " << recovered << " dB\n";
    check(std::abs(cut + 6.0) <= 0.2 && std::abs(low) <= 0.1 && std::abs(recovered) <= 0.1,
          "vocal key opens a presence pocket and releases it");
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
              && descriptions[0]->version == "0.3.0", "VST3 identity");
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
        check(plugin->getNumPrograms() == 210 && plugin->getProgramName(8) == "Voice De-Ess"
              && plugin->getProgramName(10) == "BB Floor Rumble Cut"
              && plugin->getProgramName(209) == "DNB Mid Definition", "factory programs");
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

        auto keyed = make();
        checkSidechain(*keyed);
        juce::MemoryBlock keyState;
        keyed->getStateInformation(keyState);
        auto restoredKey = make();
        restoredKey->setStateInformation(keyState.getData(), static_cast<int>(keyState.getSize()));
        check(parameter(*restoredKey, "Band 01 External Key").getValue() == 1.0f, "external key source saved");
        check(renderKey(*restoredKey, 2) == renderKey(*keyed, 2), "external signal remains identical when reloaded");
        auto vocalPocket = make();
        checkVocalPocket(*vocalPocket);

        if (argc >= 3)
        {
            juce::OwnedArray<juce::PluginDescription> oldDescriptions;
            formats.getFormat(0)->findAllTypesForFile(oldDescriptions, juce::String::fromUTF8(argv[2]));
            check(oldDescriptions.size() == 1
                  && (oldDescriptions[0]->version == "0.1.0" || oldDescriptions[0]->version == "0.2.0")
                  && descriptions[0]->uniqueId == oldDescriptions[0]->uniqueId, "legacy VST3 identity retained");
            const auto hasLegacyKey = oldDescriptions[0]->version == "0.2.0";
            juce::String error;
            auto old = formats.createPluginInstance(*oldDescriptions[0], rate, 256, error);
            check(old != nullptr, "legacy instance");
            for (int p = 0; p < old->getNumPrograms(); ++p)
                check(old->getProgramName(p) == plugin->getProgramName(p), "legacy program index retained");
            set(*old, "Band 01 Enabled", 1.0f);
            set(*old, "Band 01 Dynamic", 1.0f);
            set(*old, "Band 01 Gain", 0.625f);
            const auto baseline = render(*old, input, 2, 127);
            juce::MemoryBlock oldState;
            old->getStateInformation(oldState);
            auto migration = make();
            set(*migration, "Band 01 External Key", 1.0f);
            render(*migration, input, 2, 256);
            migration->setStateInformation(oldState.getData(), static_cast<int>(oldState.getSize()));
            check(parameter(*migration, "Band 01 External Key").getValue() == 0.0f, "legacy state restores self detection");
            check(same(baseline, render(*migration, input, 2, 127), 2), "legacy session audio stays bit-exact");
            if (hasLegacyKey)
            {
                set(*old, "Band 01 Gain", 0.5f);
                checkSidechain(*old);
                const auto keyedBaseline = renderKey(*old, 2);
                juce::MemoryBlock oldKeyState;
                old->getStateInformation(oldKeyState);
                migration->setStateInformation(oldKeyState.getData(), static_cast<int>(oldKeyState.getSize()));
                check(renderKey(*migration, 2) == keyedBaseline, "0.2 keyed session audio stays bit-exact");
            }
        }
        std::cout << "HamloTochka: " << checks << " VST3 checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "HamloTochka VST3 check failed: " << error.what() << '\n';
        return 1;
    }
}
