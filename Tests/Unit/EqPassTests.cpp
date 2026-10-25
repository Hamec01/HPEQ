// EQ pass (owner, 2026-10-25): EQ points get SOLO (listen) and DYNAMIC gain, their own Black Steel page
// (HamloTochka) with a picture shape chooser, the spectrum zooms, the whole window takes the Black Steel
// style; fixes: closing HPEQ hung the DAW (Direct2D teardown), and adding a module to the MIX chain of a
// fresh window dropped the selection. Thresholds fixed before the runs.

#include <cmath>
#include <memory>

#include "DSP/LayerEngine.h"
#include "Harness/AudioMetrics.h"
#include "Plugin/FxPluginEditor.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/HamloTochkaPanel.h"
#include "UI/SpectrumWorkspace.h"
#include "UI/SteelLookAndFeel.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;

juce::AudioBuffer<float> tones(std::initializer_list<std::pair<double, double>> partials, int samples)
{
    juce::AudioBuffer<float> b(2, samples);
    b.clear();
    for (const auto& [hz, db] : partials)
    {
        const double a = std::pow(10.0, db / 20.0);
        for (int i = 0; i < samples; ++i)
        {
            const auto v = static_cast<float>(a * std::sin(2.0 * juce::MathConstants<double>::pi * hz * i / kFs));
            b.addSample(0, i, v);
            b.addSample(1, i, v);
        }
    }
    return b;
}

juce::AudioBuffer<float> renderEq(const juce::AudioBuffer<float>& x, const std::vector<EqNodeState>& points)
{
    LayerState layer;
    layer.eqNodes = points;
    auto e = std::make_unique<LayerEngine>();
    e->prepare(kFs, 512);
    e->publish(makeSnapshot(layer));
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += 512)
    {
        float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        e->process(c, 2, juce::jmin(512, y.getNumSamples() - start));
    }
    return y;
}

// Level of the `hz` component over the last half (projection, dBFS of a sine's amplitude).
double toneDb(const juce::AudioBuffer<float>& y, double hz)
{
    const int from = y.getNumSamples() / 2, to = y.getNumSamples();
    double s = 0.0, c = 0.0;
    for (int i = from; i < to; ++i)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * hz * i / kFs;
        s += y.getSample(0, i) * std::sin(w);
        c += y.getSample(0, i) * std::cos(w);
    }
    return 20.0 * std::log10(2.0 * std::sqrt(s * s + c * c) / (to - from) + 1.0e-12);
}

EqNodeState bell(double hz, double gain, double q = 1.0)
{
    EqNodeState n;
    n.id = 1;
    n.shape = EqShape::Bell;
    n.frequencyHz = hz;
    n.gainDb = gain;
    n.q = q;
    return n;
}

template <typename T>
T* findChild(juce::Component* root)
{
    juce::Array<juce::Component*> stack { root };
    while (!stack.isEmpty())
    {
        auto* c = stack.removeAndReturn(stack.size() - 1);
        for (auto* child : c->getChildren())
            stack.add(child);
        if (auto* t = dynamic_cast<T*>(c))
            return t;
    }
    return nullptr;
}

int visibleButtonsContaining(juce::Component* root, const juce::String& text, bool exact = false)
{
    int n = 0;
    juce::Array<juce::Component*> stack { root };
    while (!stack.isEmpty())
    {
        auto* c = stack.removeAndReturn(stack.size() - 1);
        for (auto* child : c->getChildren())
            stack.add(child);
        if (auto* b = dynamic_cast<juce::TextButton*>(c);
            b != nullptr && b->isVisible() && (exact ? b->getButtonText() == text : b->getButtonText().contains(text)))
            ++n;
    }
    return n;
}
} // namespace

class EqPassTests final : public juce::UnitTest
{
public:
    EqPassTests() : juce::UnitTest("EQ pass (solo, dynamic, page, zoom, steel)", "HPEQ") {}

