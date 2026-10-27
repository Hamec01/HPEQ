// HamloUdarnik_Reference <reference HamloUdarnik.vst3> [out.md]
// Loads the reference plug-in (HPEQ's HamloUdarnik module VST3) and this repository's processor, renders the
// same signals with the same settings through both and reports, per case, whether the outputs are
// bit-identical and the difference (out - reference) relative to the reference's RMS. Exit 0 when every
// case is within kLimitDb.

#include <iostream>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Plugin/HamloUdarnikProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"
#include "TestSignals.h"

namespace
{
using namespace hamlo;
constexpr double kLimitDb = -120.0; // fixed before the runs: below float rounding of a 24-bit render

struct Case
{
    juce::String name;
    std::vector<std::pair<const char*, double>> values; // parameter id, actual value
};

void setAll(juce::AudioProcessor& target, udarnik::HamloUdarnikProcessor& ours, const Case& c)
{
    // ours first (actual values, from the preset base as a program does), then the reference gets the same
    // normalised values
    for (const auto& spec : udarnik::parameterSpecs())
        ours.setParameterValue(spec.id, spec.defaultValue);
    for (const auto& [id, v] : c.values)
        ours.setParameterValue(id, v);
    // a hosted VST3 parameter's ID is the VST3 hash, so the parameters are matched by name
    for (auto* p : target.getParameters())
    {
        const auto name = p->getName(64);
        bool matched = false;
        for (const auto& spec : udarnik::parameterSpecs())
            if (name == spec.name)
            {
                p->setValueNotifyingHost(ours.parameter(spec.id)->getValue());
                matched = true;
            }
        if (!matched && name == "Bypass")
            p->setValueNotifyingHost(0.0f);
    }
}

juce::AudioBuffer<float> render(juce::AudioProcessor& p, const juce::AudioBuffer<float>& x, double fs, int block)
{
    p.setRateAndBufferSizeDetails(fs, block);
    p.prepareToPlay(fs, block);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), y.getNumChannels(), start, juce::jmin(block, y.getNumSamples() - start));
        p.processBlock(view, midi);
    }
    p.releaseResources();
    return y;
}
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
    {
        std::cout << "usage: HamloUdarnik_Reference <reference HamloUdarnik.vst3> [report.md]\n";
        return 2;
    }
    juce::AudioPluginFormatManager formats;
    formats.addFormat(std::make_unique<juce::VST3PluginFormat>());
    juce::OwnedArray<juce::PluginDescription> found;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile(found, juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]).getFullPathName());
    if (found.isEmpty())
    {
        std::cout << "no plug-in in " << argv[1] << "\n";
        return 2;
    }

    std::vector<Case> cases { { "Default", {} } };
    for (const auto& preset : udarnik::factoryPresets())
    {
        Case c { juce::String("Preset ") + preset.name, { { "mix", preset.mix } } };
        for (const auto& kv : preset.values)
            c.values.push_back(kv);
        cases.push_back(c);
    }
    // every mode x look-ahead with SNAP and BODY set; then PERESTROIKA's RISE / DECAY, SENSITIVITY, PRESERVE, LISTEN, OUTPUT, MIX
    for (int style = 0; style < 3; ++style)
        for (int lookahead = 0; lookahead < 3; ++lookahead)
            cases.push_back({ "Mode " + juce::String(style) + ", look-ahead " + juce::String(lookahead),
                              { { "attackDb", 7 }, { "sustainDb", -4 }, { "speed", 0.6 }, { "style", style }, { "lookahead", lookahead } } });
    cases.push_back({ "PERESTROIKA rise 1.6, decay 0.6", { { "attackDb", 4 }, { "style", 2 }, { "rise", 1.6 }, { "decay", 0.6 }, { "lookahead", 2 } } });
    cases.push_back({ "UDARNIK sensitivity 0.4, preserve 0", { { "attackDb", 8 }, { "style", 1 }, { "sensitivity", 0.4 }, { "preserve", 0 } } });
    cases.push_back({ "PRAVDA listen attack", { { "attackDb", 6 }, { "sustainDb", 3 }, { "listen", 1 } } });
    cases.push_back({ "PRAVDA listen body", { { "attackDb", 6 }, { "sustainDb", 3 }, { "listen", 2 } } });
    cases.push_back({ "UDARNIK output -5, mix 0.6", { { "attackDb", 5 }, { "style", 1 }, { "outputDb", -5 }, { "mix", 0.6 } } });

    juce::String report = "| Rate / block | Signal | Case | Bit-identical from 0.5 s | Difference from 0.5 s | First 0.5 s (start-up, not judged) |\n|---|---|---|---|---|---|\n";
    int bad = 0, identical = 0, total = 0;
    double worst = -400.0;
    for (const double fs : { 48000.0, 44100.0 })
        for (const int block : { 512, 97 })
        {
            // the signals are built at 48 kHz sample counts; at 44.1 kHz they are the same samples, read slower
            const std::vector<std::pair<juce::String, juce::AudioBuffer<float>>> signals { { "drums", test::drums(6.0) },
                                                                                            { "sine -6 dBFS", test::sine(3.0, 220.0, -6.0) } };
            for (const auto& [signalName, x] : signals)
                for (const auto& c : cases)
                {
                    juce::String error;
                    auto reference = formats.createPluginInstance(*found[0], fs, block, error);
                    if (reference == nullptr)
                    {
                        std::cout << "could not load the reference: " << error << "\n";
                        return 2;
                    }
                    reference->enableAllBuses();
                    udarnik::HamloUdarnikProcessor ours;
                    setAll(*reference, ours, c);
                    const auto a = render(*reference, x, fs, block);
                    const auto b = render(ours, x, fs, block);
                    if (juce::SystemStats::getEnvironmentVariable("HAMLOCOMP_REF_DEBUG", {}).isNotEmpty())
                    {
                        juce::String names;
                        for (auto* p : reference->getParameters())
                            if (auto* h = dynamic_cast<juce::HostedAudioProcessorParameter*>(p))
                                names << h->getParameterID() << "=" << juce::String(h->getValue(), 3) << " ";
                        for (int s = 0; s + 24000 <= a.getNumSamples(); s += 24000)
                            std::cout << "  " << s / 48000.0 << " s: ref " << test::rmsDb(test::segment(a, s, 24000)) << " ours " << test::rmsDb(test::segment(b, s, 24000))
                                      << " diff " << test::differenceDb(test::segment(b, s, 24000), test::segment(a, s, 24000)) << std::endl;
                        int first = -1;
                        for (int i = 0; i < a.getNumSamples() && first < 0; ++i)
                            if (a.getSample(0, i) != b.getSample(0, i))
                                first = i;
                        std::cout << "  first different sample " << first << ": in " << x.getSample(0, juce::jmax(0, first)) << " ref " << a.getSample(0, juce::jmax(0, first)) << " ours " << b.getSample(0, juce::jmax(0, first)) << std::endl;
                        std::cout << c.name << ": in " << test::rmsDb(x) << " ref " << test::rmsDb(a) << " ours " << test::rmsDb(b) << " | " << names << std::endl;
                    }
                    // steady state from 0.5 s: in its first 20 ms HPEQ fades the module in from the dry signal,
                    // HamloUdarnik starts at once (reported separately, not judged)
                    const int settle = static_cast<int>(0.5 * fs), length = a.getNumSamples() - settle;
                    const auto as = test::segment(a, settle, length), bs = test::segment(b, settle, length);
                    const bool same = test::bitExact(bs, as);
                    const double diff = test::differenceDb(bs, as);
                    const double startDiff = test::differenceDb(test::segment(b, 0, settle), test::segment(a, 0, settle));
                    ++total;
                    identical += same ? 1 : 0;
                    worst = std::max(worst, diff);
                    bad += diff <= kLimitDb ? 0 : 1;
                    report << "| " << juce::String(fs / 1000.0, 1) << " kHz / " << block << " | " << signalName << " | " << c.name << " | " << (same ? "yes" : "no")
                           << " | " << (same ? juce::String("-inf") : juce::String(diff, 1) + " dB") << (diff <= kLimitDb ? "" : " **over**") << " | "
                           << juce::String(startDiff, 1) << " dB |\n";
                }
        }
    const auto summary = juce::String(total) + " cases: " + juce::String(identical) + " bit-identical, worst difference "
                       + (worst <= -400.0 ? juce::String("-inf") : juce::String(worst, 1) + " dB") + " (limit " + juce::String(kLimitDb, 0) + " dB), "
                       + juce::String(bad) + " over the limit";
    std::cout << summary << "\n";
    if (argc > 2)
        juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]).replaceWithText("# HamloUdarnik vs reference\n\nReference: `" + juce::String(argv[1]) + "`\n\n"
                                                                                          + summary + "\n\n" + report);
    return bad == 0 ? 0 : 1;
}
