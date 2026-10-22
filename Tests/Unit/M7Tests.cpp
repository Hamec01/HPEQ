// M7: ten layers, Parallel (delta) and Serial routing, layer controls, Mute / Solo / Bypass, cross-layer
// latency, lazy processing, full 10-layer state restore, T10 and allocations. Thresholds fixed here
// before the runs.

#include <cmath>
#include <functional>
#include <memory>
#include <set>

#include "DSP/ProjectEngine.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginProcessor.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;

EqNodeState bell(double hz, double db, double q = 1.0)
{
    EqNodeState n;
    n.shape = EqShape::Bell;
    n.frequencyHz = hz;
    n.gainDb = db;
    n.q = q;
    return n;
}

FxSlotState slot(FxType type, std::initializer_list<std::pair<const char*, double>> values)
{
    FxSlotState s;
    s.type = type;
    for (const auto& [key, value] : values)
        s.parameters.set(juce::Identifier(key), value);
    return s;
}

FxNodeState fxNode(double lo, double hi, std::initializer_list<FxSlotState> chain)
{
    FxNodeState n;
    n.lowHz = lo;
    n.highHz = hi;
    n.chain = chain;
    return n;
}

// Gives every node of the project a unique id (as the editor does).
void numberNodes(FxProjectState& p)
{
    for (auto& layer : p.layers)
    {
        for (auto& n : layer.eqNodes)
            n.id = p.nextNodeId++;
        for (auto& n : layer.fxNodes)
            n.id = p.nextNodeId++;
    }
}

juce::AudioBuffer<float> renderProject(const FxProjectState& project, const juce::AudioBuffer<float>& x, int block = 512,
                                       const std::function<void(ProjectEngine&, int)>& beforeBlock = {}, std::uint64_t* allocations = nullptr)
{
    auto e = std::make_unique<ProjectEngine>();
    e->prepare(kFs, block);
    e->publish(project);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        if (beforeBlock)
            beforeBlock(*e, start);
        float* chunk[2] { y.getWritePointer(0) + start, y.getNumChannels() > 1 ? y.getWritePointer(1) + start : nullptr };
        ScopedAllocationProbe probe;
        e->process(chunk, y.getNumChannels(), juce::jmin(block, y.getNumSamples() - start));
        if (allocations != nullptr)
            *allocations += probe.counts().allocations;
    }
    return y;
}

// Independent model of one layer: a standalone LayerEngine, then the documented controls
// (gain, M/S width, linear balance). Static controls only (no ramps).
juce::AudioBuffer<float> modelLayer(const LayerState& layer, const juce::AudioBuffer<float>& x)
{
    auto e = std::make_unique<LayerEngine>();
    e->prepare(kFs, 512);
    e->publish(makeSnapshot(layer));
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        float* chunk[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        e->process(chunk, 2, juce::jmin(512, y.getNumSamples() - start));
    }
    const double g = std::pow(10.0, layer.gainDb / 20.0);
    for (int i = 0; i < y.getNumSamples(); ++i)
    {
        const double l = y.getSample(0, i) * g, r = y.getSample(1, i) * g;
        const double m = 0.5 * (l + r), s = 0.5 * (l - r) * layer.width;
        y.setSample(0, i, static_cast<float>((m + s) * (layer.pan > 0.0f ? 1.0 - layer.pan : 1.0)));
        y.setSample(1, i, static_cast<float>((m - s) * (layer.pan < 0.0f ? 1.0 + layer.pan : 1.0)));
    }
    return y;
}

juce::AudioBuffer<float> segment(const juce::AudioBuffer<float>& x, int start, int length)
{
    juce::AudioBuffer<float> y(x.getNumChannels(), length);
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, 0, x, ch, start, length);
    return y;
}

