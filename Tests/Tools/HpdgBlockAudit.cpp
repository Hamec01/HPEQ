// M0: measure HPDG's existing DSP blocks, read-only, as the baseline HPEQ's rewrites must beat
// (DECISIONS.md D3). Compiled against HPDG sources only when HPEQ_HPDG_SOURCE_DIR is set.

#include <chrono>
#include <cmath>

#include <juce_dsp/juce_dsp.h>

#include "Harness/AudioMetrics.h"
#include "Harness/Json.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"

#if HPEQ_WITH_HPDG_BLOCKS
 #include "Audio/DrumCompressorBlock.h"
 #include "Audio/MonstaFxBlock.h"
#endif

namespace hpeq::test
{
#if !HPEQ_WITH_HPDG_BLOCKS
juce::var runHpdgBlockAudit()
{
    return JsonObject().set("status", "BLOCKED").set("reason", "built without HPEQ_HPDG_SOURCE_DIR");
}
#else
namespace
{
constexpr double kFs = 48000.0;
constexpr int kPreparedBlock = 512;

juce::dsp::ProcessSpec specFor(int maxBlock, int channels = 2)
{
    return { kFs, static_cast<juce::uint32>(maxBlock), static_cast<juce::uint32>(channels) };
}

bbg::CompressorState referenceCompressor()
{
    bbg::CompressorState state;
    state.enabled = true;
    state.ratio = 4.0f;
    state.thresholdDb = -20.0f;
    state.mix = 1.0f;
    state.attackMs = 10.0f;
    state.releaseMs = 120.0f;
    state.saturation = 0.0f;
    state.autoMakeup = false;
    state.character = bbg::DrumCompressorCharacter::Glue;
    return state;
}

// Runs `audio` through `process(block)` in blocks of `blockSize`; returns allocations seen inside.
template <typename Fn>
AllocationCounts processInBlocks(juce::AudioBuffer<float>& audio, int blockSize, Fn&& process)
{
    AllocationCounts total;
    for (int start = 0; start < audio.getNumSamples(); start += blockSize)
    {
        const int n = juce::jmin(blockSize, audio.getNumSamples() - start);
        juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), audio.getNumChannels(), start, n);
        ScopedAllocationProbe probe;
        process(view, start);
        total.allocations += probe.counts().allocations;
        total.frees += probe.counts().frees;
    }
    return total;
}

double rmsDbOfTail(const juce::AudioBuffer<float>& audio, double fromSeconds)
{
    const int start = static_cast<int>(fromSeconds * kFs);
    juce::AudioBuffer<float> tail(audio.getNumChannels(), audio.getNumSamples() - start);
    for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        tail.copyFrom(ch, 0, audio, ch, start, tail.getNumSamples());
    return measureAll(tail).rmsDbfs;
}

