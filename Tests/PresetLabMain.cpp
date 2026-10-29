// HamloMag_PresetLab calibrate|check <material dir> [report.md] (calibrate: [keys; ...]; check: [report.md] [key filter])
// docs/GENRE_PRESETS.md. Each genre preset plays on its group's material (the owner's clips, the loudest 20 s of each,
// gained to an active RMS of -18 dBFS and held to a -3 dBFS peak, mixes -14 - as the other Hamlo preset labs). The
// depth is the saturation's own share: N = y(x) - 2 y(x / 2) (the linear parts - the models' filters, tape wow, the
// samplers' rates - cancel), work = 10 log10(E[N^2] / E[y^2]) with the noises (hiss, hum, crackle) off.
// `calibrate` prints, per genre preset, the DRIVE that gives the middle of its depth's band (CAL lines for
// Tools/genre_presets.py); `check` measures every genre preset and writes the table. Exit code = presets that fail.

#include <cmath>
#include <iostream>
#include <map>
#include <set>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloMagProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"

namespace
{
using hamlo::saturator::HamloMagProcessor;
using hamlo::saturator::Preset;
constexpr double kFs = 48000.0;
constexpr int kBlock = 512;

const std::map<juce::String, juce::StringArray> kMaterial {
    { "drums", { "drumbus1" } }, { "kick", { "kick" } }, { "snare", { "snare" } }, { "hats", { "hihat" } }, { "perc", { "perc" } },
    { "vocal", { "vocal_lead1", "vocal_lead2", "vocal_lead3" } }, { "backing", { "vocal_back1", "vocal_back2", "vocal_back3" } },
    { "bass", { "bass1", "bass2" } }, { "keys", { "keys", "keys2" } }, { "samples", { "guitar", "synth", "keys2" } },
    { "mix", { "mixdown_Ardnold", "mixdown_boom_bap_3", "mixdown_sad_trap" } } };

double blockRms(const juce::AudioBuffer<float>& x, int start, int n)
{
    double s = 0.0;
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        for (int i = start; i < start + n; ++i)
            s += static_cast<double>(x.getSample(ch, i)) * x.getSample(ch, i);
    return std::sqrt(s / (x.getNumChannels() * static_cast<double>(n)));
}

double activeRms(const juce::AudioBuffer<float>& x)
{
    double s = 0.0;
    int count = 0;
    for (int start = 0; start + kBlock <= x.getNumSamples(); start += kBlock)
    {
        const double r = blockRms(x, start, kBlock);
        if (r > std::pow(10.0, -50.0 / 20.0))
        {
            s += r * r;
            ++count;
        }
    }
    return count > 0 ? std::sqrt(s / count) : 0.0;
}

juce::AudioBuffer<float> load(const juce::File& file, double targetDb)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(file));
    if (r == nullptr)
        return {};
    juce::AudioBuffer<float> all(2, static_cast<int>(r->lengthInSamples));
    r->read(&all, 0, all.getNumSamples(), 0, true, true);
    const int length = std::min(all.getNumSamples(), static_cast<int>(10.0 * kFs)); // the loudest 10 s (HamloMag renders three times per measure)
    int best = 0;
    double bestEnergy = -1.0;
    for (int start = 0; start + length <= all.getNumSamples(); start += static_cast<int>(kFs))
    {
        const double e = blockRms(all, start, length);
        if (e > bestEnergy)
        {
            bestEnergy = e;
            best = start;
        }
    }
    juce::AudioBuffer<float> x(2, length);
    for (int ch = 0; ch < 2; ++ch)
        x.copyFrom(ch, 0, all, ch, best, length);
    x.applyGain(static_cast<float>(std::pow(10.0, targetDb / 20.0) / std::max(1.0e-9, activeRms(x))));
    // and the peak no higher than -3 dBFS (a track with headroom; at -18 dBFS RMS the hats and percussion peaked over
    // LIMIT, so the limiter cut them at LOUDNESS 0 - second calibration run, docs/GENRE_PRESETS.md)
    if (const float peak = x.getMagnitude(0, x.getNumSamples()); peak > 0.70794578f)
        x.applyGain(0.70794578f / peak);
    return x;
}

struct Measure
{
    bool finite = true;
    double work = -200, outPeakDb = -120, rmsDelta = 0;
};

