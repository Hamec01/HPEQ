// Presets pass 2 (owner, 2026-10-26): the factory library, rebuilt and measured. Every preset is designed
// here, then run on real material and calibrated:
//  - track presets (drums, bass, vocals, instruments, mix bus, showcase) are LEVEL-MATCHED: the Output gain
//    is set so the integrated loudness (BS.1770) out equals in on the category's material (the owner had to
//    raise a vocal preset by 8 dB by hand);
//  - mastering presets drive HamloRedWall (ceiling -1 dBTP, true peak) to a genre loudness target measured
//    on unmastered mixdowns (the owner's own masters sit at -10.6 .. -11.8 LUFS).
// Usage: HPEQ_Lab presets --material <dir of 48 kHz wav> --out <Assets/Presets/Factory> [--only <text>] [--keep <library>]
// The material is not in the repository (the owner's stems and mixes); the report lists what was used.

#include <cmath>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <iostream>
#include <map>
#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>

#include "Analysis/LoudnessMeter.h"
#include "Core/FxProjectState.h"
#include "Core/ModulePresets.h"
#include "Core/Revolution.h"
#include "Plugin/FxPluginProcessor.h"
#include "Plugin/Presets.h"

namespace hpeq::lab
{
namespace
{
using KV = std::initializer_list<std::pair<const char*, double>>;
constexpr double kRate = 48000.0;
constexpr double kPeakRoomDb = 6.0;

// ---------------------------------------------------------------- builders
FxSlotState mod(FxType t, KV kv, float mix = 1.0f)
{
    auto m = defaultModule(t);
    m.mix = mix;
    for (const auto& [k, v] : kv)
        m.parameters.set(k, v);
    return m;
}
FxSlotState comp(double thr, double ratio, double atk, double rel, int character = 1, int detector = 1, double knee = 6, float mix = 1.0f)
{
    return mod(FxType::Compressor, { { "thresholdDb", thr }, { "ratio", ratio }, { "attackMs", atk }, { "releaseMs", rel }, { "character", character },
                                     { "detector", detector }, { "kneeDb", knee }, { "autoMakeup", 1 }, { "makeupDb", 0 } }, mix);
}
FxSlotState wall(double thr, double ceiling = -1.0, int style = 0)
{
    return mod(FxType::Limiter, { { "thresholdDb", thr }, { "ceilingDb", ceiling }, { "style", style }, { "truePeak", 1 }, { "autoRelease", 1 },
                                  { "lookaheadMs", 2 }, { "link", 1 } });
}
FxSlotState mag(int model, double drive, float mix = 1.0f, KV extra = {})
{
    auto m = mod(FxType::Saturator, { { "model", model }, { "driveDb", drive }, { "autoGain", 1 }, { "oversampling", 1 } }, mix);
    for (const auto& [k, v] : extra)
        m.parameters.set(k, v);
    return m;
}
FxSlotState verb(int space, double decay, double predelay, float mix, double lowCut = 150, double highCut = 12000, KV extra = {})
{
    auto m = mod(FxType::Reverb, { { "space", space }, { "color", 2 }, { "decaySeconds", decay }, { "predelayMs", predelay }, { "lowCutHz", lowCut },
                                   { "highCutHz", highCut } }, mix);
    for (const auto& [k, v] : extra)
        m.parameters.set(k, v);
    return m;
}
FxSlotState echo(int character, int sync, double feedback, float mix, KV extra = {})
{
    auto m = mod(FxType::Delay, { { "character", character }, { "sync", sync }, { "feedback", feedback }, { "lowCutHz", 180 }, { "dampingHz", 7000 },
                                  { "link", 1 } }, mix);
    for (const auto& [k, v] : extra)
        m.parameters.set(k, v);
    return m;
}
FxSlotState punch(double attack, double sustain, double speed = 0.6, int style = 1, float mix = 1.0f)
{
    return mod(FxType::TransientShaper, { { "attackDb", attack }, { "sustainDb", sustain }, { "speed", speed }, { "style", style } }, mix);
}
FxSlotState gate(double thr, double range, double hold, double rel, int style = 0, double keyHp = 20, double keyLp = 20000)
{
    return mod(FxType::GateExpander, { { "thresholdDb", thr }, { "rangeDb", range }, { "holdMs", hold }, { "releaseMs", rel }, { "style", style },
                                       { "keyHpHz", keyHp }, { "keyLpHz", keyLp }, { "attackMs", 0.3 } });
}
FxSlotState wide(double width, double monoBass = 0, int style = 0, KV extra = {})
{
    auto m = mod(FxType::StereoProcessor, { { "width", width }, { "monoBassHz", monoBass }, { "style", style } });
    for (const auto& [k, v] : extra)
        m.parameters.set(k, v);
    return m;
}
FxSlotState fader(double gainDb, KV extra = {})
{
    auto m = mod(FxType::Gain, { { "gainDb", gainDb } });
    for (const auto& [k, v] : extra)
        m.parameters.set(k, v);
    return m;
}
FxSlotState revo(int character, int seed, float mix, KV extra = {})
{
    presets::ModulePreset p { "lab", mix, { { "character", static_cast<double>(character) }, { "revSeed", static_cast<double>(seed) } } };
    for (const auto& kv : extra)
        p.values.push_back(kv);
    return presets::makeModule(FxType::MonstaFx, p);
}

EqNodeState pt(EqShape s, double f, double g = 0, double q = 0.707, int slope = 12)
{
    EqNodeState n;
    n.shape = s;
    n.frequencyHz = f;
    n.gainDb = g;
    n.q = q;
    n.slopeDbPerOct = slope;
    return n;
}
EqNodeState lowCut(double f, int slope = 18) { return pt(EqShape::LowCut, f, 0, 0.707, slope); }
EqNodeState highCut(double f, int slope = 12) { return pt(EqShape::HighCut, f, 0, 0.707, slope); }
EqNodeState bell(double f, double g, double q = 1.0) { return pt(EqShape::Bell, f, g, q); }
EqNodeState lowShelf(double f, double g) { return pt(EqShape::LowShelf, f, g, 0.7); }
EqNodeState highShelf(double f, double g) { return pt(EqShape::HighShelf, f, g, 0.7); }
// A dynamic point: the gain moves by up to `range` as the band passes `thr` (DETECT BAND).
EqNodeState dyn(EqNodeState n, double thr, double range, double ratio = 3, double atk = 5, double rel = 120)
{
    n.dynamic = true;
    n.thresholdDb = thr;
    n.rangeDb = range;
    n.ratio = ratio;
    n.attackMs = atk;
    n.releaseMs = rel;
    return n;
}
// The DE-ESS point (DETECT VOICE: the "s" against the voice body).
EqNodeState deess(double f = 7000, double range = -9, double thr = -28, double q = 0.8)
{
    auto n = dyn(bell(f, 0, q), thr, range, 4, 0.5, 40);
    n.detect = 1;
    return n;
}
// A point with modules on its band.
EqNodeState withModules(EqNodeState n, std::vector<FxSlotState> chain)
{
    n.chain = std::move(chain);
    return n;
}
FxNodeState region(double lo, double hi, std::vector<FxSlotState> chain, float mix = 1.0f)
{
    FxNodeState r;
    r.lowHz = lo;
    r.highHz = hi;
    r.mix = mix;
    r.chain = std::move(chain);
    return r;
}

struct Layer
{
    juce::String name;
    std::vector<EqNodeState> eq;
    std::vector<FxNodeState> regions;
    std::vector<FxSlotState> mix;
    float gainDb = 0.0f, layerMix = 1.0f, width = 1.0f, pan = 0.0f;
};

struct Design
{
    std::vector<Layer> layers;
    LayerRouting routing = LayerRouting::Parallel;
    bool lenin = false, stalin = false;
    double leninMix = 1.0, stalinMix = 1.0;
};

FxProjectState buildProject(const juce::String& name, const Design& d)
{
    FxProjectState p;
    p.routing = d.routing;
    p.lenin = d.lenin;
    p.stalin = d.stalin;
    p.leninMix = d.leninMix;
    p.stalinMix = d.stalinMix;
    std::uint32_t id = 1;
    for (size_t i = 0; i < d.layers.size() && i < static_cast<size_t>(kNumLayers); ++i)
    {
        const auto& src = d.layers[i];
        auto& l = p.layers[i];
        l.name = i == 0 ? name : src.name;
        l.gainDb = src.gainDb;
        l.mix = src.layerMix;
        l.width = src.width;
        l.pan = src.pan;
        l.eqNodes = src.eq;
        l.fxNodes = src.regions;
        l.mixChain = src.mix;
        for (auto& n : l.eqNodes)
            n.id = id++;
        for (auto& n : l.fxNodes)
            n.id = id++;
        assignEngineSlots(l);
    }
    p.nextNodeId = id;
    sanitize(p);
    return p;
}

// ---------------------------------------------------------------- the library
enum class Cal { Match, Target };
struct Preset
{
    juce::String pack, category, group, name, intent;
    std::vector<juce::String> material;
    Design design;
    Cal cal = Cal::Match;
    double targetLufs = 0.0;
};

std::vector<Preset> library();

// ---------------------------------------------------------------- material and measurement
struct Audio
{
    juce::AudioBuffer<float> buffer;
};

std::map<juce::String, Audio> loadMaterial(const juce::File& dir)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::map<juce::String, Audio> out;
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.wav"))
        if (std::unique_ptr<juce::AudioFormatReader> r { formats.createReaderFor(f) })
        {
            Audio a;
            // the loudest 24 s of the clip (stems have long silences): enough for integrated loudness, short
            // enough for ~200 presets x several clips
            juce::AudioBuffer<float> all(2, static_cast<int>(r->lengthInSamples));
            r->read(&all, 0, all.getNumSamples(), 0, true, true);
            const int length = std::min(all.getNumSamples(), 24 * 48000), hop = 48000;
            int best = 0;
            double bestEnergy = -1.0;
            for (int start = 0; start + length <= all.getNumSamples(); start += hop)
            {
                double e = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = start; i < start + length; i += 16)
                        e += static_cast<double>(all.getSample(ch, i)) * all.getSample(ch, i);
                if (e > bestEnergy)
                {
                    bestEnergy = e;
                    best = start;
                }
            }
            a.buffer.setSize(2, length);
            for (int ch = 0; ch < 2; ++ch)
                a.buffer.copyFrom(ch, 0, all, ch, best, length);
            out[f.getFileNameWithoutExtension()] = std::move(a);
        }
    return out;
}

struct Measure
{
    double inLufs = 0, outLufs = 0, outTp = -100, inTp = -100;
};

Measure render(const FxProjectState& project, double outputDb, const juce::AudioBuffer<float>& x)
{
    auto plugin = std::make_unique<FxPluginProcessor>();
    plugin->setProject(project);
    if (auto* p = plugin->getParameters().getParameter("outputGain"))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(outputDb)));
    plugin->prepareToPlay(kRate, 512);
    LoudnessMeter in, out;
    in.prepare(kRate, true);
    out.prepare(kRate, true);
    juce::AudioBuffer<float> block(2, 512);
    juce::MidiBuffer midi;
    for (int s = 0; s + 512 <= x.getNumSamples(); s += 512)
    {
        for (int ch = 0; ch < 2; ++ch)
            block.copyFrom(ch, 0, x, ch, s, 512);
        in.process(block.getArrayOfReadPointers(), 2, 512);
        plugin->processBlock(block, midi);
        out.process(block.getArrayOfReadPointers(), 2, 512);
    }
    return { in.integratedLufs(), out.integratedLufs(), out.truePeakDbtp(), in.truePeakDbtp() };
}

FxSlotState* findLimiter(FxProjectState& p)
{
    for (auto& l : p.layers)
        for (auto it = l.mixChain.rbegin(); it != l.mixChain.rend(); ++it)
            if (it->type == FxType::Limiter)
                return &*it;
    return nullptr;
}

juce::String slug(const juce::String& s)
{
    juce::String out;
    for (auto c : s.toLowerCase())
        out << (juce::CharacterFunctions::isLetterOrDigit(c) ? juce::String::charToString(c) : juce::String("-"));
    while (out.contains("--"))
        out = out.replace("--", "-");
    return out.trimCharactersAtStart("-").trimCharactersAtEnd("-");
}

juce::String folderOf(const juce::String& s) { return s.replace(" & ", "-and-").replace(", ", "-").replace(" / ", "-").replace(" ", ""); }
} // namespace

