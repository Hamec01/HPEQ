// Module pass 7: HamloZastava gate / expander (GATE_DESIGN.md §4).
// Thresholds fixed in the design note before the runs.

#include <cmath>
#include <memory>

#include "DSP/ProcessorsM6.h"
#include "DSP/ProjectEngine.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/HamloZastavaPanel.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;
constexpr double kTwoPi = 2.0 * juce::MathConstants<double>::pi;

struct Gt
{
    float threshold = -40.0f, ratio = 4.0f, range = 40.0f, attack = 0.5f, hold = 20.0f, release = 100.0f, hysteresis = 4.0f;
    int style = 0, look = 0;
    float hp = 0.0f, lp = 0.0f; // 0 = off (snapshot encoding)
    int listen = 0, flip = 0;
    float outputDb = 0.0f;
};

FxSlotParams gt(const Gt& g)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::GateExpander;
    s.values[slot::kGateThresholdDb] = g.threshold;
    s.values[slot::kGateRatio] = g.ratio;
    s.values[slot::kGateRangeDb] = g.range;
    s.values[slot::kGateAttackMs] = g.attack;
    s.values[slot::kGateHoldMs] = g.hold;
    s.values[slot::kGateReleaseMs] = g.release;
    s.values[slot::kGateHysteresisDb] = g.hysteresis;
    s.values[slot::kGateStyle] = static_cast<float>(g.style);
    s.values[slot::kGateLookahead] = static_cast<float>(g.look);
    s.values[slot::kGateKeyHpHz] = g.hp;
    s.values[slot::kGateKeyLpHz] = g.lp;
    s.values[slot::kGateKeyListen] = static_cast<float>(g.listen);
    s.values[slot::kGateFlip] = static_cast<float>(g.flip);
    s.values[slot::kGateOutputDb] = g.outputDb;
    return s;
}

int latencyOf(const FxSlotParams& p) { return GateProcessor::lookaheadSamples(p, kFs); }

juce::AudioBuffer<float> run(const juce::AudioBuffer<float>& x, const FxSlotParams& p, int block = 256, int* openings = nullptr)
{
    GateProcessor gate;
    gate.prepare(kFs);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        gate.process(c, 2, juce::jmin(block, y.getNumSamples() - start), p);
    }
    if (openings != nullptr)
        *openings = gate.zastavaActive() ? gate.zastava().meters().openings : static_cast<int>(gate.stateChanges());
    return y;
}

juce::AudioBuffer<float> silence(double seconds)
{
    juce::AudioBuffer<float> b(2, static_cast<int>(seconds * kFs));
    b.clear();
    return b;
}

void addHit(juce::AudioBuffer<float>& b, int start, double amp, double hz, double taMs, double tdMs)
{
    const int length = juce::jmin(b.getNumSamples() - start, static_cast<int>(8.0 * tdMs * 1.0e-3 * kFs));
    for (int i = 0; i < length; ++i)
    {
        const double t = i / kFs;
        const double v = amp * (1.0 - std::exp(-t / (taMs * 1.0e-3))) * std::exp(-t / (tdMs * 1.0e-3)) * std::sin(kTwoPi * hz * t);
        for (int ch = 0; ch < 2; ++ch)
            b.addSample(ch, start + i, static_cast<float>(v));
    }
}

double rmsDb(const juce::AudioBuffer<float>& b, int from, int length)
{
    double e = 0.0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = from; i < from + length; ++i)
            e += static_cast<double>(b.getSample(ch, i)) * b.getSample(ch, i);
    return 10.0 * std::log10(e / (2.0 * length) + 1.0e-30);
}

