// HamloComp_PresetLab calibrate|check <material dir> [report.md]
// docs/COMPRESSOR_MODELS.md section 4. Each preset plays on its material (the owner's clips, the loudest 20 s
// of each, gained to an active RMS of -18 dBFS, mixes -14). `calibrate` prints, for the presets of the presets
// pass, the threshold that gives the middle of their depth's band; `check` measures every preset (P1 .. P5)
// and writes the table. Exit code = presets that fail.

#include <cmath>
#include <iostream>
#include <map>
#include <set>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloCompProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"

namespace
{
using hamlo::comp::HamloCompProcessor;
using hamlo::comp::Preset;
constexpr double kFs = 48000.0;
constexpr int kBlock = 512;
constexpr int kFirstNewPreset = 16; // presets 0..15 existed before the presets pass
// the model presets of the models pass whose thresholds were set by hand; recalibrated by the same method on the
// owner's word (2026-10-27). HPEQ's 7 presets (0..6) stay as they are.
const std::set<juce::String> kRecalibrated { "Drum Bus VCA Glue", "Drum Smash FET (parallel)", "Vocal FET Upfront", "Keys Opto Smooth", "Bass FET Grip" };

const std::map<juce::String, juce::StringArray> kMaterial {
    { "drums", { "drumbus1" } }, { "kick", { "kick" } }, { "snare", { "snare" } }, { "hats", { "hihat" } },
    { "vocal", { "vocal_lead1", "vocal_lead2", "vocal_lead3" } }, { "backing", { "vocal_back1", "vocal_back2", "vocal_back3" } },
    { "bass", { "bass1", "bass2" } }, { "keys", { "keys", "keys2" } }, { "guitar", { "guitar" } }, { "synth", { "synth" } },
    { "perc", { "perc" } }, { "samples", { "guitar", "synth", "keys2" } }, // the genre presets' groups
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
    return x;
}

struct Measure
{
    bool finite = true;
    double avgGr = 0, peakDelta = 0, rmsDelta = 0;
};

Measure play(int program, const juce::AudioBuffer<float>& x, const double* threshold)
{
    HamloCompProcessor p;
    p.setCurrentProgram(program);
    if (threshold != nullptr)
        p.setParameterValue("thresholdDb", *threshold);
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
            gr += p.gainReductionDb();
            ++count;
        }
    }
    Measure m;
    m.avgGr = count > 0 ? gr / count : 0.0;
    float inPeak = 0.0f, outPeak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < y.getNumSamples(); ++i)
        {
            m.finite = m.finite && std::isfinite(y.getSample(ch, i));
            inPeak = std::max(inPeak, std::abs(x.getSample(ch, i)));
            outPeak = std::max(outPeak, std::abs(y.getSample(ch, i)));
        }
    m.peakDelta = 20.0 * std::log10(std::max(1.0e-9f, outPeak) / std::max(1.0e-9f, inPeak));
    m.rmsDelta = 20.0 * std::log10(std::max(1.0e-12, activeRms(y)) / std::max(1.0e-12, activeRms(x)));
    return m;
}

Measure playAll(int program, const std::vector<juce::AudioBuffer<float>>& clips, const double* threshold)
{
    Measure sum;
    sum.peakDelta = -1.0e9;
    for (const auto& c : clips)
    {
        const auto m = play(program, c, threshold);
        sum.finite = sum.finite && m.finite;
        sum.avgGr += m.avgGr / static_cast<double>(clips.size());
        sum.rmsDelta += m.rmsDelta / static_cast<double>(clips.size());
        sum.peakDelta = std::max(sum.peakDelta, m.peakDelta);
    }
    return sum;
}

