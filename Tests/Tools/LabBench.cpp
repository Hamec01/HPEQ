// HPEQ_Lab bench --out <file.json> [--commit <sha>] [--reps 3] [--blocks 2000]
//
// CPU benchmark of fixed engine configurations, for before/after comparisons on the same machine.
// Every configuration: fresh engine, 200 warm-up blocks, then `blocks` timed blocks of 512 stereo
// samples at 48 kHz; repeated `reps` times with the configurations interleaved (so slow drifts of
// clock speed hit all of them alike). The thread runs at raised priority. Reported per
// configuration: median over repetitions of p50 / p95 / p99, the worst max, and "spikes" (blocks
// slower than 3 x p50) with their mean spacing in blocks, to tell periodic from random outliers.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>

#include <juce_core/juce_core.h>

#include "DSP/LayerEngine.h"
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
constexpr double kFs = 48000.0;
constexpr int kBlock = 512;

FxSlotParams gainSlot(float db)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Gain;
    s.values[slot::kGainDb] = db;
    return s;
}

FxSlotParams compSlot()
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Compressor;
    s.values = { -30.0f, 4.0f, 6.0f, 5.0f, 100.0f, 0.0f, 0.0f, 0.0f };
    return s;
}

FxSlotParams reverbSlot()
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Reverb;
    s.values[slot::kPredelayMs] = 10.0f;
    s.values[slot::kDecaySeconds] = 2.0f;
    s.values[slot::kHfRatio] = 0.5f;
    s.values[slot::kSize] = 1.0f;
    s.values[slot::kWidth] = 1.0f;
    s.values[slot::kLowCutHz] = 80.0f;
    return s;
}

// HamloVerb pass: the UI default (Hall, NOW, modulation 0.4, attack 0.2) - what a new reverb costs.
FxSlotParams hamloVerbSlot()
{
    auto s = reverbSlot();
    s.values[slot::kRevSpace] = 3.0f;
    s.values[slot::kRevColor] = 2.0f;
    s.values[slot::kRevAttack] = 0.2f;
    s.values[slot::kRevEarlyDiff] = 0.7f;
    s.values[slot::kRevLateDiff] = 0.7f;
    s.values[slot::kRevModRate] = 0.6f;
    s.values[slot::kRevModDepth] = 0.4f;
    s.values[slot::kRevBassMult] = 1.2f;
    s.values[slot::kRevBassXover] = 300.0f;
    s.values[slot::kRevHighXover] = 4000.0f;
    s.values[slot::kRevHighCut] = 12000.0f;
    s.values[slot::kRevDistance] = 0.4f;
    return s;
}

// HamloEcho pass: the UI default (Tape, 1/4 linked, feedback 0.35, drive 3 dB, wobble 0.3).
FxSlotParams hamloEchoSlot(float pitch = 0.0f)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Delay;
    s.values[slot::kDelayTimeMs] = 250.0f;
    s.values[slot::kDelaySync] = 8.0f;
    s.values[slot::kFeedback] = 0.35f;
    s.values[slot::kDampingHz] = 8000.0f;
    s.values[slot::kDelayLowCutHz] = 120.0f;
    s.values[slot::kDelayWidth] = 1.0f;
    s.values[slot::kDlyCharacter] = 2.0f;
    s.values[slot::kDlyTimeRMs] = 375.0f;
    s.values[slot::kDlySyncR] = 6.0f;
    s.values[slot::kDlyLink] = 1.0f;
    s.values[slot::kDlyDriveDb] = 3.0f;
    s.values[slot::kDlyModRate] = 0.5f;
    s.values[slot::kDlyModDepth] = 0.3f;
    s.values[slot::kDlyBandHz] = 1000.0f;
    s.values[slot::kDlyBandQ] = 1.0f;
    s.values[slot::kDlyPitch] = pitch;
    return s;
}

// HamloMag pass: the UI default (TAPE 19 cm/s, drive 6 dB, 2x, hiss 15 %, wow 15 %) and a busy setting.
FxSlotParams hamloMagSlot(int model, int oversampling)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Saturator;
    s.values[slot::kDriveDb] = 6.0f;
    s.values[slot::kOversampling] = static_cast<float>(oversampling);
    s.values[slot::kAutoGain] = 1.0f;
    s.values[slot::kSatModel] = static_cast<float>(model);
    s.values[slot::kSatLowCutHz] = 20.0f;
    s.values[slot::kSatHighCutHz] = 20000.0f;
    s.values[slot::kSatHarmonics] = 0.5f;
    s.values[slot::kSatTapeBias] = 0.5f;
    s.values[slot::kSatWow] = 0.15f;
    s.values[slot::kSatHiss] = 0.15f;
    s.values[slot::kSatNoiseFollow] = 1.0f;
    return s;
}

