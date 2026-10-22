// HPEQ_Lab m7 --out <dir> [--commit <sha>]
//
// M7 evidence (T12): CPU of the project engine for 1 / 5 / 10 layers x 1 / 5 FX nodes at 44.1 / 48 /
// 96 / 192 kHz (block 512, p50 / p95 / p99 of the callback time, budget = 512 / fs), a heavy profile,
// lazy-layer cost, reported latency, and the memory of a prepared engine (working set).

#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>

#include "DSP/ProjectEngine.h"
#include "Harness/AudioFileIO.h"
#include "Harness/AudioMetrics.h"
#include "Harness/Json.h"
#include "Harness/MachineInfo.h"
#include "Harness/TestSignalFactory.h"

using namespace hpeq;
using namespace hpeq::dsp;
using namespace hpeq::test;

namespace
{
double workingSetMb()
{
    PROCESS_MEMORY_COUNTERS counters {};
    GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters));
    return static_cast<double>(counters.WorkingSetSize) / (1024.0 * 1024.0);
}

double peakWorkingSetMb()
{
    PROCESS_MEMORY_COUNTERS counters {};
    GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters));
    return static_cast<double>(counters.PeakWorkingSetSize) / (1024.0 * 1024.0);
}

FxSlotState slotState(FxType type, std::initializer_list<std::pair<const char*, double>> values)
{
    FxSlotState s;
    s.type = type;
    for (const auto& [key, value] : values)
        s.parameters.set(juce::Identifier(key), value);
    return s;
}

// `layers` layers, each with one EQ bell and `nodes` FX nodes; chain = `chain` per node.
FxProjectState profile(int layers, int nodes, const std::vector<FxSlotState>& chain, LayerRouting routing)
{
    FxProjectState p;
    p.routing = routing;
    for (int j = 0; j < layers; ++j)
    {
        auto& layer = p.layers[static_cast<size_t>(j)];
        EqNodeState bell;
        bell.id = p.nextNodeId++;
        bell.frequencyHz = 200.0 * (j + 1);
        bell.gainDb = 2.0;
        layer.eqNodes.push_back(bell);
        for (int n = 0; n < nodes; ++n)
        {
            FxNodeState node;
            node.id = p.nextNodeId++;
            node.lowHz = 60.0 * std::pow(2.2, n);
            node.highHz = node.lowHz * 1.9;
            node.chain = chain;
            layer.fxNodes.push_back(node);
        }
    }
    return p;
}

juce::var timing(const FxProjectState& project, double fs, double seconds = 4.0)
{
    auto e = std::make_unique<ProjectEngine>();
    e->prepare(fs, 512);
    e->publish(project);
    const auto source = makePinkNoise({ fs, 2, seconds, kFixtureSeed }, -12.0);
    juce::AudioBuffer<float> work(2, 512);
    std::vector<double> us;
    int block = 0;
    for (int start = 0; start + 512 <= source.getNumSamples(); start += 512, ++block)
    {
        for (int ch = 0; ch < 2; ++ch)
            work.copyFrom(ch, 0, source, ch, start, 512);
        const auto t0 = std::chrono::steady_clock::now();
        e->process(work.getArrayOfWritePointers(), 2, 512);
        const double t = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        if (block >= 20) // warm-up
            us.push_back(t);
    }
    const auto t = summarise(std::move(us));
    const double budget = 1.0e6 * 512.0 / fs;
    return JsonObject()
        .set("p50_us", t.p50).set("p95_us", t.p95).set("p99_us", t.p99).set("max_us", t.max)
        .set("p99_budget_fraction", t.p99 / budget)
        .set("layers_processed", e->layersProcessedLastBlock())
        .set("latency_samples", e->getLatencySamples());
}
} // namespace

int runM7Lab(const juce::File& out, const juce::String& commit)
{
    out.createDirectory();
    const std::vector<FxSlotState> one { slotState(FxType::Compressor, { { "thresholdDb", -30.0 } }) };
    const std::vector<FxSlotState> heavy { slotState(FxType::Compressor, { { "thresholdDb", -30.0 } }), slotState(FxType::Saturator, { { "oversampling", 1.0 } }),
                                           slotState(FxType::TransientShaper, { { "attackDb", 4.0 } }), slotState(FxType::Limiter, { { "lookaheadMs", 1.0 } }) };

    std::cout << "T12\n";
    auto t12 = jsonArray();
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int layers : { 1, 5, 10 })
            for (int nodes : { 1, 5 })
            {
                const auto r = timing(profile(layers, nodes, one, LayerRouting::Parallel), fs);
                std::cout << "  " << fs << " Hz, " << layers << " layers x " << nodes << " nodes: p99 "
                          << static_cast<double>(r["p99_us"]) << " us\n";
                t12.append(JsonObject().set("fs", fs).set("layers", layers).set("nodes_per_layer", nodes).set("chain", "compressor").set("routing", "Parallel").set("result", r));
            }

    std::cout << "profiles\n";
    auto profiles = JsonObject();
    profiles.set("serial_10x5_compressor_48k", timing(profile(10, 5, one, LayerRouting::Serial), 48000.0));
    profiles.set("parallel_10x5_heavy_48k", timing(profile(10, 5, heavy, LayerRouting::Parallel), 48000.0));
    profiles.set("serial_10x5_heavy_48k", timing(profile(10, 5, heavy, LayerRouting::Serial), 48000.0));
    {
        auto bypassed = profile(10, 5, heavy, LayerRouting::Parallel);
        for (auto& layer : bypassed.layers)
            layer.enabled = false;
        profiles.set("parallel_10x5_heavy_all_bypassed_48k", timing(bypassed, 48000.0));
        auto muted = profile(10, 5, heavy, LayerRouting::Parallel);
        for (auto& layer : muted.layers)
            layer.mute = true;
        profiles.set("parallel_10x5_heavy_all_muted_48k", timing(muted, 48000.0));
    }
    profiles.set("empty_project_48k", timing(FxProjectState {}, 48000.0));

    std::cout << "memory\n";
    auto memory = JsonObject();
    for (double fs : { 48000.0, 192000.0 })
    {
        const double before = workingSetMb();
        auto e = std::make_unique<ProjectEngine>();
        e->prepare(fs, 512);
        const double after = workingSetMb();
        memory.set("prepared_engine_mb_" + juce::String(static_cast<int>(fs)), after - before);
        e.reset();
    }
    memory.set("process_peak_working_set_mb", peakWorkingSetMb());

    const auto report = JsonObject()
                            .set("milestone", "M7")
                            .set("commit", commit)
                            .set("machine", describeMachine())
                            .set("block_size", 512)
                            .set("t12", t12)
                            .set("profiles", profiles)
                            .set("memory", memory);
    writeJson(out.getChildFile("m7_lab.json"), report);
    std::cout << juce::JSON::toString(profiles) << "\n" << juce::JSON::toString(memory) << "\n";
    return 0;
}
