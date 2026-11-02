// R2 (docs/REVOLUTION_V2.md section 7): the band split (V5 flat sum, isolation), SHAPE A / B (V4 no clicks, the
// targets, CURVE), V3 blocks, V13 state, the page.

#include <cmath>
#include <complex>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Core/Revolution.h"
#include "Standalone/ModulePluginEditor.h"
#include "Standalone/ModulePluginProcessor.h"
#include "TestSignals.h"
#include "UI/HamloRevolutionPanel.h"

namespace hamlo::test
{
namespace
{
using hpeq::standalone::ModuleKind;
using hpeq::standalone::ModulePluginProcessor;
namespace rev = hpeq::revolution;
using State = ModulePluginProcessor::RevolutionState;
constexpr double kPi = 3.14159265358979323846;
constexpr int kShapeA = 3, kShapeB = 4;

class Head final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm(120.0);
        info.setIsPlaying(true);
        info.setPpqPosition(ppq);
        return info;
    }
    double ppq = 0.0;
};

// Only the lanes under test: TIME, GLITCH, MOTION off unless asked.
std::unique_ptr<ModulePluginProcessor> make(bool engines = false)
{
    auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
    p->setRevolutionState(State {});
    if (!engines)
        for (const char* id : { "timeOn", "glitchOn", "motionOn" })
            p->setParameterValue(id, 0.0);
    return p;
}

juce::AudioBuffer<float> play(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x, int block = 512, int prepared = 512)
{
    Head head;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, prepared);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        head.ppq = static_cast<double>(start) * 2.0 / kFs;
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
    }
    p.setPlayHead(nullptr);
    return y;
}

juce::AudioBuffer<float> constant(double seconds, float v)
{
    juce::AudioBuffer<float> x(2, static_cast<int>(seconds * kFs));
    for (int ch = 0; ch < 2; ++ch)
        juce::FloatVectorOperations::fill(x.getWritePointer(ch), v, x.getNumSamples());
    return x;
}

juce::AudioBuffer<float> sines(double seconds, std::initializer_list<std::pair<double, double>> parts)
{
    juce::AudioBuffer<float> x(2, static_cast<int>(seconds * kFs));
    for (int i = 0; i < x.getNumSamples(); ++i)
    {
        double v = 0.0;
        for (const auto& [hz, amp] : parts)
            v += amp * std::sin(2.0 * kPi * hz * i / kFs);
        x.setSample(0, i, static_cast<float>(v));
        x.setSample(1, i, static_cast<float>(v));
    }
    return x;
}

// Amplitude of one frequency over [from, from + n) (a Goertzel-style projection).
double amplitudeAt(const juce::AudioBuffer<float>& y, int ch, int from, int n, double hz)
{
    double re = 0.0, im = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double w = 2.0 * kPi * hz * (from + i) / kFs;
        re += y.getSample(ch, from + i) * std::cos(w);
        im += y.getSample(ch, from + i) * std::sin(w);
    }
    return 2.0 * std::sqrt(re * re + im * im) / n;
}

State withLevels(int lane, std::initializer_list<int> levels)
{
    State s;
    int i = 0;
    for (int v : levels)
        s.lanes[static_cast<size_t>(lane)].first[static_cast<size_t>(i++)] = static_cast<std::uint8_t>(v);
    return s;
}

double dB(double ratio) { return 20.0 * std::log10(std::max(ratio, 1.0e-12)); }
} // namespace

class ShapeTests final : public juce::UnitTest
{
public:
    ShapeTests() : juce::UnitTest("Shape and bands", "HamloRevolution") {}