FxSlotParams classicSaturatorSlot()
{
    auto s = hamloMagSlot(0, 1);
    s.values[slot::kCurve] = 0.0f;
    return s;
}

FxSlotParams classicDelaySlot()
{
    auto s = hamloEchoSlot();
    s.values[slot::kDlyCharacter] = 0.0f;
    return s;
}

FxNodeParams node(float lo, float hi, std::initializer_list<FxSlotParams> chain)
{
    FxNodeParams n;
    n.active = true;
    n.lowHz = lo;
    n.highHz = hi;
    for (const auto& s : chain)
        n.slots[static_cast<size_t>(n.numSlots++)] = s;
    return n;
}

EqBandParams bell(int i)
{
    EqBandParams b;
    b.active = true;
    b.shape = EqShape::Bell;
    b.frequencyHz = 40.0f * std::pow(1.3f, static_cast<float>(i));
    b.gainDb = 3.0f;
    b.q = 1.0f;
    return b;
}

FxSlotParams limiterSlot(float truePeak, float style, float autoRelease, float link = 1.0f, float threshold = 0.0f)
{
    FxSlotParams l;
    l.active = true;
    l.type = FxType::Limiter;
    l.values[slot::kCeilingDb] = -1.0f;
    l.values[slot::kLimiterReleaseMs] = 60.0f;
    l.values[slot::kLookaheadMs] = 2.0f;
    l.values[slot::kLimThresholdDb] = threshold;
    l.values[slot::kLimTruePeak] = truePeak;
    l.values[slot::kLimAutoRelease] = autoRelease;
    l.values[slot::kLimStyle] = style;
    l.values[slot::kLimLink] = link;
    return l;
}

FxSlotParams transientSlot(float snap, float style, float lookahead, float riseLog2 = 0.0f, float decayLog2 = 0.0f)
{
    FxSlotParams t;
    t.active = true;
    t.type = FxType::TransientShaper;
    t.values[slot::kAttackGainDb] = snap;
    t.values[slot::kSustainGainDb] = -3.0f;
    t.values[slot::kTransientSpeed] = 0.5f;
    t.values[slot::kTrStyle] = style;
    t.values[slot::kTrLookahead] = lookahead;
    t.values[slot::kTrRiseLog2] = riseLog2;
    t.values[slot::kTrDecayLog2] = decayLog2;
    t.values[slot::kTrPreserve] = style > 0.0f ? 1.0f : 0.0f;
    return t;
}

struct Config
{
    const char* name;
    std::function<LayerSnapshot()> make;
};

