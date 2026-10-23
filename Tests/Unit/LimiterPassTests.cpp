// Module pass 5: HamloRedWall limiter (LIMITER_DESIGN.md §5) and modules on the whole layer (D7).
// Thresholds fixed in the design note before the runs.

#include <cmath>
#include <memory>

#include "Analysis/LoudnessMeter.h"
#include "DSP/ProcessorsM6.h"
#include "DSP/ProjectEngine.h"
#include "Harness/AudioMetrics.h"
#include "Harness/RtAllocationProbe.h"
#include "Harness/TestSignalFactory.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/HamloRedWallPanel.h"
#include "UI/NodeList.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;

FxSlotParams lim(float ceilingDb, float releaseMs, float lookMs, float threshold = 0, float truePeak = 0, float autoRelease = 0, float style = 0,
                 float link = 1, float monitor = 0)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::Limiter;
    s.values[slot::kCeilingDb] = ceilingDb;
    s.values[slot::kLimiterReleaseMs] = releaseMs;
    s.values[slot::kLookaheadMs] = lookMs;
    s.values[slot::kLimThresholdDb] = threshold;
    s.values[slot::kLimTruePeak] = truePeak;
    s.values[slot::kLimAutoRelease] = autoRelease;
    s.values[slot::kLimStyle] = style;
    s.values[slot::kLimLink] = link;
    s.values[slot::kLimMonitor] = monitor;
    return s;
}

juce::AudioBuffer<float> run(const juce::AudioBuffer<float>& x, const FxSlotParams& p, int block = 256, std::vector<float>* gr = nullptr)
{
    LimiterProcessor l;
    l.prepare(kFs);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        l.process(c, 2, juce::jmin(block, y.getNumSamples() - start), p);
        if (gr != nullptr)
            gr->push_back(l.lastGainReductionDb());
    }
    return y;
}

juce::AudioBuffer<float> segment(const juce::AudioBuffer<float>& x, int start, int length)
{
    juce::AudioBuffer<float> y(x.getNumChannels(), length);
    for (int ch = 0; ch < x.getNumChannels(); ++ch)
        y.copyFrom(ch, 0, x, ch, start, length);
    return y;
}

// Independent 8x inter-sample peak (64-tap windowed sinc per phase), dBFS.
double interSamplePeakDb(const juce::AudioBuffer<float>& y, int from)
{
    double peak = 0.0;
    constexpr int half = 32;
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
        for (int n = from + half; n < y.getNumSamples() - half; ++n)
            for (int p = 0; p < 8; ++p)
            {
                const double f = p / 8.0;
                double s = 0.0;
                for (int k = -half + 1; k <= half; ++k)
                {
                    const double d = k - f;
                    const double sinc = std::abs(d) < 1e-12 ? 1.0 : std::sin(juce::MathConstants<double>::pi * d) / (juce::MathConstants<double>::pi * d);
                    const double w = 0.5 + 0.5 * std::cos(juce::MathConstants<double>::pi * d / (half + 1));
                    s += y.getSample(ch, n + k) * sinc * w;
                }
                peak = std::max(peak, std::abs(s));
            }
    return 20.0 * std::log10(peak);
}

double integratedLufs(const juce::AudioBuffer<float>& y)
{
    LoudnessMeter m;
    m.prepare(kFs, false);
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        const float* c[2] { y.getReadPointer(0) + start, y.getReadPointer(1) + start };
        m.process(c, 2, juce::jmin(512, y.getNumSamples() - start));
    }
    return m.integratedLufs();
}
} // namespace

class LimiterPassTests final : public juce::UnitTest
{
public:
    LimiterPassTests() : juce::UnitTest("Limiter pass", "HPEQ") {}

