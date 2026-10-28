// HamloRedWall_PresetLab calibrate|check <material dir> [report.md]
// docs/GENRE_PRESETS.md. Each genre preset plays on its group's material (the owner's clips, the loudest 20 s of each,
// gained to an active RMS of -18 dBFS, mixes -14 - as HamloComp_PresetLab). `calibrate` prints, per genre preset,
// the LOUDNESS that gives the middle of its depth's band of crest-factor reduction (CAL lines for
// Tools/genre_presets.py); `check` measures every genre preset and writes the table. Exit code = presets that fail.
// The depth is the crest-factor reduction (input peak - RMS minus output peak - RMS), not the limiter's gain
// reduction: with DRIVE (relative to LIMIT) or OCTOBER (soft clip first) the peaks are taken before the limiter, whose
// gain reduction then reads about 0 however hard it is pushed (first calibration run; docs/GENRE_PRESETS.md).

#include <cmath>
#include <iostream>
#include <map>
#include <set>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloRedWallProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"

namespace
{
using hamlo::redwall::HamloRedWallProcessor;
using hamlo::redwall::Preset;
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
    const int length = std::min(all.getNumSamples(), static_cast<int>(20.0 * kFs));
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
    double avgGr = 0, outPeakDb = -120, rmsDelta = 0, crestCut = 0;
    // the preset's work: the larger of the crest-factor cut (clip / drive / limiting of peaks) and the limiter's average
    // gain reduction (a long release on sustained material turns it down evenly and leaves the crest factor)
    double work() const { return std::max(crestCut, avgGr); }
};

Measure play(int program, const juce::AudioBuffer<float>& x, const double* loudness)
{
    HamloRedWallProcessor p;
    p.setCurrentProgram(program);
    if (loudness != nullptr)
        p.setParameterValue("thresholdDb", *loudness);
    p.prepareToPlay(kFs, kBlock);
    auto y = x;
    juce::MidiBuffer midi;
    double gr = 0.0;
    int count = 0;
    for (int start = 0; start + kBlock <= y.getNumSamples(); start += kBlock)
    {
        const bool active = blockRms(x, start, kBlock) > std::pow(10.0, -50.0 / 20.0);
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, kBlock);
        p.processBlock(view, midi);
        if (active)
        {
            gr += std::max(0.0f, p.limiter().lastGainReductionDb());
            ++count;
        }
    }
    Measure m;
    m.avgGr = count > 0 ? gr / count : 0.0;
    float outPeak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < y.getNumSamples(); ++i)
        {
            m.finite = m.finite && std::isfinite(y.getSample(ch, i));
            outPeak = std::max(outPeak, std::abs(y.getSample(ch, i)));
        }
    m.outPeakDb = 20.0 * std::log10(std::max(1.0e-9f, outPeak));
    m.rmsDelta = 20.0 * std::log10(std::max(1.0e-12, activeRms(y)) / std::max(1.0e-12, activeRms(x)));
    const double inPeakDb = 20.0 * std::log10(std::max(1.0e-9f, x.getMagnitude(0, x.getNumSamples())));
    m.crestCut = (inPeakDb - 20.0 * std::log10(std::max(1.0e-12, activeRms(x)))) - (m.outPeakDb - 20.0 * std::log10(std::max(1.0e-12, activeRms(y))));
    return m;
}

Measure playAll(int program, const std::vector<juce::AudioBuffer<float>>& clips, const double* loudness)
{
    Measure sum;
    for (const auto& c : clips)
    {
        const auto m = play(program, c, loudness);
        sum.finite = sum.finite && m.finite;
        sum.avgGr += m.avgGr / static_cast<double>(clips.size());
        sum.rmsDelta += m.rmsDelta / static_cast<double>(clips.size());
        sum.crestCut += m.crestCut / static_cast<double>(clips.size());
        sum.outPeakDb = std::max(sum.outPeakDb, m.outPeakDb);
    }
    return sum;
}

