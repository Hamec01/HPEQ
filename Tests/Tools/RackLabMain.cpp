// HamloRack RACK1. The same immutable fixture and renderer produce the before
// and after receipts. This is a compatibility probe, not preset calibration.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_cryptography/juce_cryptography.h>
#include "Plugin/FxPluginProcessor.h"
#include "Plugin/Presets.h"

namespace
{
constexpr double rate = 48000.0;
constexpr int preparedBlock = 256, hostBlock = 127, frames = 144000;

struct Head final : juce::AudioPlayHead
{
    int sample = 0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm(120.0); p.setPpqPosition(sample * 120.0 / (60.0 * rate));
        p.setTimeInSamples(sample); p.setIsPlaying(true);
        return p;
    }
};

float triangle(int i, int period)
{
    const auto phase = static_cast<float>(i % period) / static_cast<float>(period);
    return 1.0f - 4.0f * std::abs(phase - 0.5f);
}

std::vector<float> fixture()
{
    std::vector<float> x(static_cast<size_t>(frames) * 2);
    std::uint32_t seed = 0x684d1753u;
    for (int i = 0; i < frames; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        const float noise = static_cast<float>(static_cast<int>(seed >> 9) - 4194304) / 4194304.0f;
        const int hit = i % 12000;
        const float env = hit < 2400 ? static_cast<float>(2400-hit) / 2400.0f : 0.0f;
        const float note = (i / 6000) % 3 == 2 ? 0.0f : .045f;
        const float kick = .24f * env * triangle(i, 820);
        const float snare = ((i / 12000) % 2 == 1 ? .12f : .025f) * env * noise;
        const float hat = i % 3000 < 250 ? .035f * noise : 0.0f;
        const float body = .055f * triangle(i, 437) + note * triangle(i, 109);
        if (i < 96000)
        {
            x[static_cast<size_t>(2*i)] = kick + snare + hat + body;
            x[static_cast<size_t>(2*i+1)] = kick + .92f*snare - .8f*hat + .96f*body;
        }
    }
    return x;
}

juce::String sha(const void* data, size_t size) { return juce::SHA256(data, size).toHexString(); }

int legacy(bool checking, const juce::File& reference, const juce::File& reportFile)
{
    reference.createDirectory();
    const auto x = fixture();
    const auto fixtureHash = sha(x.data(), x.size()*sizeof(float));
    const juce::String configuration = "rack1-legacy-v1;48000Hz;stereo;prepare256;block127;120BPM;input2s;tail1s";
    juce::Array<juce::var> entries;
    int failures = 0, rendered = 0, cached = 0;
    auto plugin = std::make_unique<hpeq::FxPluginProcessor>();
    Head head; plugin->setPlayHead(&head);
    const auto& presets = hpeq::presets::factoryPresets();
    for (size_t index = 0; index < presets.size(); ++index)
    {
        const auto& preset = presets[index];
        const auto id = juce::String(static_cast<int>(index)+1).paddedLeft('0',3);
        const auto file = reference.getChildFile(id+".f32");
        const auto identity = reference.getChildFile(id+".json");
        const auto stateHash = sha(preset.data, static_cast<size_t>(preset.size));
        const auto previous = identity.existsAsFile() ? juce::JSON::parse(identity) : juce::var();
        if (!checking && file.existsAsFile() && previous["stateSha256"].toString()==stateHash
            && previous["fixtureSha256"].toString()==fixtureHash && previous["configuration"].toString()==configuration)
        {
            juce::MemoryBlock bytes; file.loadFileAsData(bytes);
            if (sha(bytes.getData(),bytes.getSize())==previous["outputSha256"].toString())
            { entries.add(previous); ++cached; continue; }
        }
        juce::DynamicObject::Ptr row = new juce::DynamicObject();
        row->setProperty("id",id); row->setProperty("name",preset.name);
        row->setProperty("configuration",configuration); row->setProperty("stateSha256",stateHash);
        row->setProperty("fixtureSha256",fixtureHash);
        if (!plugin->loadPreset(preset.data,preset.size,preset.name))
        { ++failures; row->setProperty("error","load failed"); entries.add(juce::var(row.get())); continue; }
        head.sample = 0;
        plugin->prepareToPlay(rate,preparedBlock);
        std::vector<float> y(x.size());
        juce::AudioBuffer<float> work(2,hostBlock);
        juce::MidiBuffer midi;
        bool finite = true;
        for (int offset = 0; offset < frames; offset += hostBlock)
        {
            const int n = std::min(hostBlock,frames-offset);
            work.setSize(2,n,false,false,true);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n; ++i) work.setSample(ch,i,x[static_cast<size_t>((offset+i)*2+ch)]);
            head.sample = offset;
            plugin->processBlock(work,midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n; ++i)
                {
                    const float value = work.getSample(ch,i);
                    finite = finite && std::isfinite(value);
                    y[static_cast<size_t>((offset+i)*2+ch)] = value;
                }
        }
        plugin->releaseResources(); ++rendered;
        const auto bytes = y.size()*sizeof(float);
        const auto outputHash = sha(y.data(),bytes);
        row->setProperty("outputSha256",outputHash); row->setProperty("latency",plugin->getLatencySamples());
        row->setProperty("finite",finite);
        bool passed = finite;
        if (checking)
        {
            juce::MemoryBlock old; const bool read = file.loadFileAsData(old);
            passed = passed && read && old.getSize()==bytes && std::memcmp(y.data(),old.getData(),bytes)==0
                && previous["stateSha256"].toString()==stateHash && previous["fixtureSha256"].toString()==fixtureHash
                && previous["configuration"].toString()==configuration
                && static_cast<int>(previous["latency"])==plugin->getLatencySamples();
            row->setProperty("exact",passed);
        }
        else
        {
            passed = passed && file.replaceWithData(y.data(),bytes);
            identity.replaceWithText(juce::JSON::toString(juce::var(row.get()),true));
        }
        if (!passed) { ++failures; std::cout << "FAIL " << id << ' ' << preset.name << std::endl; }
        else if ((index+1)%16==0) std::cout << (checking?"checked ":"rendered ") << index+1 << '/' << presets.size() << std::endl;
        entries.add(juce::var(row.get()));
    }
    auto report = std::make_unique<juce::DynamicObject>();
    report->setProperty("mode",checking?"legacy-check":"legacy-write"); report->setProperty("configuration",configuration);
    report->setProperty("fixtureSha256",fixtureHash); report->setProperty("rendered",rendered);
    report->setProperty("cached",cached); report->setProperty("failures",failures); report->setProperty("presets",entries);
    reportFile.getParentDirectory().createDirectory();
    if (!reportFile.replaceWithText(juce::JSON::toString(juce::var(report.release()),true))) return 1;
    std::cout << "legacy: " << presets.size() << " presets, " << rendered << " rendered, " << cached << " cached, " << failures << " failed\n";
    return failures==0 && presets.size()==224 ? 0 : 1;
}
}

int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc==4 && (juce::String(argv[1])=="legacy-write" || juce::String(argv[1])=="legacy-check"))
        return legacy(juce::String(argv[1])=="legacy-check",juce::File(argv[2]),juce::File(argv[3]));
    std::cerr << "HPEQ_RackLab legacy-write|legacy-check <reference directory> <report.json>\n";
    return 2;
}
