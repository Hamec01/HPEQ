// HamloRedWall_Reference <reference HamloRedWall.vst3> [out.md]
// Loads the reference plug-in (HPEQ's HamloRedWall module VST3) and this repository's processor, renders the
// same signals with the same settings through both and reports, per case, whether the outputs are
// bit-identical and the difference (out - reference) relative to the reference's RMS. Exit 0 when every
// case is within kLimitDb.

#include <iostream>
#include <map>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Plugin/HamloRedWallProcessor.h"
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

void setAll(juce::AudioProcessor& target, redwall::HamloRedWallProcessor& ours, const Case& c)
{
    // ours first (actual values, from the preset base as a program does), then the reference gets the same
    // normalised values
    for (const auto& spec : redwall::parameterSpecs())
        ours.setParameterValue(spec.id, spec.defaultValue);
    for (const auto& [id, v] : c.values)
        ours.setParameterValue(id, v);
    ours.setParameterValue("engine", 0.0); // the reference (HPEQ) has only the CLASSIC laws
    // a hosted VST3 parameter's ID is the VST3 hash, so the parameters are matched by name - and by which
    // occurrence of that name it is, since RedWall has two named "Release" (the time and Manual / Auto); both
    // lists keep the specs' order. (First version matched by name only and set both from the second.)
    std::map<juce::String, int> seen;
    for (auto* p : target.getParameters())
    {
        const auto name = p->getName(64);
        const int occurrence = seen[name]++;
        int k = 0;
        bool matched = false;
        for (const auto& spec : redwall::parameterSpecs())
            if (name == spec.name && k++ == occurrence)
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
    // the input then `latency` zeros, the output read from `latency`: HamloRedWall's colour stage adds a constant
    // delay HPEQ's module does not have, so both are compared aligned to their input
    const int latency = p.getLatencySamples();
    juce::AudioBuffer<float> y(x.getNumChannels(), x.getNumSamples() + latency);
    y.clear();
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, 0, x, ch, 0, x.getNumSamples());
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), y.getNumChannels(), start, juce::jmin(block, y.getNumSamples() - start));
        p.processBlock(view, midi);
    }
    p.releaseResources();
    juce::AudioBuffer<float> out(x.getNumChannels(), x.getNumSamples());
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        out.copyFrom(ch, 0, y, ch, latency, x.getNumSamples());
    return out;
}
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
    {
        std::cout << "usage: HamloRedWall_Reference <reference HamloRedWall.vst3> [report.md]\n";
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
    for (const auto& preset : redwall::factoryPresets())
    {
        // HPEQ's six presets only: they run CLASSIC; the v2 presets use the colour stage HPEQ does not have
        if (redwall::presetValue(preset, "engine", 1.0) != 0.0)
            continue;
        Case c { juce::String("Preset ") + preset.name, { { "mix", preset.mix } } };
        for (const auto& kv : preset.values)
            c.values.push_back(kv);
        cases.push_back(c);
    }
    // every style x TRUE PEAK x auto release pushed 6 dB; then link, monitor, output, mix, manual release / lookahead
    for (int style = 0; style < 3; ++style)
        for (int truePeak = 0; truePeak < 2; ++truePeak)
            for (int autoRelease = 0; autoRelease < 2; ++autoRelease)
                cases.push_back({ "Style " + juce::String(style) + ", true peak " + juce::String(truePeak) + ", auto " + juce::String(autoRelease),
                                  { { "thresholdDb", -6 }, { "ceilingDb", -1 }, { "style", style }, { "truePeak", truePeak }, { "autoRelease", autoRelease } } });
    cases.push_back({ "Link 0.3, release 15 ms, lookahead 0.5 ms", { { "thresholdDb", -9 }, { "link", 0.3 }, { "releaseMs", 15 }, { "lookaheadMs", 0.5 }, { "autoRelease", 0 } } });
    cases.push_back({ "Lookahead 10 ms, release 400 ms", { { "thresholdDb", -4 }, { "lookaheadMs", 10 }, { "releaseMs", 400 }, { "autoRelease", 0 } } });
    cases.push_back({ "Monitor unity", { { "thresholdDb", -6 }, { "monitor", 1 } } });
    cases.push_back({ "Monitor delta", { { "thresholdDb", -6 }, { "monitor", 2 } } });
    cases.push_back({ "Output -4, mix 0.6", { { "thresholdDb", -6 }, { "outputDb", -4 }, { "mix", 0.6 } } });

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
                    redwall::HamloRedWallProcessor ours;
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
                    // HamloRedWall starts at once (reported separately, not judged)
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
        juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]).replaceWithText("# HamloRedWall vs reference\n\nReference: `" + juce::String(argv[1]) + "`\n\n"
                                                                                          + summary + "\n\n" + report);
    return bad == 0 ? 0 : 1;
}
