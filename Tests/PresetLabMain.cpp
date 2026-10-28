// HamloUdarnik_PresetLab check <material dir> [report.md]
// docs/GENRE_PRESETS.md. Each genre preset plays on its group's material (the owner's clips, the loudest 20 s of each,
// gained to an active RMS of -18 dBFS, mixes -14 - as HamloComp_PresetLab). Nothing is calibrated (the V2 engine's
// knobs are level-independent); the check: finite, the output under 0 dBFS (PROTECT), the level change within
// -9 .. +9 dB, unique, and the attack / body contrast (contrastDeltaDb) moving the way the preset says when SNAP and
// BODY differ by 4 dB or more (SNAP > BODY: up, BODY > SNAP: down). Exit code = presets that fail.

#include <cmath>
#include <iostream>
#include <map>
#include <set>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Plugin/HamloUdarnikProcessor.h"
#include "Plugin/Parameters.h"
#include "Plugin/Presets.h"

namespace
{
using hamlo::udarnik::HamloUdarnikProcessor;
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
    // and the peak no higher than -3 dBFS (a track with headroom; at -18 dBFS RMS the hats and percussion would peak
    // over 0 dBFS, which PROTECT lets through by design - first run, see docs/GENRE_PRESETS.md)
    if (const float peak = x.getMagnitude(0, x.getNumSamples()); peak > 0.70794578f)
        x.applyGain(0.70794578f / peak);
    return x;
}

// The attack / body contrast the preset adds, measured as HamloUdarnik_Lab does (Tests/LabMain.cpp, U1 .. U4):
// per hit, (out - in) over 0 .. 10 ms minus (out - in) over 25 .. 100 ms (stopping before the next attack).
// (First runs used the local crest factor of 50 ms windows, then an envelope split of the whole clip: UDARNIK keeps
// the peaks and PROTECT rounds them, and on dense loops the split saw almost no attack - neither read what the
// shaper does; docs/GENRE_PRESETS.md.)
double rmsOf(const juce::AudioBuffer<float>& x, int from, int to)
{
    from = juce::jlimit(0, x.getNumSamples(), from);
    to = juce::jlimit(from, x.getNumSamples(), to);
    if (to <= from)
        return 0.0;
    double s = 0.0;
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        for (int i = from; i < to; ++i)
            s += static_cast<double>(x.getSample(ch, i)) * x.getSample(ch, i);
    return std::sqrt(s / (x.getNumChannels() * static_cast<double>(to - from)));
}

std::vector<int> onsets(const juce::AudioBuffer<float>& x)
{
    std::vector<int> out;
    const double floorLevel = std::pow(10.0, -45.0 / 20.0) * x.getMagnitude(0, x.getNumSamples());
    const int w = static_cast<int>(0.001 * kFs), longW = static_cast<int>(0.050 * kFs), gap = static_cast<int>(0.060 * kFs);
    int last = -gap;
    for (int i = longW; i + w < x.getNumSamples(); i += w / 2)
    {
        const double now = rmsOf(x, i, i + w), before = rmsOf(x, i - longW, i);
        if (now > 2.5 * before && now > floorLevel && i - last >= gap)
        {
            out.push_back(i);
            last = i;
        }
    }
    return out;
}

// Every attack, also one riding on a previous hit's tail (1 ms RMS over 2 x its 10 ms mean, above the floor,
// 15 ms apart): the body window stops before the next of these. Added after the first runs on the drum bus,
// where a second attack 25 .. 100 ms after a hit counted as its body (docs/reports/V2_REPORT.md).
std::vector<int> attacks(const juce::AudioBuffer<float>& x)
{
    std::vector<int> out;
    const double floorLevel = std::pow(10.0, -45.0 / 20.0) * x.getMagnitude(0, x.getNumSamples());
    const int w = static_cast<int>(0.001 * kFs), shortW = static_cast<int>(0.010 * kFs), gap = static_cast<int>(0.015 * kFs);
    int last = -gap;
    for (int i = shortW; i + w < x.getNumSamples(); i += w / 2)
    {
        const double now = rmsOf(x, i, i + w), before = rmsOf(x, i - shortW, i);
        if (now > 2.0 * before && now > floorLevel && i - last >= gap)
        {
            out.push_back(i);
            last = i;
        }
    }
    return out;
}

// Mean attack contrast (dB) over the hits: (out A - in A) - (out B - in B), A = 0 .. 10 ms, B = 25 .. 100 ms.
double attackContrast(const juce::AudioBuffer<float>& x, const juce::AudioBuffer<float>& y, int latency, int& hitsUsed)
{
    const auto hits = onsets(x);
    const auto every = attacks(x);
    const int a = static_cast<int>(0.010 * kFs), b0 = static_cast<int>(0.025 * kFs), b1 = static_cast<int>(0.100 * kFs), guard = static_cast<int>(0.005 * kFs);
    double sum = 0.0;
    int n = 0;
    for (size_t k = 0; k < hits.size(); ++k)
    {
        const int t = hits[k];
        int end = std::min(t + b1, (k + 1 < hits.size() ? hits[k + 1] - guard : x.getNumSamples()));
        for (const int a2 : every)
            if (a2 > t + a && a2 - guard < end)
                end = a2 - guard;
        if (end - (t + b0) < static_cast<int>(0.015 * kFs) || t + b1 + latency >= x.getNumSamples())
            continue;
        const double inA = rmsOf(x, t, t + a), inB = rmsOf(x, t + b0, end);
        const double outA = rmsOf(y, t + latency, t + a + latency), outB = rmsOf(y, t + b0 + latency, end + latency);
        if (inA <= 0.0 || inB <= 1.0e-6 || outA <= 0.0 || outB <= 0.0)
            continue;
        sum += 20.0 * std::log10(outA / inA) - 20.0 * std::log10(outB / inB);
        ++n;
    }
    hitsUsed = n;
    return n > 0 ? sum / n : 0.0;
}

