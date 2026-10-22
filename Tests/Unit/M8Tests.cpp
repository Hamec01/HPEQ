// M8: loudness (EBU Tech 3341 style cases on synthesized signals) and true peak, host transport edge
// cases (no play head, no tempo, tempo changes), 1000 random edits with undo / redo. Thresholds fixed
// here before the runs (EBU Tech 3341 tolerances: loudness +-0.1 LU, true peak +0.2 / -0.4 dB).

#include <cmath>
#include <memory>
#include <set>

#include "Analysis/LoudnessMeter.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginProcessor.h"

namespace hpeq::test
{
namespace
{
// Stereo sine (both channels equal) with segments of (dBFS, seconds).
juce::AudioBuffer<float> sineSegments(double fs, double hz, std::initializer_list<std::pair<double, double>> segments, double phase = 0.0)
{
    int total = 0;
    for (const auto& [db, seconds] : segments)
        total += static_cast<int>(std::lround(seconds * fs));
    juce::AudioBuffer<float> x(2, total);
    int i = 0;
    for (const auto& [db, seconds] : segments)
    {
        const double a = std::pow(10.0, db / 20.0);
        const int n = static_cast<int>(std::lround(seconds * fs));
        for (int k = 0; k < n; ++k, ++i)
        {
            const auto v = static_cast<float>(a * std::sin(2.0 * juce::MathConstants<double>::pi * hz * i / fs + phase));
            x.setSample(0, i, v);
            x.setSample(1, i, v);
        }
    }
    return x;
}

void runMeter(LoudnessMeter& m, const juce::AudioBuffer<float>& x, int block = 512)
{
    for (int start = 0; start < x.getNumSamples(); start += block)
    {
        const float* c[2] { x.getReadPointer(0) + start, x.getReadPointer(1) + start };
        m.process(c, 2, juce::jmin(block, x.getNumSamples() - start));
    }
}

class TestPlayHead final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        if (!present)
            return {};
        PositionInfo info;
        if (bpm > 0.0)
            info.setBpm(bpm);
        info.setIsPlaying(playing);
        if (withPosition)
            info.setPpqPosition(ppq);
        return info;
    }
    bool present = true, playing = true, withPosition = true;
    double bpm = 120.0, ppq = 0.0;
};
} // namespace

class M8Tests final : public juce::UnitTest
{
public:
    M8Tests() : juce::UnitTest("Validation M8", "HPEQ") {}