juce::var auditCompressor()
{
    auto result = JsonObject();

    // 1. Allocations: blocks within the prepared size, then a block larger than prepared.
    {
        bbg::DrumCompressorBlock block;
        block.prepare(specFor(kPreparedBlock));
        auto audio = makePinkNoise({ kFs, 2, 1.0, kFixtureSeed }, -12.0);
        const auto state = referenceCompressor();
        const auto inside = processInBlocks(audio, 256, [&](auto& b, int) { block.process(b, state); });
        auto audio2 = makePinkNoise({ kFs, 2, 1.0, kFixtureSeed }, -12.0);
        const auto oversize = processInBlocks(audio2, 2048, [&](auto& b, int) { block.process(b, state); });
        result.set("rt_allocations_block256_prepared512", static_cast<juce::int64>(inside.allocations))
            .set("rt_allocations_block2048_prepared512", static_cast<juce::int64>(oversize.allocations))
            .set("rt_allocation_source", "ensureScratchCapacity -> AudioBuffer::setSize when block > prepared");
    }

    // 2. Static curve vs the textbook hard-knee law out = T + (in - T)/R above T (peak-referenced).
    {
        auto curve = jsonArray();
        double worstDeviation = 0.0;
        for (double inDb = -40.0; inDb <= 0.01; inDb += 5.0)
        {
            bbg::DrumCompressorBlock block;
            block.prepare(specFor(kPreparedBlock));
            auto audio = makeSine({ kFs, 2, 1.5, kFixtureSeed }, 1000.0, inDb);
            const auto input = audio;
            processInBlocks(audio, 256, [&](auto& b, int) { block.process(b, referenceCompressor()); });
            const double measuredGainDb = rmsDbOfTail(audio, 1.0) - rmsDbOfTail(input, 1.0);
            const double expectedGainDb = inDb > -20.0 ? (-20.0 + (inDb + 20.0) / 4.0) - inDb : 0.0;
            worstDeviation = std::max(worstDeviation, std::abs(measuredGainDb - expectedGainDb));
            curve.append(JsonObject()
                             .set("input_peak_dbfs", inDb)
                             .set("measured_gain_db", measuredGainDb)
                             .set("hard_knee_expected_gain_db", expectedGainDb));
        }
        result.set("static_curve_T-20_R4", curve)
            .set("static_curve_worst_deviation_db", worstDeviation)
            .set("knee_parameter_available", false);
    }

    // 3. Effective attack/release on a -40 -> -6 dBFS burst vs the displayed 10 ms / 120 ms.
    {
        bbg::DrumCompressorBlock block;
        block.prepare(specFor(kPreparedBlock));
        auto audio = makeGatedBurst({ kFs, 2, 1.0, kFixtureSeed }, -6.0, -40.0, 0.25);
        const auto input = audio;
        processInBlocks(audio, 256, [&](auto& b, int) { block.process(b, referenceCompressor()); });

        // Gain per 1 ms window (output RMS / input RMS), around the loud segment 0.25..0.5 s.
        const int window = static_cast<int>(kFs / 1000.0);
        std::vector<double> gainDb;
        for (int start = 0; start + window <= audio.getNumSamples(); start += window)
        {
            double in = 0.0, out = 0.0;
            for (int i = start; i < start + window; ++i)
            {
                in += static_cast<double>(input.getSample(0, i)) * input.getSample(0, i);
                out += static_cast<double>(audio.getSample(0, i)) * audio.getSample(0, i);
            }
            gainDb.push_back(in > 0.0 ? 10.0 * std::log10(out / in) : 0.0);
        }
        // Time to reach 63 % of the final gain reduction (in dB) after an edge.
        const auto timeConstantMs = [&](int edgeMs, int endMs)
        {
            const double startGain = gainDb[static_cast<size_t>(juce::jmax(0, edgeMs - 2))];
            const double finalGain = gainDb[static_cast<size_t>(endMs - 2)];
            const double target = startGain + 0.632 * (finalGain - startGain);
            for (int ms = edgeMs; ms < endMs; ++ms)
                if ((finalGain < startGain && gainDb[static_cast<size_t>(ms)] <= target)
                    || (finalGain > startGain && gainDb[static_cast<size_t>(ms)] >= target))
                    return static_cast<double>(ms - edgeMs);
            return -1.0;
        };
        result.set("nominal_attack_ms", 10.0)
            .set("measured_attack_63pct_ms", timeConstantMs(250, 500))
            .set("nominal_release_ms", 120.0)
            .set("measured_release_63pct_ms", timeConstantMs(500, 750))
            .set("note", "Glue character scales attack x0.82 and release x1.12+12 ms; gain reduction for meters is block-peak");
    }

    // 4. Mix = 0 must return the dry signal exactly.
    {
        bbg::DrumCompressorBlock block;
        block.prepare(specFor(kPreparedBlock));
        auto state = referenceCompressor();
        state.mix = 0.0f;
        auto audio = makeSyntheticDrums({ kFs, 2, 1.0, kFixtureSeed });
        const auto input = audio;
        processInBlocks(audio, 256, [&](auto& b, int) { block.process(b, state); });
        const auto null = compareAligned(audio, input);
        result.set("mix0_bit_exact", null.bitExact).set("mix0_error_db", null.errorDb);
    }

    // 5. CPU on this machine, 48 kHz stereo, block 256.
    {
        bbg::DrumCompressorBlock block;
        block.prepare(specFor(kPreparedBlock));
        auto audio = makeSyntheticDrums({ kFs, 2, 10.0, kFixtureSeed });
        std::vector<double> us;
        const auto state = referenceCompressor();
        for (int start = 0; start + 256 <= audio.getNumSamples(); start += 256)
        {
            juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), 2, start, 256);
            const auto t0 = std::chrono::steady_clock::now();
            block.process(view, state);
            us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
        }
        const auto t = summarise(std::move(us));
        const double budget = 1.0e6 * 256 / kFs;
        result.set("cpu_block256_p50_us", t.p50).set("cpu_block256_p99_us", t.p99)
            .set("cpu_block256_p99_budget_fraction", t.p99 / budget);
    }
    return result;
}

juce::var auditMonsta()
{
    auto result = JsonObject();
    bbg::MonstaFxState state;
    state.enabled = true;
    state.dry = 0.0f;
    state.wet = 1.0f;
    state.chaosSeed = 1234u;

    const auto render = [&](int blockSize, AllocationCounts* allocations)
    {
        bbg::MonstaFxBlock block;
        block.prepare(specFor(kPreparedBlock));
        auto audio = makeSyntheticDrums({ kFs, 2, 4.0, kFixtureSeed });
        const auto counts = processInBlocks(audio, blockSize, [&](auto& b, int start)
        {
            bbg::MonstaFxTimelineContext timeline;
            timeline.bpm = 120.0;
            timeline.blockStartSample = start;
            timeline.blockStartQuarter = start / (kFs * 0.5);
            block.process(b, state, timeline);
        });
        if (allocations != nullptr)
            *allocations = counts;
        return audio;
    };

    AllocationCounts allocations;
    const auto reference = render(256, &allocations);
    const int blocks = (reference.getNumSamples() + 255) / 256;
    result.set("rt_allocations_block256", static_cast<juce::int64>(allocations.allocations))
        .set("blocks_processed", blocks)
        .set("rt_allocation_source", "per-call std::vector heldCrushSamples in process()");

    // T10-style block-size invariance against the 256-sample render.
    auto invariance = jsonArray();
    for (int blockSize : { 32, 127, 512 })
    {
        const auto other = render(blockSize, nullptr);
        const auto null = compareAligned(other, reference);
        invariance.append(JsonObject().set("block_size", blockSize).set("error_db_vs_256", null.errorDb).set("bit_exact", null.bitExact));
    }
    result.set("block_size_invariance", invariance);

    // Without a running transport (bpm 0 -> fallback 120, timeline frozen at 0 every block).
    {
        bbg::MonstaFxBlock block;
        block.prepare(specFor(kPreparedBlock));
        auto audio = makeSyntheticDrums({ kFs, 2, 2.0, kFixtureSeed });
        processInBlocks(audio, 256, [&](auto& b, int)
        {
            bbg::MonstaFxTimelineContext frozen;
            frozen.bpm = 0.0;
            block.process(b, state, frozen);
        });
        const auto levels = measureAll(audio);
        result.set("no_transport_output_rms_dbfs", levels.rmsDbfs).set("no_transport_all_finite", levels.allFinite);
    }
    return result;
}