    void runTest() override
    {
        const auto drums = makeSyntheticDrums({ kFs, 2, 4.0, kFixtureSeed });
        auto hot = drums;
        hot.applyGain(static_cast<float>(std::pow(10.0, 18.0 / 20.0)));

        beginTest("1. All options at their defaults = the M6 limiter (usesRedWall false, latency unchanged)");
        {
            const auto p = lim(-1, 80, 2);
            expect(!LimiterProcessor::usesRedWall(p));
            expectEquals(LimiterProcessor::lookaheadSamples(p, kFs), 96);
            auto tp = p;
            tp.values[slot::kLimTruePeak] = 1;
            expectEquals(LimiterProcessor::lookaheadSamples(tp, kFs), 96 + 8);
        }

        beginTest("2. Ceiling: output sample peak <= ceiling for every style, link, release, threshold, true peak (drums +18 dB)");
        {
            double worst = -100.0;
            for (float style : { 0.0f, 1.0f, 2.0f })
                for (float link : { 0.0f, 0.5f, 1.0f })
                    for (float th : { 0.0f, -6.0f, -12.0f })
                        for (float tp : { 0.0f, 1.0f })
                            for (float autoRel : { 0.0f, 1.0f })
                            {
                                const auto y = run(hot, lim(-1, 60, 2, th, tp, autoRel, style, link));
                                const double over = measureAll(y).peakDbfs + 1.0;
                                worst = std::max(worst, over);
                                if (over > 1.0e-4)
                                    expect(false, "style " + juce::String(style) + " link " + juce::String(link) + " th " + juce::String(th) + " tp "
                                                      + juce::String(tp) + " auto " + juce::String(autoRel) + ": over by " + juce::String(over, 5) + " dB");
                            }
            logMessage("  worst sample peak over the ceiling: " + juce::String(worst, 5) + " dB (108 settings)");
        }

        beginTest("3. True peak: with TRUE PEAK the output inter-sample peaks stay <= ceiling + 0.3 dB; without, the same signal overshoots > 0.5 dB");
        {
            // fs/4 tones at 45 degrees and drums with sharp highs: sample peaks well below their true peaks
            const int n = static_cast<int>(kFs);
            juce::AudioBuffer<float> isp(2, n);
            for (int i = 0; i < n; ++i)
            {
                const float v = static_cast<float>(1.6 * std::sin(juce::MathConstants<double>::pi * 0.5 * i + juce::MathConstants<double>::pi / 4.0)
                                                   * (0.6 + 0.4 * std::sin(2.0 * juce::MathConstants<double>::pi * 3.0 * i / kFs)));
                isp.setSample(0, i, v);
                isp.setSample(1, i, v);
            }
            const double off = interSamplePeakDb(run(isp, lim(-1, 60, 2)), 4800) + 1.0;
            const double on = interSamplePeakDb(run(isp, lim(-1, 60, 2, 0, 1)), 4800) + 1.0;
            logMessage("  inter-sample peak over the ceiling: true peak off " + juce::String(off, 2) + " dB, on " + juce::String(on, 2) + " dB");
            expect(off > 0.5);
            expect(on <= 0.3);
        }

        beginTest("4. Threshold -6 dB on drums: integrated loudness +3 LU or more at the same ceiling");
        {
            const auto base = run(drums, lim(-1, 60, 2, 0, 0, 1));
            const auto pushed = run(drums, lim(-1, 60, 2, -6, 0, 1));
            const double gain = integratedLufs(pushed) - integratedLufs(base);
            logMessage("  loudness: threshold 0 " + juce::String(integratedLufs(base), 2) + " LUFS, -6 dB " + juce::String(integratedLufs(pushed), 2)
                       + " LUFS (+" + juce::String(gain, 2) + " LU), peak " + juce::String(measureAll(pushed).peakDbfs, 2) + " dBFS");
            expect(gain >= 3.0);
        }

        beginTest("5. Auto release on a bass line: gain-reduction modulation (std over 1 s) at least 20 % lower than the fixed release");
        {
            const int n = static_cast<int>(6.0 * kFs);
            juce::AudioBuffer<float> bass(2, n);
            for (int i = 0; i < n; ++i)
            {
                const double t = i / kFs;
                const double env = 0.5 + 0.5 * std::abs(std::sin(juce::MathConstants<double>::pi * 2.0 * t)); // notes every 0.5 s
                const float v = static_cast<float>(1.4 * env * std::sin(2.0 * juce::MathConstants<double>::pi * 55.0 * t));
                bass.setSample(0, i, v);
                bass.setSample(1, i, v);
            }
            const auto modulation = [&](float autoRel)
            {
                std::vector<float> gr;
                run(bass, lim(-6, 40, 2, 0, 0, autoRel), 64, &gr);
                // std of GR over the steady part (64-sample blocks), mean over 1 s windows
                const int perSecond = static_cast<int>(kFs / 64.0);
                double sum = 0.0;
                int windows = 0;
                for (int w = 2; w + 1 <= static_cast<int>(gr.size()) / perSecond; ++w, ++windows)
                {
                    double m = 0.0, v = 0.0;
                    for (int k = 0; k < perSecond; ++k)
                        m += gr[static_cast<size_t>(w * perSecond + k)];
                    m /= perSecond;
                    for (int k = 0; k < perSecond; ++k)
                        v += std::pow(gr[static_cast<size_t>(w * perSecond + k)] - m, 2.0);
                    sum += std::sqrt(v / perSecond);
                }
                return sum / std::max(1, windows);
            };
            const double fixed = modulation(0), autoMod = modulation(1);
            for (float rel : { 120.0f, 400.0f }) // diagnostic: what longer fixed releases give on this metric
            {
                std::vector<float> gr;
                run(bass, lim(-6, rel, 2, 0, 0, 0), 64, &gr);
                double mean = 0.0, var = 0.0;
                const int from = static_cast<int>(gr.size() / 3);
                for (int k = from; k < static_cast<int>(gr.size()); ++k) mean += gr[static_cast<size_t>(k)];
                mean /= static_cast<double>(gr.size() - static_cast<size_t>(from));
                for (int k = from; k < static_cast<int>(gr.size()); ++k) var += std::pow(gr[static_cast<size_t>(k)] - mean, 2.0);
                logMessage("    diagnostic fixed release " + juce::String(rel) + " ms: GR std " + juce::String(std::sqrt(var / static_cast<double>(gr.size() - static_cast<size_t>(from))), 3) + " dB");
            }
            logMessage("  GR modulation: fixed " + juce::String(fixed, 3) + " dB, auto " + juce::String(autoMod, 3) + " dB");
            expect(autoMod <= 0.8 * fixed);
        }

        beginTest("6. STAKHANOV: samples ducked by more than 0.5 dB before a single transient are <= 30 % of GLASNOST's");
        {
            const int n = static_cast<int>(0.2 * kFs), at = static_cast<int>(0.1 * kFs);
            juce::AudioBuffer<float> x(2, n);
            for (int i = 0; i < n; ++i)
            {
                const float v = 0.2f * static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * 300.0 * i / kFs)) + (i == at ? 2.0f : 0.0f);
                x.setSample(0, i, v);
                x.setSample(1, i, v);
            }
            const auto ducked = [&](float style)
            {
                const auto p = lim(-1, 60, 5, 0, 0, 0, style);
                const int latency = LimiterProcessor::lookaheadSamples(p, kFs);
                const auto y = run(x, p);
                int count = 0;
                for (int i = at - latency; i < at; ++i) // output samples before the transient's own output time
                {
                    const double in = x.getSample(0, i), out = y.getSample(0, i + latency);
                    if (std::abs(in) > 1e-4 && 20.0 * std::log10(std::abs(out) / std::abs(in)) < -0.5)
                        ++count;
                }
                return count;
            };
            const int glasnost = ducked(0), stakhanov = ducked(1);
            logMessage("  ducked samples before the transient: GLASNOST " + juce::String(glasnost) + ", STAKHANOV " + juce::String(stakhanov));
            expect(glasnost > 0);
            expect(stakhanov <= 0.3 * glasnost);
        }

        beginTest("7. OCTOBER: louder than GLASNOST by >= 1 LU at the same settings, with more 3rd harmonic");
        {
            const auto g = run(drums, lim(-1, 60, 2, -9, 0, 0, 0)), o = run(drums, lim(-1, 60, 2, -9, 0, 0, 2));
            const double d = integratedLufs(o) - integratedLufs(g);
            logMessage("  OCTOBER vs GLASNOST: +" + juce::String(d, 2) + " LU");
            expect(d >= 1.0);
            const auto sine = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 997.0, -3.0);
            const auto third = [&](float style)
            {
                const auto y = run(sine, lim(-6, 60, 2, -6, 0, 0, style));
                double f1r = 0, f1i = 0, f3r = 0, f3i = 0;
                for (int i = 24000; i < 48000; ++i)
                {
                    const double v = y.getSample(0, i), t = 2.0 * juce::MathConstants<double>::pi * 997.0 * i / kFs;
                    f1r += v * std::cos(t); f1i += v * std::sin(t); f3r += v * std::cos(3 * t); f3i += v * std::sin(3 * t);
                }
                return 10.0 * std::log10((f3r * f3r + f3i * f3i + 1e-30) / (f1r * f1r + f1i * f1i));
            };
            const double tg = third(0), to = third(2);
            logMessage("  3rd harmonic: GLASNOST " + juce::String(tg, 1) + " dB, OCTOBER " + juce::String(to, 1) + " dB");
            expect(to > tg);
        }

        beginTest("8. LINK 0: a left-only burst leaves the right channel untouched (exact, delayed); LINK 1 ducks both");
        {
            const int n = static_cast<int>(0.5 * kFs);
            juce::AudioBuffer<float> x(2, n);
            for (int i = 0; i < n; ++i)
            {
                x.setSample(0, i, (i > 4800 && i < 9600 ? 3.0f : 0.3f) * static_cast<float>(std::sin(0.05 * i)));
                x.setSample(1, i, 0.3f * static_cast<float>(std::sin(0.031 * i)));
            }
            const auto p0 = lim(-1, 60, 2, 0, 0, 0, 0, 0), p1 = lim(-1, 60, 2, 0, 0, 0, 0, 1);
            const int latency = LimiterProcessor::lookaheadSamples(p0, kFs);
            const auto y0 = run(x, p0), y1 = run(x, p1);
            bool rightExact = true;
            double rightDuck = 0.0;
            for (int i = 0; i + latency < n; ++i)
            {
                rightExact = rightExact && y0.getSample(1, i + latency) == x.getSample(1, i);
                if (std::abs(x.getSample(1, i)) > 0.1f)
                    rightDuck = std::max(rightDuck, -20.0 * std::log10(std::abs(y1.getSample(1, i + latency)) / std::abs(x.getSample(1, i))));
            }
            logMessage("  link 1: right channel ducked up to " + juce::String(rightDuck, 1) + " dB");
            expect(rightExact, "link 0 changed the right channel");
            expect(rightDuck > 6.0);
        }

        beginTest("9. MONITOR: UNITY keeps small signals at the input level (+-0.5 dB); DELTA = delayed core input - output");
        {
            const auto quiet = makeSine({ kFs, 2, 1.0, kFixtureSeed }, 500.0, -30.0);
            const auto u = run(quiet, lim(-1, 60, 2, -12, 0, 0, 0, 1, 1));
            const double level = measureAll(segment(u, 9600, 24000)).rmsDbfs - measureAll(segment(quiet, 9600, 24000)).rmsDbfs;
            logMessage("  UNITY level change " + juce::String(level, 3) + " dB");
            expect(std::abs(level) <= 0.5);
            const auto normal = run(hot, lim(-1, 60, 2, -6, 0, 0, 0, 1, 0));
            const auto delta = run(hot, lim(-1, 60, 2, -6, 0, 0, 0, 1, 2));
            const int latency = LimiterProcessor::lookaheadSamples(lim(-1, 60, 2), kFs);
            const float push = static_cast<float>(std::pow(10.0, 6.0 / 20.0));
            double worst = 0.0;
            for (int i = 0; i + latency < hot.getNumSamples(); ++i)
                worst = std::max(worst, std::abs(static_cast<double>(delta.getSample(0, i + latency))
                                                 - (hot.getSample(0, i) * push - normal.getSample(0, i + latency))));
            expect(worst < 1.0e-5, "delta differs: " + juce::String(worst));
        }

        beginTest("10. Latency reported = measured (true peak on/off, styles); blocks 1/64/512 identical; 0 allocations");
        {
            const auto quiet = makeWhiteNoise({ kFs, 2, 0.5, kFixtureSeed }, -40.0);
            for (float tp : { 0.0f, 1.0f })
                for (float style : { 0.0f, 1.0f })
                {
                    const auto p = lim(0, 60, 3, 0, tp, 0, style, 0.999f); // link 0.999: the HamloRedWall path
                    expectEquals(estimateDelay(run(quiet, p), quiet, 600), LimiterProcessor::lookaheadSamples(p, kFs));
                }
            const auto p = lim(-1, 60, 2, -6, 1, 1, 2, 0.7f);
            const auto ref = run(hot, p, 1);
            for (int block : { 64, 512 })
                expect(compareAligned(run(hot, p, block), ref).bitExact, "block " + juce::String(block));
            LimiterProcessor l;
            l.prepare(kFs);
            auto y = hot;
            std::uint64_t allocations = 0;
            for (int start = 0; start < y.getNumSamples(); start += 512)
            {
                float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                ScopedAllocationProbe probe;
                l.process(c, 2, juce::jmin(512, y.getNumSamples() - start), p);
                allocations += probe.counts().allocations;
            }
            expectEquals(static_cast<int>(allocations), 0);
        }

        beginTest("12. HamloRedWall page (real editor): fills the body; knobs and levers edit the project; an older limiter shows the M6 defaults");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            FxNodeState n;
            n.id = 1;
            FxSlotState old;
            old.type = FxType::Limiter; // a limiter saved before this pass: no HamloRedWall keys
            old.parameters.set("ceilingDb", -3.0);
            n.chain = { old };
            p.layers[0].fxNodes = { n };
            p.nextNodeId = 2;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1400, 900);
            editor->selectNode(true, 1, 0);
            ui::HamloRedWallPanel* panel = nullptr;
            juce::Array<juce::Component*> stack { editor.get() };
            while (!stack.isEmpty())
            {
                auto* c = stack.removeAndReturn(stack.size() - 1);
                for (auto* child : c->getChildren())
                    stack.add(child);
                if (auto* w = dynamic_cast<ui::HamloRedWallPanel*>(c))
                    panel = w;
            }
            expect(panel != nullptr, "no HamloRedWall panel on the limiter page");
            if (panel != nullptr)
            {
                expect(panel->getWidth() > 1000 && panel->getHeight() > 500);
                const auto& v = panel->getValues();
                expect(v.knobs[ui::HamloRedWallPanel::Ceiling] == -3.0 && v.knobs[ui::HamloRedWallPanel::Link] == 1.0
                       && v.switches[ui::HamloRedWallPanel::TruePeak] == 0 && v.knobs[ui::HamloRedWallPanel::Threshold] == 0.0);
                panel->onKnobChange(ui::HamloRedWallPanel::Threshold, -9.0);
                panel->onKnobChange(ui::HamloRedWallPanel::Style, 2.0);
                panel->onSwitchChange(ui::HamloRedWallPanel::Monitor, 2);
                const auto project = plugin->getProject(); // a copy: keep it alive while reading from it
                const auto& m = project.layers[0].fxNodes[0].chain[0];
                expectEquals(static_cast<double>(m.parameters["thresholdDb"]), -9.0);
                expectEquals(static_cast<double>(m.parameters["style"]), 2.0);
                expectEquals(static_cast<int>(m.parameters["monitor"]), 2);
                expect(LimiterProcessor::usesRedWall(makeSnapshot(project.layers[0]).fx[0].slots[0]));
            }
        }

        beginTest("11. Whole-layer chain (D7): equals the module on the plain layer signal; latency adds; state round trip; menus list all modules");
        {
            // A compressor on the MIX chain of an otherwise empty layer = the compressor on the input.
            FxProjectState p;
            p.layers[0].mixChain = { defaultModule(FxType::Compressor) };
            auto e = std::make_unique<ProjectEngine>();
            e->prepare(kFs, 512);
            e->publish(p);
            auto y = drums;
            for (int start = 0; start < y.getNumSamples(); start += 512)
            {
                float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                e->process(c, 2, juce::jmin(512, y.getNumSamples() - start));
            }
            CompressorProcessor comp;
            comp.prepare(kFs);
            const auto snap = makeSnapshot(p.layers[0]);
            auto ref = drums;
            for (int start = 0; start < ref.getNumSamples(); start += 512)
            {
                float* c[2] { ref.getWritePointer(0) + start, ref.getWritePointer(1) + start };
                comp.process(c, 2, juce::jmin(512, ref.getNumSamples() - start), snap.mix.slots[0], true);
            }
            const auto null = compareAligned(segment(y, 4800, y.getNumSamples() - 4800), segment(ref, 4800, ref.getNumSamples() - 4800));
            logMessage("  MIX compressor vs the compressor on the input (after the 20 ms fade-in): " + juce::String(null.errorDb, 1) + " dB");
            expect(null.errorDb <= -120.0);

            // latency: regions max + mix chain
            FxProjectState q;
            FxNodeState n;
            n.id = 1;
            n.chain = { defaultModule(FxType::Limiter) }; // 2 ms + true peak = 104
            q.layers[0].fxNodes = { n };
            q.layers[0].mixChain = { defaultModule(FxType::Limiter) };
            sanitize(q);
            expectEquals(ProjectEngine::latencyFor(q, kFs), 104 + 104);

            const auto back = fromValueTree(juce::ValueTree::fromXml(*toValueTree(q).createXml()));
            expect(back.has_value() && back->layers[0].mixChain.size() == 1 && back->layers[0].mixChain[0].type == FxType::Limiter);

            auto menu = ui::NodeList::buildAddMenu(q.layers[0]);
            int items = 0;
            for (juce::PopupMenu::MenuItemIterator it(menu, true); it.next();)
                items += it.getItem().itemID >= 100 ? 1 : 0;
            expectEquals(items, 9 + 10 + 10);
            FxProjectState r;
            const auto selection = ui::NodeList::applyAddMenu(r, 301); // Limiter on the whole layer
            expect(selection.kind == ui::Selection::Kind::Mix && r.layers[0].mixChain.size() == 1 && r.layers[0].mixChain[0].type == FxType::Limiter);
        }
    }
};

static LimiterPassTests limiterPassTests;
} // namespace hpeq::test