// The same RBJ Butterworth filters as the design note, computed here independently (double, direct form I).
struct Filter
{
    double b0, b1, b2, a1, a2, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    Filter(double hz, bool highPass)
    {
        const double w = kTwoPi * hz / kFs, cw = std::cos(w), alpha = std::sin(w) / std::sqrt(2.0), a0 = 1.0 + alpha;
        b0 = (highPass ? (1.0 + cw) : (1.0 - cw)) / 2.0 / a0;
        b1 = (highPass ? -(1.0 + cw) : (1.0 - cw)) / a0;
        b2 = b0;
        a1 = -2.0 * cw / a0;
        a2 = (1.0 - alpha) / a0;
    }
    double operator()(double x)
    {
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }
};
} // namespace

class GatePassTests final : public juce::UnitTest
{
public:
    GatePassTests() : juce::UnitTest("Gate pass", "HPEQ") {}

    void runTest() override
    {
        const auto drums = makeSyntheticDrums({ kFs, 2, 3.0, kFixtureSeed });

        beginTest("G1. Keys absent = M6 (latency 0); RANGE 0 in every mode and look-ahead = input delayed by L, bit-exact");
        {
            expect(!GateProcessor::usesZastava(gt({})));
            expectEquals(latencyOf(gt({})), 0);
            for (int style : { 0, 1, 2 })
                for (int lk : { 0, 1, 2 })
                {
                    Gt g;
                    g.style = style;
                    g.look = lk;
                    g.range = 0.0f;
                    g.hp = 300.0f;
                    g.outputDb = style == 0 && lk == 0 ? 0.0f : 0.0f;
                    const auto p = gt(g);
                    expect(GateProcessor::usesZastava(p));
                    expect(compareAligned(run(drums, p), drums, latencyOf(p)).maxAbsError == 0.0, "style " + juce::String(style) + " look " + juce::String(lk));
                }
        }

        beginTest("G2. Latency reported = measured; blocks 1 / 64 / 512 identical; 0 allocations");
        {
            const auto noise = makeWhiteNoise({ kFs, 2, 0.5, kFixtureSeed }, -10.0);
            for (int lk : { 0, 1, 2 })
            {
                Gt g;
                g.look = lk;
                g.style = 1;
                g.threshold = -60.0f;
                const auto p = gt(g);
                expectEquals(estimateDelay(run(noise, p), noise, 600), latencyOf(p));
            }
            for (int style : { 0, 1, 2 })
            {
                Gt g;
                g.style = style;
                g.look = 2;
                g.threshold = -24.0f;
                g.hp = 150.0f;
                g.lp = 9000.0f;
                g.outputDb = -1.0f;
                const auto p = gt(g);
                const auto ref = run(drums, p, 1);
                for (int block : { 64, 512 })
                    expect(compareAligned(run(drums, p, block), ref).bitExact, "style " + juce::String(style) + " block " + juce::String(block));
                GateProcessor gate;
                gate.prepare(kFs);
                auto y = drums;
                std::uint64_t allocations = 0;
                for (int start = 0; start < y.getNumSamples(); start += 512)
                {
                    float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    ScopedAllocationProbe probe;
                    gate.process(c, 2, juce::jmin(512, y.getNumSamples() - start), p);
                    allocations += probe.counts().allocations;
                }
                expectEquals(static_cast<int>(allocations), 0);
            }
        }

        beginTest("G3. Look-ahead: ZASTAVA range 40, attack 0.5 ms, hits from silence: gain on the first sample >= -0.92 dB (90 %) with VOSTOK, SOYUZ");
        {
            auto hits = silence(3.0);
            std::vector<int> onsets;
            for (int k = 0; k < 10; ++k)
            {
                onsets.push_back(static_cast<int>((0.1 + 0.27 * k) * kFs));
                addHit(hits, onsets.back(), 0.5, 2000.0 + 150.0 * k, 0.05, 30.0);
            }
            for (int lk : { 0, 1, 2 })
            {
                Gt g;
                g.look = lk;
                g.outputDb = lk == 0 ? -0.0001f : 0.0f; // SPUTNIK through the HamloZastava core too
                const auto p = gt(g);
                const int L = latencyOf(p);
                const auto y = run(hits, p);
                double worst = 0.0;
                for (int onset : onsets)
                {
                    int first = onset;
                    while (std::abs(hits.getSample(0, first)) < 0.005f)
                        ++first;
                    worst = std::min(worst, 20.0 * std::log10(std::abs(y.getSample(0, first + L) / hits.getSample(0, first))));
                }
                logMessage("  " + juce::String(lk == 0 ? "SPUTNIK" : lk == 1 ? "VOSTOK" : "SOYUZ") + ": worst gain on the first sample " + juce::String(worst, 2) + " dB");
                if (lk > 0)
                    expect(worst >= -0.92);
            }
        }

        beginTest("G4. Key filter: steady bass -6 dBFS + hats -20 dBFS, threshold -30: KEY HP 2 kHz opens only on the hats");
        {
            auto mix = silence(4.0);
            std::vector<int> hats;
            for (int i = 0; i < mix.getNumSamples(); ++i)
            {
                const auto v = static_cast<float>(0.5 * std::sin(kTwoPi * 60.0 * i / kFs));
                mix.setSample(0, i, v);
                mix.setSample(1, i, v);
            }
            for (int k = 0; k < 14; ++k)
            {
                hats.push_back(static_cast<int>((0.2 + 0.25 * k) * kFs));
                addHit(mix, hats.back(), 0.1, 8000.0, 0.05, 15.0);
            }
            Gt g;
            g.threshold = -30.0f;
            g.look = 1;
            int withFilter = 0, without = 0;
            g.hp = 2000.0f;
            run(mix, gt(g), 256, &withFilter);
            g.hp = 0.0f;
            const auto y = run(mix, gt(g), 256, &without);
            const double bassOnly = rmsDb(y, static_cast<int>(0.1 * kFs) + 96, static_cast<int>(0.08 * kFs)) - rmsDb(mix, static_cast<int>(0.1 * kFs), static_cast<int>(0.08 * kFs));
            logMessage("  openings with KEY HP 2 kHz: " + juce::String(withFilter) + " (hats " + juce::String(static_cast<int>(hats.size())) + "); without: "
                       + juce::String(without) + ", bass passes at " + juce::String(bassOnly, 2) + " dB");
            expectEquals(withFilter, static_cast<int>(hats.size()));
            expect(without <= 1 && std::abs(bassOnly) < 0.5); // without the filter the bass keeps it open
        }

        beginTest("G5. No chatter: 50 Hz sine slowly crossing the threshold (+-1 dB), every mode: <= 1 state change per second");
        {
            const int n = static_cast<int>(6.0 * kFs);
            juce::AudioBuffer<float> sine(2, n);
            for (int i = 0; i < n; ++i)
            {
                const double t = i / kFs;
                const double db = -30.0 + std::sin(kTwoPi * 0.25 * t); // +-1 dB around -30
                const auto v = static_cast<float>(std::pow(10.0, db / 20.0) * std::sin(kTwoPi * 50.0 * t));
                sine.setSample(0, i, v);
                sine.setSample(1, i, v);
            }
            for (int style : { 0, 1, 2 })
            {
                Gt g;
                g.style = style;
                g.threshold = -30.0f;
                g.look = 1;
                int openings = 0;
                run(sine, gt(g), 256, &openings);
                logMessage("  style " + juce::String(style) + ": " + juce::String(openings) + " openings in 6 s");
                expect(openings <= 6);
            }
        }

        beginTest("G6. TREVOGA range 12: during key hits the signal drops by 12 +-0.5 dB; quiet key: back to 0 +-0.1 dB");
        {
            auto x = silence(4.0);
            for (int i = 0; i < x.getNumSamples(); ++i)
            {
                const auto v = static_cast<float>(0.05 * std::sin(kTwoPi * 300.0 * i / kFs)); // -26 dBFS pad
                x.setSample(0, i, v);
                x.setSample(1, i, v);
            }
            std::vector<int> hits;
            for (int k = 0; k < 6; ++k)
            {
                hits.push_back(static_cast<int>((0.3 + 0.6 * k) * kFs));
                for (int i = 0; i < static_cast<int>(0.15 * kFs); ++i) // 150 ms bursts at -6 dBFS
                    for (int ch = 0; ch < 2; ++ch)
                        x.addSample(ch, hits.back() + i, static_cast<float>(0.5 * std::sin(kTwoPi * 1000.0 * i / kFs)));
            }
            Gt g;
            g.style = 2;
            g.range = 12.0f;
            g.threshold = -20.0f;
            g.look = 1;
            g.attack = 1.0f;
            g.release = 50.0f;
            g.hold = 10.0f;
            const auto p = gt(g);
            const auto y = run(x, p);
            const int L = latencyOf(p);
            double during = 0.0, after = 0.0;
            for (int h : hits)
            {
                const int a = h + static_cast<int>(0.05 * kFs), w = static_cast<int>(0.05 * kFs);
                during += (rmsDb(y, a + L, w) - rmsDb(x, a, w)) / static_cast<double>(hits.size());
                const int q = h + static_cast<int>(0.45 * kFs);
                after += (rmsDb(y, q + L, w) - rmsDb(x, q, w)) / static_cast<double>(hits.size());
            }
            logMessage("  during hits " + juce::String(during, 2) + " dB, after " + juce::String(after, 3) + " dB");
            expect(std::abs(during + 12.0) <= 0.5);
            expect(std::abs(after) <= 0.1);
        }

        beginTest("G7. FLIP: gate output + flip output = input delayed by L (<= 1e-6)");
        {
            for (int style : { 0, 1, 2 })
            {
                Gt g;
                g.style = style;
                g.look = 1;
                g.threshold = -24.0f;
                const auto gated = run(drums, gt(g));
                g.flip = 1;
                const auto flipped = run(drums, gt(g));
                const int L = latencyOf(gt(g));
                double worst = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i + L < drums.getNumSamples(); ++i)
                        worst = std::max(worst, static_cast<double>(std::abs(gated.getSample(ch, i + L) + flipped.getSample(ch, i + L) - drums.getSample(ch, i))));
                logMessage("  style " + juce::String(style) + ": worst |gate + flip - input| " + juce::String(worst));
                expect(worst <= 1.0e-6);
            }
        }

