// Revolution presets (owner, 2026-10-26: "more presets for Revolution, only good ones"). Every HamloRevolution
// module preset must do what it says on a drum loop, at a sane level. Thresholds fixed before the runs:
//  - each engine the preset leaves on (mix > 0) has at least one step that is not PASS (MOTION: level > 0);
//  - no two presets make the same module (settings + pattern); a preset named after an action has it in its pattern;
//  - in the MIX chain on a 120 BPM drum loop (kick, snare, hats, a held tone), over bars 5-8: finite output,
//    RMS out - in within -6 .. +1.5 dB, sample peak out <= peak in + 3 dB, and the difference out - in at least
//    -24 dB under the input's RMS (the preset audibly changes the loop).

#include <array>
#include <cmath>
#include <memory>
#include <tuple>
#include <vector>

#include "Core/ModulePresets.h"
#include "Core/Revolution.h"
#include "Plugin/FxPluginProcessor.h"
#include "Plugin/Presets.h"

namespace hpeq::test
{
namespace
{
constexpr double kFs = 48000.0;
constexpr int kBeat = 24000; // 120 BPM
constexpr int kBars = 8, kLength = kBars * 4 * kBeat;

// Kick on every beat, snare on 2 and 4, hats on the eighths (alternating sides), a quiet held tone under them
// (seeded, fixed).
juce::AudioBuffer<float> drumLoop()
{
    juce::AudioBuffer<float> x(2, kLength);
    x.clear();
    juce::Random rng(11);
    for (int i = 0; i < kLength; ++i)
    {
        const int inBeat = i % kBeat, beat = (i / kBeat) % 4, inEighth = i % (kBeat / 2);
        const double tb = inBeat / kFs, te = inEighth / kFs, t = i / kFs;
        double v = 0.5 * std::exp(-tb / 0.08) * std::sin(2.0 * juce::MathConstants<double>::pi * (50.0 + 80.0 * std::exp(-tb / 0.01)) * tb);
        const double noise = 2.0 * rng.nextDouble() - 1.0;
        if (beat == 1 || beat == 3)
            v += 0.3 * std::exp(-tb / 0.06) * noise;
        // the hats alternate left / right and the tone is a little wider on the right: a stereo loop (WIDTH needs a side)
        const double hat = 0.08 * std::exp(-te / 0.01) * noise, side = ((i / (kBeat / 2)) % 2 == 0) ? 1.0 : -1.0;
        const double tone = 0.05 * std::sin(2.0 * juce::MathConstants<double>::pi * 220.0 * t), toneR = 0.05 * std::sin(2.0 * juce::MathConstants<double>::pi * 221.5 * t);
        x.setSample(0, i, static_cast<float>(v + hat * (side > 0 ? 1.0 : 0.3) + tone));
        x.setSample(1, i, static_cast<float>(v + hat * (side > 0 ? 0.3 : 1.0) + toneR));
    }
    return x;
}

struct Result
{
    bool finite = true;
    double levelDb = 0, peakDb = 0, diffDb = 0;
};

Result measure(const FxSlotState& module, const juce::AudioBuffer<float>& x)
{
    auto plugin = std::make_unique<FxPluginProcessor>();
    FxProjectState project;
    project.layers[0].mixChain = { module };
    plugin->setProject(project);
    plugin->prepareToPlay(kFs, 512);
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> block(2, 512);
    double in2 = 0, out2 = 0, d2 = 0, inPeak = 0, outPeak = 0;
    Result r;
    for (int s = 0; s + 512 <= x.getNumSamples(); s += 512)
    {
        for (int ch = 0; ch < 2; ++ch)
            block.copyFrom(ch, 0, x, ch, s, 512);
        plugin->processBlock(block, midi);
        if (s < kLength / 2)
            continue;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
            {
                const double a = x.getSample(ch, s + i), b = block.getSample(ch, i);
                r.finite = r.finite && std::isfinite(b);
                in2 += a * a;
                out2 += b * b;
                d2 += (b - a) * (b - a);
                inPeak = std::max(inPeak, std::abs(a));
                outPeak = std::max(outPeak, std::abs(b));
            }
    }
    r.levelDb = 10.0 * std::log10((out2 + 1.0e-30) / in2);
    r.peakDb = 20.0 * std::log10((outPeak + 1.0e-30) / inPeak);
    r.diffDb = 10.0 * std::log10((d2 + 1.0e-30) / in2);
    return r;
}
} // namespace

class RevolutionPresetTests final : public juce::UnitTest
{
public:
    RevolutionPresetTests() : juce::UnitTest("Revolution presets", "HPEQ") {}