// bands of the preset's work (dB, Measure::work)
const char* depthName(Preset::Depth d) { return d == Preset::Gentle ? "GENTLE" : d == Preset::Medium ? "MEDIUM" : "HEAVY"; }
double bandLo(Preset::Depth d) { return d == Preset::Gentle ? 0.5 : d == Preset::Medium ? 2.0 : 4.0; }
double bandHi(Preset::Depth d) { return d == Preset::Gentle ? 2.0 : d == Preset::Medium ? 4.0 : 8.0; }
double bandMid(Preset::Depth d) { return d == Preset::Gentle ? 1.2 : d == Preset::Medium ? 3.0 : 6.0; }
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 3)
    {
        std::cout << "usage: HamloRedWall_PresetLab calibrate|check <material dir> [report.md]\n";
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
    const auto& list = hamlo::redwall::factoryPresets();
    int bad = 0, judged = 0;
    juce::String table = "| # | Genre / group / preset | Material | Depth | Crest cut | Avg limiter GR | Out peak | RMS out - in | Pass |\n|---|---|---|---|---|---|---|---|---|\n";
    std::set<juce::String> seen;
    for (size_t k = 0; k < list.size(); ++k)
    {
        const auto& preset = list[k];
        if (juce::String(preset.genre).isEmpty())
            continue;
        const int program = static_cast<int>(k) + 1;
        const juce::String key = juce::String(preset.genre) + "/" + preset.group + "/" + preset.name;
        const auto it = clips.find(preset.material);
        if (it == clips.end())
        {
            std::cout << key << ": NO MATERIAL '" << preset.material << "'\n";
            ++bad;
            continue;
        }
        if (mode == "probe") // argv[3] = preset name; argv[4] = id=value,... overrides; one line at each LOUDNESS
        {
            if (argc < 5 || juce::String(preset.name) != juce::String(argv[3]))
                continue;
            for (double t = 0.0; t >= -30.0; t -= 10.0)
            {
                Measure sum;
                for (const auto& c : it->second)
                {
                    HamloRedWallProcessor q;
                    q.setCurrentProgram(program);
                    for (const auto& kv : juce::StringArray::fromTokens(argv[4], ",", ""))
                        q.setParameterValue(kv.upToFirstOccurrenceOf("=", false, false), kv.fromFirstOccurrenceOf("=", false, false).getDoubleValue());
                    q.setParameterValue("thresholdDb", t);
                    q.prepareToPlay(kFs, kBlock);
                    auto y = c;
                    juce::MidiBuffer midi;
                    for (int start = 0; start + kBlock <= y.getNumSamples(); start += kBlock)
                    {
                        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, kBlock);
                        q.processBlock(view, midi);
                    }
                    const double outPeak = 20.0 * std::log10(y.getMagnitude(0, y.getNumSamples()) + 1e-9), inPeak = 20.0 * std::log10(c.getMagnitude(0, c.getNumSamples()) + 1e-9);
                    const double cut = (inPeak - 20.0 * std::log10(activeRms(c))) - (outPeak - 20.0 * std::log10(activeRms(y)));
                    std::cout << "PROBE " << t << ": cut " << cut << " out peak " << outPeak << " rms " << 20.0 * std::log10(activeRms(y) / activeRms(c)) << std::endl;
                }
            }
            continue;
        }
        if (mode == "sweep") // crest cut, limiter GR, out peak and level change against LOUDNESS for argv[3] presets
        {
            if (argc < 4 || !juce::String(preset.name).containsIgnoreCase(argv[3]))
                continue;
            for (double t = 0.0; t >= -30.0; t -= 5.0)
            {
                const auto m = playAll(program, it->second, &t);
                std::cout << "SWEEP	" << key << "	" << t << "	cut " << m.crestCut << "	gr " << m.avgGr << "	peak " << m.outPeakDb << "	rms " << m.rmsDelta << std::endl;
            }
            continue;
        }
        if (mode == "calibrate")
        {
            // argv[3] (optional): only the presets whose name contains one of these ';'-separated parts
            if (argc > 3 && ![&] { for (const auto& part : juce::StringArray::fromTokens(argv[3], ";", "")) if (juce::String(preset.name).contains(part)) return true; return false; }())
                continue;
            double lo = -30.0, hi = 0.0; // the crest-factor reduction falls as LOUDNESS rises
            for (int i = 0; i < 10; ++i)
            {
                const double mid = 0.5 * (lo + hi);
                (playAll(program, it->second, &mid).work() > bandMid(preset.depth) ? lo : hi) = mid;
            }
            const double t = std::round(0.5 * (lo + hi) * 2.0) / 2.0;
            const auto m = playAll(program, it->second, &t);
            std::cout << "CAL\t" << key << "\t" << t << "\t" << m.work() << std::endl;
            continue;
        }
        ++judged;
        const auto m = playAll(program, it->second, nullptr);
        juce::String values = juce::String(preset.group) + ":"; // two presets of one group must differ (across genres too)
        for (const auto& spec : hamlo::redwall::parameterSpecs())
            values << spec.id << "=" << hamlo::redwall::presetValue(preset, spec.id, spec.defaultValue) << ";";
        const bool unique = seen.insert(values).second;
        const double ceiling = hamlo::redwall::presetValue(preset, "ceilingDb", -1.0);
        const bool peakOk = preset.mix < 1.0f || m.outPeakDb <= ceiling + 0.05;
        const bool ok = m.finite && m.work() >= bandLo(preset.depth) && m.work() <= bandHi(preset.depth) && peakOk && m.rmsDelta >= -6.0
                        && m.rmsDelta <= 12.0 && unique;
        bad += ok ? 0 : 1;
        table << "| " << program << " | " << key << " | " << preset.material << " | " << depthName(preset.depth) << " | " << juce::String(m.crestCut, 2) << " dB | " << juce::String(m.avgGr, 2) << " dB | "
              << juce::String(m.outPeakDb, 2) << " dBFS | " << juce::String(m.rmsDelta, 2) << " dB | " << (ok ? "yes" : "**no**") << (unique ? "" : " (duplicate)")
              << (m.finite ? "" : " (not finite)") << (peakOk ? "" : " (over LIMIT)") << " |\n";
        std::cout << program << " " << key << ": crest cut " << m.crestCut << " GR " << m.avgGr << " peak " << m.outPeakDb << " rms " << m.rmsDelta << (ok ? "" : "  FAIL") << std::endl;
    }
    if (mode == "check")
    {
        const auto summary = juce::String(judged) + " genre presets, " + juce::String(bad) + " outside their band / LIMIT";
        std::cout << summary << std::endl;
        if (argc > 3)
            juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]).replaceWithText("# HamloRedWall genre presets on their material\n\n" + summary + "\n\n" + table);
    }
    return bad;
}