// Three layers with different jobs, latency 0 (used by the formula tests).
FxProjectState threeLayers(LayerRouting routing)
{
    FxProjectState p;
    p.routing = routing;
    p.layers[0].eqNodes = { bell(200.0, 6.0) };
    p.layers[0].fxNodes = { fxNode(150.0, 900.0, { slot(FxType::Compressor, { { "thresholdDb", -30.0 }, { "ratio", 4.0 } }) }) };
    p.layers[3].fxNodes = { fxNode(2000.0, 6000.0, { slot(FxType::Saturator, { { "driveDb", 18.0 }, { "oversampling", 0.0 } }) }) };
    p.layers[7].eqNodes = { bell(5000.0, -4.0, 0.7) };
    p.layers[7].gainDb = -6.0f;
    p.layers[7].pan = 0.3f;
    p.layers[7].width = 1.5f;
    p.layers[7].mix = 0.5f;
    numberNodes(p);
    return p;
}
} // namespace

class M7Tests final : public juce::UnitTest
{
public:
    M7Tests() : juce::UnitTest("Layers M7", "HPEQ") {}

    void runTest() override
    {
        const auto drums = makeSyntheticDrums({ kFs, 2, 2.0, kFixtureSeed });

        beginTest("Gate 7: ten empty layers, and ten bypassed layers with content, give the input bit-exactly (Parallel and Serial)");
        {
            for (auto routing : { LayerRouting::Parallel, LayerRouting::Serial })
            {
                FxProjectState empty;
                empty.routing = routing;
                expect(compareAligned(renderProject(empty, drums), drums).bitExact, "empty");

                FxProjectState bypassed;
                bypassed.routing = routing;
                for (auto& layer : bypassed.layers)
                {
                    layer.enabled = false;
                    layer.eqNodes = { bell(300.0, 9.0) };
                    layer.fxNodes = { fxNode(1000.0, 4000.0, { slot(FxType::Compressor, { { "thresholdDb", -40.0 } }) }) };
                    layer.gainDb = 6.0f;
                }
                numberNodes(bypassed);
                expect(compareAligned(renderProject(bypassed, drums), drums).bitExact, "bypassed");
            }
        }

        beginTest("No dry duplication: ten enabled Parallel layers with a 0 dB bell keep the input level (+-0.01 dB, null <= -100 dB)");
        {
            FxProjectState p;
            for (auto& layer : p.layers)
                layer.eqNodes = { bell(1000.0, 0.0) };
            numberNodes(p);
            const auto y = renderProject(p, drums);
            const auto null = compareAligned(y, drums);
            const double levelDb = measureAll(y).rmsDbfs - measureAll(drums).rmsDbfs;
            logMessage("  level change " + juce::String(levelDb, 4) + " dB, null " + juce::String(null.errorDb, 1) + " dB (a dry sum would be +20 dB)");
            expect(std::abs(levelDb) <= 0.01);
            expect(null.errorDb <= -100.0);
        }

        beginTest("Parallel equals the documented formula y = x + sum w_j (L_j{x} - x) (independent model, <= -120 dB)");
        {
            const auto p = threeLayers(LayerRouting::Parallel);
            const auto y = renderProject(p, drums);
            auto model = drums;
            for (int j : { 0, 3, 7 })
            {
                const auto& layer = p.layers[static_cast<size_t>(j)];
                const auto lj = modelLayer(layer, drums);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < drums.getNumSamples(); ++i)
                        model.setSample(ch, i, model.getSample(ch, i) + layer.mix * (lj.getSample(ch, i) - drums.getSample(ch, i)));
            }
            const auto null = compareAligned(y, model);
            logMessage("  parallel vs model: " + juce::String(null.errorDb, 1) + " dB");
            expect(null.errorDb <= -120.0);
        }

        beginTest("Serial equals x_{j+1} = x_j + w_j (L_j{x_j} - x_j) in layer order (independent model, <= -120 dB); order matters");
        {
            const auto p = threeLayers(LayerRouting::Serial);
            const auto y = renderProject(p, drums);
            auto model = drums;
            for (int j : { 0, 3, 7 })
            {
                const auto& layer = p.layers[static_cast<size_t>(j)];
                const auto lj = modelLayer(layer, model);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < drums.getNumSamples(); ++i)
                        model.setSample(ch, i, model.getSample(ch, i) + layer.mix * (lj.getSample(ch, i) - model.getSample(ch, i)));
            }
            const auto null = compareAligned(y, model);
            logMessage("  serial vs model: " + juce::String(null.errorDb, 1) + " dB");
            expect(null.errorDb <= -120.0);

