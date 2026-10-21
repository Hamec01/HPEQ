// HPEQ_Harness — Milestone 0 measurement tool.
//
//   HPEQ_Harness selfcheck
//   HPEQ_Harness fixtures  --out <dir> [--commit <sha>]
//   HPEQ_Harness baseline  --out <dir> [--commit <sha>]
//
// `selfcheck` validates the metrics against analytic answers (exit code != 0 on any failure).
// `fixtures` renders fx_addon_v1 to 32-bit float WAV + manifest.json with SHA-256 per file.
// `baseline` writes baseline_metrics.json: self-check numbers, fixture level stats, timing floor of
// this machine, and the HPDG block audit (when built with HPEQ_HPDG_SOURCE_DIR).

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>

#include <juce_audio_formats/juce_audio_formats.h>

#include "Harness/AudioFileIO.h"
#include "Harness/AudioMetrics.h"
#include "Harness/Json.h"
#include "Harness/MachineInfo.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"

namespace hpeq::test
{
juce::var runHpdgBlockAudit(); // HpdgBlockAudit.cpp
}

using namespace hpeq::test;

namespace
{
const std::vector<double> kSampleRates { 44100.0, 48000.0, 96000.0, 192000.0 };
const std::vector<int> kBlockSizes { 1, 16, 32, 64, 127, 256, 512, 1024, 2048 };

struct Check
{
    juce::String name;
    double measured = 0.0;
    double expected = 0.0;
    double tolerance = 0.0;
    bool pass = false;
    juce::String note;
};

Check within(const juce::String& name, double measured, double expected, double tolerance, juce::String note = {})
{
    return { name, measured, expected, tolerance, std::abs(measured - expected) <= tolerance, std::move(note) };
}

Check isTrue(const juce::String& name, bool condition, juce::String note = {})
{
    return { name, condition ? 1.0 : 0.0, 1.0, 0.0, condition, std::move(note) };
}

// Power spectral density slope in dB/octave between two frequencies (averaged 1/3-octave windows).
double psdSlopeDbPerOctave(const juce::AudioBuffer<float>& audio, double sampleRate, double fLow, double fHigh)
{
    std::vector<double> grid;
    for (double f = fLow / 1.122; f <= fLow * 1.122; f *= 1.01) grid.push_back(f);
    const size_t lowCount = grid.size();
    for (double f = fHigh / 1.122; f <= fHigh * 1.122; f *= 1.01) grid.push_back(f);

    // |X(f)|^2 via the transfer API against a unit impulse (H = X).
    auto impulse = makeImpulse({ sampleRate, 1, audio.getNumSamples() / sampleRate, kFixtureSeed }, 0, 1.0f);
    const auto points = measureTransfer(audio, impulse, sampleRate, grid, 0, -400.0);
    double lowSum = 0.0, highSum = 0.0;
    for (size_t i = 0; i < points.size(); ++i)
    {
        const double power = std::pow(10.0, points[i].magnitudeDb.value_or(kSilenceDb) / 10.0);
        (i < lowCount ? lowSum : highSum) += power;
    }
    const double lowDb = 10.0 * std::log10(lowSum / static_cast<double>(lowCount));
    const double highDb = 10.0 * std::log10(highSum / static_cast<double>(points.size() - lowCount));
    return (highDb - lowDb) / std::log2(fHigh / fLow);
}

std::vector<Check> runSelfChecks()
{
    std::vector<Check> checks;
    const SignalRequest base { 48000.0, 2, 1.0, kFixtureSeed };

    // Determinism: same request -> same PCM hash.
    checks.push_back(isTrue("determinism_pink_sha256",
                            sha256Hex(makePinkNoise(base, -20.0)) == sha256Hex(makePinkNoise(base, -20.0))));
    checks.push_back(isTrue("determinism_drums_sha256",
                            sha256Hex(makeSyntheticDrums(base)) == sha256Hex(makeSyntheticDrums(base))));
    {
        SignalRequest other = base;
        other.seed = kFixtureSeed + 1;
        checks.push_back(isTrue("seed_changes_noise",
                                sha256Hex(makeWhiteNoise(base, -20.0)) != sha256Hex(makeWhiteNoise(other, -20.0))));
    }

    const auto pink = makePinkNoise(base, -20.0);

    // Null metric: identity, known gain error, known delay.
    {
        const auto r = compareAligned(pink, pink);
        checks.push_back(isTrue("null_identity_bit_exact", r.bitExact));
        checks.push_back(within("null_identity_error_db", r.errorDb, kSilenceDb, 0.0));
    }
    {
        auto y = pink;
        y.applyGain(1.0f + 1.0e-3f);
        checks.push_back(within("null_gain_1e-3_error_db", compareAligned(y, pink).errorDb, -60.0, 0.01,
                                "20*log10(1e-3) = -60 dB"));
    }
    {
        constexpr int delay = 37;
        juce::AudioBuffer<float> y(pink.getNumChannels(), pink.getNumSamples() + delay);
        y.clear();
        for (int ch = 0; ch < pink.getNumChannels(); ++ch)
            y.copyFrom(ch, delay, pink, ch, 0, pink.getNumSamples());
        checks.push_back(within("delay_estimate_samples", estimateDelay(y, pink, 256), delay, 0.0));
        checks.push_back(within("null_after_alignment_error_db", compareAligned(y, pink, delay).errorDb, kSilenceDb, 0.0));
    }

    // Transfer function: identity and -6.02 dB.
    {
        const auto sweep = makeExpSweep(base, 20.0, maxValidFrequency(base.sampleRate), -6.0);
        const std::vector<double> grid { 31.5, 100.0, 1000.0, 10000.0, 20000.0 };
        double worstIdentity = 0.0, worstHalf = 0.0;
        auto half = sweep;
        half.applyGain(0.5f);
        for (const auto& p : measureTransfer(sweep, sweep, base.sampleRate, grid))
            worstIdentity = std::max(worstIdentity, std::abs(p.magnitudeDb.value_or(99.0)));
        for (const auto& p : measureTransfer(half, sweep, base.sampleRate, grid))
            worstHalf = std::max(worstHalf, std::abs(p.magnitudeDb.value_or(99.0) + 6.0206));
        checks.push_back(within("transfer_identity_worst_abs_db", worstIdentity, 0.0, 1.0e-6));
        checks.push_back(within("transfer_half_gain_worst_error_db", worstHalf, 0.0, 1.0e-3));
    }

    // Generators against analytic values at every fixture rate.
    for (double fs : kSampleRates)
    {
        const SignalRequest r { fs, 2, 1.0, kFixtureSeed };
        const auto sine = makeSine(r, 1000.0, -6.0);
        const auto label = juce::String(static_cast<int>(fs));
        checks.push_back(within("sine_1k_frequency_hz_" + label, dominantFrequency(sine, fs), 1000.0, 0.5));
        checks.push_back(within("sine_-6dBFS_rms_db_" + label, measureAll(sine).rmsDbfs, -6.0 - 3.0103, 0.01,
                                "peak -6 dBFS sine -> RMS -9.03 dBFS"));
        checks.push_back(within("pink_psd_slope_db_per_oct_" + label,
                                psdSlopeDbPerOctave(makePinkNoise({ fs, 1, 4.0, kFixtureSeed }, -20.0), fs, 100.0, 10000.0),
                                -3.01, 0.5));
        checks.push_back(within("white_rms_dbfs_" + label, measureAll(makeWhiteNoise(r, -20.0)).rmsDbfs, -20.0, 0.01));
    }
    checks.push_back(isTrue("na_frequency_above_0.45fs", !isFrequencyValid(25000.0, 44100.0) && isFrequencyValid(15000.0, 44100.0)));

    // Stereo placement semantics, M = (L+R)/2, S = (L-R)/2.
    {
        const auto mid = measureStereo(makeStereoPlacement(base, StereoPlacement::MidOnly, -20.0));
        const auto side = measureStereo(makeStereoPlacement(base, StereoPlacement::SideOnly, -20.0));
        const auto left = measureStereo(makeStereoPlacement(base, StereoPlacement::LeftOnly, -20.0));
        checks.push_back(within("mid_only_side_rms_db", mid.sideRmsDbfs, kSilenceDb, 0.0));
        checks.push_back(within("side_only_mid_rms_db", side.midRmsDbfs, kSilenceDb, 0.0));
        checks.push_back(within("mid_only_correlation", mid.correlation, 1.0, 1.0e-9));
        checks.push_back(within("side_only_correlation", side.correlation, -1.0, 1.0e-9));
        checks.push_back(within("left_only_correlation", left.correlation, 0.0, 0.0));
    }

    // Subnormal fixture really is subnormal.
    {
        const auto sub = makeSubnormal({ 48000.0, 1, 0.01, kFixtureSeed });
        checks.push_back(within("subnormal_fixture_count", measureAll(sub).subnormalCount, sub.getNumSamples(), 0.0));
    }

    // WAV round trip is lossless for float32.
    {
        const auto file = juce::File::createTempFile(".wav");
        const bool written = writeFloatWav(file, pink, base.sampleRate);
        juce::AudioBuffer<float> back;
        double fs = 0.0;
        const bool read = written && readWav(file, back, fs);
        checks.push_back(isTrue("wav_float32_roundtrip_bit_exact", read && compareAligned(back, pink).bitExact && fs == base.sampleRate));
        file.deleteFile();
    }

    // Allocation probe sees a C++ allocation on this thread.
    {
        ScopedAllocationProbe probe;
        auto* p = new int(7);
        const auto counts = probe.counts();
        delete p;
        checks.push_back(isTrue("alloc_probe_counts_new", counts.allocations >= 1, allocationCoverageName()));
    }

    return checks;
}

juce::var checksToJson(const std::vector<Check>& checks)
{
    auto array = jsonArray();
    for (const auto& c : checks)
        array.append(JsonObject()
                         .set("name", c.name)
                         .set("measured", c.measured)
                         .set("expected", c.expected)
                         .set("tolerance", c.tolerance)
                         .set("pass", c.pass)
                         .set("note", c.note));
    return array;
}

int printChecks(const std::vector<Check>& checks)
{
    int failures = 0;
    for (const auto& c : checks)
    {
        std::cout << (c.pass ? "[PASS] " : "[FAIL] ") << c.name << " measured=" << c.measured
                  << " expected=" << c.expected << " tol=" << c.tolerance << "\n";
        failures += c.pass ? 0 : 1;
    }
    std::cout << checks.size() - static_cast<size_t>(failures) << "/" << checks.size() << " self-checks passed\n";
    return failures;
}

juce::var levelsToJson(const LevelStats& s)
{
    return JsonObject()
        .set("peak_dbfs", s.peakDbfs)
        .set("rms_dbfs", s.rmsDbfs)
        .set("dc", s.dc)
        .set("crest_db", s.crestDb)
        .set("all_finite", s.allFinite)
        .set("subnormal_count", s.subnormalCount);
}

juce::String argValue(const juce::StringArray& args, const juce::String& key, const juce::String& fallback = {})
{
    const int index = args.indexOf(key);
    return index >= 0 && index + 1 < args.size() ? args[index + 1] : fallback;
}

int runFixtures(const juce::File& outDir, const juce::String& commit)
{
    auto files = jsonArray();
    int written = 0, notApplicable = 0;
    for (double fs : kSampleRates)
    {
        const SignalRequest request { fs, 2, 1.0, kFixtureSeed };
        for (auto& fixture : makeFixtureSet(request))
        {
            auto entry = JsonObject()
                             .set("name", juce::String(fixture.name))
                             .set("description", juce::String(fixture.description))
                             .set("sample_rate", fs)
                             .set("channels", fixture.audio.getNumChannels())
                             .set("samples", fixture.audio.getNumSamples())
                             .set("valid", fixture.valid);
            if (!fixture.valid)
            {
                ++notApplicable;
                entry.set("status", "N/A");
                files.append(entry);
                continue;
            }
            const auto relative = juce::String(static_cast<int>(fs)) + "/" + juce::String(fixture.name) + ".wav";
            const auto file = outDir.getChildFile(relative);
            if (!writeFloatWav(file, fixture.audio, fs))
            {
                std::cerr << "failed to write " << file.getFullPathName() << "\n";
                return 2;
            }
            ++written;
            entry.set("file", relative)
                .set("sha256_file", sha256Hex(file))
                .set("sha256_pcm", sha256Hex(fixture.audio))
                .set("levels", levelsToJson(measureAll(fixture.audio)))
                .set("spectral_centroid_hz", spectralCentroid(fixture.audio, fs));
            files.append(entry);
        }
    }

    const auto manifest = JsonObject()
                              .set("fixture_set", kFixtureSetName)
                              .set("seed", static_cast<juce::int64>(kFixtureSeed))
                              .set("format", "WAV 32-bit float, stereo, 1.0 s")
                              .set("commit", commit)
                              .set("machine", describeMachine())
                              .set("files_written", written)
                              .set("not_applicable", notApplicable)
                              .set("files", files);
    writeJson(outDir.getChildFile("manifest.json"), manifest);
    std::cout << "fixtures: " << written << " WAV written, " << notApplicable << " N/A -> "
              << outDir.getFullPathName() << "\n";
    return 0;
}

// Timing floor of this machine: a plain buffer copy per block. Any DSP measured later is compared
// against budget = block/fs and against this floor, on the same machine and build.
juce::var measureTimingFloor()
{
    auto results = jsonArray();
    constexpr double fs = 48000.0;
    const auto source = makePinkNoise({ fs, 2, 10.0, kFixtureSeed }, -20.0);
    for (int block : kBlockSizes)
    {
        juce::AudioBuffer<float> work(2, block);
        std::vector<double> microseconds;
        const int blocks = juce::jmin(20000, source.getNumSamples() / block);
        microseconds.reserve(static_cast<size_t>(blocks));
        for (int b = 0; b < blocks; ++b)
        {
            const auto start = std::chrono::steady_clock::now();
            for (int ch = 0; ch < 2; ++ch)
                work.copyFrom(ch, 0, source, ch, b * block, block);
            const auto end = std::chrono::steady_clock::now();
            microseconds.push_back(std::chrono::duration<double, std::micro>(end - start).count());
        }
        const auto t = summarise(std::move(microseconds));
        const double budget = 1.0e6 * block / fs;
        results.append(JsonObject()
                           .set("block_size", block)
                           .set("sample_rate", fs)
                           .set("budget_us", budget)
                           .set("blocks", t.count)
                           .set("p50_us", t.p50)
                           .set("p95_us", t.p95)
                           .set("p99_us", t.p99)
                           .set("max_us", t.max)
                           .set("p99_budget_fraction", t.p99 / budget));
    }
    return results;
}

int runBaseline(const juce::File& outDir, const juce::String& commit)
{
    const auto checks = runSelfChecks();
    const int failures = printChecks(checks);

    auto fixtureLevels = jsonArray();
    for (auto& fixture : makeFixtureSet({ 48000.0, 2, 1.0, kFixtureSeed }))
    {
        fixtureLevels.append(JsonObject()
                                 .set("name", juce::String(fixture.name))
                                 .set("valid", fixture.valid)
                                 .set("sha256_pcm", sha256Hex(fixture.audio))
                                 .set("levels", levelsToJson(measureAll(fixture.audio)))
                                 .set("stereo", JsonObject()
                                                    .set("mid_rms_dbfs", measureStereo(fixture.audio).midRmsDbfs)
                                                    .set("side_rms_dbfs", measureStereo(fixture.audio).sideRmsDbfs)
                                                    .set("correlation", measureStereo(fixture.audio).correlation)));
    }

    std::cout << "measuring timing floor...\n";
    const auto timing = measureTimingFloor();
    std::cout << "running HPDG block audit...\n";
    const auto hpdg = runHpdgBlockAudit();

    const auto report = JsonObject()
                            .set("milestone", "M0")
                            .set("commit", commit)
                            .set("machine", describeMachine())
                            .set("self_checks", checksToJson(checks))
                            .set("self_check_failures", failures)
                            .set("fixture_levels_48k", fixtureLevels)
                            .set("timing_floor_copy", timing)
                            .set("hpdg_block_audit", hpdg);
    const auto file = outDir.getChildFile("baseline_metrics_" + buildConfiguration().toLowerCase() + ".json");
    writeJson(file, report);
    std::cout << "baseline -> " << file.getFullPathName() << "\n";
    return failures == 0 ? 0 : 1;
}
} // namespace

int main(int argc, char* argv[])
{
    juce::StringArray args;
    for (int i = 1; i < argc; ++i)
        args.add(argv[i]);

    const auto command = args.isEmpty() ? juce::String("selfcheck") : args[0];
    const auto outDir = juce::File::getCurrentWorkingDirectory().getChildFile(argValue(args, "--out", "hpeq_m0_out"));
    const auto commit = argValue(args, "--commit", "unknown");

    if (command == "selfcheck")
        return printChecks(runSelfChecks()) == 0 ? 0 : 1;
    if (command == "fixtures")
        return runFixtures(outDir, commit);
    if (command == "baseline")
        return runBaseline(outDir, commit);

    std::cerr << "usage: HPEQ_Harness selfcheck | fixtures --out <dir> | baseline --out <dir> [--commit <sha>]\n";
    return 64;
}