int runPresetLibrary(const juce::File& materialDir, const juce::File& outDir, const juce::String& only, const juce::File& keepDir)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto material = loadMaterial(materialDir);
    std::cout << "material: " << material.size() << " files\n";
    auto presets = library();
    if (only.isNotEmpty()) // a check of one part: e.g. --only Mastering
        presets.erase(std::remove_if(presets.begin(), presets.end(), [&](const Preset& p)
                                     { return !(p.category + " " + p.name).containsIgnoreCase(only); }), presets.end());
    // --keep <library>: a preset already in that library keeps its file (state and Output gain, byte for byte)
    // and its measured calibration; only the presets new to it are calibrated (on this material). Pass 3: the
    // first material is lost, so the 197 presets calibrated on it stay as they were.
    struct Kept
    {
        juce::MemoryBlock state;
        juce::var entry;
    };
    std::map<juce::String, Kept> kept;
    const juce::String packNames[] { "Drums & Bass", "Vocals, Mastering & More", "Hamlo Showcase" }; // the library's packs (named below)
    if (keepDir != juce::File())
        for (const auto& pack : packNames)
        {
            const auto folder = keepDir.getChildFile(folderOf(pack));
            const auto manifest = juce::JSON::parse(folder.getChildFile("manifest.json"));
            if (const auto* list = manifest["presets"].getArray())
                for (const auto& e : *list)
                {
                    Kept k;
                    k.entry = e;
                    folder.getChildFile(e["path"].toString()).loadFileAsData(k.state);
                    jassert(k.state.getSize() > 0);
                    kept[pack + "/" + e["name"].toString()] = std::move(k);
                }
        }
    std::cout << "kept from the library: " << kept.size() << "\n";
    for (const auto& pack : packNames)
        outDir.getChildFile(folderOf(pack)).deleteRecursively();

    std::map<juce::String, juce::Array<juce::var>> manifests;
    std::map<juce::String, int> numbers;
    juce::String report = "| # | Pack / category | Preset | Material | In LUFS | Out LUFS | Output dB | TP dBTP |\n|---|---|---|---|---|---|---|---|\n";
    int index = 0, failures = 0;
    // calibrate on all cores (each preset is independent), then write in library order
    struct Result
    {
        FxProjectState project;
        double outputDb = 0.0;
        bool peakLimited = false, ok = false, valid = false;
        Measure m;
    };
    std::vector<Result> results(presets.size());
    std::atomic<size_t> next { 0 };
    std::mutex printLock;
    const auto calibrate = [&](size_t k)
    {
        const auto& preset = presets[k];
        auto& r = results[k];
        if (kept.count(preset.pack + "/" + preset.name) != 0)
        {
            r.valid = r.ok = true;
            return;
        }
        r.project = buildProject(preset.name, preset.design);
        auto& project = r.project;
        std::vector<const juce::AudioBuffer<float>*> clips;
        for (const auto& name : preset.material)
            if (auto it = material.find(name); it != material.end())
                clips.push_back(&it->second.buffer);
        if (clips.empty())
            return;
        const auto measureAll = [&](double outputDb)
        {
            Measure sum;
            for (const auto* c : clips)
            {
                const auto one = render(project, outputDb, *c);
                sum.inLufs += one.inLufs / static_cast<double>(clips.size());
                sum.outLufs += one.outLufs / static_cast<double>(clips.size());
                sum.outTp = std::max(sum.outTp, one.outTp);
                sum.inTp = std::max(sum.inTp, one.inTp);
            }
            return sum;
        };
        auto& m = r.m;
        if (preset.cal == Cal::Target)
        {
            auto* limiter = findLimiter(project);
            jassert(limiter != nullptr);
            // the RedWall threshold drives the loudness (searched on the mean of the clips); the tubes
            // run after the layers, so with LENIN / STALIN on the ceiling comes down until the true peak at
            // the plug-in's output is -1 dBTP
            double ceiling = -1.0;
            for (int round = 0; round < 4; ++round)
            {
                limiter->parameters.set("ceilingDb", ceiling);
                // bisection on [-24, 0] dB (louder as the threshold goes down); a style that cannot reach
                // the target hands over to the next (measured: GLASNOST tops out near -9 LUFS on these mixes,
                // STAKHANOV and OCTOBER go further)
                const int designStyle = static_cast<int>(limiter->parameters["style"]);
                for (int style : { designStyle, 1, 2 })
                {
                    if (style < designStyle)
                        continue;
                    limiter->parameters.set("style", style);
                    double lo = -24.0, hi = 0.0;
                    for (int i = 0; i < 9; ++i)
                    {
                        const double mid = 0.5 * (lo + hi);
                        limiter->parameters.set("thresholdDb", mid);
                        (measureAll(0).outLufs < preset.targetLufs ? hi : lo) = mid;
                    }
                    limiter->parameters.set("thresholdDb", std::round(0.5 * (lo + hi) * 10.0) / 10.0);
                    if (hi > -23.9)
                        break;
                }
                m = measureAll(0);
                if (m.outTp <= -0.95)
                    break;
                ceiling = std::round((ceiling - (m.outTp + 1.0) - 0.1) * 10.0) / 10.0;
            }
            r.ok = std::abs(m.outLufs - preset.targetLufs) <= 0.5 && m.outTp <= -0.7;
        }
        else
        {
            m = measureAll(0);
            // level-matched, but the peaks may not rise more than 6 dB over the input's (a transient
            // shaper on a hot kick would otherwise buy loudness with +10 dBTP peaks)
            const double match = m.inLufs - m.outLufs, peakRoom = m.inTp + kPeakRoomDb - m.outTp;
            r.peakLimited = peakRoom < match;
            r.outputDb = juce::jlimit(-24.0, 24.0, std::floor(std::min(match, peakRoom) * 10.0) / 10.0);
            m = measureAll(r.outputDb);
            r.ok = r.peakLimited ? m.outTp <= m.inTp + kPeakRoomDb + 0.2 : std::abs(m.outLufs - m.inLufs) <= 0.5;
        }
        r.valid = true;
        const std::scoped_lock lock(printLock);
        juce::String wallText;
        if (auto* w = findLimiter(r.project); w != nullptr && preset.cal == Cal::Target)
            wallText << " wall " << static_cast<double>(w->parameters["thresholdDb"]) << " / " << static_cast<double>(w->parameters["ceilingDb"])
                     << " style " << static_cast<int>(w->parameters["style"]);
        std::cout << preset.name << ": in " << m.inLufs << " out " << m.outLufs << " output " << r.outputDb << " tp " << m.outTp << wallText
                  << (r.peakLimited ? " peak-limited" : "") << (r.ok ? "" : "  FAIL") << std::endl;
    };
    std::vector<std::thread> workers;
    for (unsigned t = 0; t < std::max(1u, std::thread::hardware_concurrency()); ++t)
        workers.emplace_back([&]
        {
            for (size_t k = next++; k < presets.size(); k = next++)
                calibrate(k);
        });
    for (auto& w : workers)
        w.join();

    for (size_t k = 0; k < presets.size(); ++k)
    {
        const auto& preset = presets[k];
        auto& r = results[k];
        if (!r.valid)
        {
            std::cout << "NO MATERIAL for " << preset.name << "\n";
            ++failures;
            continue;
        }
        const auto& project = r.project;
        const auto keptIt = kept.find(preset.pack + "/" + preset.name);
        const bool isKept = keptIt != kept.end();
        double outputDb = r.outputDb;
        bool peakLimited = r.peakLimited;
        const bool ok = r.ok;
        auto m = r.m;
        failures += ok ? 0 : 1;

        // write the state, as the plug-in saves it (a kept preset: its file as it was)
        juce::MemoryBlock state;
        if (isKept)
        {
            const auto& e = keptIt->second.entry;
            state = keptIt->second.state;
            outputDb = static_cast<double>(e["outputGainDb"]);
            peakLimited = e["calibration"].toString().startsWith("peak");
            m.inLufs = e["measuredInLufs"];
            m.outLufs = e["measuredOutLufs"];
            m.outTp = e["measuredTruePeakDbtp"];
        }
        else
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            plugin->setProject(project);
            if (auto* p = plugin->getParameters().getParameter("outputGain"))
                p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(outputDb)));
            plugin->setPresetName(preset.name);
            plugin->getStateInformation(state);
        }
        const auto number = ++numbers[preset.pack];
        const auto fileName = juce::String(number).paddedLeft('0', 3) + "-" + slug(preset.name) + ".hpeq";
        auto folder = outDir.getChildFile(folderOf(preset.pack)).getChildFile(folderOf(preset.category));
        if (preset.group.isNotEmpty())
            folder = folder.getChildFile(folderOf(preset.group));
        folder.createDirectory();
        folder.getChildFile(fileName).replaceWithData(state.getData(), state.getSize());

        juce::String materialText;
        for (const auto& s : preset.material)
            materialText << (materialText.isEmpty() ? "" : ", ") << s;
        if (!kept.empty() && !isKept)
            materialText << " (set 2)";
        auto* entry = new juce::DynamicObject();
        entry->setProperty("number", number);
        entry->setProperty("name", preset.name);
        entry->setProperty("category", preset.category);
        if (preset.group.isNotEmpty())
            entry->setProperty("group", preset.group);
        entry->setProperty("intent", preset.intent);
        entry->setProperty("path", folder.getRelativePathFrom(outDir.getChildFile(folderOf(preset.pack))).replace("\\", "/") + "/" + fileName);
        entry->setProperty("calibration", isKept ? keptIt->second.entry["calibration"].toString()
                                                 : preset.cal == Cal::Target ? "loudness target" : (peakLimited ? "peak-limited (true peak <= input + 6 dB)" : "level-matched"));
        if (!kept.empty())
            entry->setProperty("materialSet", isKept ? 1 : 2);
        entry->setProperty("outputGainDb", outputDb);
        entry->setProperty("measuredInLufs", std::round(m.inLufs * 10.0) / 10.0);
        entry->setProperty("measuredOutLufs", std::round(m.outLufs * 10.0) / 10.0);
        entry->setProperty("measuredTruePeakDbtp", std::round(m.outTp * 10.0) / 10.0);
        manifests[preset.pack].add(juce::var(entry));

        report << "| " << ++index << " | " << preset.pack << " / " << preset.category << (preset.group.isNotEmpty() ? " / " + preset.group : juce::String()) << " | "
               << preset.name << " | " << materialText << " | " << juce::String(m.inLufs, 1) << " | " << juce::String(m.outLufs, 1) << " | "
               << juce::String(outputDb, 1) << (peakLimited ? " (peak)" : "") << " | " << juce::String(m.outTp, 1) << (ok ? "" : " **!**") << " |\n";
    }

    auto* root = new juce::DynamicObject();
    root->setProperty("format", "HPEQ-Factory-Library");
    root->setProperty("formatVersion", 2);
    root->setProperty("pluginStateSchema", kStateSchemaVersion);
    root->setProperty("presetCount", index);
    juce::Array<juce::var> packs;
    for (auto& [pack, entries] : manifests)
    {
        auto* m = new juce::DynamicObject();
        m->setProperty("format", "HPEQ-JUCE-State");
        m->setProperty("pack", pack);
        m->setProperty("presetCount", entries.size());
        m->setProperty("presets", entries);
        outDir.getChildFile(folderOf(pack)).getChildFile("manifest.json").replaceWithText(juce::JSON::toString(juce::var(m)));
        auto* ref = new juce::DynamicObject();
        ref->setProperty("name", pack);
        ref->setProperty("presetCount", entries.size());
        ref->setProperty("manifest", folderOf(pack) + "/manifest.json");
        packs.add(juce::var(ref));
    }
    root->setProperty("packs", packs);
    outDir.getChildFile("manifest.json").replaceWithText(juce::JSON::toString(juce::var(root)));
    outDir.getChildFile("CALIBRATION.md").replaceWithText("# Factory library calibration\n\nGenerated by `HPEQ_Lab presets` (Tests/Tools/LabPresets.cpp). "
                                                          "Track presets: Output gain so the integrated loudness out = in on the material, unless that would put the true peak more than 6 dB over the input's (then the peak sets it: marked (peak)). Mastering: "
                                                          "HamloRedWall threshold so the mean integrated loudness hits the target, ceiling -1 dBTP.\n\n"
                                                          + (kept.empty() ? juce::String()
                                                                          : juce::String("Two material sets: presets kept from the library (--keep) carry their calibration on material set 1 (the owner's "
                                                                                         "stems and mixes of pass 2, since lost); presets marked (set 2) were calibrated on material set 2 (pass 3, see "
                                                                                         "docs/fx-addon/reports/PRESET_PASS_REPORT.md for what it is).\n\n"))
                                                          + report);
    std::cout << index << " presets, " << failures << " outside tolerance" << std::endl;
    return failures == 0 ? 0 : 1;
}