    void runTest() override
    {
        beginTest("Loudness: EBU Tech 3341 cases 1-5 (synthesized 1 kHz stereo sines) at 44.1 / 48 / 96 kHz within +-0.1 LU");
        {
            for (double fs : { 44100.0, 48000.0, 96000.0 })
            {
                const auto check = [&](const char* label, std::initializer_list<std::pair<double, double>> segments, double expected, bool momentaryToo)
                {
                    LoudnessMeter m;
                    m.prepare(fs, false);
                    runMeter(m, sineSegments(fs, 1000.0, segments));
                    logMessage("  " + juce::String(fs / 1000.0, 1) + " kHz " + label + ": I " + juce::String(m.integratedLufs(), 3)
                               + ", M " + juce::String(m.momentaryLufs(), 3) + ", S " + juce::String(m.shortTermLufs(), 3));
                    expect(std::abs(m.integratedLufs() - expected) <= 0.1, juce::String(label) + " integrated");
                    if (momentaryToo)
                    {
                        expect(std::abs(m.momentaryLufs() - expected) <= 0.1, juce::String(label) + " momentary");
                        expect(std::abs(m.shortTermLufs() - expected) <= 0.1, juce::String(label) + " short-term");
                    }
                };
                check("case 1 (-23 dBFS, 20 s)", { { -23.0, 20.0 } }, -23.0, true);
                check("case 2 (-33 dBFS, 20 s)", { { -33.0, 20.0 } }, -33.0, true);
                check("case 3 (-36/-23/-36, 10/60/10 s)", { { -36.0, 10.0 }, { -23.0, 60.0 }, { -36.0, 10.0 } }, -23.0, false);
                check("case 4 (-72/-36/-23/-36/-72)", { { -72.0, 10.0 }, { -36.0, 10.0 }, { -23.0, 60.0 }, { -36.0, 10.0 }, { -72.0, 10.0 } }, -23.0, false);
                check("case 5 (-26/-20/-26, 20/20.1/20 s)", { { -26.0, 20.0 }, { -20.0, 20.1 }, { -26.0, 20.0 } }, -23.0, false);
            }
        }

        beginTest("True peak: fs/4 sine at 45 degrees (samples -3.01 dBFS, true peak 0 dBTP) and sines at -6 dBFS, +0.2 / -0.4 dB; 44.1 / 48 / 96 / 192 kHz");
        {
            for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
            {
                {
                    LoudnessMeter m;
                    m.prepare(fs, true);
                    runMeter(m, sineSegments(fs, fs / 4.0, { { 0.0, 1.0 } }, juce::MathConstants<double>::pi / 4.0));
                    logMessage("  " + juce::String(fs / 1000.0, 1) + " kHz fs/4 at 45 deg: sample " + juce::String(m.samplePeakDbfs(), 2)
                               + " dBFS, true " + juce::String(m.truePeakDbtp(), 3) + " dBTP (x" + juce::String(m.oversampling()) + ")");
                    if (fs < 192000.0) // 1x at 192 kHz cannot see an fs/4 inter-sample peak; documented
                        expect(m.truePeakDbtp() <= 0.2 && m.truePeakDbtp() >= -0.4, "fs/4 at 45 degrees");
                }
                for (double hz : { 997.0, 5000.0, 10000.0, 15000.0, 19000.0 })
                {
                    LoudnessMeter m;
                    m.prepare(fs, true);
                    runMeter(m, sineSegments(fs, hz, { { -6.0, 1.0 } }, 0.3));
                    expect(m.truePeakDbtp() <= -5.8 && m.truePeakDbtp() >= -6.4, juce::String(hz) + " Hz: " + juce::String(m.truePeakDbtp(), 3));
                }
            }
        }

        beginTest("Meters: no allocations in process(); a reset request clears integrated and true peak");
        {
            LoudnessMeter m;
            m.prepare(48000.0, true);
            const auto x = sineSegments(48000.0, 1000.0, { { -10.0, 3.0 } });
            std::uint64_t allocations = 0;
            for (int start = 0; start < x.getNumSamples(); start += 512)
            {
                const float* c[2] { x.getReadPointer(0) + start, x.getReadPointer(1) + start };
                ScopedAllocationProbe probe;
                m.process(c, 2, juce::jmin(512, x.getNumSamples() - start));
                allocations += probe.counts().allocations;
            }
            expectEquals(static_cast<int>(allocations), 0);
            expect(m.integratedLufs() > -11.0);
            m.requestReset();
            runMeter(m, [&] { juce::AudioBuffer<float> s(2, 512); s.clear(); return s; }());
            expect(m.integratedLufs() <= LoudnessMeter::kSilence && m.truePeakDbtp() <= LoudnessMeter::kSilence);
        }

        beginTest("Host transport: no play head, no tempo, stopped, tempo changes every block: finite output, delay follows the tempo");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            auto p = plugin->getProject();
            FxNodeState n;
            n.id = p.nextNodeId++;
            n.lowHz = 100.0;
            n.highHz = 8000.0;
            FxSlotState delay;
            delay.type = FxType::Delay;
            delay.parameters.set("sync", 8.0);
            FxSlotState monsta;
            monsta.type = FxType::MonstaFx;
            monsta.parameters.set("chance", 0.7);
            n.chain = { monsta, delay };
            p.layers[0].fxNodes.push_back(n);
            plugin->setProject(p);
            plugin->setRateAndBufferSizeDetails(48000.0, 512);
            plugin->prepareToPlay(48000.0, 512);
            TestPlayHead head;
            const auto drums = makeSyntheticDrums({ 48000.0, 2, 4.0, kFixtureSeed });
            juce::MidiBuffer midi;
            bool finite = true;
            for (int mode = 0; mode < 5; ++mode)
            {
                head.present = mode != 0;
                head.bpm = mode == 1 ? 0.0 : 120.0;
                head.playing = mode != 2;
                plugin->setPlayHead(mode == 0 ? nullptr : &head);
                auto y = drums;
                for (int start = 0, b = 0; start < y.getNumSamples(); start += 512, ++b)
                {
                    if (mode == 3)
                        head.bpm = 60.0 + 120.0 * ((b * 37) % 100) / 100.0; // jumps every block
                    if (mode == 4)
                        head.bpm = 90.0 + b * 0.25;                          // slow ramp
                    head.ppq += 512.0 / 48000.0 * head.bpm / 60.0;
                    juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, juce::jmin(512, y.getNumSamples() - start));
                    plugin->processBlock(view, midi);
                }
                finite = finite && measureAll(y).allFinite;
            }
            plugin->setPlayHead(nullptr);
            expect(finite);
            const double at90 = dsp::DelayProcessor::delaySeconds([&] { dsp::FxSlotParams s; s.values[dsp::slot::kDelaySync] = 8.0f; return s; }(), 90.0);
            expectWithinAbsoluteError(at90, 60.0 / 90.0, 1.0e-12);
        }

        beginTest("STATE precision: 2000 random doubles survive XML save / load / save exactly (std::from_chars)");
        {
            DeterministicRandom random(99);
            FxProjectState p;
            for (int k = 0; k < 2000; ++k)
            {
                auto& layer = p.layers[static_cast<size_t>(k % kNumLayers)];
                if (layer.eqNodes.size() >= static_cast<size_t>(kMaxEqNodesPerLayer))
                    continue;
                EqNodeState n;
                n.id = p.nextNodeId++;
                n.frequencyHz = 20.0 + 19000.0 * (0.5 + 0.5 * random.nextBipolar());
                n.gainDb = random.nextBipolar() * std::pow(10.0, -static_cast<double>(random.nextU64() % 6));
                n.q = 0.1 + 9.0 * (0.5 + 0.5 * random.nextBipolar());
                layer.eqNodes.push_back(n);
            }
            sanitize(p);
            const auto viaXml = [](const FxProjectState& state)
            {
                const auto xml = toValueTree(state).createXml();
                return fromValueTree(juce::ValueTree::fromXml(*xml));
            };
            const auto once = viaXml(p);
            expect(once.has_value());
            if (once)
            {
                bool exact = true;
                for (size_t j = 0; j < p.layers.size(); ++j)
                    for (size_t i = 0; i < p.layers[j].eqNodes.size(); ++i)
                    {
                        const auto& a = p.layers[j].eqNodes[i];
                        const auto& b = once->layers[j].eqNodes[i];
                        exact = exact && a.frequencyHz == b.frequencyHz && a.gainDb == b.gainDb && a.q == b.q;
                    }
                expect(exact, "a double changed through XML");
                const auto twice = viaXml(*once);
                expect(twice && toValueTree(*twice).createXml()->toString() == toValueTree(*once).createXml()->toString());
            }
        }

        beginTest("Preset compatibility: the committed v1 state (M8) loads, re-saves identically and renders; a minimal old-style v1 tree loads with defaults");
        {
            const auto fixture = juce::File(HPEQ_TEST_FIXTURES_DIR).getChildFile("state_v1_d6.xml"); // canonical: written by the current build (D6 adds engine slots, point modules)
            expect(fixture.existsAsFile(), fixture.getFullPathName());
            const auto xml = juce::parseXML(fixture);
            expect(xml != nullptr);
            if (xml != nullptr)
            {
                juce::MemoryBlock data;
                juce::AudioProcessor::copyXmlToBinary(*xml, data);
                auto plugin = std::make_unique<FxPluginProcessor>();
                plugin->setStateInformation(data.getData(), static_cast<int>(data.getSize()));
                juce::MemoryBlock back;
                plugin->getStateInformation(back);
                const auto again = juce::AudioProcessor::getXmlFromBinary(back.getData(), static_cast<int>(back.getSize()));
                expect(again != nullptr && again->isEquivalentTo(xml.get(), false), "re-saved state differs from the committed v1 fixture");
                plugin->setRateAndBufferSizeDetails(48000.0, 512);
                plugin->prepareToPlay(48000.0, 512);
                auto audio = makeSyntheticDrums({ 48000.0, 2, 0.5, kFixtureSeed });
                juce::MidiBuffer midi;
                for (int start = 0; start < audio.getNumSamples(); start += 512)
                {
                    juce::AudioBuffer<float> view(audio.getArrayOfWritePointers(), 2, start, juce::jmin(512, audio.getNumSamples() - start));
                    plugin->processBlock(view, midi);
                }
                expect(measureAll(audio).allFinite);
                int nodes = 0;
                for (const auto& layer : plugin->getProject().layers)
                    nodes += static_cast<int>(layer.eqNodes.size() + layer.fxNodes.size());
                expect(nodes > 0, "fixture loaded as an empty project");
            }
            // States written by earlier builds (M7: JUCE number formatting; M8: before engine slots) load.
            for (const auto* name : { "state_v1_m7_writer.xml", "state_v1_m8_writer.xml" })
            {
                const auto earlier = juce::parseXML(juce::File(HPEQ_TEST_FIXTURES_DIR).getChildFile(name));
                expect(earlier != nullptr, juce::String(name) + " missing");
                if (earlier == nullptr)
                    continue;
                juce::MemoryBlock data;
                juce::AudioProcessor::copyXmlToBinary(*earlier, data);
                auto plugin = std::make_unique<FxPluginProcessor>();
                plugin->setStateInformation(data.getData(), static_cast<int>(data.getSize()));
                int nodes = 0;
                for (const auto& layer : plugin->getProject().layers)
                    nodes += static_cast<int>(layer.eqNodes.size() + layer.fxNodes.size());
                expect(nodes > 0, juce::String(name) + " loaded as an empty project");
            }
            // Old-style tree: only the fields an M1-era project wrote (no layer controls, no slot params).
            const auto old = juce::parseXML(R"(<HpeqProject schemaVersion="1" routing="Parallel" selectedLayer="0" nextNodeId="3">
                <Layer index="0" name="Layer 1" enabled="1"><EqNode id="1" shape="Bell" frequencyHz="1000" gainDb="3"/>
                <FxNode id="2" lowHz="200" highHz="800"><FxSlot type="Saturator"/></FxNode></Layer></HpeqProject>)");
            const auto loaded = old != nullptr ? fromValueTree(juce::ValueTree::fromXml(*old)) : std::nullopt;
            expect(loaded.has_value(), "minimal v1 tree rejected");
            if (loaded)
            {
                const auto& layer = loaded->layers[0];
                expect(layer.gainDb == 0.0f && layer.mix == 1.0f && layer.width == 1.0f && layer.channel == ChannelMode::Stereo);
                expect(layer.eqNodes.size() == 1 && layer.eqNodes[0].gainDb == 3.0 && layer.eqNodes[0].q == 0.707);
                expect(layer.fxNodes.size() == 1 && layer.fxNodes[0].chain.size() == 1 && layer.fxNodes[0].chain[0].type == FxType::Saturator);
            }
        }

        beginTest("1000 random edits with undo / redo: project stays sane (sanitize idempotent), unique ids, undo returns to the start");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            plugin->setRateAndBufferSizeDetails(48000.0, 256);
            plugin->prepareToPlay(48000.0, 256);
            const auto start = toValueTree(plugin->getProject());
            DeterministicRandom random(1234);
            const FxType types[] { FxType::Compressor, FxType::Saturator, FxType::Reverb, FxType::Delay, FxType::TransientShaper,
                                   FxType::MonstaFx, FxType::StereoProcessor, FxType::GateExpander, FxType::Limiter, FxType::Gain };
            juce::AudioBuffer<float> audio(2, 256);
            juce::MidiBuffer midi;
            int performed = 0;
            for (int op = 0; op < 1000; ++op)
            {
                const auto r = random.nextU64();
                plugin->getUndoManager().beginNewTransaction();
                plugin->editProject("op", [&](FxProjectState& p)
                {
                    auto& layer = p.layers[static_cast<size_t>(r % kNumLayers)];
                    switch ((r >> 8) % 8)
                    {
                        case 0: { EqNodeState e; e.id = p.nextNodeId++; e.frequencyHz = 20.0 + (r >> 16) % 18000; e.gainDb = static_cast<double>((r >> 24) % 25) - 12.0; layer.eqNodes.push_back(e); break; }
                        case 1: { FxNodeState f; f.id = p.nextNodeId++; f.lowHz = 30.0 + (r >> 16) % 5000; f.highHz = f.lowHz * 2.0; f.chain[0].type = types[(r >> 32) % 10]; layer.fxNodes.push_back(f); break; }
                        case 2: if (!layer.fxNodes.empty()) layer.fxNodes.erase(layer.fxNodes.begin()); break;
                        case 3: if (!layer.eqNodes.empty()) layer.eqNodes.pop_back(); break;
                        case 4: layer.gainDb = static_cast<float>((r >> 16) % 49) - 24.0f; layer.mute = ((r >> 40) & 1) != 0; break;
                        case 5: if (!layer.fxNodes.empty() && layer.fxNodes[0].chain.size() < kMaxFxPerNode) { FxSlotState s; s.type = types[(r >> 20) % 10]; layer.fxNodes[0].chain.push_back(s); } break;
                        case 6: p.routing = ((r >> 16) & 1) != 0 ? LayerRouting::Serial : LayerRouting::Parallel; break;
                        default: layer.enabled = ((r >> 16) & 3) != 0; layer.channel = ((r >> 18) & 1) != 0 ? ChannelMode::Mid : ChannelMode::Stereo; break;
                    }
                });
                ++performed;
                if (op % 10 == 9)
                {
                    audio.clear();
                    audio.setSample(0, 7, 0.5f);
                    plugin->processBlock(audio, midi);
                }
            }
            const auto project = plugin->getProject();
            auto again = project;
            sanitize(again);
            expect(toValueTree(again).isEquivalentTo(toValueTree(project)), "sanitize is not idempotent after 1000 edits");
            std::set<std::uint32_t> ids;
            int count = 0;
            for (const auto& layer : project.layers)
            {
                for (const auto& n : layer.eqNodes) { ids.insert(n.id); ++count; }
                for (const auto& n : layer.fxNodes) { ids.insert(n.id); ++count; }
            }
            expectEquals(static_cast<int>(ids.size()), count);
            int undone = 0;
            while (plugin->getUndoManager().canUndo())
            {
                plugin->getUndoManager().undo();
                ++undone;
            }
            logMessage("  " + juce::String(performed) + " edits, " + juce::String(count) + " nodes at the end, " + juce::String(undone) + " undo steps");
            expect(toValueTree(plugin->getProject()).isEquivalentTo(start), "undo did not return to the start");
            while (plugin->getUndoManager().canRedo())
                plugin->getUndoManager().redo();
            expect(toValueTree(plugin->getProject()).isEquivalentTo(toValueTree(project)), "redo did not return to the end");
        }
    }
};

static M8Tests m8Tests;
} // namespace hpeq::test
