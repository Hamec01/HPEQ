// HPEQ_Lab t11 --out <dir> [--count 200]
//
// T11 fixtures for HPEQ_HostSmoke --t11 <dir>: random full projects (10 layers, every FX type,
// routing, layer controls) saved as plugin state, plus reference renders made in-process with the
// same code, so the host tool can check through the real .vst3 binary that state round-trips,
// latency matches and the sound is bit-identical, also across sample rates and (variable) block sizes.

#include <iostream>
#include <memory>

#include "Harness/AudioFileIO.h"
#include "Harness/Json.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginProcessor.h"

using namespace hpeq;
using namespace hpeq::test;

namespace
{
FxProjectState randomProject(DeterministicRandom& random)
{
    const FxType types[] { FxType::Compressor, FxType::Saturator, FxType::Reverb, FxType::Delay, FxType::TransientShaper,
                           FxType::MonstaFx, FxType::StereoProcessor, FxType::GateExpander, FxType::Limiter, FxType::Gain };
    const EqShape shapes[] { EqShape::Bell, EqShape::LowShelf, EqShape::HighShelf, EqShape::LowCut, EqShape::HighCut, EqShape::Notch,
                             EqShape::BandPass, EqShape::Tilt, EqShape::AllPass };
    const ChannelMode modes[] { ChannelMode::Stereo, ChannelMode::Left, ChannelMode::Right, ChannelMode::Mid, ChannelMode::Side };
    const auto pick = [&](int n) { return static_cast<int>(random.nextU64() % static_cast<std::uint64_t>(n)); };
    const auto unit = [&] { return 0.5 + 0.5 * random.nextBipolar(); };

    FxProjectState p;
    p.routing = pick(2) == 0 ? LayerRouting::Parallel : LayerRouting::Serial;
    p.selectedLayer = pick(kNumLayers);
    const int layers = 1 + pick(kNumLayers);
    for (int j = 0; j < layers; ++j)
    {
        auto& layer = p.layers[static_cast<size_t>(pick(kNumLayers))];
        layer.name = "L" + juce::String(j);
        layer.enabled = pick(6) != 0;
        layer.mute = pick(8) == 0;
        layer.solo = pick(12) == 0;
        layer.gainDb = static_cast<float>(pick(25) - 12);
        layer.mix = static_cast<float>(unit());
        layer.pan = static_cast<float>(random.nextBipolar() * 0.5);
        layer.width = static_cast<float>(0.5 + unit());
        layer.channel = modes[pick(5)];
        for (int e = pick(4); e > 0; --e)
        {
            EqNodeState n;
            n.id = p.nextNodeId++;
            n.shape = shapes[pick(9)];
            n.frequencyHz = 30.0 * std::pow(2.0, unit() * 9.0);
            n.gainDb = random.nextBipolar() * 9.0;
            n.q = 0.4 + unit() * 3.0;
            n.slopeDbPerOct = 12 * (1 + pick(4));
            if (pick(3) == 0) // D6: modules on the point's band
                for (int m = 1 + pick(3); m > 0; --m)
                {
                    FxSlotState slot;
                    slot.type = types[pick(10)];
                    slot.mix = static_cast<float>(unit());
                    n.chain.push_back(slot);
                }
            layer.eqNodes.push_back(n);
        }
        for (int m = pick(4) == 0 ? 1 + pick(3) : 0; m > 0; --m) // D7: modules on the whole layer
            layer.mixChain.push_back(defaultModule(types[pick(10)]));
        for (int f = pick(4); f > 0; --f)
        {
            FxNodeState n;
            n.id = p.nextNodeId++;
            n.lowHz = 40.0 * std::pow(2.0, unit() * 7.0);
            n.highHz = n.lowHz * (1.5 + unit() * 4.0);
            n.mix = static_cast<float>(unit());
            n.chain.clear();
            for (int s = 1 + pick(kMaxFxPerNode); s > 0; --s)
            {
                FxSlotState slot;
                slot.type = types[pick(10)];
                slot.mix = static_cast<float>(unit());
                slot.enabled = pick(8) != 0;
                if (slot.type == FxType::Saturator)
                    slot.parameters.set("oversampling", pick(3));
                if (slot.type == FxType::Limiter)
                    slot.parameters.set("lookaheadMs", 0.1 + unit() * 5.0);
                if (slot.type == FxType::Delay)
                    slot.parameters.set("sync", pick(12));
                if (slot.type == FxType::TransientShaper && pick(2) == 0) // HamloUdarnik (module pass 6)
                {
                    slot.parameters.set("attackDb", random.nextBipolar() * 9.0);
                    slot.parameters.set("style", pick(3));
                    slot.parameters.set("lookahead", pick(3));
                    slot.parameters.set("rise", std::pow(2.0, random.nextBipolar()));
                    slot.parameters.set("decay", std::pow(2.0, random.nextBipolar()));
                    slot.parameters.set("sensitivity", unit());
                    slot.parameters.set("preserve", unit());
                }
                n.chain.push_back(slot);
            }
            layer.fxNodes.push_back(n);
        }
    }
    sanitize(p);
    return p;
}

juce::AudioBuffer<float> render(FxPluginProcessor& plugin, const juce::AudioBuffer<float>& input, double fs, const std::vector<int>& blocks)
{
    int maxBlock = 1;
    for (int b : blocks)
        maxBlock = juce::jmax(maxBlock, b);
    plugin.setRateAndBufferSizeDetails(fs, maxBlock);
    plugin.prepareToPlay(fs, maxBlock);
    auto audio = input;
    juce::MidiBuffer midi;
    for (int start = 0, k = 0; start < audio.getNumSamples(); ++k)
    {
        const int n = juce::jmin(blocks[static_cast<size_t>(k) % blocks.size()], audio.getNumSamples() - start);
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), 2, start, n);
        plugin.processBlock(view, midi);
        start += n;
    }
    plugin.releaseResources();
    return audio;
}