    void runTest() override
    {
        namespace rev = hpeq::revolution;
        const auto& list = presets::factoryModulePresets(FxType::MonstaFx);
        beginTest("Revolution presets: every engine left on has events; no two presets make the same module");
        {
            const char* names[rev::kEngines] { "time", "glitch", "motion" };
            for (const auto& preset : list)
            {
                const auto m = presets::makeModule(FxType::MonstaFx, preset);
                const auto pattern = rev::readPattern(m.parameters);
                juce::String counts;
                for (int e = 0; e < rev::kEngines; ++e)
                {
                    const juce::String key(names[e]);
                    const bool on = static_cast<int>(m.parameters[key + "On"]) != 0 && static_cast<double>(m.parameters[key + "Mix"]) > 0.0;
                    int events = 0;
                    for (auto v : pattern.steps[static_cast<size_t>(e)])
                        events += v != 0 ? 1 : 0;
                    counts << " " << key << (on ? " " + juce::String(events) : juce::String(" off"));
                    if (on)
                        expect(events > 0, juce::String(preset.name) + ": " + key + " is on with an empty lane");
                }
                logMessage("  " + juce::String(preset.name) + ":" + counts);
            }
            // the action a preset's name promises is in its pattern (TIME: 1 Half, 2 Reverse, 3 Freeze, 4 Tape stop,
            // 5 2X; GLITCH: 2 Stutter, 3 Ratchet, 5 Gap, 7 Scratch)
            const std::vector<std::tuple<const char*, int, int>> promised { { "Tape Stop Drops", rev::Time, rev::TapeStop }, { "Half-Time Vocal (Clean)", rev::Time, rev::Half },
                { "Reverse Tails", rev::Time, rev::TimeReverse }, { "Double-Time Rush", rev::Time, rev::Double }, { "Freeze Pads", rev::Time, rev::Freeze },
                { "Trap Hat Ratchets", rev::Glitch, rev::Ratchet }, { "Triplet Stutter", rev::Glitch, rev::Stutter }, { "Scratch DJ", rev::Glitch, rev::Scratch },
                { "Gap Chopper", rev::Glitch, rev::Gap } };
            for (const auto& [name, engine, action] : promised)
            {
                int found = 0;
                for (const auto& preset : list)
                    if (juce::String(preset.name) == name)
                        for (auto v : rev::readPattern(presets::makeModule(FxType::MonstaFx, preset).parameters).steps[static_cast<size_t>(engine)])
                            found += v == action ? 1 : 0;
                expect(found > 0, juce::String(name) + ": the pattern has no " + rev::valueName(engine, action));
            }
            for (size_t a = 0; a < list.size(); ++a)
                for (size_t b = a + 1; b < list.size(); ++b)
                    expect(!moduleToTree(presets::makeModule(FxType::MonstaFx, list[a])).isEquivalentTo(moduleToTree(presets::makeModule(FxType::MonstaFx, list[b]))),
                           juce::String(list[a].name) + " = " + list[b].name);
        }

        beginTest("Revolution presets on a 120 BPM drum loop: finite; level -6 .. +1.5 dB; peak <= input + 3 dB; changes the loop (difference >= -24 dB)");
        {
            const auto x = drumLoop();
            int bad = 0;
            for (const auto& preset : list)
            {
                const auto r = measure(presets::makeModule(FxType::MonstaFx, preset), x);
                const bool ok = r.finite && r.levelDb >= -6.0 && r.levelDb <= 1.5 && r.peakDb <= 3.0 && r.diffDb >= -24.0;
                bad += ok ? 0 : 1;
                logMessage("  " + juce::String(preset.name) + ": level " + juce::String(r.levelDb, 2) + " dB, peak " + juce::String(r.peakDb, 2) + " dB, difference "
                           + juce::String(r.diffDb, 1) + " dB" + (r.finite ? "" : " NOT FINITE") + (ok ? "" : "  FAIL"));
            }
            logMessage("  " + juce::String(static_cast<int>(list.size())) + " Revolution presets, " + juce::String(bad) + " outside the thresholds");
            expectEquals(bad, 0);
        }

        // presets pass 3: the factory presets that carry HamloRevolution (level-matched by HPEQ_Lab on material)
        beginTest("Factory presets with Revolution: every engine left on has events; a preset named after an action has it in its pattern");
        {
            const std::vector<std::tuple<const char*, int, int>> promised { { "Hat Ratchet Rolls", rev::Glitch, rev::Ratchet }, { "Perc Triplet Stutter", rev::Glitch, rev::Stutter },
                { "808 Tape Stop Drops", rev::Time, rev::TapeStop }, { "Vocal Half-Time Hook", rev::Time, rev::Half }, { "Backing Reverse Swells", rev::Time, rev::TimeReverse },
                { "Guitar Freeze Filter", rev::Time, rev::Freeze }, { "Vocal Tape Stop Ends", rev::Time, rev::TapeStop }, { "Vocal Reverse Tails", rev::Time, rev::TimeReverse },
                { "Scratch Break", rev::Glitch, rev::Scratch }, { "Double-Time Drum Rush", rev::Time, rev::Double }, { "Mix Gap Chopper", rev::Glitch, rev::Gap } };
            const char* names[rev::kEngines] { "time", "glitch", "motion" };
            int withRevolution = 0, found = 0;
            for (const auto& p : presets::factoryPresets())
            {
                FxPluginProcessor plugin;
                if (!plugin.loadPreset(p.data, p.size, p.name))
                    continue;
                std::vector<FxSlotState> modules;
                for (const auto& layer : plugin.getProject().layers)
                    for (const auto& m : layer.mixChain)
                        if (m.type == FxType::MonstaFx)
                            modules.push_back(m);
                if (modules.empty())
                    continue;
                ++withRevolution;
                std::array<int, rev::kEngines * 8> counts {};
                for (const auto& m : modules)
                {
                    const auto pattern = rev::readPattern(m.parameters);
                    for (int e = 0; e < rev::kEngines; ++e)
                    {
                        const juce::String key(names[e]);
                        int events = 0;
                        for (auto v : pattern.steps[static_cast<size_t>(e)])
                        {
                            events += v != 0 ? 1 : 0;
                            if (v < 8)
                                ++counts[static_cast<size_t>(e * 8 + v)];
                        }
                        if (static_cast<int>(m.parameters[key + "On"]) != 0 && static_cast<double>(m.parameters[key + "Mix"]) > 0.0)
                            expect(events > 0, p.name + ": " + key + " is on with an empty lane");
                    }
                }
                for (const auto& [name, engine, action] : promised)
                    if (p.name == name)
                    {
                        ++found;
                        expect(counts[static_cast<size_t>(engine * 8 + action)] > 0, p.name + ": the pattern has no " + rev::valueName(engine, action));
                    }
            }
            logMessage("  " + juce::String(withRevolution) + " factory presets carry Revolution; " + juce::String(found) + " named after an action");
            expectEquals(found, static_cast<int>(promised.size()));
        }
    }
};

static RevolutionPresetTests revolutionPresetTests;
} // namespace hpeq::test