std::vector<Config> configs()
{
    return {
        { "tree_1node_gain", [] { LayerSnapshot s; s.fx[0] = node(250, 700, { gainSlot(-3) }); return s; } },
        { "tree_5nodes_gain", [] { LayerSnapshot s; const float e[] { 80, 200, 500, 1200, 3000, 8000 };
                                   for (int i = 0; i < 5; ++i) s.fx[static_cast<size_t>(i)] = node(e[i], e[i + 1], { gainSlot(-3) }); return s; } },
        { "tree_8nodes_gain", [] { LayerSnapshot s; for (int i = 0; i < 8; ++i) s.fx[static_cast<size_t>(i)] = node(60.0f * std::pow(2.0f, static_cast<float>(i)), 120.0f * std::pow(2.0f, static_cast<float>(i)), { gainSlot(-3) }); return s; } },
        { "comp_1node", [] { LayerSnapshot s; s.fx[0] = node(250, 700, { compSlot() }); return s; } },
        { "comp_8nodes_x6", [] { LayerSnapshot s; for (int i = 0; i < 8; ++i) s.fx[static_cast<size_t>(i)] = node(60.0f * std::pow(2.0f, static_cast<float>(i)), 150.0f * std::pow(2.0f, static_cast<float>(i)), { compSlot(), compSlot(), compSlot(), compSlot(), compSlot(), compSlot() }); return s; } },
        { "reverb_1node", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { reverbSlot() }); return s; } },
        { "reverb_8nodes", [] { LayerSnapshot s; for (int i = 0; i < 8; ++i) s.fx[static_cast<size_t>(i)] = node(60.0f * std::pow(2.0f, static_cast<float>(i)), 150.0f * std::pow(2.0f, static_cast<float>(i)), { reverbSlot() }); return s; } },
        { "hamloverb_1node", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { hamloVerbSlot() }); return s; } },
        { "hamloverb_8nodes", [] { LayerSnapshot s; for (int i = 0; i < 8; ++i) s.fx[static_cast<size_t>(i)] = node(60.0f * std::pow(2.0f, static_cast<float>(i)), 150.0f * std::pow(2.0f, static_cast<float>(i)), { hamloVerbSlot() }); return s; } },
        { "hamloverb_plate_1node", [] { LayerSnapshot s; auto v = hamloVerbSlot(); v.values[slot::kRevSpace] = 4.0f; s.fx[0] = node(200, 10000, { v }); return s; } },
        { "hamloverb_attack0_1node", [] { LayerSnapshot s; auto v = hamloVerbSlot(); v.values[slot::kRevAttack] = 0.0f; s.fx[0] = node(200, 10000, { v }); return s; } },
        { "hamloverb_mod0_1node", [] { LayerSnapshot s; auto v = hamloVerbSlot(); v.values[slot::kRevModDepth] = 0.0f; s.fx[0] = node(200, 10000, { v }); return s; } },
        { "delay_classic_1node", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { classicDelaySlot() }); return s; } },
        { "hamloecho_1node", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { hamloEchoSlot() }); return s; } },
        { "hamloecho_pitch_1node", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { hamloEchoSlot(7.0f) }); return s; } },
        { "hamloecho_8nodes", [] { LayerSnapshot s; for (int i = 0; i < 8; ++i) s.fx[static_cast<size_t>(i)] = node(60.0f * std::pow(2.0f, static_cast<float>(i)), 150.0f * std::pow(2.0f, static_cast<float>(i)), { hamloEchoSlot() }); return s; } },
        { "saturator_classic_2x", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { classicSaturatorSlot() }); return s; } },
        { "hamlomag_tape_2x", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { hamloMagSlot(1, 1) }); return s; } },
        { "hamlomag_tape_4x", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { hamloMagSlot(1, 2) }); return s; } },
        { "hamlomag_tube_2x", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { hamloMagSlot(2, 1) }); return s; } },
        { "hamlomag_tape_2x_8nodes", [] { LayerSnapshot s; for (int i = 0; i < 8; ++i) s.fx[static_cast<size_t>(i)] = node(60.0f * std::pow(2.0f, static_cast<float>(i)), 150.0f * std::pow(2.0f, static_cast<float>(i)), { hamloMagSlot(1, 1) }); return s; } },
        { "limiter_m6_1node", [] { LayerSnapshot s; s.fx[0] = node(40, 160, { limiterSlot(0, 0, 0) }); return s; } },
        { "redwall_glasnost_1node", [] { LayerSnapshot s; s.fx[0] = node(40, 160, { limiterSlot(0, 0, 1, 1, -6) }); return s; } },
        { "redwall_truepeak_1node", [] { LayerSnapshot s; s.fx[0] = node(40, 160, { limiterSlot(1, 0, 1, 1, -6) }); return s; } },
        { "redwall_october_tp_link50_1node", [] { LayerSnapshot s; s.fx[0] = node(40, 160, { limiterSlot(1, 2, 1, 0.5f, -6) }); return s; } },
        { "redwall_truepeak_mix", [] { LayerSnapshot s; s.mix = node(20, 20000, { limiterSlot(1, 0, 1, 1, -6) }); return s; } },
        { "transient_m6_1node", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { transientSlot(6, 0, 0) }); return s; } },
        { "udarnik_pravda_vostok_1node", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { transientSlot(6, 0, 1) }); return s; } },
        { "udarnik_udarnik_soyuz_1node", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { transientSlot(6, 1, 2) }); return s; } },
        { "udarnik_perestroika_soyuz_1node", [] { LayerSnapshot s; s.fx[0] = node(200, 10000, { transientSlot(0, 2, 2, -0.5f, 0.5f) }); return s; } },
        { "udarnik_udarnik_soyuz_mix", [] { LayerSnapshot s; s.mix = node(20, 20000, { transientSlot(6, 1, 2) }); return s; } },
        { "udarnik_perestroika_8nodes", [] { LayerSnapshot s; for (int i = 0; i < 8; ++i) s.fx[static_cast<size_t>(i)] = node(60.0f * std::pow(2.0f, static_cast<float>(i)), 150.0f * std::pow(2.0f, static_cast<float>(i)), { transientSlot(3, 2, 2, -0.5f, 0.5f) }); return s; } },
        { "eq_24_bells", [] { LayerSnapshot s; for (int i = 0; i < 24; ++i) s.eq[static_cast<size_t>(i)] = bell(i); return s; } },
        { "layer_ceiling_24bells_8x6comp", [] { LayerSnapshot s; for (int i = 0; i < 24; ++i) s.eq[static_cast<size_t>(i)] = bell(i);
                                                for (int i = 0; i < 8; ++i) s.fx[static_cast<size_t>(i)] = node(60.0f * std::pow(2.0f, static_cast<float>(i)), 150.0f * std::pow(2.0f, static_cast<float>(i)), { compSlot(), compSlot(), compSlot(), compSlot(), compSlot(), compSlot() }); return s; } },
    };
}