    void runTest() override
    {
        beginTest("DYNAMIC bell (1 kHz, range -12, threshold -30, ratio 4): a loud tone is cut 12 dB (+-1), a quiet one is untouched (+-0.1)");
        {
            auto p = bell(1000.0, 0.0);
            p.dynamic = true;
            p.rangeDb = -12.0;
            p.thresholdDb = -30.0;
            p.ratio = 4.0;
            const auto loud = tones({ { 1000.0, -6.0 } }, 96000), quiet = tones({ { 1000.0, -50.0 } }, 96000);
            const double cut = toneDb(renderEq(loud, { p }), 1000.0) - toneDb(loud, 1000.0);
            const double kept = toneDb(renderEq(quiet, { p }), 1000.0) - toneDb(quiet, 1000.0);
            logMessage("  loud: " + juce::String(cut, 2) + " dB, quiet: " + juce::String(kept, 3) + " dB");
            expect(std::abs(cut + 12.0) <= 1.0 && std::abs(kept) <= 0.1);
        }

        beginTest("DYNAMIC: positive range lifts a loud band (+6 +-1); a loud tone far from the point does not trigger it (+-0.5 at 1 kHz)");
        {
            auto p = bell(1000.0, 0.0);
            p.dynamic = true;
            p.rangeDb = 6.0;
            p.thresholdDb = -30.0;
            p.ratio = 20.0;
            const auto loud = tones({ { 1000.0, -6.0 } }, 96000);
            const double lift = toneDb(renderEq(loud, { p }), 1000.0) - toneDb(loud, 1000.0);
            auto cut = bell(1000.0, 0.0, 2.0);
            cut.dynamic = true;
            cut.rangeDb = -12.0;
            const auto mixed = tones({ { 1000.0, -40.0 }, { 80.0, -6.0 } }, 96000);
            const double far = toneDb(renderEq(mixed, { cut }), 1000.0) - toneDb(mixed, 1000.0);
            logMessage("  lift " + juce::String(lift, 2) + " dB; 1 kHz with a loud 80 Hz: " + juce::String(far, 3) + " dB");
            expect(std::abs(lift - 6.0) <= 1.0 && std::abs(far) <= 0.5);
        }

        beginTest("SOLO: a bell (1 kHz, Q 2) keeps its region (+-1 dB) and takes 100 Hz and 8 kHz down >= 20 dB; a low shelf listens low; solo off is bit-exact");
        {
            const auto x = tones({ { 100.0, -12.0 }, { 1000.0, -12.0 }, { 8000.0, -12.0 } }, 96000);
            auto p = bell(1000.0, 0.0, 2.0);
            p.gainDb = 0.5; // a running point (0 dB points are not processed)
            p.solo = true;
            const auto y = renderEq(x, { p });
            const double keep = toneDb(y, 1000.0) - toneDb(x, 1000.0), low = toneDb(y, 100.0) - toneDb(x, 100.0), high = toneDb(y, 8000.0) - toneDb(x, 8000.0);
            auto shelf = p;
            shelf.shape = EqShape::LowShelf;
            shelf.frequencyHz = 300.0;
            const auto z = renderEq(x, { shelf });
            const double shelfLow = toneDb(z, 100.0) - toneDb(x, 100.0), shelfHigh = toneDb(z, 8000.0) - toneDb(x, 8000.0);
            logMessage("  bell solo: 1 kHz " + juce::String(keep, 2) + ", 100 Hz " + juce::String(low, 1) + ", 8 kHz " + juce::String(high, 1)
                       + " dB; low shelf solo: 100 Hz " + juce::String(shelfLow, 2) + ", 8 kHz " + juce::String(shelfHigh, 1) + " dB");
            expect(std::abs(keep - 0.5) <= 1.0 && low <= -20.0 && high <= -20.0);
            expect(std::abs(shelfLow) <= 2.0 && shelfHigh <= -20.0);
            auto plain = p;
            plain.solo = false;
            auto reference = p;
            reference.solo = false;
            reference.dynamic = false;
            expect(compareAligned(renderEq(x, { plain }), renderEq(x, { reference })).bitExact);
        }

        beginTest("State: solo and the dynamic settings round-trip; a point without them saves no new properties");
        {
            FxProjectState project;
            auto p = bell(2500.0, 3.0);
            p.solo = true;
            p.dynamic = true;
            p.thresholdDb = -22.5;
            p.ratio = 6.0;
            p.attackMs = 3.0;
            p.releaseMs = 250.0;
            p.rangeDb = 9.0;
            auto plain = bell(500.0, -2.0);
            plain.id = 2;
            project.layers[0].eqNodes = { p, plain };
            project.nextNodeId = 3;
            sanitize(project);
            const auto tree = toValueTree(project);
            const auto back = fromValueTree(juce::ValueTree::fromXml(*tree.createXml()));
            expect(back.has_value());
            if (back)
            {
                const auto& q = back->layers[0].eqNodes[0];
                expect(q.solo && q.dynamic && q.thresholdDb == -22.5 && q.ratio == 6.0 && q.attackMs == 3.0 && q.releaseMs == 250.0 && q.rangeDb == 9.0);
                const auto& r = back->layers[0].eqNodes[1];
                expect(!r.solo && !r.dynamic && r.rangeDb == -6.0);
            }
            const auto xml = tree.createXml()->toString();
            expectEquals(xml.indexOf("dynThresholdDb"), xml.lastIndexOf("dynThresholdDb")); // written once (for the dynamic point only)
        }

        beginTest("Zoom: x <-> Hz round trip; zooming keeps the frequency under the mouse; at least one octave; the dB range scales the curve");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1280, 840);
            auto* ws = findChild<ui::SpectrumWorkspace>(editor.get());
            expect(ws != nullptr);
            if (ws != nullptr)
            {
                const auto plot = ws->plotArea();
                const float x1k = ws->xFor(1000.0, plot);
                ws->zoomFrequency(0.5, 1000.0);
                expect(std::abs(ws->xFor(1000.0, plot) - x1k) < 0.5f, "the frequency under the mouse moved");
                expectWithinAbsoluteError(std::log2(ws->viewHighHz() / ws->viewLowHz()), 0.5 * std::log2(1000.0), 1.0e-9);
                for (int i = 0; i < 20; ++i)
                    ws->zoomFrequency(0.5, 1000.0);
                expectWithinAbsoluteError(std::log2(ws->viewHighHz() / ws->viewLowHz()), 1.0, 1.0e-9);
                expectWithinAbsoluteError(ws->hzFor(ws->xFor(1234.0, plot), plot), 1234.0, 1.0e-3); // x is a float pixel
                ws->setEqRange(6.0);
                expectWithinAbsoluteError(static_cast<double>(ws->yForEqDb(6.0, plot)), static_cast<double>(plot.getY()), 1.0e-3);
                ws->resetView();
                expect(ws->viewLowHz() == 20.0 && ws->viewHighHz() == 20000.0 && ws->eqRangeDb() == 24.0);
            }
            plugin->editorBeingDeleted(editor.get());
        }

        beginTest("Fix: adding a module to the MIX chain of a fresh window keeps MIX selected");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1280, 840);
            editor->selectMix(-1);
            plugin->getUndoManager().beginNewTransaction("Add module");
            plugin->editProject("Add module", [](FxProjectState& p) { p.layers[0].mixChain.push_back(defaultModule(FxType::MonstaFx)); });
            plugin->dispatchPendingMessages();
            expect(editor->isMixSelected(), "the first project change dropped the MIX selection");
            plugin->editorBeingDeleted(editor.get());
        }

        beginTest("Point page: DYNAMIC, SOLO (one point at a time) and the shape chooser edit the project");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            auto a = bell(1000.0, 3.0), b = bell(200.0, -2.0);
            b.id = 2;
            b.solo = true;
            p.layers[0].eqNodes = { a, b };
            p.nextNodeId = 3;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1280, 840);
            editor->selectNode(false, 1, -1);
            auto* page = findChild<ui::HamloTochkaPanel>(editor.get());
            expect(page != nullptr, "no point page");
            if (page != nullptr)
            {
                expectEquals(page->getValues().number, 1);
                page->setDynamic(true);
                page->pressSolo();
                page->chooseShape(EqShape::LowShelf);
                const auto now = plugin->getProject();
                const auto& n = now.layers[0].eqNodes;
                expect(n[0].dynamic && n[0].solo && n[0].shape == EqShape::LowShelf && !n[1].solo);
                expectEquals(visibleButtonsContaining(editor.get(), "+", true), 6); // the point's six module places
            }
            plugin->editorBeingDeleted(editor.get());
        }

        beginTest("De-esser on the point (DE-ESS, VOICE): an \"s\" loses >= 4 dB; a vowel, also 6 dB louder, changes <= 0.5 dB; DETECT round-trips; the page button sets the point");
        {
            // a synthetic voice: a vowel (180 Hz with formants near 700 / 1200 / 2600 Hz), then an "s" (noise
            // band-passed around 7 kHz), then the vowel 6 dB louder
            const int segment = 24000;
            juce::AudioBuffer<float> x(2, 3 * segment);
            juce::Random r(7);
            double z1 = 0.0, z2 = 0.0;
            const double w = 2.0 * juce::MathConstants<double>::pi * 7000.0 / kFs, alpha = std::sin(w) / 2.0, a0 = 1.0 + alpha;
            const double b0 = alpha / a0, b2 = -alpha / a0, a1 = -2.0 * std::cos(w) / a0, a2 = (1.0 - alpha) / a0;
            for (int i = 0; i < x.getNumSamples(); ++i)
            {
                double v = 0.0;
                const int seg = i / segment;
                if (seg != 1)
                {
                    for (int h = 1; h * 180.0 < 4000.0; ++h)
                    {
                        const double f = h * 180.0;
                        const double formant = std::exp(-std::pow((f - 700.0) / 250.0, 2.0)) + 0.7 * std::exp(-std::pow((f - 1200.0) / 300.0, 2.0))
                                               + 0.35 * std::exp(-std::pow((f - 2600.0) / 400.0, 2.0)) + 0.02;
                        v += 0.05 * formant * std::sin(2.0 * juce::MathConstants<double>::pi * f * i / kFs);
                    }
                    if (seg == 2)
                        v *= 2.0;
                }
                else
                {
                    const double n = 2.0 * r.nextDouble() - 1.0;
                    const double y = b0 * n + z1;
                    z1 = -a1 * y + z2;
                    z2 = b2 * n - a2 * y;
                    v = 0.6 * y;
                }
                x.setSample(0, i, static_cast<float>(v));
                x.setSample(1, i, static_cast<float>(v));
            }
            EqNodeState p;
            p.id = 1;
            p.shape = EqShape::Bell;
            p.frequencyHz = 7000.0;
            p.q = 0.8;
            p.dynamic = true;
            p.detect = 1;
            p.thresholdDb = -28.0;
            p.ratio = 4.0;
            p.rangeDb = -9.0;
            p.attackMs = 0.5;
            p.releaseMs = 40.0;
            const auto y = renderEq(x, { p });
            const auto rmsDb = [](const juce::AudioBuffer<float>& b, int from, int count)
            {
                double e = 0.0;
                for (int i = from; i < from + count; ++i)
                    e += static_cast<double>(b.getSample(0, i)) * b.getSample(0, i);
                return 10.0 * std::log10(e / count + 1.0e-20);
            };
            const int skip = 4800; // past the attack / release around each change
            const double vowel = rmsDb(y, skip, segment - skip) - rmsDb(x, skip, segment - skip);
            const double ess = rmsDb(y, segment + skip, segment - skip) - rmsDb(x, segment + skip, segment - skip);
            const double loud = rmsDb(y, 2 * segment + skip, segment - skip) - rmsDb(x, 2 * segment + skip, segment - skip);
            logMessage("  vowel " + juce::String(vowel, 3) + " dB, s " + juce::String(ess, 2) + " dB, vowel +6 dB " + juce::String(loud, 3) + " dB");
            expect(ess <= -4.0 && std::abs(vowel) <= 0.5 && std::abs(loud) <= 0.5);

            FxProjectState project;
            project.layers[0].eqNodes = { p };
            project.nextNodeId = 2;
            sanitize(project);
            const auto back = fromValueTree(juce::ValueTree::fromXml(*toValueTree(project).createXml()));
            expect(back.has_value() && back->layers[0].eqNodes[0].detect == 1);

            ui::HamloTochkaPanel page;
            page.setSize(1100, 340);
            EqNodeState sent;
            page.onChange = [&](const EqNodeState& n, const juce::String&) { sent = n; };
            page.pressDeEss();
            expect(sent.dynamic && sent.detect == 1 && sent.shape == EqShape::Bell && sent.frequencyHz == 7000.0 && sent.rangeDb == -9.0 && sent.attackMs == 0.5);
        }

        beginTest("Steel style: the window and its menus use SteelLookAndFeel while open; the default goes back when the last window closes");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            {
                auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
                expect(dynamic_cast<ui::SteelLookAndFeel*>(&editor->getLookAndFeel()) != nullptr);
                expect(dynamic_cast<ui::SteelLookAndFeel*>(&juce::LookAndFeel::getDefaultLookAndFeel()) != nullptr);
                plugin->editorBeingDeleted(editor.get());
            }
            expect(dynamic_cast<ui::SteelLookAndFeel*>(&juce::LookAndFeel::getDefaultLookAndFeel()) == nullptr);
        }
    }
};

static EqPassTests eqPassTests;
} // namespace hpeq::test
