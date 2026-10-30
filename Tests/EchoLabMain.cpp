// The same executable source captures M0 and verifies extracted/new Legacy processing.
// Raw renders are stereo/mono planar IEEE float32; no audio codec changes sample bits.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_cryptography/juce_cryptography.h>
#include "Standalone/ModulePluginProcessor.h"

using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;

namespace
{
struct Event { int sample; const char* id; double value; };
struct Case
{
    juce::String name;
    double rate = 48000, seconds = 2.0;
    int program = 0, block = 256, channels = 2;
    std::vector<std::pair<juce::String, double>> initial;
    std::vector<Event> events;
};

juce::String hash(const juce::MemoryBlock& bytes) { return juce::SHA256(bytes).toHexString(); }

std::vector<Case> cases()
{
    std::vector<Case> out;
    auto probe = std::make_unique<ModulePluginProcessor>(ModuleKind::Echo);
    for (double rate : { 44100.0, 48000.0, 96000.0 })
        for (int ch : { 1, 2 })
            for (int p = 0; p < probe->getNumPrograms(); ++p)
            {
                Case c;
                c.name = "program_" + juce::String(p) + "_" + juce::String(static_cast<int>(rate)) + "_ch" + juce::String(ch);
                c.rate = rate; c.program = p; c.channels = ch;
                out.push_back(c);
            }
    for (const auto& spec : probe->getSpec().parameters)
        for (int end : { 0, 1 })
        {
            Case c;
            c.name = "edge_" + spec.id + (end ? "_max" : "_min");
            c.seconds = spec.id.startsWith("time") ? 4.5 : 2.0;
            c.initial = { { "sync", 0 }, { "syncR", 0 }, { "link", 0 },
                          { spec.id, end ? spec.maximum : spec.minimum } };
            out.push_back(c);
        }
    for (int character = 0; character < 5; ++character)
        for (int mode : { 0, 1 })
            for (int block : { 1, 64, 127, 512, 4096 })
            {
                Case c;
                c.name = "engine_" + juce::String(character) + "_mode" + juce::String(mode) + "_block" + juce::String(block);
                c.block = block;
                c.initial = { { "character", character }, { "timeMode", mode }, { "sync", 0 },
                              { "syncR", 0 }, { "timeMs", 81 }, { "timeRMs", 139 }, { "link", 0 },
                              { "pitch", character == 0 ? 0 : 7 }, { "diffusion", .45 }, { "cross", .4 } };
                out.push_back(c);
            }
    for (int block : { 1, 64, 127, 512, 4096 })
    {
        Case c;
        c.name = "automation_block" + juce::String(block);
        c.block = block; c.seconds = 3.0;
        c.initial = { { "sync", 0 }, { "syncR", 0 }, { "timeMs", 85 }, { "timeRMs", 143 }, { "link", 0 } };
        c.events = { { 1234, "mix", .23 }, { 7001, "feedback", .8 }, { 12007, "character", 0 },
                     { 18003, "character", 4 }, { 24011, "timeMode", 1 }, { 28001, "timeMs", 217 },
                     { 37009, "pitch", -12 }, { 45013, "freeze", 1 }, { 67003, "freeze", 0 },
                     { 73001, "bypass", 1 }, { 91003, "bypass", 0 }, { 105007, "mix", 1 } };
        out.push_back(c);
    }
    // Exercise stop/restart after the 50 ms silent-tail retirement, not only a short bypass.
    for (int block : { 64, 127, 4096 })
    {
        Case c;
        c.name = "retire_restart_block" + juce::String(block);
        c.seconds = 3.0; c.block = block;
        c.initial = { { "sync", 0 }, { "timeMs", 10 }, { "feedback", 0 }, { "modDepth", 0 } };
        c.events = { { 16001, "bypass", 1 }, { 72001, "bypass", 0 } };
        out.push_back(c);
    }
    return out;
}

juce::MemoryBlock render(ModulePluginProcessor& p, const Case& c)
{
    const int total = static_cast<int>(std::lround(c.seconds * c.rate));
    juce::AudioBuffer<float> audio(c.channels, total);
    // Locally specified reproducible source: short bursts, asymmetric impulses, chirp and noise.
    uint32_t seed = 0x6d2b79f5;
    for (int i = 0; i < total; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        const double t = i / c.rate;
        const double noise = (static_cast<double>(seed >> 8) / 8388608.0 - 1.0) * .025;
        const double gate = (i % static_cast<int>(c.rate / 3.0)) < static_cast<int>(c.rate * .06) ? 1.0 : 0.0;
        for (int ch = 0; ch < c.channels; ++ch)
            audio.setSample(ch, i, static_cast<float>(gate * (.17 * std::sin(6.283185307179586 * (131.0 + ch * 53.0) * t)
                                                      + noise) + (i == 2000 + 137 * ch ? .6 : 0.0)));
    }
    p.prepareToPlay(c.rate, 256);
    juce::MidiBuffer midi;
    size_t event = 0;
    for (int start = 0; start < total;)
    {
        while (event < c.events.size() && c.events[event].sample == start)
        {
            p.setParameterValue(c.events[event].id, c.events[event].value);
            ++event;
        }
        int count = std::min(c.block, total - start);
        if (event < c.events.size()) count = std::min(count, c.events[event].sample - start);
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), c.channels, start, count);
        p.processBlock(view, midi);
        start += count;
    }
    juce::MemoryBlock bytes;
    for (int ch = 0; ch < c.channels; ++ch)
        bytes.append(audio.getReadPointer(ch), static_cast<size_t>(total) * sizeof(float));
    return bytes;
}