        beginTest("G8. PATRUL ratio 2, range 30: a steady tone 10 dB under the threshold is lowered by 10 +-0.5 dB");
        {
            const auto tone = makeSine({ kFs, 2, 2.0, kFixtureSeed }, 1000.0, -40.0);
            Gt g;
            g.style = 1;
            g.ratio = 2.0f;
            g.range = 30.0f;
            g.threshold = -30.0f;
            const auto y = run(tone, gt(g));
            const int from = static_cast<int>(1.0 * kFs), w = static_cast<int>(0.5 * kFs);
            const double change = rmsDb(y, from, w) - rmsDb(tone, from, w);
            logMessage("  change " + juce::String(change, 3) + " dB");
            expect(std::abs(change + 10.0) <= 0.5);
        }

        beginTest("G9. KEY LISTEN = the key filter's output (independent filter) delayed by L (<= 1e-5)");
        {
            Gt g;
            g.look = 2;
            g.hp = 500.0f;
            g.lp = 4000.0f;
            g.listen = 1;
            const auto p = gt(g);
            const auto y = run(drums, p);
            const int L = latencyOf(p);
            double worst = 0.0;
            for (int ch = 0; ch < 2; ++ch)
            {
                Filter h(500.0, true), l(4000.0, false);
                for (int i = 0; i + L < drums.getNumSamples(); ++i)
                    worst = std::max(worst, std::abs(y.getSample(ch, i + L) - l(h(drums.getSample(ch, i)))));
            }
            logMessage("  worst |listen - key| " + juce::String(worst));
            expect(worst <= 1.0e-5);
        }