            // Swap the saturator layer and a -12 dB layer: the result must change.
            FxProjectState a;
            a.routing = LayerRouting::Serial;
            a.layers[0].fxNodes = { fxNode(100.0, 8000.0, { slot(FxType::Saturator, { { "driveDb", 24.0 }, { "autoGain", 0.0 } }) }) };
            a.layers[1].gainDb = -12.0f;
            numberNodes(a);
            auto b = a;
            std::swap(b.layers[0], b.layers[1]);
            const auto order = compareAligned(renderProject(b, drums), renderProject(a, drums));
            logMessage("  saturate > -12 dB vs -12 dB > saturate: difference " + juce::String(order.errorDb, 1) + " dB");
            expect(order.errorDb > -20.0);
        }

        beginTest("Cross-layer latency: Parallel = max, Serial = sum; output = zero-latency project delayed by it (<= -120 dB after 1 s)");
        {
            const auto limiter = [](double ms) { return slot(FxType::Limiter, { { "ceilingDb", 0.0 }, { "lookaheadMs", ms } }); };
            const auto gain0 = slot(FxType::Gain, { { "gainDb", 0.0 } });
            auto quiet = drums;
            quiet.applyGain(0.1f);
            for (auto routing : { LayerRouting::Parallel, LayerRouting::Serial })
            {
                FxProjectState withLatency;
                withLatency.routing = routing;
                withLatency.layers[0].fxNodes = { fxNode(200.0, 2000.0, { limiter(2.0) }) };
                withLatency.layers[2].fxNodes = { fxNode(3000.0, 9000.0, { slot(FxType::Compressor, { { "thresholdDb", -40.0 } }) }) };
                withLatency.layers[5].fxNodes = { fxNode(80.0, 300.0, { limiter(1.0) }) };
                numberNodes(withLatency);
                auto plain = withLatency;
                plain.layers[0].fxNodes[0].chain = { gain0 };
                plain.layers[5].fxNodes[0].chain = { gain0 };

                const int expected = routing == LayerRouting::Serial ? 96 + 48 : 96;
                expectEquals(ProjectEngine::latencyFor(withLatency, kFs), expected);
                int reported = -1;
                const auto y = renderProject(withLatency, quiet, 512, [&](ProjectEngine& e, int start) { if (start > 0) reported = e.getLatencySamples(); });
                expectEquals(reported, expected);
                const auto reference = renderProject(plain, quiet);
                const int skip = static_cast<int>(kFs);
                const int length = quiet.getNumSamples() - skip - expected;
                const auto null = compareAligned(segment(y, skip + expected, length), segment(reference, skip, length));
                logMessage("  " + toString(routing) + ": latency " + juce::String(expected) + ", aligned null " + juce::String(null.errorDb, 1) + " dB");
                expect(null.errorDb <= -120.0);
            }
        }

        beginTest("Mute, layer Mix 0 and Solo: a muted / mix-0 layer is exactly absent; Solo hears only soloed layers; mutes and solos are not lost");
        {
            for (auto routing : { LayerRouting::Parallel, LayerRouting::Serial })
            {
                auto p = threeLayers(routing);
                auto muted = p;
                muted.layers[3].mute = true;
                auto without = p;
                without.layers[3] = LayerState {};
                expect(compareAligned(renderProject(muted, drums), renderProject(without, drums)).bitExact, toString(routing) + ": mute");
                auto mixZero = p;
                mixZero.layers[3].mix = 0.0f; // MIX-0 for a layer
                expect(compareAligned(renderProject(mixZero, drums), renderProject(without, drums)).bitExact, toString(routing) + ": layer mix 0");

                auto soloed = p;
                soloed.layers[3].solo = true;
                soloed.layers[0].mute = true; // stays muted after solo is released
                auto only = p;
                only.layers[0] = LayerState {};
                only.layers[7] = LayerState {};
                expect(compareAligned(renderProject(soloed, drums), renderProject(only, drums)).bitExact, toString(routing) + ": solo");
                const auto snapshot = makeProjectSnapshot(soloed);
                expect(snapshot.layers[7].weight == 0.0f && snapshot.layers[3].weight == 1.0f);
            }
        }

        beginTest("Mute keeps the layer running: after un-muting (and the 20 ms fade) the output is bit-exact to never muted");
        {
            const auto p = threeLayers(LayerRouting::Parallel);
            const auto reference = renderProject(p, drums);
            auto muted = p;
            muted.layers[0].mute = true;
            const auto y = renderProject(p, drums, 512, [&](ProjectEngine& e, int start)
            {
                if (start == 24576) e.publish(muted);
                if (start == 49152) e.publish(p);
            });
            const int from = 49152 + 2048;
            const auto null = compareAligned(segment(y, from, drums.getNumSamples() - from), segment(reference, from, drums.getNumSamples() - from));
            expect(null.bitExact, "after un-mute: " + juce::String(null.errorDb) + " dB");
            const auto during = compareAligned(segment(y, 30000, 10000), segment(reference, 30000, 10000));
            expect(during.errorDb > -60.0, "mute had no effect");
        }

        beginTest("Lazy layers: only layers with content are processed; bypassed layers stop after their fade");
        {
            auto p = threeLayers(LayerRouting::Parallel);
            int processed = -1, afterBypass = -1;
            auto bypassed = p;
            bypassed.layers[3].enabled = false;
            renderProject(p, drums, 512, [&](ProjectEngine& e, int start)
            {
                if (start == 4096) processed = e.layersProcessedLastBlock();
                if (start == 8192) e.publish(bypassed);
                if (start == 16384) afterBypass = e.layersProcessedLastBlock();
            });
            expectEquals(processed, 3);
            expectEquals(afterBypass, 2);
        }

        beginTest("Layer channel modes: Side layer leaves a mid-only signal exact-ish (<= -120 dB) and boosts the side (+6 +-0.1 dB); Left layer leaves R bit-exact");
        {
            FxProjectState side;
            side.layers[0].channel = ChannelMode::Side;
            side.layers[0].eqNodes = { bell(1000.0, 6.0, 2.0) };
            numberNodes(side);
            const auto mid = makeStereoPlacement({ kFs, 2, 1.0, kFixtureSeed }, StereoPlacement::MidOnly, -18.0);
            expect(compareAligned(renderProject(side, mid), mid).errorDb <= -120.0, "mid-only through a Side layer");

            auto tone = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 1000.0, -18.0);
            for (int i = 0; i < tone.getNumSamples(); ++i)
                tone.setSample(1, i, -tone.getSample(0, i)); // side only
            const auto y = renderProject(side, tone);
            const double gainDb = measureAll(segment(y, 24000, 24000)).rmsDbfs - measureAll(segment(tone, 24000, 24000)).rmsDbfs;
            logMessage("  side tone at 1 kHz: " + juce::String(gainDb, 3) + " dB");
            expect(std::abs(gainDb - 6.0) <= 0.1);

            FxProjectState left;
            left.layers[0].channel = ChannelMode::Left;
            left.layers[0].eqNodes = { bell(500.0, 9.0) };
            numberNodes(left);
            const auto yl = renderProject(left, drums);
            bool rightExact = true;
            for (int i = 0; i < drums.getNumSamples(); ++i)
                rightExact = rightExact && yl.getSample(1, i) == drums.getSample(1, i);
            expect(rightExact, "right channel changed by a Left layer");
            expect(compareAligned(yl, drums).errorDb > -40.0, "left not processed");
        }

        beginTest("T10 with layers: Parallel and Serial, four layers of mixed types, blocks 1/64/127/512 bit-exact, 0 allocations");
        {
            FxProjectState p;
            p.layers[0].eqNodes = { bell(120.0, 4.0) };
            p.layers[0].fxNodes = { fxNode(60.0, 250.0, { slot(FxType::Compressor, { { "thresholdDb", -30.0 } }), slot(FxType::Saturator, { { "oversampling", 1.0 } }) }) };
            p.layers[1].fxNodes = { fxNode(3000.0, 9000.0, { slot(FxType::Reverb, { { "decaySeconds", 1.2 } }) }) };
            p.layers[1].width = 1.4f;
            p.layers[4].fxNodes = { fxNode(500.0, 2500.0, { slot(FxType::Limiter, { { "ceilingDb", -12.0 }, { "lookaheadMs", 1.0 } }), slot(FxType::Delay, { { "sync", 0.0 }, { "timeMs", 90.0 } }) }) };
            p.layers[4].channel = ChannelMode::Mid;
            p.layers[9].fxNodes = { fxNode(1000.0, 5000.0, { slot(FxType::TransientShaper, { { "attackDb", 6.0 } }), slot(FxType::MonstaFx, { { "chance", 0.5 } }) }) };
            p.layers[9].mix = 0.6f;
            p.layers[9].pan = -0.4f;
            numberNodes(p);
            const auto x = segment(drums, 0, 48000);
            for (auto routing : { LayerRouting::Parallel, LayerRouting::Serial })
            {
                p.routing = routing;
                std::uint64_t allocations = 0;
                const auto reference = renderProject(p, x, 1, {}, &allocations);
                for (int block : { 64, 127, 512 })
                {
                    const auto null = compareAligned(renderProject(p, x, block, {}, &allocations), reference);
                    expect(null.bitExact, toString(routing) + " block " + juce::String(block) + ": " + juce::String(null.errorDb) + " dB");
                }
                expectEquals(static_cast<int>(allocations), 0);
                expect(measureAll(reference).allFinite);
            }
        }

        beginTest("Stress: 10 layers x 8 nodes x 6 slots of every type, Serial and Parallel, 2000 control changes: finite, no allocations, starvation reported");
        {
            FxProjectState p;
            const FxType types[] { FxType::Compressor, FxType::Saturator, FxType::Reverb, FxType::Delay, FxType::TransientShaper,
                                   FxType::MonstaFx, FxType::StereoProcessor, FxType::GateExpander, FxType::Limiter, FxType::Gain };
            int t = 0;
            for (auto& layer : p.layers)
            {
                layer.eqNodes = { bell(100.0 + 37.0 * t, 3.0), bell(4000.0 - 91.0 * t, -3.0) };
                for (int n = 0; n < kMaxFxNodesPerLayer; ++n)
                {
                    FxNodeState node = fxNode(60.0 * std::pow(1.9, n), 60.0 * std::pow(1.9, n + 1), {});
                    node.chain.clear();
                    for (int k = 0; k < kMaxFxPerNode; ++k)
                        node.chain.push_back(slot(types[(t++) % 10], {}));
                    layer.fxNodes.push_back(node);
                }
            }
            numberNodes(p);
            const auto x = segment(drums, 0, 48000);
            for (auto routing : { LayerRouting::Parallel, LayerRouting::Serial })
            {
                p.routing = routing;
                DeterministicRandom random(7);
                std::uint64_t allocations = 0;
                auto changing = p;
                int starved = 0;
                const auto y = renderProject(p, x, 256, [&](ProjectEngine& e, int start)
                {
                    for (int k = 0; k < 11; ++k)
                    {
                        auto& layer = changing.layers[static_cast<size_t>(random.nextU64() % kNumLayers)];
                        layer.gainDb = static_cast<float>(random.nextBipolar() * 12.0);
                        layer.mix = static_cast<float>(0.5 + 0.5 * random.nextBipolar());
                        layer.pan = static_cast<float>(random.nextBipolar());
                        layer.width = static_cast<float>(1.0 + random.nextBipolar());
                        layer.mute = random.nextU64() % 7 == 0;
                        layer.enabled = random.nextU64() % 9 != 0;
                    }
                    e.publish(changing);
                    if (start + 256 >= x.getNumSamples())
                        for (int j = 0; j < kNumLayers; ++j)
                            for (int n = 0; n < kMaxFxNodesPerLayer; ++n)
                                for (int k = 0; k < kMaxFxPerNode; ++k)
                                    starved += e.getLayer(j).getNode(n).slotStarved(k) ? 1 : 0;
                }, &allocations);
                const auto stats = measureAll(y);
                logMessage("  " + toString(routing) + ": peak " + juce::String(stats.peakDbfs, 1) + " dBFS, starved slots " + juce::String(starved));
                expect(stats.allFinite);
                expectEquals(static_cast<int>(allocations), 0);
                expect(starved > 0, "pools cannot hold 60 reverbs/delays/monstas: starvation must be reported");
            }
        }

        beginTest("STATE: a full 10-layer project round-trips (value tree) and the plugin renders identically after restore");
        {
            auto p = threeLayers(LayerRouting::Serial);
            for (int j = 0; j < kNumLayers; ++j)
            {
                auto& layer = p.layers[static_cast<size_t>(j)];
                layer.name = "Layer " + juce::String(j + 1);
                if (layer.fxNodes.empty())
                    layer.fxNodes = { fxNode(100.0 * (j + 1), 400.0 * (j + 1), { slot(FxType::TransientShaper, { { "attackDb", 3.0 } }) }) };
                layer.mute = j == 5;
                layer.solo = false;
                layer.enabled = j != 6;
                layer.channel = j == 8 ? ChannelMode::Side : layer.channel;
            }
            p.selectedLayer = 4;
            numberNodes(p);
            sanitize(p);
            const auto restored = fromValueTree(toValueTree(p));
            expect(restored.has_value());
            if (restored)
                expect(toValueTree(*restored).isEquivalentTo(toValueTree(p)), "value tree differs after round trip");

            const auto render = [&](FxPluginProcessor& plugin)
            {
                plugin.setRateAndBufferSizeDetails(kFs, 512);
                plugin.prepareToPlay(kFs, 512);
                auto y = segment(drums, 0, 48000);
                juce::MidiBuffer midi;
                for (int start = 0; start < y.getNumSamples(); start += 512)
                {
                    juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(512, y.getNumSamples() - start));
                    plugin.processBlock(view, midi);
                }
                return y;
            };
            auto a = std::make_unique<FxPluginProcessor>();
            a->setProject(p);
            juce::MemoryBlock state;
            a->getStateInformation(state);
            auto b = std::make_unique<FxPluginProcessor>();
            b->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            expectEquals(b->getProject().selectedLayer, 4);
            expect(toValueTree(b->getProject()).isEquivalentTo(toValueTree(a->getProject())));
            expect(compareAligned(render(*b), render(*a)).bitExact, "restored plugin renders differently");
            expectEquals(b->getLatencySamples(), a->getLatencySamples());
        }

        beginTest("Plugin layer operations: copy/paste and duplicate give fresh unique ids; paste is undoable; selectLayer is not an undo step");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            plugin->setProject(threeLayers(LayerRouting::Parallel));
            plugin->getUndoManager().clearUndoHistory();
            plugin->selectLayer(3);
            expect(!plugin->getUndoManager().canUndo(), "selecting a layer created an undo step");
            plugin->copyLayer(0);
            plugin->pasteLayer(5);
            const auto p = plugin->getProject();
            expect(p.layers[5].fxNodes.size() == p.layers[0].fxNodes.size());
            std::set<std::uint32_t> ids;
            int count = 0;
            for (const auto& layer : p.layers)
            {
                for (const auto& n : layer.eqNodes) { ids.insert(n.id); ++count; }
                for (const auto& n : layer.fxNodes) { ids.insert(n.id); ++count; }
            }
            expectEquals(static_cast<int>(ids.size()), count);
            plugin->getUndoManager().undo();
            expect(plugin->getProject().layers[5].fxNodes.empty(), "paste not undone");
        }
    }
};

static M7Tests m7Tests;
} // namespace hpeq::test