juce::var contract(ModulePluginProcessor& p)
{
    auto* root = new juce::DynamicObject;
    root->setProperty("stateTag", "HamloEcho");
    juce::Array<juce::var> params, programs;
    for (auto* raw : p.getParameters())
    {
        auto* item = new juce::DynamicObject;
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(raw);
        item->setProperty("id", ranged->paramID);
        item->setProperty("name", raw->getName(256));
        item->setProperty("defaultNormalised", raw->getDefaultValue());
        item->setProperty("steps", raw->getNumSteps());
        const auto& range = ranged->getNormalisableRange();
        item->setProperty("minimum", range.start); item->setProperty("maximum", range.end);
        item->setProperty("interval", range.interval); item->setProperty("skew", range.skew);
        juce::Array<juce::var> labels;
        for (const auto& text : raw->getAllValueStrings()) labels.add(text);
        item->setProperty("labels", labels);
        params.add(item);
    }
    for (int i = 0; i < p.getNumPrograms(); ++i) programs.add(p.getProgramName(i));
    root->setProperty("parameters", params); root->setProperty("programs", programs);
    return juce::var(root);
}

int run(bool capture, const juce::File& reference, const juce::File& report)
{
    if (capture && reference.exists())
        throw std::runtime_error("Refusing to overwrite an existing baseline directory");
    if (capture && !reference.createDirectory()) throw std::runtime_error("Cannot create baseline directory");
    juce::Array<juce::var> rows;
    auto probe = std::make_unique<ModulePluginProcessor>(ModuleKind::Echo);
    const auto layout = juce::JSON::toString(contract(*probe), true);
    const auto contractFile = reference.getChildFile("contract.json");
    if (capture) contractFile.replaceWithText(layout);
    int failures = !capture && contractFile.loadFileAsString() != layout ? 1 : 0;
    const auto all = cases();
    for (const auto& c : all)
    {
        auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Echo);
        p->setCurrentProgram(c.program);
        for (const auto& value : c.initial) p->setParameterValue(value.first, value.second);
        juce::MemoryBlock state;
        p->getStateInformation(state);
        const auto stateFile = reference.getChildFile(c.name + ".state");
        const auto rawFile = reference.getChildFile(c.name + ".f32");
        bool statePass = true;
        if (capture)
        {
            if (!stateFile.replaceWithData(state.getData(), state.getSize())) throw std::runtime_error("Cannot save state");
        }
        else
        {
            juce::MemoryBlock oldState;
            if (!stateFile.loadFileAsData(oldState)) throw std::runtime_error("Missing baseline state");
            // Test loading over an already changed instance; compare all original parameter values.
            p->setCurrentProgram(6);
            for (const auto& spec : p->getSpec().parameters) p->setParameterValue(spec.id, spec.maximum);
            p->setParameterValue("bypass", 1);
            p->setStateInformation(oldState.getData(), static_cast<int>(oldState.getSize()));
            juce::MemoryBlock restored;
            p->getStateInformation(restored);
            statePass = restored == state;
        }
        auto bytes = render(*p, c);
        bool pass = true, finite = true;
        double peak = 0;
        for (size_t i = 0; i < bytes.getSize() / sizeof(float); ++i)
        {
            const auto x = static_cast<const float*>(bytes.getData())[i];
            finite = finite && std::isfinite(x); peak = std::max(peak, std::abs(static_cast<double>(x)));
        }
        if (capture)
        {
            if (!rawFile.replaceWithData(bytes.getData(), bytes.getSize())) throw std::runtime_error("Cannot save render");
        }
        else
        {
            juce::MemoryBlock old;
            if (!rawFile.loadFileAsData(old)) throw std::runtime_error("Missing baseline render");
            pass = bytes == old;
        }
        if (!pass || !finite || !statePass || p->getLatencySamples() != 0) ++failures;
        auto* row = new juce::DynamicObject;
        row->setProperty("case", c.name); row->setProperty("rate", c.rate);
        row->setProperty("channels", c.channels); row->setProperty("block", c.block);
        row->setProperty("renderSha256", hash(bytes)); row->setProperty("stateSha256", hash(state));
        row->setProperty("bitExact", capture ? juce::var("n/a: baseline capture") : juce::var(pass));
        row->setProperty("stateRoundTrip", capture ? juce::var("n/a: baseline capture") : juce::var(statePass));
        row->setProperty("finite", finite); row->setProperty("peak", peak);
        row->setProperty("latency", p->getLatencySamples());
        row->setProperty("reportedTailSeconds", p->getTailLengthSeconds());
        rows.add(row);
        std::cout << (capture ? "CAPTURE " : pass && statePass ? "PASS " : "FAIL ") << c.name << std::endl;
    }
    auto* root = new juce::DynamicObject;
    root->setProperty("method", "EchoLab M0 capture v1; planar float32; prepare(256); exact event splits; source specified in source code");
    root->setProperty("cases", rows); root->setProperty("failures", failures);
    root->setProperty("newFeatures", "n/a: score, note targets, multiband, space, glitch and Send do not exist in M0");
    report.getParentDirectory().createDirectory();
    if (!report.replaceWithText(juce::JSON::toString(juce::var(root)))) throw std::runtime_error("Cannot save report");
    std::cout << all.size() << " cases, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    try
    {
        if (argc != 4 || (juce::String(argv[1]) != "capture" && juce::String(argv[1]) != "verify"))
        {
            std::cerr << "Usage: HamloEcho_Lab capture|verify <reference-dir> <report.json>\n";
            return 2;
        }
        return run(juce::String(argv[1]) == "capture", juce::File(argv[2]), juce::File(argv[3]));
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