juce::var blocksVar(const std::vector<int>& blocks)
{
    auto v = jsonArray();
    for (int b : blocks)
        v.append(b);
    return v;
}
} // namespace

int runT11Fixtures(const juce::File& out, int count)
{
    out.deleteRecursively();
    out.createDirectory();
    DeterministicRandom random(20261022);
    auto items = jsonArray();
    const auto add = [&](const FxProjectState& project, const juce::String& name, double fs, const std::vector<int>& blocks, bool withReference)
    {
        auto plugin = std::make_unique<FxPluginProcessor>();
        plugin->setRateAndBufferSizeDetails(fs, 512);
        plugin->prepareToPlay(fs, 512); // latency is computed for this rate
        plugin->setProject(project);
        juce::MemoryBlock state;
        plugin->getStateInformation(state);
        out.getChildFile(name + ".state").replaceWithData(state.getData(), state.getSize());
        auto item = JsonObject().set("name", name).set("fs", fs).set("blocks", blocksVar(blocks)).set("latency", plugin->getLatencySamples());
        if (withReference)
        {
            const auto input = makeSyntheticDrums({ fs, 2, 0.6, kFixtureSeed });
            writeFloatWav(out.getChildFile(name + ".in.wav"), input, fs);
            writeFloatWav(out.getChildFile(name + ".ref.wav"), render(*plugin, input, fs, blocks), fs);
        }
        item.set("reference", withReference);
        items.append(item);
    };

    for (int i = 0; i < count; ++i)
        add(randomProject(random), "random_" + juce::String(i).paddedLeft('0', 3), 48000.0, { 512 }, i % 10 == 0);

    auto matrixProject = randomProject(random);
    while (dsp::ProjectEngine::latencyFor(matrixProject, 48000.0) == 0) // the matrix must exercise PDC
        matrixProject = randomProject(random);
    for (double fs : { 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 })
        for (int block : { 32, 64, 256, 441, 1024, 4096 })
            add(matrixProject, "matrix_" + juce::String(static_cast<int>(fs)) + "_" + juce::String(block), fs, { block }, true);
    std::vector<int> variable;
    for (int k = 0; k < 64; ++k)
        variable.push_back(1 + static_cast<int>(random.nextU64() % 1500));
    add(matrixProject, "variable_blocks_48000", 48000.0, variable, true);

    writeJson(out.getChildFile("manifest.json"), JsonObject().set("items", items));
    std::cout << "T11 fixtures: " << items.size() << " items -> " << out.getFullPathName() << "\n";
    return 0;
}