juce::AudioBuffer<float> renderWith(int program, const juce::AudioBuffer<float>& x, const double* drive, bool quiet)
{
    HamloMagProcessor p;
    p.setCurrentProgram(program);
    if (drive != nullptr)
        p.setParameterValue("driveDb", *drive);
    if (quiet)
        for (const char* id : { "hiss", "hum", "crackle" })
            p.setParameterValue(id, 0.0);
    p.prepareToPlay(kFs, kBlock);
    const int latency = p.getLatencySamples();
    juce::AudioBuffer<float> y(2, x.getNumSamples() + latency);
    y.clear();
    for (int ch = 0; ch < 2; ++ch)
        y.copyFrom(ch, 0, x, ch, 0, x.getNumSamples());
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += kBlock)
    {
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, std::min(kBlock, y.getNumSamples() - start));
        p.processBlock(view, midi);
    }
    juce::AudioBuffer<float> out(2, x.getNumSamples());
    for (int ch = 0; ch < 2; ++ch)
        out.copyFrom(ch, 0, y, ch, latency, x.getNumSamples());
    return out;
}

Measure play(int program, const juce::AudioBuffer<float>& x, const double* drive)
{
    Measure m;
    const auto y = renderWith(program, x, drive, true);
    auto half = x;
    half.applyGain(0.5f);
    const auto yh = renderWith(program, half, drive, true);
    double n = 0.0, e = 0.0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < y.getNumSamples(); ++i)
        {
            const double a = y.getSample(ch, i), d = a - 2.0 * yh.getSample(ch, i);
            m.finite = m.finite && std::isfinite(a);
            n += d * d;
            e += a * a;
        }
    m.work = 10.0 * std::log10((n + 1.0e-30) / (e + 1.0e-30));
    // Calibration ranks work plus the existing level / peak constraints on the quiet output. The independent
    // final check still renders with noises and judges the complete preset.
    if (drive != nullptr)
    {
        m.outPeakDb = 20.0 * std::log10(std::max(1.0e-9f, y.getMagnitude(0, y.getNumSamples())));
        m.rmsDelta = 20.0 * std::log10(std::max(1.0e-12, activeRms(y)) / std::max(1.0e-12, activeRms(x)));
        return m;
    }
    const auto full = renderWith(program, x, drive, false); // as it plays, noises in
    m.outPeakDb = 20.0 * std::log10(std::max(1.0e-9f, full.getMagnitude(0, full.getNumSamples())));
    m.rmsDelta = 20.0 * std::log10(std::max(1.0e-12, activeRms(full)) / std::max(1.0e-12, activeRms(x)));
    return m;
}

Measure playAll(int program, const std::vector<juce::AudioBuffer<float>>& clips, const double* drive)
{
    Measure sum;
    sum.work = 0.0;
    for (const auto& c : clips)
    {
        const auto m = play(program, c, drive);
        sum.finite = sum.finite && m.finite;
        sum.work += m.work / static_cast<double>(clips.size());
        sum.rmsDelta += m.rmsDelta / static_cast<double>(clips.size());
        sum.outPeakDb = std::max(sum.outPeakDb, m.outPeakDb);
    }
    return sum;
}