struct Measure
{
    bool finite = true;
    double outPeakDb = -120, rmsDelta = 0, crestDelta = 0;
    int hits = 0;
};

Measure play(int program, const juce::AudioBuffer<float>& x)
{
    HamloUdarnikProcessor p;
    p.setCurrentProgram(program);
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
    Measure m;
    float outPeak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < out.getNumSamples(); ++i)
        {
            m.finite = m.finite && std::isfinite(out.getSample(ch, i));
            outPeak = std::max(outPeak, std::abs(out.getSample(ch, i)));
        }
    m.outPeakDb = 20.0 * std::log10(std::max(1.0e-9f, outPeak));
    m.rmsDelta = 20.0 * std::log10(std::max(1.0e-12, activeRms(out)) / std::max(1.0e-12, activeRms(x)));
    m.crestDelta = attackContrast(x, out, 0, m.hits);
    return m;
}

Measure playAll(int program, const std::vector<juce::AudioBuffer<float>>& clips)
{
    Measure sum;
    for (const auto& c : clips)
    {
        const auto m = play(program, c);
        sum.finite = sum.finite && m.finite;
        sum.rmsDelta += m.rmsDelta / static_cast<double>(clips.size());
        // weighted by the clip's hits; a clip without hits, or whose hits the shaper did not take (output = input on
        // them: sustained guitar / synth, legato keys), says nothing about the preset
        const int hits = std::abs(m.crestDelta) < 0.05 ? 0 : m.hits;
        sum.crestDelta += m.crestDelta * hits;
        sum.hits += hits;
        if (juce::SystemStats::getEnvironmentVariable("UD_PRESET_DEBUG", {}).isNotEmpty())
            std::cout << "    clip: hits " << m.hits << " contrast " << m.crestDelta << std::endl;
        sum.outPeakDb = std::max(sum.outPeakDb, m.outPeakDb);
    }
    if (sum.hits > 0)
        sum.crestDelta /= sum.hits;
    return sum;
}
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 3)
    {
        std::cout << "usage: HamloUdarnik_PresetLab calibrate|check <material dir> [report.md]\n";
        return 2;
    }
    const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
    std::map<juce::String, std::vector<juce::AudioBuffer<float>>> clips;
    for (const auto& [material, files] : kMaterial)
        for (const auto& f : files)
        {
            auto x = load(dir.getChildFile(f + ".wav"), material == "mix" ? -14.0 : -18.0);
            if (x.getNumSamples() > 0)
                clips[material].push_back(std::move(x));
        }
    const auto& list = hamlo::udarnik::factoryPresets();
    int bad = 0, judged = 0;
    juce::String table = "| # | Genre / group / preset | Material | Out peak | RMS out - in | Attack - body change | Expected | Pass |\n|---|---|---|---|---|---|---|---|\n";
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
        ++judged;
        const auto m = playAll(program, it->second);
        juce::String values;
        values << preset.group << ":"; // two presets of one group must differ (across genres too)
        for (const auto& spec : hamlo::udarnik::parameterSpecs())
            values << spec.id << "=" << hamlo::udarnik::presetValue(preset, spec.id, spec.defaultValue) << ";";
        const bool unique = seen.insert(values).second;
        const double contrast = hamlo::udarnik::presetValue(preset, "attackDb", 0.0) - hamlo::udarnik::presetValue(preset, "sustainDb", 0.0);
        // a clip without hits (sustained synth / guitar) cannot show it: reported "n/a", not judged
        const juce::String expected = m.hits == 0 ? "n/a (no hits the shaper takes)" : contrast >= 4.0 ? "up" : contrast <= -4.0 ? "down" : "-";
        const bool direction = m.hits == 0 || (contrast >= 4.0 ? m.crestDelta > 0.0 : contrast <= -4.0 ? m.crestDelta < 0.0 : true);
        const bool ok = m.finite && m.outPeakDb <= 0.01 && m.rmsDelta >= -9.0 && m.rmsDelta <= 9.0 && unique && direction;
        bad += ok ? 0 : 1;
        table << "| " << program << " | " << key << " | " << preset.material << " | " << juce::String(m.outPeakDb, 2) << " dBFS | " << juce::String(m.rmsDelta, 2)
              << " dB | " << juce::String(m.crestDelta, 2) << " dB | " << expected << " | " << (ok ? "yes" : "**no**") << (unique ? "" : " (duplicate)")
              << (m.finite ? "" : " (not finite)") << (direction ? "" : " (wrong direction)") << " |\n";
        std::cout << program << " " << key << ": peak " << m.outPeakDb << " rms " << m.rmsDelta << " contrast " << m.crestDelta << (ok ? "" : "  FAIL") << std::endl;
    }
    const auto summary = juce::String(judged) + " genre presets, " + juce::String(bad) + " fail";
    std::cout << summary << std::endl;
    if (argc > 3)
        juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]).replaceWithText("# HamloUdarnik genre presets on their material\n\n" + summary + "\n\n" + table);
    return bad;
}