const char* depthName(Preset::Depth d) { return d == Preset::Gentle ? "GENTLE" : d == Preset::Medium ? "MEDIUM" : "HEAVY"; }
double bandLo(Preset::Depth d) { return d == Preset::Gentle ? 1.0 : d == Preset::Medium ? 3.0 : 6.0; }
double bandHi(Preset::Depth d) { return d == Preset::Gentle ? 3.0 : d == Preset::Medium ? 6.0 : 12.0; }
double bandMid(Preset::Depth d) { return d == Preset::Gentle ? 2.0 : d == Preset::Medium ? 4.5 : 9.0; }
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 3)
    {
        std::cout << "usage: HamloComp_PresetLab calibrate|check <material dir> [report.md]\n";
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

    const auto& list = hamlo::comp::factoryPresets();
    int bad = 0;
    juce::String table = "| # | Preset | Material | Depth | Avg GR | Peak out - in | RMS out - in | Pass |\n|---|---|---|---|---|---|---|---|\n";
    std::set<juce::String> seen;
    for (size_t k = 0; k < list.size(); ++k)
    {
        const auto& preset = list[k];
        const int program = static_cast<int>(k) + 1;
        const auto it = clips.find(preset.material);
        if (it == clips.end())
        {
            std::cout << preset.name << ": NO MATERIAL '" << preset.material << "'\n";
            ++bad;
            continue;
        }
        if (mode == "sweep") // the average GR against the threshold, for one preset (argv[3]: part of its name)
        {
            if (argc < 4 || !juce::String(preset.name).containsIgnoreCase(argv[3]))
                continue;
            for (double th = -60.0; th <= 0.0; th += 5.0)
                std::cout << "SWEEP	" << preset.name << "	" << th << "	" << playAll(program, it->second, &th).avgGr << std::endl;
            continue;
        }
        const bool genre = juce::String(preset.genre).isNotEmpty();
        if (mode == "calibrate-genre") // docs/GENRE_PRESETS.md: the genre presets only, keyed genre / group / name
        {
            if (!genre)
                continue;
            // argv[3] (optional): only the presets whose name contains one of these ';'-separated parts
            if (argc > 3 && ![&] { for (const auto& part : juce::StringArray::fromTokens(argv[3], ";", "")) if (juce::String(preset.name).contains(part)) return true; return false; }())
                continue;
            double lo = -60.0, hi = 0.0;
            for (int i = 0; i < 14; ++i)
            {
                const double mid = 0.5 * (lo + hi);
                (playAll(program, it->second, &mid).avgGr > bandMid(preset.depth) ? lo : hi) = mid;
            }
            const double t = std::round(0.5 * (lo + hi) * 2.0) / 2.0;
            const auto m = playAll(program, it->second, &t);
            std::cout << "CAL\t" << preset.genre << "/" << preset.group << "/" << preset.name << "\t" << t << "\t" << m.avgGr << std::endl;
            continue;
        }
        if (mode == "calibrate")
        {
            if (genre)
                continue;
            if (static_cast<int>(k) < kFirstNewPreset && kRecalibrated.count(preset.name) == 0)
                continue;
            double lo = -60.0, hi = 0.0; // average GR falls as the threshold rises
            for (int i = 0; i < 14; ++i)
            {
                const double mid = 0.5 * (lo + hi);
                (playAll(program, it->second, &mid).avgGr > bandMid(preset.depth) ? lo : hi) = mid;
            }
            const double t = std::round(0.5 * (lo + hi) * 2.0) / 2.0;
            const auto m = playAll(program, it->second, &t);
            std::cout << "CAL\t" << preset.name << "\t" << t << "\t" << m.avgGr << std::endl;
            continue;
        }
        const auto m = playAll(program, it->second, nullptr);
        juce::String key;
        for (const auto& spec : hamlo::comp::parameterSpecs())
            key << spec.id << "=" << hamlo::comp::presetValue(preset, spec.id, spec.presetBase) << ";";
        const bool unique = seen.insert(key).second;
        const bool ok = m.finite && m.avgGr >= bandLo(preset.depth) && m.avgGr <= bandHi(preset.depth) && m.peakDelta <= 6.0
                        && m.rmsDelta >= -12.0 && m.rmsDelta <= 6.0 && unique;
        const bool newPreset = static_cast<int>(k) >= kFirstNewPreset || kRecalibrated.count(preset.name) != 0;
        bad += ok || !newPreset ? 0 : 1; // the presets made before this pass are reported, not judged
        table << "| " << program << " | " << (genre ? juce::String(preset.genre) + " / " + preset.group + " / " : juce::String()) << preset.name << (newPreset ? "" : " (before)") << " | " << preset.material << " | " << depthName(preset.depth) << " | "
              << juce::String(m.avgGr, 2) << " dB | " << juce::String(m.peakDelta, 2) << " dB | " << juce::String(m.rmsDelta, 2) << " dB | "
              << (ok ? "yes" : (newPreset ? "**no**" : "no")) << (unique ? "" : " (duplicate)") << (m.finite ? "" : " (not finite)") << " |\n";
        std::cout << program << " " << preset.name << ": GR " << m.avgGr << " peak " << m.peakDelta << " rms " << m.rmsDelta << (ok ? "" : "  FAIL") << std::endl;
    }
    if (mode == "check")
    {
        const auto summary = juce::String(static_cast<int>(list.size()) - kFirstNewPreset + static_cast<int>(kRecalibrated.size()))
                           + " calibrated presets (35 of the presets pass + 5 recalibrated + 200 genre presets), " + juce::String(bad) + " outside P1 .. P5";
        std::cout << summary << std::endl;
        if (argc > 3)
            juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]).replaceWithText("# HamloComp presets on their material\n\n" + summary + "\n\n" + table);
    }
    return bad;
}