// bands of the saturation's share (dB re the output)
const char* depthName(Preset::Depth d) { return d == Preset::Gentle ? "GENTLE" : d == Preset::Medium ? "MEDIUM" : "HEAVY"; }
double bandLo(Preset::Depth d) { return d == Preset::Gentle ? -36.0 : d == Preset::Medium ? -24.0 : -14.0; }
double bandHi(Preset::Depth d) { return d == Preset::Gentle ? -24.0 : d == Preset::Medium ? -14.0 : -4.0; }
double bandMid(Preset::Depth d) { return d == Preset::Gentle ? -30.0 : d == Preset::Medium ? -19.0 : -9.0; }
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 3)
    {
        std::cout << "usage: HamloMag_PresetLab calibrate|check <material dir> [report.md | keys] [check key filter]\n";
        return 2;
    }
    const juce::String mode(argv[1]);
    const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
    std::map<juce::String, std::vector<juce::AudioBuffer<float>>> clips;
    for (const auto& [material, files] : kMaterial)
        for (const auto& f : files)
        {
            auto x = load(dir.getChildFile(f + ".wav"), material == "mix" ? -14.0 : -18.0);
            if (x.getNumSamples() > 0)
                clips[material].push_back(std::move(x));
        }
    const auto& list = hamlo::saturator::factoryPresets();
    int bad = 0, judged = 0;
    juce::String table = "| # | Genre / group / preset | Material | Depth | Saturation share | Out peak | RMS out - in | Pass |\n|---|---|---|---|---|---|---|---|\n";
    std::set<juce::String> seen;
    for (size_t k = 0; k < list.size(); ++k)
    {
        const auto& preset = list[k];
        if (juce::String(preset.genre).isEmpty())
            continue;
        const int program = static_cast<int>(k) + 1;
        const juce::String key = juce::String(preset.genre) + "/" + preset.group + "/" + preset.name;
        juce::String values = juce::String(preset.group) + ":";
        for (const auto& spec : hamlo::saturator::parameterSpecs())
            values << spec.id << "=" << hamlo::saturator::presetValue(preset, spec.id, spec.defaultValue) << ";";
        // Include every preceding preset even in a filtered run: duplicates across genres must still fail.
        const bool unique = seen.insert(values).second;
        // Optional subset for iterative recipe repairs; the release check omits this filter and judges all 200.
        if (mode == "check" && argc > 4 && !key.contains(argv[4]))
            continue;
        const auto it = clips.find(preset.material);
        if (it == clips.end())
        {
            std::cout << key << ": NO MATERIAL '" << preset.material << "'\n";
            ++bad;
            continue;
        }
        if (mode == "calibrate")
        {
            // argv[3] (optional): name or genre/group/name substring; ';' separates alternatives.
            if (argc > 3 && ![&] { for (const auto& part : juce::StringArray::fromTokens(argv[3], ";", "")) if (key.contains(part)) return true; return false; }())
                continue;
            // First run assumed monotonically rising saturation share, sending bounded HamloLaw recipes to 48 dB
            // on the wrong branch. Scan for the first target crossing, then bisect in either direction.
            // Metric, material, depth bands and final check are unchanged (docs/reports/v2/V2_REPORT.md).
            double lo = 0.0, hi = 0.0;
            double bestDrive = lo, bestError = 1.0e9;
            const auto measure = [&](double drive)
            {
                const auto m = playAll(program, it->second, &drive);
                // Prefer the existing level / peak constraints to a closer midpoint on an inaudibly quiet branch.
                // No target changes: the independent noisy check remains authoritative.
                const bool safe = m.finite && m.outPeakDb <= 3.0 && std::abs(m.rmsDelta) <= 6.0;
                const double error = std::abs(m.work - bandMid(preset.depth)) + (safe ? 0.0 : 1000.0 + std::abs(m.rmsDelta));
                if (error < bestError)
                {
                    bestDrive = drive;
                    bestError = error;
                }
                return m.work;
            };
            double previous = measure(lo);
            bool bracketed = false;
            for (double candidate = 4.0; candidate <= 48.0; candidate += 4.0)
            {
                const double value = measure(candidate);
                if ((previous - bandMid(preset.depth)) * (value - bandMid(preset.depth)) <= 0.0)
                {
                    lo = candidate - 4.0;
                    hi = candidate;
                    bracketed = true;
                    break;
                }
                previous = value;
            }
            if (bracketed)
                for (int i = 0; i < 6; ++i)
                {
                    const double mid = 0.5 * (lo + hi);
                    const double value = measure(mid);
                    if ((previous - bandMid(preset.depth)) * (value - bandMid(preset.depth)) <= 0.0)
                        hi = mid;
                    else
                    {
                        lo = mid;
                        previous = value;
                    }
                }
            const double t = std::round(bestDrive * 2.0) / 2.0;
            const auto m = playAll(program, it->second, &t);
            std::cout << "CAL\t" << key << "\t" << t << "\t" << m.work << "\t" << m.outPeakDb << "\t" << m.rmsDelta << std::endl;
            continue;
        }
        ++judged;
        const auto m = playAll(program, it->second, nullptr);
        const bool peakOk = m.outPeakDb <= 3.0; // the clips peak at -3 dBFS: a saturator may add up to 6 dB of peak
        const bool ok = m.finite && m.work >= bandLo(preset.depth) && m.work <= bandHi(preset.depth) && peakOk && m.rmsDelta >= -6.0
                        && m.rmsDelta <= 6.0 && unique;
        bad += ok ? 0 : 1;
        table << "| " << program << " | " << key << " | " << preset.material << " | " << depthName(preset.depth) << " | " << juce::String(m.work, 1) << " dB | "
              << juce::String(m.outPeakDb, 2) << " dBFS | " << juce::String(m.rmsDelta, 2) << " dB | " << (ok ? "yes" : "**no**") << (unique ? "" : " (duplicate)")
              << (m.finite ? "" : " (not finite)") << (peakOk ? "" : " (peak)") << " |\n";
        std::cout << program << " " << key << ": share " << m.work << " peak " << m.outPeakDb << " rms " << m.rmsDelta << (ok ? "" : "  FAIL") << std::endl;
    }
    if (mode == "check")
    {
        const auto summary = juce::String(judged) + " genre presets, " + juce::String(bad) + " outside their band / peak / level / uniqueness criteria";
        std::cout << summary << std::endl;
        if (argc > 3)
            juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]).replaceWithText("# HamloMag genre presets on their material\n\n" + summary + "\n\n" + table);
    }
    return bad;
}