namespace
{
// ---------------------------------------------------------------- designs
const juce::String DB = "Drums & Bass", VM = "Vocals, Mastering & More", SH = "Hamlo Showcase";
const std::vector<juce::String> kVox { "vocal_lead1", "vocal_lead2", "vocal_lead3" }, kBack { "vocal_back1", "vocal_back2", "vocal_back3" },
    kMixdowns { "mixdown_Ardnold", "mixdown_boom_bap_3", "mixdown_sad_trap" };

Preset track(const juce::String& pack, const juce::String& cat, const juce::String& name, const juce::String& intent, std::vector<juce::String> material, Design d)
{
    Preset p;
    p.pack = pack;
    p.category = cat;
    p.name = name;
    p.intent = intent;
    p.material = std::move(material);
    p.design = std::move(d);
    return p;
}
Design one(std::vector<EqNodeState> eq, std::vector<FxSlotState> mix, std::vector<FxNodeState> regions = {})
{
    Design d;
    d.layers.push_back({ {}, std::move(eq), std::move(regions), std::move(mix) });
    return d;
}
Design tubes(Design d, bool lenin, bool stalin, double lm = 1.0, double sm = 1.0)
{
    d.lenin = lenin;
    d.stalin = stalin;
    d.leninMix = lm;
    d.stalinMix = sm;
    return d;
}
Design plus(Design d, Layer extra)
{
    d.layers.push_back(std::move(extra));
    return d;
}

void drums(std::vector<Preset>& v)
{
    const std::vector<juce::String> K { "kick" }, S { "snare" }, H { "hihat" }, P { "perc" }, B { "bass1", "bass2" }, D { "drumbus1" };
    // Kick
    v.push_back(track(DB, "Kick", "Club Foundation", "Round four-on-the-floor kick: sub kept, boxiness out, a click on top.", K,
                      one({ lowCut(28, 24), lowShelf(55, 2), bell(330, -3, 1.4), bell(3800, 2, 1.2) }, { punch(3, -1, 0.6), comp(-22, 3, 12, 90, 1, 0) })));
    v.push_back(track(DB, "Kick", "Hip-Hop Knock", "Boom-bap kick with knock at 100 Hz and dusty tape.", K,
                      one({ lowCut(32, 18), bell(100, 3, 1.1), bell(420, -3, 1.6), highCut(9000) }, { punch(5, -2, 0.55), mag(1, 6, 0.6) })));
    v.push_back(track(DB, "Kick", "Trap Kick Click", "Short trap kick: tight tail, beater click for small speakers.", K,
                      one({ lowCut(30, 24), bell(60, 2, 1.0), bell(250, -4, 1.3), bell(5000, 3, 1.4) }, { punch(7, -6, 0.7), wall(-2, -0.5, 1) })));
    v.push_back(track(DB, "Kick", "Rock Kick Punch", "Live rock kick: low punch, mud cut, the beater forward.", K,
                      one({ lowCut(35, 18), bell(75, 2.5, 1.2), bell(380, -4, 1.5), bell(2800, 3, 1.3) }, { gate(-32, 30, 40, 120, 0, 40, 4000), comp(-20, 4, 8, 80, 1, 0) })));
    v.push_back(track(DB, "Kick", "Sub Thump", "More weight under the kick: low shelf with a dynamic point that stays tidy.", K,
                      one({ lowCut(24, 24), dyn(lowShelf(60, 4), -18, -3, 3, 10, 120), bell(500, -2, 1.2) }, { mag(3, 6, 0.5), comp(-18, 2, 20, 120, 2, 1) })));
    v.push_back(track(DB, "Kick", "Lo-Fi Dusty Kick", "Muffled, saturated kick for lo-fi beats (Vinyl 33).", K,
                      one({ lowCut(40, 12), highCut(6000, 18), bell(120, 2, 1.0) }, { mag(5, 8, 0.8, { { "crackle", 0.15 }, { "speed", 0 } }), comp(-24, 4, 10, 100) })));
    v.push_back(track(DB, "Kick", "Techno Rumble Kick", "Kick into a reverb rumble, the low end of the reverb kept mono.", K,
                      one({ lowCut(30, 24), bell(50, 2, 1.0), bell(300, -3, 1.4) }, { punch(4, 0, 0.6), verb(3, 2.5, 0, 0.25, 30, 400), mag(2, 10, 0.4), wide(1.0, 200) })));
    v.push_back(track(DB, "Kick", "Gated Kick Tight", "Kick from a live kit with the bleed gated out.", K,
                      one({ lowCut(35, 18), bell(400, -3, 1.4), bell(3000, 2, 1.2) }, { gate(-30, 40, 30, 80, 0, 40, 1500), punch(3, -3, 0.6) })));
    // Snare
    v.push_back(track(DB, "Snare", "Snare Crack", "Crack forward, ring tamed by a dynamic point.", S,
                      one({ lowCut(90, 18), bell(200, 2, 1.2), dyn(bell(900, 0, 4), -26, -5, 3, 2, 80), bell(5000, 3, 1.0) }, { punch(6, -1, 0.7), comp(-20, 4, 5, 80, 1, 0) })));
    v.push_back(track(DB, "Snare", "Fat Snare Body", "More body at 180 Hz, softer top, tape glue.", S,
                      one({ lowCut(70, 18), bell(180, 3, 1.0), bell(6000, -1, 1.0) }, { comp(-24, 3, 15, 120, 2, 1), mag(1, 6, 0.5) })));
    v.push_back(track(DB, "Snare", "80s Snare Plate", "Big gated-plate snare (80s): plate, gate on the tail.", S,
                      one({ lowCut(100, 18), bell(250, 2, 1.0), bell(4500, 2, 1.0) }, { punch(4, 0, 0.6), verb(4, 1.8, 0, 0.35, 200, 9000, { { "color", 1 } }), gate(-30, 25, 120, 60, 0) })));
    v.push_back(track(DB, "Snare", "Trap Clap Snap", "Tight clap / snare for trap, short room.", S,
                      one({ lowCut(150, 24), bell(1000, -2, 1.5), highShelf(8000, 2) }, { punch(5, -4, 0.75), verb(1, 0.5, 5, 0.18, 300) })));
    v.push_back(track(DB, "Snare", "Boom Bap Snare", "Dusty boom-bap snare, Iron saturation and a chamber.", S,
                      one({ lowCut(110, 18), bell(220, 2, 1.0), highCut(10000) }, { mag(3, 8, 0.6), comp(-22, 4, 10, 100, 1), verb(2, 0.9, 10, 0.15, 250) })));
    v.push_back(track(DB, "Snare", "Rimshot Bright", "Rimshot / side-stick that cuts: upper mids and a short ambience.", S,
                      one({ lowCut(200, 18), bell(2500, 3, 1.2), highShelf(9000, 2) }, { punch(4, -2, 0.7, 0), verb(6, 0.4, 0, 0.15, 400) })));
    v.push_back(track(DB, "Snare", "Snare Gate Live", "Live snare, hi-hat bleed gated, key filtered to the snare.", S,
                      one({ lowCut(90, 18), bell(400, -2, 1.4), bell(5000, 2, 1.0) }, { gate(-28, 30, 40, 120, 0, 150, 8000), comp(-20, 3, 8, 90) })));
    // Hi hat
    v.push_back(track(DB, "HiHat", "Hat Silk", "Smooth hats: harshness at 6 kHz held by a dynamic point, air on top.", H,
                      one({ lowCut(300, 24), dyn(bell(6000, 0, 2), -30, -4, 3, 1, 60), highShelf(12000, 2) }, { comp(-26, 2, 5, 60, 0, 1) })));
    v.push_back(track(DB, "HiHat", "Trap Hat Tick", "Short, ticking trap hats.", H,
                      one({ lowCut(500, 24), bell(9000, 2, 1.0) }, { punch(4, -8, 0.8), wide(1.2) })));
    v.push_back(track(DB, "HiHat", "Tape Hats", "Tape-softened hats for lo-fi and boom bap.", H,
                      one({ lowCut(350, 18), highCut(12000, 12) }, { mag(1, 6, 0.7, { { "speed", 1 } }) })));
    v.push_back(track(DB, "HiHat", "Wide Shimmer Hats", "Hats wider and brighter, mono-safe.", H,
                      one({ lowCut(400, 24), highShelf(10000, 3) }, { wide(1.4, 0, 1, { { "stereoize", 0.4 } }) })));
    v.push_back(track(DB, "HiHat", "Hats Pulled Back", "Hats sitting behind the groove: softer attack, darker.", H,
                      one({ lowCut(350, 18), highShelf(8000, -3) }, { punch(-4, 0, 0.5, 0), comp(-28, 3, 10, 80) })));
    v.push_back(track(DB, "HiHat", "Hat Groove Glitch", "Revolution on the hats: musical stutters on the 1/16 grid.", H,
                      one({ lowCut(350, 18) }, { revo(1, 9, 0.6, { { "motionOn", 0 } }) })));
    // Percussion
    v.push_back(track(DB, "Percussion", "Perc Snap", "Percussion with snap and a little room.", P,
                      one({ lowCut(120, 18), bell(3000, 2, 1.0) }, { punch(4, -2, 0.7), verb(1, 0.6, 5, 0.12, 250) })));
    v.push_back(track(DB, "Percussion", "Shaker Space", "Shakers and tambourine in a wide ambience.", P,
                      one({ lowCut(300, 18), highShelf(9000, 1.5) }, { verb(6, 0.8, 10, 0.2, 400), wide(1.3) })));
    v.push_back(track(DB, "Percussion", "Conga Warm", "Hand percussion warmer: body up, tube saturation.", P,
                      one({ lowCut(60, 18), bell(220, 2, 1.0), bell(4000, -1, 1.0) }, { mag(2, 6, 0.5), comp(-24, 3, 15, 120, 2) })));
    v.push_back(track(DB, "Percussion", "Perc Ping-Pong", "Percussion echoes bouncing L-R on 1/8.", P,
                      one({ lowCut(150, 18) }, { echo(1, 5, 0.35, 0.22, { { "pingPong", 1 } }) })));
    v.push_back(track(DB, "Percussion", "Perc Dub Space", "Dub-style tape echo and spring-like chamber on percussion.", P,
                      one({ lowCut(150, 18), highCut(8000) }, { echo(2, 9, 0.5, 0.25, { { "duck", 0.4 } }), verb(2, 1.2, 20, 0.12, 300) })));
    v.push_back(track(DB, "Percussion", "Perc Tight Gate", "Tight percussion, room tails gated.", P,
                      one({ lowCut(100, 18) }, { gate(-34, 25, 15, 60), punch(3, -4, 0.7) })));
    // Drum bus
    v.push_back(track(DB, "DrumBus", "VCA Glue", "Gentle glue: 2:1 slow attack, auto release feel.", D,
                      one({ lowCut(25, 18) }, { comp(-18, 2, 30, 150, 2, 1, 8) })));
    v.push_back(track(DB, "DrumBus", "Punch Bus", "Drums hit harder: attack through, sustain held.", D,
                      one({ lowCut(28, 18), bell(400, -2, 1.0), highShelf(10000, 1.5) }, { punch(4, -2, 0.6), comp(-20, 4, 15, 90, 1, 0) })));
    v.push_back(track(DB, "DrumBus", "Tape Bus (Lenin)", "Tape and BRING LENIN BACK on the drums: warm, rounded peaks.", D,
                      tubes(one({ lowCut(25, 18) }, { mag(1, 6, 0.8), comp(-20, 2, 30, 150, 2) }), true, false, 0.8)));
    v.push_back(track(DB, "DrumBus", "Boom Bap Crunch", "Boom-bap bus: Iron saturation, dark top, room.", D,
                      one({ lowCut(30, 18), bell(100, 2, 1.0), highCut(11000) }, { mag(3, 12, 0.6), comp(-22, 4, 10, 100, 1, 0), verb(1, 0.6, 5, 0.1, 200) })));
    v.push_back(track(DB, "DrumBus", "Multiband Drum Control", "Lows and highs held separately: two regions with their own compressors.", D,
                      one({ lowCut(25, 18) }, { comp(-16, 1.5, 30, 150, 2) },
                          { region(20, 150, { comp(-22, 3, 20, 120, 1, 1) }), region(5000, 20000, { comp(-26, 3, 2, 60, 0, 1) }) })));
    v.push_back(track(DB, "DrumBus", "Drum Room Air", "A big room under the drums, ducked so the hits stay dry.", D,
                      one({ lowCut(30, 18) }, { verb(2, 1.4, 10, 0.25, 250, 10000, { { "duck", 0.5 } }) })));
    // Bass
    v.push_back(track(DB, "Bass", "Sub Mono Foundation", "Sub centred: mono below 120 Hz, mud cut.", B,
                      one({ lowCut(24, 24), lowShelf(55, 1.5), bell(220, -2, 1.1) }, { comp(-24, 3, 20, 150, 1, 1), wide(1.0, 120) })));
    v.push_back(track(DB, "Bass", "808 Translation", "808 audible on small speakers: harmonics from Tube, sub kept.", B,
                      one({ lowCut(25, 24), bell(55, 1.5, 1.0) }, { mag(2, 10, 0.5, { { "harmonics", 0.7 } }), comp(-20, 3, 15, 120), wide(1.0, 120) })));
    v.push_back(track(DB, "Bass", "Growl Multiband", "Sub kept clean; growl saturated only above 300 Hz (a region).", B,
                      one({ lowCut(25, 24) }, { wide(1.0, 120) }, { region(300, 4000, { mag(4, 16, 0.7), comp(-20, 4, 5, 80) }) })));
    v.push_back(track(DB, "Bass", "Finger Bass Round", "Live bass rounder and even.", B,
                      one({ lowCut(35, 18), bell(90, 1.5, 1.0), bell(250, -2, 1.2), bell(800, 1.5, 1.0) }, { comp(-26, 4, 20, 140, 2, 1) })));
    v.push_back(track(DB, "Bass", "Pick Bass Attack", "Pick attack forward, low end even.", B,
                      one({ lowCut(35, 18), bell(1500, 2.5, 1.0) }, { punch(4, 0, 0.6, 1), comp(-22, 4, 8, 100) })));
    v.push_back(track(DB, "Bass", "Bass Tape Warmth", "Tape 9.5 cm/s: soft top, warm saturation.", B,
                      one({ lowCut(30, 18), highCut(7000, 12) }, { mag(1, 8, 0.8, { { "speed", 1 } }), comp(-24, 3, 25, 150, 2) })));
    v.push_back(track(DB, "Bass", "Bass Duck Ready", "Even, controlled bass with a dynamic low shelf (tames booming notes).", B,
                      one({ lowCut(28, 24), dyn(lowShelf(90, 0), -16, -5, 3, 15, 150) }, { comp(-22, 3, 15, 120) })));
}

Preset leadVox(const juce::String& name, const juce::String& intent, Design d)
{
    return track(VM, "LeadVocal", name, intent, kVox, std::move(d));
}

void vocals(std::vector<Preset>& v)
{
    // Every lead / backing preset carries the DE-ESS point (DETECT VOICE).
    v.push_back(leadVox("Pop Lead Polish", "Modern pop lead: mud cut, presence, air, de-esser, plate.",
                        one({ lowCut(90), bell(300, -2, 1.2), bell(3200, 2, 1.0), highShelf(12000, 3), deess() },
                            { comp(-26, 3, 5, 100, 0, 1), comp(-18, 2, 20, 150, 2, 1), verb(4, 1.4, 40, 0.12, 250, 9000, { { "duck", 0.4 } }) })));
    v.push_back(leadVox("Rap Vocal Upfront", "Rap vocal dry and loud in front: fast compression, presence, de-esser.",
                        one({ lowCut(100), bell(250, -2, 1.2), bell(4000, 2.5, 1.0), highShelf(10000, 1.5), deess(6500) },
                            { comp(-28, 4, 2, 80, 1, 0), comp(-20, 2, 15, 120, 2, 1), mag(2, 4, 0.3) })));
    v.push_back(leadVox("Ballad Open Vocal", "Open ballad vocal: gentle compression, air, hall.",
                        one({ lowCut(75), bell(280, -1.5, 1.0), bell(2700, 1.5, 0.9), highShelf(12500, 2), deess(7200, -7) },
                            { comp(-24, 2.5, 15, 180, 2, 1), verb(3, 2.2, 50, 0.15, 200, 10000, { { "duck", 0.5 } }), echo(2, 8, 0.25, 0.06, { { "duck", 0.6 } }) })));
    v.push_back(leadVox("Trap Vocal Air", "Trap lead: tight, bright, a 1/8D echo that ducks under the words.",
                        one({ lowCut(110), bell(400, -2, 1.2), highShelf(9000, 3.5), deess(7500) },
                            { comp(-30, 5, 1, 60, 1, 0), echo(1, 6, 0.35, 0.15, { { "duck", 0.7 } }), verb(4, 1.2, 30, 0.08, 300) })));
    v.push_back(leadVox("Rock Vocal Grit", "Rock lead with Iron grit and a slapback.",
                        one({ lowCut(100), bell(800, 1.5, 1.0), bell(3000, 2, 1.0), deess(6000) },
                            { comp(-24, 4, 5, 100, 1, 0), mag(3, 10, 0.5), echo(2, 0, 0.1, 0.15, { { "timeMs", 110 } }) })));
    v.push_back(leadVox("Soft Intimate Vocal", "Close, soft vocal: warm, lightly compressed, small room.",
                        one({ lowCut(70), lowShelf(200, 1), highShelf(10000, 1.5), deess(6800, -6) },
                            { comp(-26, 2, 20, 200, 2, 1), mag(2, 3, 0.3), verb(1, 0.7, 10, 0.1, 200) })));
    v.push_back(leadVox("Radio Telephone Voice", "Telephone / radio voice: band-limited and crushed (Germanium).",
                        one({ lowCut(400, 24), highCut(3500, 24), bell(1500, 4, 1.0) }, { mag(4, 18, 0.8), comp(-26, 6, 2, 80, 1, 0) })));
    v.push_back(leadVox("Vintage Tube Vocal (Lenin)", "Vintage lead: LA-style slow compression, tape, BRING LENIN BACK.",
                        tubes(one({ lowCut(80), bell(250, -1.5, 1.0), highShelf(9000, 2), deess(6500) },
                                  { comp(-24, 3, 10, 250, 2, 1), mag(1, 4, 0.4), verb(4, 1.6, 30, 0.1, 250, 8000, { { "color", 0 } }) }),
                              true, false, 0.7)));
    v.push_back(leadVox("Vocal Doubler Wide", "Layers: layer 2 is a short pitched echo pushed wide - a natural double.",
                        plus(one({ lowCut(90), bell(300, -2, 1.2), highShelf(11000, 2), deess() }, { comp(-26, 3, 5, 100, 0, 1) }),
                             { "Double", { lowCut(150) }, {}, { echo(1, 0, 0.0, 1.0, { { "timeMs", 18 }, { "timeRMs", 27 }, { "link", 0 }, { "pitch", 0.15 }, { "modDepth", 0.4 } }), wide(1.6) }, 0.0f, 0.45f })));
    v.push_back(leadVox("Vocal Throw Layer", "Layers: layer 2 is only reverb + 1/4 echo on the vocal's top band, ducked - a send in one preset.",
                        plus(one({ lowCut(90), bell(300, -2, 1.2), bell(3500, 1.5, 1.0), deess() }, { comp(-26, 3, 5, 100, 0, 1) }),
                             { "Throw", { lowCut(400), highCut(9000) }, {}, { echo(2, 8, 0.45, 0.6, { { "duck", 0.8 } }), verb(3, 2.8, 40, 0.5, 300, 9000, { { "duck", 0.6 } }) }, 0.0f, 0.5f })));
    v.push_back(leadVox("Stutter Vocal (Revolution)", "HamloRevolution on the vocal: musical stutters and half-time moments.",
                        one({ lowCut(90), bell(3000, 2, 1.0), deess() }, { comp(-24, 3, 5, 100), revo(1, 17, 0.7, { { "motionOn", 0 } }) })));
    v.push_back(leadVox("Ad-Lib Space", "Ad-libs: band-limited, wide, ping-pong echo and a big plate.",
                        one({ lowCut(200), highCut(9000), deess() }, { comp(-26, 4, 3, 80), echo(1, 5, 0.4, 0.3, { { "pingPong", 1 } }), verb(4, 2.0, 20, 0.25, 300), wide(1.4) })));
    v.push_back(leadVox("Vocal Dynamic Clarity", "Dynamic EQ instead of static: mud and harsh bands move only when they stick out.",
                        one({ lowCut(85), dyn(bell(300, 0, 1.2), -24, -4, 3, 10, 150), dyn(bell(2800, 0, 1.5), -24, -3, 3, 3, 80), highShelf(11000, 2), deess() },
                            { comp(-24, 2.5, 10, 120, 2, 1) })));
    v.push_back(leadVox("Lo-Fi Bedroom Vocal", "Lo-fi vocal: cassette, narrow, a dusty room.",
                        one({ lowCut(200, 24), highCut(7000, 18), deess(6000, -6) }, { comp(-26, 4, 10, 100), mag(1, 8, 0.7, { { "speed", 2 }, { "hiss", 0.1 }, { "wow", 0.3 } }), verb(1, 0.6, 10, 0.12) })));
    // Backing
    const auto back = [&](const juce::String& name, const juce::String& intent, Design d) { v.push_back(track(VM, "BackingVocal", name, intent, kBack, std::move(d))); };
    back("Backing Pad Wide", "Backing vocals pushed back and wide, darker than the lead.",
         one({ lowCut(150), bell(3000, -2, 1.0), highShelf(10000, -1), deess(6500, -8) }, { comp(-26, 4, 10, 120, 2), wide(1.5, 0, 1, { { "stereoize", 0.5 } }), verb(3, 2.0, 20, 0.2, 300) }));
    back("Gang Vocal Stack", "Group shouts: dense compression, wide, room.",
         one({ lowCut(140), bell(1000, 1.5, 1.0), deess(6500) }, { comp(-28, 6, 3, 80, 1, 0), wide(1.4), verb(2, 1.0, 10, 0.15) }));
    back("Harmony Air", "Harmonies airy and soft, behind the lead.",
         one({ lowCut(180), highShelf(12000, 2), bell(400, -2, 1.0), deess(7000, -8) }, { comp(-24, 3, 10, 150, 2), verb(4, 1.6, 30, 0.18, 300) }));
    back("Choir Cathedral", "A choir in a cathedral.",
         one({ lowCut(120), deess(6500, -6) }, { comp(-24, 2, 20, 200, 2), verb(5, 5.0, 40, 0.3, 200, 9000) }));
    back("Backing Doubler", "Layers: a pitched short double on layer 2, wide.",
         plus(one({ lowCut(150), deess() }, { comp(-26, 3, 10, 120) }),
              { "Double", { lowCut(200) }, {}, { echo(1, 0, 0.0, 1.0, { { "timeMs", 22 }, { "timeRMs", 31 }, { "link", 0 }, { "pitch", -0.12 } }), wide(1.7) }, 0.0f, 0.5f }));
    back("Backing Tape Blend", "Backing vocals glued with tape and Lenin.",
         tubes(one({ lowCut(150), highShelf(9000, -1), deess() }, { comp(-24, 3, 15, 150, 2), mag(1, 6, 0.5) }), true, false, 0.6));
    // Guitar
    const std::vector<juce::String> G { "guitar" };
    const auto guitar = [&](const juce::String& name, const juce::String& intent, Design d) { v.push_back(track(VM, "Guitar", name, intent, G, std::move(d))); };
    guitar("Acoustic Sparkle", "Acoustic guitar: boom cut, sparkle, a small room.", one({ lowCut(80), bell(200, -2.5, 1.2), highShelf(10000, 2.5) }, { comp(-24, 3, 10, 120, 0), verb(1, 0.8, 10, 0.12) }));
    guitar("Electric Crunch Body", "Electric rhythm: low mids even, Iron crunch.", one({ lowCut(90), bell(500, -1.5, 1.0), bell(2500, 2, 1.0), highCut(10000) }, { mag(3, 10, 0.5), comp(-22, 3, 10, 100) }));
    guitar("Clean Chorus Echo", "Clean electric with BBD echo modulation (chorus-like) and a plate.", one({ lowCut(90), highShelf(8000, 1.5) }, { echo(3, 7, 0.3, 0.25, { { "modDepth", 0.5 }, { "modRate", 0.8 } }), verb(4, 1.5, 20, 0.15) }));
    guitar("Wide Rhythm", "Rhythm guitar made wide (stereoize, mono-safe).", one({ lowCut(100), bell(300, -2, 1.0) }, { comp(-22, 3, 15, 120, 2), wide(1.5, 0, 1, { { "stereoize", 0.6 } }) }));
    guitar("Ambient Guitar Wash", "Ambient guitar: long tape echo, cathedral, ducked.", one({ lowCut(120), highCut(9000) }, { echo(2, 9, 0.6, 0.3, { { "duck", 0.5 } }), verb(5, 6.0, 40, 0.35, 200) }));
    guitar("Guitar Revolution Glitch", "HamloRevolution on a guitar part: experimental chops and filter motion.", one({ lowCut(100) }, { revo(2, 23, 0.8), wide(1.2) }));
    // Keys
    const std::vector<juce::String> KY { "keys", "keys2" };
    const auto keys = [&](const juce::String& name, const juce::String& intent, Design d) { v.push_back(track(VM, "Keys", name, intent, KY, std::move(d))); };
    keys("Piano Clear", "Piano / keys clearer: low mids out, presence.", one({ lowCut(50), bell(300, -2, 1.0), bell(3000, 1.5, 1.0), highShelf(10000, 1.5) }, { comp(-24, 2, 20, 150, 2) }));
    keys("Rhodes Tube Warmth", "Electric piano: Tube saturation and a gentle wide chorus.", one({ lowCut(60), highCut(9000) }, { mag(2, 8, 0.6), echo(3, 0, 0, 0.2, { { "timeMs", 12 }, { "modDepth", 0.6 } }), wide(1.3) }));
    keys("Felt Lo-Fi Keys", "Lo-fi felt keys: Vinyl crackle, dark, narrow.", one({ lowCut(120), highCut(6000, 18) }, { mag(5, 6, 0.8, { { "crackle", 0.25 }, { "speed", 0 } }), wide(0.8) }));
    keys("Keys Hall", "Keys in a lush hall.", one({ lowCut(60) }, { verb(3, 3.0, 30, 0.25, 200, 10000) }));
    keys("Keys Freeze Pad", "Layers: layer 2 freezes the keys into an endless reverb pad under the dry part.",
         plus(one({ lowCut(60) }, { comp(-24, 2, 20, 150, 2) }), { "Pad", { lowCut(200), highCut(8000) }, {}, { verb(5, 12.0, 0, 1.0, 200, 8000) }, -6.0f, 0.35f }));
    // Synth
    const std::vector<juce::String> SY { "synth" };
    const auto synth = [&](const juce::String& name, const juce::String& intent, Design d) { v.push_back(track(VM, "Synth", name, intent, SY, std::move(d))); };
    synth("Synth Lead Cut", "Lead synth through the mix: presence, tape drive, 1/8D echo.", one({ lowCut(150), bell(2500, 2, 1.0) }, { mag(1, 8, 0.5), echo(1, 6, 0.3, 0.12, { { "duck", 0.6 } }) }));
    synth("Pad Wide Soft", "Pads: wide, soft, darker, in a hall.", one({ lowCut(120), highShelf(8000, -2) }, { wide(1.5, 0, 1, { { "stereoize", 0.5 } }), verb(3, 3.0, 20, 0.2) }));
    synth("Pluck Ping-Pong", "Plucks bouncing L-R on 1/8.", one({ lowCut(150) }, { punch(3, -2, 0.7), echo(1, 5, 0.4, 0.25, { { "pingPong", 1 } }) }));
    synth("Synth Motion Filter", "HamloRevolution as an auto-filter: low-pass motion on the grid, no chops.", one({ lowCut(80) }, { revo(1, 31, 1.0, { { "timeOn", 0 }, { "glitchOn", 0 }, { "motionOn", 1 }, { "motionTarget", 1 }, { "motionDepth", 0.8 } }) }));
    synth("Arp Destruction", "Revolution DESTRUCTION on an arp: crush motion and glitches (mix 60 %).", one({ lowCut(100) }, { revo(3, 66, 0.6, { { "motionTarget", 4 } }) }));
    synth("Synth Bass Growl", "Synth bass: sub mono, growl saturation on a region above 250 Hz.", one({ lowCut(30, 24) }, { wide(1.0, 120) }, { region(250, 5000, { mag(4, 14, 0.6) }) }));
    // Mix bus
    const auto bus = [&](const juce::String& name, const juce::String& intent, Design d) { v.push_back(track(VM, "MixBus", name, intent, kMixdowns, std::move(d))); };
    bus("Mix Bus Glue", "2-bus glue: 2:1, slow attack, auto-release feel.", one({ lowCut(20, 18) }, { comp(-16, 2, 30, 200, 2, 1, 8) }));
    bus("Mix Bus Analog (Lenin)", "Analog bus: tape and BRING LENIN BACK.", tubes(one({ lowCut(20, 18) }, { mag(1, 4, 0.6), comp(-16, 1.5, 30, 200, 2) }), true, false, 0.8));
    bus("Mix Bus Stalin Heat", "BRING STALIN BACK at 50 %: transformer weight and harder harmonics.", tubes(one({ lowCut(20, 18) }, { comp(-16, 2, 30, 200, 2) }), false, true, 1.0, 0.5));
    bus("Mix Bus Air & Weight", "Tilt-like: a low shelf and air shelf, wide but mono below 120 Hz.", one({ lowCut(20, 18), lowShelf(80, 1), highShelf(12000, 1.5) }, { comp(-16, 1.5, 30, 200, 2), wide(1.1, 120) }));
    bus("Mix Bus Multiband", "Three regions: lows held, mids gentle, highs smoothed.", one({ lowCut(20, 18) }, {},
         { region(20, 150, { comp(-18, 2.5, 30, 150, 2) }), region(150, 4000, { comp(-16, 1.5, 30, 200, 2) }), region(4000, 20000, { comp(-22, 2, 5, 80, 0) }) }));
    bus("Mix Bus Parallel Punch", "Layers: layer 2 is a crushed copy of the mix blended in (New York style).",
         plus(one({ lowCut(20, 18) }, { comp(-16, 1.5, 30, 200, 2) }), { "Crush", { lowCut(30), bell(100, 2, 1.0), highShelf(8000, 2) }, {}, { comp(-36, 10, 1, 60, 1, 0) }, 0.0f, 0.3f }));
}

Preset master(const juce::String& group, const juce::String& name, const juce::String& intent, double target, Design d)
{
    Preset p;
    p.pack = VM;
    p.category = "Mastering";
    p.group = group;
    p.name = name;
    p.intent = intent + " Target " + juce::String(target, 1) + " LUFS integrated, ceiling -1 dBTP.";
    p.material = kMixdowns;
    p.design = std::move(d);
    p.cal = Cal::Target;
    p.targetLufs = target;
    return p;
}

void mastering(std::vector<Preset>& v)
{
    // Targets: the owner's own masters measure -10.6 .. -11.8 LUFS integrated; genre conventions put
    // pop / rock near -9..-10, rap -8.5..-9.5, trap / EDM / techno -7.5..-8.5; every genre also has a
    // -14 LUFS streaming master (what the platforms normalise to).
    struct Genre { juce::String group; double loud; double lowDb; double airDb; double width; int satModel; };
    const std::vector<Genre> genres { { "Pop", -9.5, 0.5, 1.5, 1.1, 1 }, { "Rap", -9.0, 1.0, 1.0, 1.0, 3 }, { "Trap", -8.0, 1.5, 1.5, 1.05, 2 },
                                      { "Techno", -8.5, 1.0, 0.5, 1.0, 3 }, { "House-EDM", -8.5, 1.0, 1.5, 1.1, 1 }, { "Rock-Indie", -9.5, 0.5, 1.0, 1.1, 3 } };
    for (const auto& g : genres)
    {
        const auto base = [&](std::vector<FxSlotState> before)
        {
            std::vector<FxSlotState> chain = std::move(before);
            chain.push_back(wall(-4, -1.0, g.group == "Techno" || g.group == "Trap" ? 1 : 0));
            return chain;
        };
        const std::vector<EqNodeState> eq { lowCut(22, 24), lowShelf(70, g.lowDb), bell(300, -0.5, 0.8), highShelf(12000, g.airDb) };
        const auto name = g.group.replace("-", " / ");
        v.push_back(master(g.group, name + " Clean Master", "Transparent: gentle glue, tonal balance, true-peak limiting.", g.loud,
                           one(eq, base({ comp(-14, 1.5, 30, 200, 2, 1, 10), wide(g.width, 120) }))));
        v.push_back(master(g.group, name + " Warm Master (Lenin)", "Warm: tape saturation and BRING LENIN BACK at 30 % (the ceiling leaves room for the tube).", g.loud,
                           tubes(one(eq, base({ comp(-14, 1.5, 30, 200, 2, 1, 10), mag(1, 3, 0.5), wide(g.width, 120) })), true, false, 0.3)));
        v.push_back(master(g.group, name + " Punch Master", "Punch: transients kept before the limiter, low end held by a region compressor.", g.loud + 0.5,
                           one(eq, base({ punch(2, 0, 0.6, 1, 0.6f), comp(-14, 2, 20, 120, 1, 1, 8), wide(g.width, 120) }), { region(20, 120, { comp(-18, 2, 30, 150, 2) }) })));
        v.push_back(master(g.group, name + " Multiband Master", "Multiband: three regions with their own compressors and saturation on the mids.", g.loud,
                           one(eq, base({ wide(g.width, 120) }),
                               { region(20, 150, { comp(-18, 2, 30, 150, 2) }), region(150, 4000, { comp(-16, 1.5, 30, 200, 2), mag(g.satModel, 3, 0.3) }),
                                 region(4000, 20000, { comp(-22, 2, 5, 80, 0) }) })));
        v.push_back(master(g.group, name + " Streaming -14", "Streaming: no loudness war, the dynamics kept; the platforms play it at -14 LUFS anyway.", -14.0,
                           one(eq, base({ comp(-16, 1.5, 30, 200, 2, 1, 10) }))));
    }
}

void showcase(std::vector<Preset>& v)
{
    const auto show = [&](const juce::String& cat, const juce::String& name, const juce::String& intent, std::vector<juce::String> material, Design d)
    { v.push_back(track(SH, cat, name, intent, std::move(material), std::move(d))); };
    // Layers
    {
        Design d;
        d.routing = LayerRouting::Parallel;
        d.layers.push_back({ {}, { lowCut(90), bell(300, -2, 1.2), deess() }, {}, { comp(-26, 3, 5, 100, 0, 1) } });
        d.layers.push_back({ "Air layer", { lowCut(3000, 24), highShelf(10000, 4) }, {}, { comp(-30, 6, 1, 60), wide(1.5) }, 0.0f, 0.4f });
        d.layers.push_back({ "Space layer", { lowCut(300), highCut(8000) }, {}, { verb(5, 4.0, 60, 1.0, 300, 8000, { { "duck", 0.7 } }) }, -6.0f, 0.35f });
        d.layers.push_back({ "Echo layer", { lowCut(400), highCut(6000) }, {}, { echo(2, 6, 0.5, 1.0, { { "pingPong", 1 }, { "duck", 0.8 } }) }, -6.0f, 0.3f });
        show("Layers", "Vocal Cinema (4 layers)", "Four parallel layers: the cleaned vocal, an air layer, a ducked cathedral and a ducked ping-pong echo - each with its own EQ.", kVox, d);
    }
    {
        Design d;
        d.routing = LayerRouting::Serial;
        d.layers.push_back({ {}, { lowCut(25, 18) }, {}, { gate(-40, 12, 20, 120, 1) } });
        d.layers.push_back({ "Color", {}, {}, { mag(3, 10, 0.6), punch(3, -1, 0.6) } });
        d.layers.push_back({ "Glue", { highShelf(10000, 1.5) }, {}, { comp(-18, 3, 20, 120, 2) } });
        show("Layers", "Drum Factory (serial layers)", "Serial layers as a production line: clean-up, colour, glue - each stage on its own layer.", { "drumbus1" }, d);
    }
    {
        Design d;
        d.layers.push_back({ {}, { lowCut(25, 18) }, {}, { comp(-18, 2, 30, 150, 2) } });
        d.layers.push_back({ "Crush", { lowCut(40), bell(120, 3, 1.0), highShelf(6000, 3) }, {}, { comp(-40, 20, 0.5, 50, 1, 0), mag(4, 12, 1.0) }, 0.0f, 0.35f });
        d.layers.push_back({ "Glitch", { lowCut(200) }, {}, { revo(1, 41, 1.0, { { "motionOn", 0 } }) }, 0.0f, 0.25f });
        show("Layers", "Drums Parallel Monster", "Three layers: the bus, a destroyed parallel crush, and a Revolution glitch layer blended at 25 %.", { "drumbus1" }, d);
    }
    // Revolution
    show("Revolution", "Beat Switch-Up", "HamloRevolution MUSICAL on the drums: tape stops, repeats and half time - a different bar every time.", { "drumbus1" },
         one({}, { revo(1, 77, 1.0), comp(-18, 2, 20, 120, 2) }));
    show("Revolution", "Vocal Chop Machine", "Revolution EXPERIMENTAL on a vocal: chops, reverses and width motion.", kVox,
         one({ lowCut(100), deess() }, { revo(2, 5, 0.8, { { "motionTarget", 3 } }), verb(4, 1.5, 20, 0.15) }));
    show("Revolution", "Full Mix Destruction", "Revolution DESTRUCTION on the whole mix at 50 % with STALIN: a breakdown in one click.", kMixdowns,
         tubes(one({}, { revo(3, 99, 0.5) }), false, true, 1.0, 0.6));
    show("Revolution", "Bass Gated Motion", "Revolution as a rhythmic volume gate on bass / pads (no chops).", { "bass2", "keys" },
         one({}, { revo(1, 12, 1.0, { { "timeOn", 0 }, { "glitchOn", 0 }, { "motionTarget", 0 }, { "motionDepth", 0.9 } }) }));
    // Points with modules, regions, tubes
    show("Precision", "De-Ess + Air Exciter", "A point with modules: the 8-14 kHz band gets its own Tube exciter while the DE-ESS point holds the s.", kVox,
         one({ lowCut(90), deess(), withModules(bell(10000, 1, 0.7), { mag(2, 12, 0.6) }) }, { comp(-24, 3, 5, 100) }));
    show("Precision", "Kick-Only Reverb", "A region: only 40-120 Hz goes into a big reverb - a huge kick tail without washing the mix.", { "drumbus1" },
         one({}, {}, { region(40, 120, { verb(3, 2.0, 0, 0.3, 30, 400) }) }));
    show("Precision", "Vocal Band Saturation", "Saturation on the voice's presence band only (1.5-5 kHz region): grit without harshness.", kVox,
         one({ lowCut(90), deess() }, { comp(-24, 3, 5, 100) }, { region(1500, 5000, { mag(3, 14, 0.5) }) }));
    show("Tubes", "Lenin + Stalin Full Chain", "Both tubes: LENIN into STALIN on the whole mix (MIX 70 / 50 %).", kMixdowns,
         tubes(one({ lowCut(20, 18) }, { comp(-16, 1.5, 30, 200, 2) }), true, true, 0.7, 0.5));
    show("Tubes", "Stalin Vocal Radio", "STALIN at full on a band-limited vocal: a 1950s broadcast.", kVox,
         tubes(one({ lowCut(300, 24), highCut(5000, 24), deess() }, { comp(-24, 4, 5, 100) }), false, true, 1.0, 1.0));
}

void more(std::vector<Preset>& v)
{
    const std::vector<juce::String> K { "kick" }, S { "snare" }, H { "hihat" }, P { "perc" }, B { "bass1", "bass2" }, D { "drumbus1" };
    const auto add = [&](const juce::String& pack, const juce::String& cat, const juce::String& name, const juce::String& intent, std::vector<juce::String> m, Design d)
    { v.push_back(track(pack, cat, name, intent, std::move(m), std::move(d))); };
    // Kick
    add(DB, "Kick", "Kick Stalin Drive", "BRING STALIN BACK on a kick: transformer weight, harder harmonics.", K, tubes(one({ lowCut(30, 24), bell(350, -3, 1.4) }, { punch(3, -2, 0.6) }), false, true, 1.0, 0.8));
    add(DB, "Kick", "Drum & Bass Kick", "Fast DnB kick: tight low end, bright beater, short.", K, one({ lowCut(40, 24), bell(80, 1.5, 1.2), bell(4500, 3, 1.2) }, { punch(6, -5, 0.8), comp(-18, 4, 3, 60, 1, 0) }));
    add(DB, "Kick", "Kick Soft Felt", "Soft kick for acoustic / indie: beater down, warm.", K, one({ lowCut(35, 18), highCut(5000, 12), bell(90, 1.5, 1.0) }, { punch(-3, 1, 0.5, 0), mag(2, 4, 0.4) }));
    add(DB, "Kick", "Kick Multiband Click", "A region on the click (2-6 kHz) gets its own transient shaper; the low end is left alone.", K,
        one({ lowCut(28, 24), bell(300, -3, 1.3) }, { comp(-20, 3, 10, 90) }, { region(2000, 6000, { punch(8, -4, 0.8) }) }));
    // Snare
    add(DB, "Snare", "Snare Tape Crack", "Snare through tape 19 cm/s: crack rounded, body thicker.", S, one({ lowCut(90, 18), bell(200, 2, 1.0) }, { punch(4, 0, 0.7), mag(1, 10, 0.6) }));
    add(DB, "Snare", "Snare Long Hall", "A ballad snare in a long hall, ducked by the hit.", S, one({ lowCut(100, 18) }, { verb(3, 2.8, 20, 0.3, 250, 9000, { { "duck", 0.4 } }) }));
    add(DB, "Snare", "Snare Echo Throw", "Snare with a 1/4 tape echo throw (reggae / dub).", S, one({ lowCut(120, 18), highCut(9000) }, { echo(2, 8, 0.5, 0.3, { { "duck", 0.3 } }) }));
    add(DB, "Snare", "Snare Body Only", "Snare ring and top tamed: only the body stays forward.", S, one({ lowCut(80, 18), bell(200, 2, 1.0), dyn(bell(6000, 0, 1.0), -30, -6, 3, 1, 60) }, { comp(-22, 3, 10, 100, 2) }));
    // Hats
    add(DB, "HiHat", "Hats Lo-Fi Vinyl", "Hats from a record: Vinyl 45, crackle, dark.", H, one({ lowCut(400, 18), highCut(9000, 12) }, { mag(5, 4, 0.8, { { "speed", 1 }, { "crackle", 0.2 } }) }));
    add(DB, "HiHat", "Hats Ping-Pong 1/16", "Hats with a 1/16 ping-pong echo.", H, one({ lowCut(400, 18) }, { echo(1, 2, 0.3, 0.2, { { "pingPong", 1 } }) }));
    add(DB, "HiHat", "Hats Gate Chop", "Hats chopped by a hard gate (live bleed, tightness).", H, one({ lowCut(400, 24) }, { gate(-30, 40, 10, 40, 0, 2000) }));
    // Perc
    add(DB, "Percussion", "Perc Revolution Fills", "Revolution on percussion: automatic fills every bar.", P, one({ lowCut(150, 18) }, { revo(1, 27, 0.7, { { "motionOn", 0 } }) }));
    add(DB, "Percussion", "Perc Crushed", "Germanium-crushed percussion for industrial / techno.", P, one({ lowCut(150, 18) }, { mag(4, 20, 0.6), comp(-24, 6, 3, 60, 1, 0) }));
    add(DB, "Percussion", "Perc Wide Room", "Percussion wide and in a room.", P, one({ lowCut(150, 18) }, { wide(1.5, 0, 1, { { "stereoize", 0.5 } }), verb(2, 1.0, 10, 0.15) }));
    // Drum bus
    add(DB, "DrumBus", "Stalin Drum Smash", "STALIN on the drum bus with a parallel crush - aggressive.", D, tubes(one({ lowCut(30, 18) }, { comp(-30, 8, 1, 60, 1, 0, 2, 0.4f) }), false, true, 1.0, 0.7));
    add(DB, "DrumBus", "Drum Bus Wide Top", "Lows mono and tight, cymbals wider.", D, one({ lowCut(25, 18) }, { wide(1.0, 150) }, { region(5000, 20000, { wide(1.5, 0, 1, { { "stereoize", 0.4 } }) }) }));
    add(DB, "DrumBus", "Lo-Fi Drum Bus", "Lo-fi drums: cassette, narrow, dark.", D, one({ lowCut(60, 18), highCut(7000, 18) }, { mag(1, 10, 0.8, { { "speed", 2 }, { "wow", 0.2 }, { "hiss", 0.1 } }), comp(-24, 4, 10, 100), wide(0.7) }));
    add(DB, "DrumBus", "Drum Bus Transient Tamer", "Peaky drums smoothed: attack down, body up, gentle glue.", D, one({ lowCut(25, 18) }, { punch(-4, 2, 0.5, 0), comp(-18, 2, 20, 150, 2) }));
    add(DB, "DrumBus", "Drum Bus Dub Space", "Dub space on the drums: tape echo with feedback and a chamber.", D, one({ lowCut(30, 18) }, { echo(2, 9, 0.55, 0.15, { { "duck", 0.5 } }), verb(2, 1.5, 20, 0.12) }));
    add(DB, "DrumBus", "Drum Bus Limit", "Drum bus loud: RedWall STAKHANOV with a fast release.", D, one({ lowCut(25, 18) }, { comp(-18, 3, 15, 100, 1), wall(-5, -1, 1) }));
    // Bass
    add(DB, "Bass", "Bass Stalin Iron", "Bass through STALIN: transformer saturation in the lows (MIX 60 %).", B, tubes(one({ lowCut(30, 24) }, { comp(-22, 3, 15, 120) }), false, true, 1.0, 0.6));
    add(DB, "Bass", "Reese Controlled", "Reese / wide bass: sub mono, highs wide, dynamic mud control.", B, one({ lowCut(28, 24), dyn(bell(250, 0, 1.0), -22, -4) }, { wide(1.3, 150), comp(-20, 3, 10, 100) }));
    add(DB, "Bass", "Synthwave Bass", "Synthwave bass: tape, chorus-like BBD shimmer, round.", B, one({ lowCut(30, 24), bell(120, 1.5, 1.0) }, { mag(1, 6, 0.6), echo(3, 0, 0, 0.15, { { "timeMs", 10 }, { "modDepth", 0.5 } }), wide(1.0, 120) }));
    add(DB, "Bass", "Bass Pump Without Sidechain", "Revolution volume motion on the grid: a pump without a sidechain.", B, one({ lowCut(30, 24) }, { revo(0, 4, 1.0, { { "timeOn", 0 }, { "glitchOn", 0 }, { "motionTarget", 0 }, { "motionDepth", 0.7 }, { "grid", 0 } }) }));
    add(DB, "Bass", "Acid Line Focus", "Acid / 303 line: resonant mids forward, crunch.", B, one({ lowCut(40, 24), bell(1200, 3, 1.5) }, { mag(4, 10, 0.5), comp(-22, 4, 5, 80) }));

    // Lead vocals
    const auto lead = [&](const juce::String& name, const juce::String& intent, Design d) { v.push_back(leadVox(name, intent, std::move(d))); };
    lead("R&B Silk Vocal", "R&B lead: silky top, smooth compression, a short plate and a 1/8 echo.", one({ lowCut(90), bell(250, -1.5, 1.0), highShelf(11000, 3), deess(7200) }, { comp(-26, 3, 8, 150, 2), verb(4, 1.3, 30, 0.12), echo(1, 5, 0.25, 0.08, { { "duck", 0.6 } }) }));
    lead("Indie Room Vocal", "Indie vocal: mid-forward, tape, a live room.", one({ lowCut(100), bell(1200, 1.5, 1.0), highCut(14000), deess(6500) }, { comp(-24, 3, 10, 120, 1), mag(1, 6, 0.4), verb(1, 0.9, 10, 0.15) }));
    lead("Drill Vocal Tight", "Drill vocal: very tight, upfront, bright, short 1/16 echo.", one({ lowCut(120), bell(300, -3, 1.2), highShelf(9000, 3), deess(7000, -10) }, { comp(-30, 6, 1, 50, 1, 0), echo(1, 2, 0.2, 0.08, { { "duck", 0.8 } }) }));
    lead("Spoken Word Warm", "Podcast / spoken word: warm, even, no reverb.", one({ lowCut(80), lowShelf(180, 1), bell(3000, 1.5, 1.0), deess(6500) }, { comp(-26, 3, 10, 150, 2), comp(-14, 2, 5, 60, 0, 0) }));
    lead("Vocal Stalin Shout", "Screams / shouts: STALIN and Iron, compressed hard.", tubes(one({ lowCut(120), bell(1500, 2, 1.0), deess(6000) }, { comp(-30, 8, 2, 60, 1, 0), mag(3, 14, 0.5) }), false, true, 1.0, 0.8));
    lead("Vocal Big Hall Ballad", "A big ballad: long hall with predelay, tape echo, de-esser before the space.", one({ lowCut(80), highShelf(12000, 2), deess() }, { comp(-24, 2.5, 15, 180, 2), verb(3, 3.2, 70, 0.2, 250), echo(2, 8, 0.3, 0.08, { { "duck", 0.7 } }) }));
    lead("Vocal Air Multiband", "A region on the air band (8-16 kHz) compressed on its own, so the top stays smooth when it gets louder.", one({ lowCut(90), highShelf(11000, 3), deess() }, { comp(-24, 3, 5, 100) }, { region(8000, 16000, { comp(-30, 4, 1, 50, 0) }) }));
    lead("Tuning Ready Vocal", "A clean, dry, controlled vocal for pitch correction afterwards: no time effects.", one({ lowCut(100), bell(300, -2, 1.2), deess() }, { comp(-28, 4, 3, 80, 0, 0) }));
    // Backing
    const auto back = [&](const juce::String& name, const juce::String& intent, Design d) { v.push_back(track(VM, "BackingVocal", name, intent, kBack, std::move(d))); };
    back("Backing Ping-Pong Air", "Backing vocal echoes bouncing L-R, airy.", one({ lowCut(180), highShelf(11000, 2), deess() }, { comp(-26, 3, 10, 120), echo(1, 6, 0.35, 0.2, { { "pingPong", 1 } }) }));
    back("Backing Lo-Fi Chorus", "Lo-fi backing: cassette and a wide BBD shimmer.", one({ lowCut(200), highCut(8000), deess() }, { mag(1, 6, 0.6, { { "speed", 2 } }), echo(3, 0, 0, 0.3, { { "timeMs", 15 }, { "modDepth", 0.6 } }), wide(1.5) }));
    back("Backing Behind Lead", "Backing vocal tucked behind the lead: presence cut where the lead lives.", one({ lowCut(160), bell(3000, -3, 1.0), deess(6500, -8) }, { comp(-24, 3, 10, 150, 2), verb(2, 1.2, 20, 0.15) }));
    // Guitar
    const std::vector<juce::String> G { "guitar" };
    add(VM, "Guitar", "Guitar Lenin Amp Warmth", "Guitar with BRING LENIN BACK and tape: warm amp-like body.", G, tubes(one({ lowCut(90), highCut(10000) }, { mag(1, 6, 0.5), comp(-22, 2, 15, 120, 2) }), true, false, 1.0));
    add(VM, "Guitar", "Guitar Ping-Pong Delay", "Lead guitar with a dotted ping-pong echo and plate.", G, one({ lowCut(100) }, { echo(2, 6, 0.4, 0.2, { { "pingPong", 1 }, { "duck", 0.4 } }), verb(4, 1.6, 20, 0.12) }));
    add(VM, "Guitar", "Guitar Tight Gate", "High-gain rhythm: gate the noise, tight lows.", G, one({ lowCut(110, 24), bell(250, -2, 1.0) }, { gate(-45, 30, 20, 80) }));
    add(VM, "Guitar", "Guitar Freeze Swell", "Layers: layer 2 turns the guitar into a long swelling pad.", G,
        plus(one({ lowCut(100) }, {}), { "Swell", { lowCut(250), highCut(7000) }, {}, { verb(5, 15.0, 0, 1.0, 250, 7000) }, -6.0f, 0.35f }));
    // Keys
    const std::vector<juce::String> KY { "keys", "keys2" };
    add(VM, "Keys", "Keys Tape Wobble", "Keys with cassette wow and flutter.", KY, one({ lowCut(60), highCut(9000) }, { mag(1, 4, 0.8, { { "speed", 2 }, { "wow", 0.5 } }) }));
    add(VM, "Keys", "Keys Ping-Pong Dream", "Dreamy keys: ping-pong echo, cathedral, wide.", KY, one({ lowCut(80) }, { echo(2, 6, 0.45, 0.2, { { "pingPong", 1 } }), verb(5, 4.0, 30, 0.2), wide(1.3) }));
    add(VM, "Keys", "Keys Revolution Chops", "Revolution on keys: musical chops and repeats.", KY, one({ lowCut(60) }, { revo(1, 51, 0.7) }));
    add(VM, "Keys", "Organ Stalin Grit", "Organ / keys through STALIN: the transformer roar.", KY, tubes(one({ lowCut(60) }, { comp(-22, 2, 20, 120, 2) }), false, true, 1.0, 0.7));
    // Synth
    const std::vector<juce::String> SY { "synth" };
    add(VM, "Synth", "Synth Lenin Analog", "Synth with BRING LENIN BACK and Tube: analog-like.", SY, tubes(one({ lowCut(80) }, { mag(2, 6, 0.4) }), true, false, 1.0));
    add(VM, "Synth", "Synth Stereo Cosmos", "Synth in KOSMOS: surround-like width, behind the speakers.", SY, one({ lowCut(100) }, { wide(1.2, 0, 2, { { "space", 0.7 } }), verb(6, 0.8, 10, 0.15) }));
    add(VM, "Synth", "Synth Tape Stop Fx", "Revolution with only TIME: tape stops and half-time on a synth part.", SY, one({ lowCut(80) }, { revo(1, 61, 1.0, { { "glitchOn", 0 }, { "motionOn", 0 } }) }));
    add(VM, "Synth", "Synth Freeze Drone", "Layers: the synth plus a long drone layer under it.", SY,
        plus(one({ lowCut(80) }, {}), { "Drone", { lowCut(150), highCut(6000) }, {}, { verb(5, 20.0, 0, 1.0, 150, 6000), wide(1.5) }, -6.0f, 0.35f }));
    // Mix bus
    const auto bus = [&](const juce::String& name, const juce::String& intent, Design d) { v.push_back(track(VM, "MixBus", name, intent, kMixdowns, std::move(d))); };
    bus("Mix Bus Wide Mono-Safe", "Wider mix, mono below 150 Hz, SAFE on.", one({ lowCut(20, 18) }, { wide(1.2, 150) }));
    bus("Mix Bus Tape 15 ips", "Tape machine on the mix, gentle.", one({ lowCut(20, 18) }, { mag(1, 3, 0.7), comp(-16, 1.5, 30, 200, 2) }));
    bus("Mix Bus Dynamic Mud", "A dynamic point at 250 Hz cuts mud only when it builds up.", one({ lowCut(20, 18), dyn(bell(250, 0, 1.0), -24, -3, 3, 20, 200) }, { comp(-16, 1.5, 30, 200, 2) }));
    bus("Mix Bus Vintage Radio", "The whole mix as an old radio: band-limited and STALIN.", tubes(one({ lowCut(250, 24), highCut(4500, 24) }, { mag(5, 6, 0.6, { { "crackle", 0.2 } }) }), false, true, 1.0, 1.0));

    // Mastering, more variants per genre
    struct Genre { juce::String group; double loud; };
    for (const auto& g : std::vector<Genre> { { "Pop", -9.5 }, { "Rap", -9.0 }, { "Trap", -8.0 }, { "Techno", -8.5 }, { "House-EDM", -8.5 }, { "Rock-Indie", -9.5 } })
    {
        const auto name = g.group.replace("-", " / ");
        const std::vector<EqNodeState> eq { lowCut(22, 24), lowShelf(70, 0.5), highShelf(12000, 1.0) };
        v.push_back(master(g.group, name + " Loud Master", "Loud: glue, Tape and the limiter in STAKHANOV.", g.loud + 0.5,
                           one(eq, { comp(-14, 2, 20, 150, 1, 1, 8), mag(1, 3, 0.4), wide(1.05, 120), wall(-6, -1.0, 1) })));
        v.push_back(master(g.group, name + " Stalin Master", "Character: BRING STALIN BACK at 15 % and Iron on the mids (the ceiling leaves room for the tube).", g.loud,
                           tubes(one(eq, { comp(-14, 1.5, 30, 200, 2, 1, 10), wide(1.05, 120), wall(-4, -1.0, 0) }, { region(200, 3000, { mag(3, 4, 0.3) }) }), false, true, 1.0, 0.15)));
    }

    // Showcase
    const auto show = [&](const juce::String& cat, const juce::String& name, const juce::String& intent, std::vector<juce::String> material, Design d)
    { v.push_back(track(SH, cat, name, intent, std::move(material), std::move(d))); };
    {
        Design d;
        d.layers.push_back({ {}, { lowCut(25, 18) }, {}, {} });
        d.layers.push_back({ "Sub", { highCut(120, 24) }, {}, { mag(2, 12, 1.0), wide(0.0) }, 0.0f, 0.4f });
        d.layers.push_back({ "Top", { lowCut(5000, 24) }, {}, { punch(5, -3, 0.7), wide(1.5) }, 0.0f, 0.4f });
        show("Layers", "Split-Band Bass Layers", "Three layers split by EQ: the bass, a saturated mono sub layer and a wide transient top layer.", { "bass1", "bass2" }, d);
    }
    {
        Design d;
        d.layers.push_back({ {}, { lowCut(90), deess() }, {}, { comp(-26, 3, 5, 100) } });
        d.layers.push_back({ "Octave down", { lowCut(200) }, {}, { echo(1, 0, 0.0, 1.0, { { "timeMs", 30 }, { "pitch", -12 } }) }, -6.0f, 0.3f });
        d.layers.push_back({ "Octave up", { lowCut(400) }, {}, { echo(1, 0, 0.0, 1.0, { { "timeMs", 45 }, { "pitch", 12 } }), wide(1.6) }, -9.0f, 0.2f });
        show("Layers", "Vocal Octave Choir", "Layers with pitched echoes an octave down and up: one vocal becomes a small choir.", kVox, d);
    }
    show("Revolution", "Revolution Width Pulse", "Revolution motion on WIDTH: the mix breathes mono-wide on the grid.", kMixdowns,
         one({}, { revo(1, 13, 1.0, { { "timeOn", 0 }, { "glitchOn", 0 }, { "motionTarget", 3 }, { "motionDepth", 0.7 } }) }));
    show("Revolution", "Revolution Crush Fills", "Revolution with CRUSH motion and glitches on drums: bitcrushed fills.", { "drumbus1" },
         one({}, { revo(2, 88, 0.8, { { "motionTarget", 4 } }) }));
    show("Precision", "Hi-Hat Only Echo", "A region on the hats (6-16 kHz) gets a ping-pong echo; kick and snare stay dry.", { "drumbus1" },
         one({}, {}, { region(6000, 16000, { echo(1, 5, 0.4, 0.3, { { "pingPong", 1 } }) }) }));
    show("Precision", "Snare Band Reverb", "A point at 220 Hz carries a plate on its band: only the snare body blooms.", { "drumbus1" },
         one({ withModules(bell(220, 0, 1.0), { verb(4, 1.5, 0, 0.5, 100, 6000) }) }, {}));
    show("Tubes", "Lenin Vocal Warmth", "BRING LENIN BACK alone on a vocal at full: tube warmth without other colour.", kVox,
         tubes(one({ lowCut(90), deess() }, { comp(-26, 3, 5, 100) }), true, false, 1.0));
    show("Tubes", "Stalin Drum Room", "STALIN plus a crushed room on drums - the transformer roar of a 1950s studio.", { "drumbus1" },
         tubes(one({ lowCut(30) }, { verb(2, 0.8, 5, 0.2), comp(-26, 6, 5, 80, 1, 0) }), false, true, 1.0, 0.8));
}

// Presets pass 3 (owner, 2026-10-27: "more presets, only good ones, and more for Revolution"). The Revolution
// presets reuse the (character, seed, settings) of the HamloRevolution module presets that RevolutionPresetTests
// checks, so the action in the name is in the pattern; the rest are standard engineering recipes the library
// lacked (parallel compression, NY-style smash, sub focus).
void pass3(std::vector<Preset>& v)
{
    const std::vector<juce::String> K { "kick" }, S { "snare" }, H { "hihat" }, P { "perc" }, B { "bass1", "bass2" }, D { "drumbus1" }, G { "guitar" },
        KY { "keys", "keys2" }, SY { "synth" };
    const auto add = [&](const juce::String& pack, const juce::String& cat, const juce::String& name, const juce::String& intent, std::vector<juce::String> m, Design d)
    { v.push_back(track(pack, cat, name, intent, std::move(m), std::move(d))); };
    // Revolution, on the part it suits
    add(DB, "HiHat", "Hat Ratchet Rolls", "Revolution GLITCH only: trap-style ratchet rolls on the hats.", H,
        one({ lowCut(350, 18) }, { revo(1, 62, 1.0, { { "timeOn", 0 }, { "motionOn", 0 }, { "glitchDepth", 0.7 } }) }));
    add(DB, "Percussion", "Perc Triplet Stutter", "Revolution GLITCH on a triplet grid: stutters that roll against the beat.", P,
        one({ lowCut(150, 18) }, { revo(1, 17, 1.0, { { "timeOn", 0 }, { "motionOn", 0 }, { "triplet", 1 } }) }));
    add(DB, "Bass", "808 Tape Stop Drops", "Revolution TIME only: tape stops on the 808 / bass at the ends of phrases.", B,
        one({ lowCut(28, 24) }, { revo(1, 23, 1.0, { { "glitchOn", 0 }, { "motionOn", 0 }, { "timeStretch", 0 } }), wide(1.0, 120) }));
    add(VM, "LeadVocal", "Vocal Half-Time Hook", "Revolution TIME, CLEAN stretch: the hook drops to half time and back.", kVox,
        one({ lowCut(90), bell(300, -2, 1.2), deess() }, { comp(-26, 3, 5, 100, 0, 1), revo(1, 83, 1.0, { { "glitchOn", 0 }, { "motionOn", 0 }, { "timeStretch", 2 } }) }));
    add(VM, "BackingVocal", "Backing Reverse Swells", "Revolution reverses the backing line into a plate: swells into the next word.", kBack,
        one({ lowCut(160), deess(6500, -8) }, { comp(-24, 3, 10, 120, 2), revo(1, 94, 0.6, { { "glitchOn", 0 }, { "motionOn", 0 } }), verb(4, 1.8, 20, 0.2, 300) }));
    add(VM, "Guitar", "Guitar Freeze Filter", "Revolution freezes guitar moments while a smooth low-pass motion moves under them (mix 80 %).", G,
        one({ lowCut(100) }, { revo(2, 44, 0.8, { { "glitchOn", 0 }, { "motionOn", 1 }, { "motionTarget", 1 }, { "motionDepth", 0.4 }, { "motionSmooth", 0.6 } }) }));
    add(VM, "Keys", "Keys Crush Pulse", "Revolution CRUSH motion on keys with tape under it: lo-fi bit pulses on the grid.", KY,
        one({ lowCut(60), highCut(9000) }, { mag(1, 4, 0.5, { { "speed", 1 } }), revo(1, 42, 1.0, { { "timeOn", 0 }, { "glitchOn", 0 }, { "motionTarget", 4 }, { "motionDepth", 0.5 }, { "motionSmooth", 0.5 } }) }));
    add(VM, "Synth", "Pad Volume Chops", "Revolution VOLUME motion, hard edges: a pad chopped on 1/16 (trance-gate style).", SY,
        one({ lowCut(100) }, { revo(2, 18, 1.0, { { "timeOn", 0 }, { "glitchOn", 0 }, { "motionTarget", 0 }, { "motionDepth", 0.9 }, { "motionSmooth", 0.05 } }) }));
    const auto show = [&](const juce::String& cat, const juce::String& name, const juce::String& intent, std::vector<juce::String> material, Design d)
    { v.push_back(track(SH, cat, name, intent, std::move(material), std::move(d))); };
    show("Revolution", "Vocal Tape Stop Ends", "Revolution TIME on a vocal: tape stops at phrase ends, TAPE stretch.", kVox,
         one({ lowCut(90), deess() }, { comp(-26, 3, 5, 100, 0, 1), revo(1, 23, 1.0, { { "glitchOn", 0 }, { "motionOn", 0 }, { "timeStretch", 0 } }) }));
    show("Revolution", "Vocal Reverse Tails", "Revolution reverses vocal tails into a ducked plate (mix 80 %).", kVox,
         one({ lowCut(90), deess() }, { comp(-26, 3, 5, 100, 0, 1), revo(1, 94, 0.8, { { "glitchOn", 0 }, { "motionOn", 0 } }), verb(4, 1.6, 30, 0.12, 250, 9000, { { "duck", 0.5 } }) }));
    show("Revolution", "Boom Bap Swing Chops", "Revolution MUSICAL with 33 % swing on the drums: chops that keep the boom-bap pocket.", D,
         one({}, { revo(1, 16, 1.0, { { "motionOn", 0 }, { "swing", 0.33 } }), comp(-18, 2, 20, 120, 2) }));
    show("Revolution", "Scratch Break", "Revolution EXPERIMENTAL GLITCH: DJ scratches and jumps on a drum break.", D,
         one({}, { revo(2, 5, 1.0, { { "timeOn", 0 }, { "motionOn", 0 } }) }));
    show("Revolution", "Double-Time Drum Rush", "Revolution TIME, RHYTHM stretch: the drums rush in double time and land back.", D,
         one({}, { revo(2, 53, 1.0, { { "glitchOn", 0 }, { "motionOn", 0 }, { "timeStretch", 1 } }) }));
    show("Revolution", "Humanized Glitch Drums", "Revolution EXPERIMENTAL with 40 % humanize: glitches that sit off the grid like a player.", D,
         one({}, { revo(2, 10, 1.0, { { "motionOn", 0 }, { "humanize", 0.4 } }) }));
    show("Revolution", "Mix Gap Chopper", "Revolution GLITCH gaps on the whole mix: DJ-style cuts (mix 70 %).", kMixdowns,
         one({}, { revo(1, 21, 0.7, { { "timeOn", 0 }, { "motionOn", 0 } }) }));
    show("Revolution", "Mix High-Pass Riser", "Revolution HIGH-PASS motion, smooth: the mix thins out and comes back - a riser without automation.", kMixdowns,
         one({}, { revo(1, 21, 1.0, { { "timeOn", 0 }, { "glitchOn", 0 }, { "motionTarget", 2 }, { "motionDepth", 0.7 }, { "motionSmooth", 0.8 } }) }));
    show("Revolution", "Mirror Destruction (M-G-T)", "Revolution DESTRUCTION in reversed order (MOTION, GLITCH, TIME) with high-pass motion, mix 60 %.", kMixdowns,
         one({}, { revo(3, 3, 0.6, { { "order", 5 }, { "motionTarget", 2 } }) }));
    // Standard recipes the library lacked
    {
        Design d = one({ lowCut(28, 24), bell(350, -3, 1.4) }, { punch(3, -1, 0.6) });
        d.layers.push_back({ "Smash", { lowCut(35, 24), bell(80, 2, 1.0), bell(4000, 3, 1.2) }, {}, { comp(-36, 10, 0.5, 60, 1, 0), mag(3, 8, 0.6) }, 0.0f, 0.3f });
        add(DB, "Kick", "Kick Parallel Smash", "Layers: the kick plus a crushed, saturated copy at 30 % - punch without losing the transient.", K, d);
    }
    {
        Design d = one({ lowCut(90, 18), bell(200, 1.5, 1.0) }, { punch(3, -1, 0.7) });
        d.layers.push_back({ "Smash", { lowCut(120, 18), highShelf(6000, 2) }, {}, { comp(-36, 10, 0.5, 80, 1, 0), verb(2, 0.7, 0, 0.3, 250) }, 0.0f, 0.3f });
        add(DB, "Snare", "Snare Parallel Smack", "Layers: a crushed snare with a short room under the dry one (New York style).", S, d);
    }
    add(DB, "Bass", "Sub Focus Mono", "Sub and low mids even: mono below 100 Hz, a dynamic shelf on the boom, a light Tube for small speakers.", B,
        one({ lowCut(25, 24), dyn(lowShelf(70, 0), -14, -4, 3, 20, 160), bell(180, -1.5, 1.0) }, { mag(2, 4, 0.3, { { "harmonics", 0.5 } }), comp(-20, 2.5, 20, 140, 2, 1), wide(1.0, 100) }));
    {
        Design d = one({ lowCut(90), bell(300, -2, 1.2), highShelf(11000, 2), deess() }, { comp(-24, 2.5, 10, 120, 2, 1) });
        d.layers.push_back({ "Crush", { lowCut(150), bell(3000, 2, 1.0) }, {}, { comp(-38, 10, 1, 80, 1, 0) }, 0.0f, 0.3f });
        add(VM, "LeadVocal", "Vocal Parallel Compression", "Layers: the vocal lightly compressed, plus a crushed copy at 30 % - dense and present, the dynamics still there.", kVox, d);
    }
    add(VM, "Guitar", "Acoustic Strum Even", "Strummed acoustic even and full: boom held by a dynamic point, gentle opto-like compression.", G,
        one({ lowCut(80), dyn(bell(180, 0, 1.0), -30, -4, 3, 10, 150), bell(2500, 1.5, 1.0), highShelf(11000, 1.5) }, { comp(-26, 2.5, 15, 150, 2, 1) }));
    add(VM, "Keys", "Upright Piano Intimate", "Close upright piano: warm low mids, felt-soft top, a small room.", KY,
        one({ lowCut(45), bell(250, 1, 1.0), highShelf(9000, -1.5) }, { comp(-26, 2, 20, 160, 2, 1), verb(1, 0.8, 10, 0.12, 150) }));
    add(VM, "Synth", "Synth Lead Double", "Layers: the lead plus a pitched short double pushed wide.", SY,
        plus(one({ lowCut(150), bell(2500, 1.5, 1.0) }, { comp(-22, 2, 10, 100) }),
             { "Double", { lowCut(200) }, {}, { echo(1, 0, 0.0, 1.0, { { "timeMs", 16 }, { "timeRMs", 24 }, { "link", 0 }, { "pitch", 0.1 } }), wide(1.6) }, 0.0f, 0.45f }));
    add(VM, "MixBus", "Mix Bus Low-End Focus", "Low end tidy on the mix: mono below 100 Hz, a dynamic shelf on the boom, the top untouched.", kMixdowns,
        one({ lowCut(20, 18), dyn(lowShelf(90, 0), -18, -2.5, 3, 20, 200) }, { comp(-16, 1.5, 30, 200, 2), wide(1.0, 100) }));
    // Mastering: a lo-fi group (beats for streaming playlists: softer, warm)
    {
        const std::vector<EqNodeState> eq { lowCut(25, 24), lowShelf(80, 1.0), highShelf(9000, -1.5) };
        v.push_back(master("Lo-Fi", "Lo-Fi Tape Master", "Lo-fi beats: tape 9.5 cm/s and a soft top, gentle glue.", -12.0,
                           one(eq, { comp(-16, 1.5, 30, 200, 2, 1, 10), mag(1, 4, 0.5, { { "speed", 1 } }), wall(-4, -1.0, 0) })));
        v.push_back(master("Lo-Fi", "Lo-Fi Vinyl Master", "Lo-fi beats from a record: Vinyl with light crackle, narrower, gentle glue.", -12.0,
                           one(eq, { comp(-16, 1.5, 30, 200, 2, 1, 10), mag(5, 3, 0.4, { { "crackle", 0.1 } }), wide(0.9, 120), wall(-4, -1.0, 0) })));
    }
}

std::vector<Preset> library()
{
    std::vector<Preset> v;
    drums(v);
    vocals(v);
    mastering(v);
    showcase(v);
    more(v);
    pass3(v);
    // keep every category (and group) together, in the order it first appears: the menu builds from runs
    std::map<juce::String, int> order;
    for (const auto& p : v)
        order.emplace(p.pack + "/" + p.category + "/" + p.group, static_cast<int>(order.size()));
    std::stable_sort(v.begin(), v.end(), [&](const Preset& a, const Preset& b)
    { return order[a.pack + "/" + a.category + "/" + a.group] < order[b.pack + "/" + b.category + "/" + b.group]; });
    return v;
}
} // namespace
} // namespace hpeq::lab

int runPresetLibrary(const juce::File& material, const juce::File& out, const juce::String& only, const juce::File& keep)
{
    return hpeq::lab::runPresetLibrary(material, out, only, keep);
}