        beginTest("G10. OUTPUT -6 / +6 dB = output x 10^(O/20) (1e-6 relative)");
        {
            Gt g;
            g.look = 1;
            g.threshold = -24.0f;
            const auto ref = run(drums, gt(g));
            for (float o : { -6.0f, 6.0f })
            {
                g.outputDb = o;
                const auto y = run(drums, gt(g));
                const double k = std::pow(10.0, o / 20.0);
                double worst = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < y.getNumSamples(); ++i)
                        worst = std::max(worst, std::abs(y.getSample(ch, i) - ref.getSample(ch, i) * k));
                expect(worst <= 1.0e-6 * std::max(1.0, k));
            }
        }

        beginTest("G11. Page in the real editor: an older gate shows the M6 defaults; knobs, MIX, levers edit the project; latency reaches the engine");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            FxNodeState n;
            n.id = 1;
            FxSlotState old;
            old.type = FxType::GateExpander; // saved before this pass: no HamloZastava keys
            old.parameters.set("thresholdDb", -30.0);
            n.chain = { old };
            p.layers[0].fxNodes = { n };
            p.nextNodeId = 2;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1400, 900);
            editor->selectNode(true, 1, 0);
            ui::HamloZastavaPanel* panel = nullptr;
            juce::Array<juce::Component*> stack { editor.get() };
            while (!stack.isEmpty())
            {
                auto* c = stack.removeAndReturn(stack.size() - 1);
                for (auto* child : c->getChildren())
                    stack.add(child);
                if (auto* w = dynamic_cast<ui::HamloZastavaPanel*>(c))
                    panel = w;
            }
            expect(panel != nullptr, "no HamloZastava panel on the gate page");
            if (panel != nullptr)
            {
                expect(panel->getWidth() > 1000 && panel->getHeight() > 500);
                const auto& v = panel->getValues();
                expect(v.knobs[ui::HamloZastavaPanel::Threshold] == -30.0 && v.knobs[ui::HamloZastavaPanel::Range] == 40.0
                       && v.knobs[ui::HamloZastavaPanel::KeyHp] == 20.0 && v.knobs[ui::HamloZastavaPanel::KeyLp] == 20000.0
                       && v.switches[ui::HamloZastavaPanel::Lookahead] == 0 && v.knobs[ui::HamloZastavaPanel::Mode] == 0.0);
                expect(!GateProcessor::usesZastava(makeSnapshot(plugin->getProject().layers[0]).fx[0].slots[0]));
                panel->onKnobChange(ui::HamloZastavaPanel::Mode, 2.0);
                panel->onKnobChange(ui::HamloZastavaPanel::Mix, 0.5);
                panel->onKnobChange(ui::HamloZastavaPanel::KeyHp, 800.0);
                panel->onSwitchChange(ui::HamloZastavaPanel::Lookahead, 2);
                panel->onSwitchChange(ui::HamloZastavaPanel::Flip, 1);
                const auto project = plugin->getProject(); // a copy: keep it alive while reading from it
                const auto& m = project.layers[0].fxNodes[0].chain[0];
                expectEquals(static_cast<double>(m.parameters["style"]), 2.0);
                expectEquals(static_cast<double>(m.mix), 0.5);
                expectEquals(static_cast<double>(m.parameters["keyHpHz"]), 800.0);
                expectEquals(static_cast<int>(m.parameters["lookahead"]), 2);
                expectEquals(static_cast<int>(m.parameters["flip"]), 1);
                expect(GateProcessor::usesZastava(makeSnapshot(project.layers[0]).fx[0].slots[0]));
                expectEquals(ProjectEngine::latencyFor(project, kFs), 240);
            }
            FxProjectState q;
            q.layers[0].mixChain = { defaultModule(FxType::GateExpander) };
            expectEquals(ProjectEngine::latencyFor(q, kFs), 96); // a gate added from the UI: VOSTOK
        }
    }
};

static GatePassTests gatePassTests;
} // namespace hpeq::test