// HPDG's EQ pattern (applySoundLayerFx): IIR::Coefficients created per block on the audio thread,
// no smoothing, cuts fixed at 2nd order.
juce::var auditEqPattern()
{
    auto result = JsonObject();

    {
        ScopedAllocationProbe probe;
        auto coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(kFs, 1000.0f, 1.0f, 2.0f);
        result.set("allocations_per_coefficient_update", static_cast<juce::int64>(probe.counts().allocations));
    }

    // Zipper: 100 Hz sine, bell at 100 Hz swept -12 -> +12 dB over 1 s, coefficients replaced every
    // 512 samples (HPDG pattern). Energy above 2 kHz in the output is modulation/click products.
    {
        auto audio = makeSine({ kFs, 1, 1.0, kFixtureSeed }, 100.0, -12.0);
        juce::dsp::IIR::Filter<float> filter;
        filter.prepare(specFor(512, 1));
        for (int start = 0; start < audio.getNumSamples(); start += 512)
        {
            const int n = juce::jmin(512, audio.getNumSamples() - start);
            const float gainDb = -12.0f + 24.0f * static_cast<float>(start) / static_cast<float>(audio.getNumSamples());
            filter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(kFs, 100.0f, 1.0f, juce::Decibels::decibelsToGain(gainDb));
            for (int i = start; i < start + n; ++i)
                audio.setSample(0, i, filter.processSample(audio.getSample(0, i)));
        }
        juce::dsp::IIR::Filter<float> highPass;
        highPass.prepare(specFor(512, 1));
        highPass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(kFs, 2000.0f);
        auto hf = audio;
        for (int i = 0; i < hf.getNumSamples(); ++i)
            hf.setSample(0, i, highPass.processSample(hf.getSample(0, i)));
        const auto clean = makeSine({ kFs, 1, 1.0, kFixtureSeed }, 100.0, -12.0);
        result.set("zipper_hf_above_2k_rel_db", measureAll(hf).rmsDbfs - measureAll(audio).rmsDbfs)
            .set("zipper_hf_above_2k_rel_db_note",
                 "FLAWED (found in M2): 2nd-order measuring high-pass lets the 100 Hz tone through at ~-52 dB; "
                 "this number is the metric's floor, not zipper noise")
            .set("zipper_hf8_above_2k_rel_db", highBandRelativeDb(audio, kFs, 2000.0))
            .set("zipper_hf8_metric_floor_clean_sine_db", highBandRelativeDb(clean, kFs, 2000.0))
            .set("zipper_note", "8th-order measuring high-pass; baseline for HPEQ PRE-EQ automation");
    }

    // Cut slope: HPDG LowCut = makeHighPass(f, q) = 2nd order -> ~12 dB/oct; measured 25..50 Hz.
    {
        const auto sweep = makeExpSweep({ kFs, 1, 4.0, kFixtureSeed }, 10.0, 20000.0, -6.0);
        auto out = sweep;
        juce::dsp::IIR::Filter<float> hp;
        hp.prepare(specFor(512, 1));
        hp.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(kFs, 200.0f, 0.71f);
        for (int i = 0; i < out.getNumSamples(); ++i)
            out.setSample(0, i, hp.processSample(out.getSample(0, i)));
        const auto points = measureTransfer(out, sweep, kFs, { 25.0, 50.0 });
        const double slope = points[1].magnitudeDb.value_or(0.0) - points[0].magnitudeDb.value_or(0.0);
        result.set("low_cut_slope_db_per_oct_25_50hz", slope)
            .set("available_shapes", "Bell, LowCut, HighCut (7 bands, cuts 2nd order only)");
    }
    return result;
}
} // namespace

juce::var runHpdgBlockAudit()
{
    return JsonObject()
        .set("status", "MEASURED")
        .set("sample_rate", kFs)
        .set("prepared_block", kPreparedBlock)
        .set("allocation_probe", allocationCoverageName())
        .set("compressor", auditCompressor())
        .set("monsta", auditMonsta())
        .set("eq_pattern", auditEqPattern());
}
#endif
} // namespace hpeq::test