    void runTest() override
    {
        const double step = 0.125; // a 1/16 at 120 BPM, seconds

        beginTest("V5: every lane on a band with an empty pattern: the magnitude 20 Hz .. 20 kHz flat within 0.1 dB");
        {
            auto p = make(true);
            State s;
            const int bands[rev::kLanes] { rev::Low, rev::Mid, rev::High, rev::Low, rev::High };
            for (int l = 0; l < rev::kLanes; ++l)
                s.lanes[static_cast<size_t>(l)].band = bands[l];
            p->setRevolutionState(s);
            juce::AudioBuffer<float> x(2, 65536 + 8192);
            x.clear();
            x.setSample(0, 8192, 1.0f);
            x.setSample(1, 8192, 1.0f);
            const auto y = play(*p, x);
            constexpr int order = 16, size = 1 << order;
            juce::dsp::FFT fft(order);
            std::vector<float> data(2 * size, 0.0f);
            for (int i = 0; i < size; ++i)
                data[static_cast<size_t>(i)] = y.getSample(0, 8192 + i);
            fft.performFrequencyOnlyForwardTransform(data.data());
            double lo = 1.0e9, hi = -1.0e9;
            for (int b = 1; b < size / 2; ++b)
            {
                const double hz = b * kFs / size;
                if (hz < 20.0 || hz > 20000.0)
                    continue;
                const double m = dB(data[static_cast<size_t>(b)]);
                lo = std::min(lo, m);
                hi = std::max(hi, m);
            }
            logMessage("  magnitude " + juce::String(lo, 3) + " .. " + juce::String(hi, 3) + " dB");
            expect(lo >= -0.1 && hi <= 0.1, juce::String(lo) + " .. " + juce::String(hi));
        }

        beginTest("Band isolation: GLITCH GAP on HIGH over 60 Hz + 6 kHz: 60 Hz within 0.5 dB of dry, 6 kHz >= 30 dB down");
        {
            auto p = make();
            p->setParameterValue("glitchOn", 1.0);
            State s;
            s.pattern.steps[rev::Glitch][4] = rev::Gap;
            s.lanes[rev::Glitch].band = rev::High;
            p->setRevolutionState(s);
            const auto x = sines(1.0, { { 60.0, 0.3 }, { 6000.0, 0.3 } });
            const auto y = play(*p, x);
            // 0.05 s inside the gap: whole periods of both tones (3 and 300), so the projection has no leakage
            const int from = static_cast<int>((4.0 + 0.25) * step * kFs), n = static_cast<int>(0.05 * kFs);
            const double low = dB(amplitudeAt(y, 0, from, n, 60.0) / amplitudeAt(x, 0, from, n, 60.0));
            const double high = dB(amplitudeAt(y, 0, from, n, 6000.0) / amplitudeAt(x, 0, from, n, 6000.0));
            logMessage("  in the gap: 60 Hz " + juce::String(low, 2) + " dB, 6 kHz " + juce::String(high, 1) + " dB");
            expect(std::abs(low) <= 0.5 && high <= -30.0);
        }

        beginTest("V4: a 1/16 VOLUME gate (levels 15 / 0) on DC: the gain's second difference <= -40 dB (steps and CURVE)");
        for (int curve : { 0, 1 })
        {
            auto p = make();
            p->setParameterValue("shapeACurve", curve);
            p->setRevolutionState(withLevels(kShapeA, { 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0 }));
            const auto y = play(*p, constant(2.0, 0.5f));
            double worst = 0.0;
            for (int i = static_cast<int>(0.1 * kFs); i < y.getNumSamples(); ++i)
                worst = std::max(worst, std::abs(2.0 * (y.getSample(0, i) - 2.0 * y.getSample(0, i - 1) + y.getSample(0, i - 2))));
            const double gate = 1.0 - y.getSample(0, static_cast<int>(0.5 * step * kFs)) / 0.5;
            logMessage(juce::String(curve != 0 ? "  CURVE" : "  STEPS") + ": second difference " + juce::String(dB(worst), 1) + " dB, gate depth on step 1 "
                       + juce::String(gate, 3));
            expect(dB(worst) <= -40.0, juce::String(dB(worst)));
            if (curve == 0)
                expectWithinAbsoluteError(gate, 1.0, 1.0e-3, "level 15 = silent");
        }

        beginTest("CURVE passes through the step levels at the step starts");
        {
            auto p = make();
            p->setParameterValue("shapeACurve", 1);
            const int levels[] { 3, 12, 6, 15, 0, 9, 9, 3 };
            p->setRevolutionState(withLevels(kShapeA, { 3, 12, 6, 15, 0, 9, 9, 3 }));
            const auto y = play(*p, constant(1.2, 0.5f));
            double worst = 0.0;
            for (int k = 1; k < 8; ++k)
            {
                const int at = static_cast<int>(std::ceil(k * step * kFs));
                worst = std::max(worst, std::abs(1.0 - y.getSample(0, at) / 0.5 - levels[k] / 15.0));
            }
            logMessage("  largest miss " + juce::String(worst, 4));
            expect(worst < 0.01);
        }

        beginTest("Targets: LOW PASS takes 8 kHz down >= 20 dB; BAND PASS keeps its centre; PAN alternates >= 10 dB; DRIVE");
        {
            auto p = make();
            p->setParameterValue("shapeBTarget", rev::ShapeLowPass);
            p->setRevolutionState(withLevels(kShapeB, { 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 }));
            const auto x = sines(1.0, { { 8000.0, 0.3 } });
            const auto y = play(*p, x);
            const double down = dB(amplitudeAt(y, 0, 24000, 12000, 8000.0) / 0.3);
            logMessage("  LOW PASS at level 15: 8 kHz " + juce::String(down, 1) + " dB");
            expect(down <= -20.0);

            auto pan = make();
            pan->setParameterValue("shapeATarget", rev::ShapePan);
            pan->setRevolutionState(withLevels(kShapeA, { 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 }));
            const auto z = play(*pan, sines(1.0, { { 440.0, 0.3 } }));
            for (int k = 2; k < 6; ++k)
            {
                const int from = static_cast<int>((k + 0.25) * step * kFs), n = static_cast<int>(0.5 * step * kFs);
                const double balance = dB((z.getRMSLevel(0, from, n) + 1.0e-9) / (z.getRMSLevel(1, from, n) + 1.0e-9));
                expect(k % 2 == 0 ? balance >= 10.0 : balance <= -10.0, "step " + juce::String(k) + ": " + juce::String(balance));
            }

            auto drive = make();
            drive->setParameterValue("shapeATarget", rev::ShapeDrive);
            drive->setRevolutionState(withLevels(kShapeA, { 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 }));
            const double amp = 0.31622776601683794;
            const auto d = play(*drive, sines(1.0, { { 100.0, amp } }));
            const int from = 24000, n = 9600;
            const double fundamental = amplitudeAt(d, 0, from, n, 100.0) / std::sqrt(2.0);
            const double total = d.getRMSLevel(0, from, n);
            const double thd = std::sqrt(std::max(0.0, total * total - fundamental * fundamental)) / fundamental;
            const double peak = dB(d.getMagnitude(0, from, n) / amp);
            logMessage("  DRIVE at level 15: THD " + juce::String(100.0 * thd, 1) + " %, peak " + juce::String(peak, 2) + " dB");
            expect(thd >= 0.05 && std::abs(peak) <= 1.0);
        }

        beginTest("An empty SHAPE lane (ON, any target) = dry bit for bit");
        {
            const auto x = drums(1.0);
            for (int target = 0; target < rev::kShapeTargets; ++target)
            {
                auto p = make();
                p->setParameterValue("shapeATarget", target);
                expect(bitExact(play(*p, x), x), rev::shapeTargetName(target));
            }
        }

        beginTest("V3: blocks 1, 64, 127, 4096 after prepare(256) bit-identical with bands and both SHAPE lanes on");
        {
            const auto setUp = [](ModulePluginProcessor& p)
            {
                p.setCurrentProgram(2);
                p.setParameterValue("shapeATarget", rev::ShapeDrive);
                p.setParameterValue("shapeBTarget", rev::ShapeBandPass);
                p.setParameterValue("shapeBRes", 0.7);
                p.setParameterValue("shapeBCurve", 1);
                auto s = p.getRevolutionState();
                s.lanes[rev::Glitch].band = rev::High;
                s.lanes[rev::Time].band = rev::Mid;
                s.lanes[kShapeA].first = { 15, 0, 9, 0, 4, 12, 0, 7 };
                s.lanes[kShapeA].band = rev::Low;
                s.lanes[kShapeA].length = 5;
                s.lanes[kShapeB].first = { 2, 14, 6, 15, 0, 9 };
                s.lanes[kShapeB].rate = 2;
                p.setRevolutionState(s);
            };
            const auto x = drums(3.0);
            const auto render = [&](int block)
            {
                auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
                setUp(*p);
                return play(*p, x, block, 256);
            };
            const auto expected = render(256);
            expect(differenceDb(expected, x) > -40.0, "the lanes do something");
            for (int block : { 1, 64, 127, 4096 })
                expect(bitExact(render(block), expected), "block " + juce::String(block));
        }

        beginTest("V6 latency 0; V13: bands, SHAPE steps and parameters round trip");
        {
            auto p = make();
            p->prepareToPlay(kFs, 512);
            expectEquals(p->getLatencySamples(), 0);
            p->setParameterValue("shapeBTarget", rev::ShapeCrush);
            p->setParameterValue("bandLowHz", 150.0);
            auto s = withLevels(kShapeB, { 1, 2, 3, 4, 5 });
            s.lanes[kShapeB].band = rev::Mid;
            s.lanes[kShapeB].extra[1] = 9;
            s.lanes[rev::Motion].band = rev::Low;
            p->setRevolutionState(s);
            juce::MemoryBlock saved;
            p->getStateInformation(saved);
            auto q = make();
            q->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            expect(q->getRevolutionState().lanes == s.lanes, "lanes");
            expectEquals(juce::roundToInt(q->parameterValue("shapeBTarget")), static_cast<int>(rev::ShapeCrush));
            expectWithinAbsoluteError(q->parameterValue("bandLowHz"), 150.0, 0.5);
            expect(q->currentSlot(true).values == p->currentSlot(true).values, "the slots");
            // an R1 lane text (five parts) still reads
            const auto r1 = rev::laneFromText("5,2,1,0000000000000000,30000000000000000000000000000000");
            expect(r1.length == 5 && r1.rate == 2 && r1.condition == 1 && r1.probability[0] == 3 && r1.band == 0);
        }

        beginTest("Page: SHAPE levels, BAND, CURVE and TARGET reach the processor");
        {
            auto p = make();
            std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(ed.get());
            auto* page = editor != nullptr ? dynamic_cast<hpeq::ui::HamloRevolutionPanel*>(editor->getPanel()) : nullptr;
            expect(page != nullptr);
            if (page == nullptr)
                return;
            page->setLevel(kShapeA, 3, 11);
            page->clickStep(kShapeB, 0);
            page->setLaneBand(kShapeB, rev::High);
            page->setLaneBand(rev::Time, rev::Low);
            const auto s = p->getRevolutionState();
            expectEquals(static_cast<int>(s.lanes[kShapeA].first[3]), 11);
            expectEquals(static_cast<int>(s.lanes[kShapeB].first[0]), 1);
            expectEquals(s.lanes[kShapeB].band, static_cast<int>(rev::High));
            expectEquals(s.lanes[rev::Time].band, static_cast<int>(rev::Low));
        }
    }
};

static ShapeTests shapeTests;
} // namespace hamlo::test
