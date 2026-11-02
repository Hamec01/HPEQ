// Immutable M0 fixture protocol. A check reuses recorded old states and identical block/event schedules.
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_cryptography/juce_cryptography.h>
#include "Standalone/ModulePluginProcessor.h"

using Processor = hpeq::standalone::ModulePluginProcessor;
namespace
{
struct Case { juce::String name; int program; double predelay = -1, decay = -1; bool automate = false; };
std::vector<Case> cases()
{
    std::vector<Case> result;
    for (int p = 0; p < 8; ++p) result.push_back({ "program" + juce::String(p), p });
    result.push_back({ "classic250_20", 0, 250, 20 });
    result.push_back({ "classic500_30", 0, 500, 30 });
    result.push_back({ "classic0_01", 0, 0, .1 });
    result.push_back({ "classic500_01", 0, 500, .1 });
    result.push_back({ "automation", 0, -1, -1, true });
    return result;
}
void configure(Processor& p, const Case& c)
{
    p.setCurrentProgram(c.program);
    if (c.predelay >= 0)
    {
        p.setParameterValue("space", 0); p.setParameterValue("predelayMs", c.predelay);
        p.setParameterValue("decaySeconds", c.decay);
    }
}
juce::MemoryBlock render(Processor& p, double fs, int channels, int block, bool automation)
{
    const int samples = static_cast<int>(1.6 * fs);
    juce::AudioBuffer<float> audio(channels, samples);
    std::uint32_t rng = 0x38979ab;
    for (int i = 0; i < samples; ++i)
    {
        rng = 1664525u * rng + 1013904223u;
        const double noise = (static_cast<double>(rng) / 4294967296.0 - .5) * .12;
        const double t = i / fs;
        const double env = std::exp(-std::fmod(t, .173) / .027);
        const double dry = t < 1.2 ? env * (.32 * std::sin(6.283185307179586 * (110 * t + 25 * t * t)) + noise) : 0;
        audio.setSample(0, i, static_cast<float>(dry));
        if (channels == 2) audio.setSample(1, i, static_cast<float>(dry * .71 + (t < 1.2 ? noise * .29 : 0)));
    }
    const auto channelSet = channels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
    auto layout = p.getBusesLayout(); layout.inputBuses.set(0, channelSet); layout.outputBuses.set(0, channelSet);
    if (!p.setBusesLayout(layout)) throw std::runtime_error("Unsupported reference layout");
    p.prepareToPlay(fs, 256);
    const std::array<int, 5> at { static_cast<int>(fs * .25), static_cast<int>(fs * .5),
        static_cast<int>(fs * .65), static_cast<int>(fs * .9), static_cast<int>(fs * 1.1) };
    int event = 0;
    juce::MidiBuffer midi;
    for (int start = 0; start < samples;)
    {
        if (automation && event < 5 && start == at[static_cast<size_t>(event)])
        {
            switch (event++)
            {
                case 0: p.setParameterValue("mix", .17); break;
                case 1: p.setParameterValue("bypass", 1); break;
                case 2: p.setParameterValue("bypass", 0); break;
                case 3: p.setParameterValue("space", 4); break;
                case 4: p.setParameterValue("space", 2); break;
            }
        }
        int count = std::min(block, samples - start);
        if (automation && event < 5) count = std::min(count, at[static_cast<size_t>(event)] - start);
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), channels, start, count);
        p.processBlock(view, midi);
        start += count;
    }
    juce::MemoryBlock data;
    for (int ch = 0; ch < channels; ++ch) data.append(audio.getReadPointer(ch), static_cast<size_t>(samples) * sizeof(float));
    return data;
}
}
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 4) { std::cerr << "Usage: HamloVerb_Reference record|check fixture-directory report.md\n"; return 2; }
    const bool record = juce::String(argv[1]) == "record";
    const juce::File directory = juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
    const juce::File report = juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]);
    if (record) directory.createDirectory();
    auto probe = std::make_unique<Processor>(hpeq::standalone::ModuleKind::Verb);
    if (record && probe->parameter("engine") != nullptr)
    { std::cerr << "Refusing to overwrite M0 fixtures using a V2 build.\n"; return 2; }
    probe.reset();
    juce::String out = "# M0 bit-exact reference " + juce::String(record ? "capture" : "check") + "\n\n"
        "Source baseline: `02446a7787643e74c438d584713265b74be9194b`. Original VST3 SHA256: "
        "`e270991f4201250dfbdcfacfb2d5d69807038318f7251bcb57403946837b90ff`.\n\n"
        "Protocol: deterministic 1.6 s input, 1.2 s excitation + 0.4 s tail, prepare(256), "
        "mono/stereo, 44.1/48/96 kHz, blocks 1/64/127/512/4096. Automation events are split at identical sample positions. "
        "Legacy space-switch results are compared at the same block size, not falsely asserted block invariant.\n\n"
        "| Case | Rate | Channels | Block | SHA256 | Result |\n|---|---:|---:|---:|---|---|\n";
    int passed = 0, failed = 0;
    for (const auto& c : cases())
    {
        juce::MemoryBlock state;
        const auto stateFile = directory.getChildFile(c.name + ".state");
        if (record)
        {
            auto p = std::make_unique<Processor>(hpeq::standalone::ModuleKind::Verb); configure(*p, c);
            p->getStateInformation(state);
            if (!stateFile.replaceWithData(state.getData(), state.getSize())) return 2;
            if (auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), static_cast<int>(state.getSize())))
                directory.getChildFile(c.name + ".xml").replaceWithText(xml->toString());
        }
        else if (!stateFile.loadFileAsData(state)) { std::cerr << "Missing state " << c.name << "\n"; return 2; }
        for (double fs : { 44100.0, 48000.0, 96000.0 }) for (int channels : { 1, 2 }) for (int block : { 1, 64, 127, 512, 4096 })
        {
            auto p = std::make_unique<Processor>(hpeq::standalone::ModuleKind::Verb);
            p->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            const auto audio = render(*p, fs, channels, block, c.automate);
            const auto hash = juce::SHA256(audio.getData(), audio.getSize()).toHexString();
            const auto key = c.name + "_" + juce::String(static_cast<int>(fs)) + "_" + juce::String(channels) + "_" + juce::String(block);
            const auto hashFile = directory.getChildFile(key + ".sha256");
            bool success = false;
            if (record)
            {
                success = hashFile.replaceWithText(hash + "\n");
                // One raw planar float fixture per case enables investigation in addition to hashes.
                if (fs == 48000 && channels == 2 && block == 512)
                    success = directory.getChildFile(key + ".f32").replaceWithData(audio.getData(), audio.getSize()) && success;
            }
            else success = hashFile.existsAsFile() && hashFile.loadFileAsString().trim() == hash;
            success ? ++passed : ++failed;
            out += "| " + c.name + " | " + juce::String(static_cast<int>(fs)) + " | " + juce::String(channels) + " | "
                + juce::String(block) + " | " + hash + " | " + (success ? "PASS" : "FAIL") + " |\n";
            if (!success) std::cout << "FAIL " << key << "\n";
        }
        std::cout << c.name << ": " << passed << " passed, " << failed << " failed" << std::endl;
    }
    out += "\nResult: **" + juce::String(passed) + " passed, " + juce::String(failed) + " failed**.\n";
    report.getParentDirectory().createDirectory();
    if (!report.replaceWithText(out)) return 2;
    return failed == 0 ? 0 : 1;
}