struct Run
{
    std::vector<double> us;
};

Run timeOne(const Config& c, const juce::AudioBuffer<float>& source, int blocks, int warmup)
{
    auto engine = std::make_unique<LayerEngine>();
    engine->prepare(kFs, kBlock);
    engine->publish(c.make());
    juce::AudioBuffer<float> work(2, kBlock);
    Run r;
    r.us.reserve(static_cast<size_t>(blocks));
    int position = 0;
    for (int b = 0; b < warmup + blocks; ++b)
    {
        for (int ch = 0; ch < 2; ++ch)
            work.copyFrom(ch, 0, source, ch, position, kBlock);
        position = (position + kBlock) % (source.getNumSamples() - kBlock);
        const auto t0 = std::chrono::steady_clock::now();
        engine->process(work.getArrayOfWritePointers(), 2, kBlock);
        const auto t1 = std::chrono::steady_clock::now();
        if (b >= warmup)
            r.us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
    }
    return r;
}

double median(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}
} // namespace

int runBench(const juce::File& out, const juce::String& commit, int reps, int blocks, bool cold)
{
    // cold: no warm-up and normal priority, i.e. how the M2-M4 lab tools measured.
    const int warmup = cold ? 0 : 200;
    if (!cold)
        juce::Process::setPriority(juce::Process::HighPriority);
    const auto source = makePinkNoise({ kFs, 2, 12.0, kFixtureSeed }, -18.0);
    const auto list = configs();
    std::vector<std::vector<Run>> runs(list.size());
    for (int r = 0; r < reps; ++r)
        for (size_t c = 0; c < list.size(); ++c)
        {
            runs[c].push_back(timeOne(list[c], source, blocks, warmup));
            std::cout << "." << std::flush;
        }
    std::cout << "\n";

    const double budget = 1.0e6 * kBlock / kFs;
    auto results = jsonArray();
    for (size_t c = 0; c < list.size(); ++c)
    {
        std::vector<double> p50, p95, p99;
        double worst = 0.0, spikeCount = 0.0, spacingSum = 0.0;
        int spacingN = 0;
        for (const auto& run : runs[c])
        {
            const auto t = summarise(run.us);
            p50.push_back(t.p50);
            p95.push_back(t.p95);
            p99.push_back(t.p99);
            worst = std::max(worst, t.max);
            int last = -1;
            for (size_t i = 0; i < run.us.size(); ++i)
                if (run.us[i] > 3.0 * t.p50)
                {
                    ++spikeCount;
                    if (last >= 0)
                    {
                        spacingSum += static_cast<double>(static_cast<int>(i) - last);
                        ++spacingN;
                    }
                    last = static_cast<int>(i);
                }
        }
        const double m50 = median(p50), m99 = median(p99);
        results.append(JsonObject()
                           .set("name", list[c].name)
                           .set("p50_us", m50)
                           .set("p95_us", median(p95))
                           .set("p99_us", m99)
                           .set("max_us", worst)
                           .set("p50_budget_pct", 100.0 * m50 / budget)
                           .set("p99_budget_pct", 100.0 * m99 / budget)
                           .set("spikes_per_run", spikeCount / reps)
                           .set("spike_mean_spacing_blocks", spacingN > 0 ? spacingSum / spacingN : 0.0));
        std::cout << list[c].name << ": p50 " << m50 << " us, p99 " << m99 << " us (" << 100.0 * m99 / budget
                  << " %), max " << worst << ", spikes/run " << spikeCount / reps << "\n";
    }
    writeJson(out, JsonObject()
                       .set("commit", commit)
                       .set("machine", describeMachine())
                       .set("block", kBlock)
                       .set("sample_rate", kFs)
                       .set("reps", reps)
                       .set("mode", cold ? "cold: no warm-up, normal priority" : "warm: 200 blocks warm-up, high priority")
                       .set("blocks_per_run", blocks)
                       .set("budget_us", budget)
                       .set("configs", results));
    return 0;
}
