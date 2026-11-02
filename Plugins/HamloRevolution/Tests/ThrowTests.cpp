// R3 (docs/REVOLUTION_V2.md section 8): THROW A / B - DELAY, TONAL, REVERB sends; PHASER, FLANGER, VINYL,
// DOWNSAMPLE inserts. V3 blocks, V4 splices, V7 CPU, the targets' checks, an emptied lane's tail, V13 state, the page.

#include <chrono>
#include <cmath>
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
constexpr int kThrowA = rev::kFirstThrow, kThrowB = rev::kFirstThrow + 1;
constexpr int kStep = 6000; // a 1/16 at 120 BPM, samples

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

// `between(processor, sample)` runs before every block (a pattern change while playing).
template <typename Between>
juce::AudioBuffer<float> play(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x, int block, int prepared, Between between)
{
    Head head;
    p.setPlayHead(&head);
    p.prepareToPlay(kFs, prepared);
    auto y = x;
    juce::MidiBuffer midi;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        between(p, start);
        const int n = juce::jmin(block, y.getNumSamples() - start);
        head.ppq = static_cast<double>(start) * 2.0 / kFs;
        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, n);
        p.processBlock(view, midi);
    }
    p.setPlayHead(nullptr);
    return y;
}

juce::AudioBuffer<float> play(ModulePluginProcessor& p, const juce::AudioBuffer<float>& x, int block = 512, int prepared = 512)
{
    return play(p, x, block, prepared, [](ModulePluginProcessor&, int) {});
}

juce::AudioBuffer<float> tone(double seconds, double hz, double amp)
{
    juce::AudioBuffer<float> x(2, static_cast<int>(seconds * kFs));
    for (int i = 0; i < x.getNumSamples(); ++i)
        for (int ch = 0; ch < 2; ++ch)
            x.setSample(ch, i, static_cast<float>(amp * std::sin(2.0 * kPi * hz * i / kFs)));
    return x;
}

juce::AudioBuffer<float> silence(double seconds)
{
    juce::AudioBuffer<float> x(2, static_cast<int>(seconds * kFs));
    x.clear();
    return x;
}

// Amplitude of one frequency over [from, from + n) (a projection).
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

// The frequency over [from, from + n) from the zero crossings (interpolated), -1 without two crossings.
double crossingHz(const juce::AudioBuffer<float>& y, int ch, int from, int n)
{
    double first = -1.0, last = -1.0;
    int count = 0;
    for (int i = from + 1; i < from + n; ++i)
    {
        const double a = y.getSample(ch, i - 1), b = y.getSample(ch, i);
        if (a < 0.0 && b >= 0.0)
        {
            const double t = (i - 1) + a / (a - b);
            if (first < 0.0)
                first = t;
            last = t;
            ++count;
        }
    }
    return count < 2 ? -1.0 : (count - 1) * kFs / (last - first);
}

State withLevels(int lane, std::initializer_list<int> levels)
{
    State s;
    int i = 0;
    for (int v : levels)
        s.lanes[static_cast<size_t>(lane)].first[static_cast<size_t>(i++)] = static_cast<std::uint8_t>(v);
    return s;
}

State allSteps(int lane, int level)
{
    State s;
    s.lanes[static_cast<size_t>(lane)].first.fill(static_cast<std::uint8_t>(level));
    return s;
}

double dB(double ratio) { return 20.0 * std::log10(std::max(ratio, 1.0e-12)); }

// The fourth difference (the residual of a cubic through the four samples before).
double fourth(const juce::AudioBuffer<float>& y, int ch, int i)
{
    return std::abs(static_cast<double>(y.getSample(ch, i)) - 4.0 * y.getSample(ch, i - 1) + 6.0 * y.getSample(ch, i - 2)
                    - 4.0 * y.getSample(ch, i - 3) + y.getSample(ch, i - 4));
}
} // namespace

class ThrowTests final : public juce::UnitTest
{
public:
    ThrowTests() : juce::UnitTest("Throw", "HamloRevolution") {}

    void runTest() override
    {
        beginTest("An empty THROW lane (ON, every target, A and B) = dry bit for bit");
        {
            const auto x = drums(1.0);
            for (int target = 0; target < rev::kThrowTargets; ++target)
            {
                auto p = make();
                p->setParameterValue("throwATarget", target);
                p->setParameterValue("throwBTarget", target);
                expect(bitExact(play(*p, x), x), rev::throwTargetName(target));
            }
        }

        beginTest("DELAY: echoes at TIME within 1 sample, ping-pong, echo 2 / echo 1 = 0.9 x TAIL, no send from level 0");
        {
            const auto setUp = [](ModulePluginProcessor& p)
            {
                p.setParameterValue("throwATarget", rev::ThrowDelay);
                p.setParameterValue("throwATail", 0.5);
                p.setParameterValue("throwATone", 1.0);
                p.setRevolutionState(withLevels(kThrowA, { 15 })); // only step 1 sends
            };
            // timing: an impulse on step 1 -> the first echo on the left 1/8 dotted (18000 samples) later
            auto p = make();
            setUp(*p);
            auto x = silence(1.0);
            x.setSample(0, 3000, 1.0f);
            x.setSample(1, 3000, 1.0f);
            const auto y = play(*p, x);
            int at = 0;
            for (int i = 3001; i < y.getNumSamples(); ++i)
                if (std::abs(y.getSample(0, i)) > std::abs(y.getSample(0, at)))
                    at = i;
            logMessage("  impulse at 3000: the left echo's peak at " + juce::String(at) + " (expected 21000), right channel there "
                       + juce::String(y.getSample(1, at), 6));
            expect(std::abs(at - 21000) <= 1, juce::String(at));
            expect(std::abs(y.getSample(1, at)) < 1.0e-6f, "echo 1 only on the left");

            // levels: a 500 Hz burst on step 1, another on step 2 (level 0, must not be sent)
            auto q = make();
            setUp(*q);
            auto b = silence(1.5);
            for (int i = 0; i < 1920; ++i)
                for (int ch = 0; ch < 2; ++ch)
                {
                    const auto v = static_cast<float>(0.5 * std::sin(2.0 * kPi * 500.0 * i / kFs));
                    b.setSample(ch, 1000 + i, v);
                    b.setSample(ch, 7000 + i, 0.5f * v);
                }
            const auto z = play(*q, b);
            const int n = 960; // 20 whole periods inside each burst
            const double echo1 = amplitudeAt(z, 0, 19480, n, 500.0), echo2 = amplitudeAt(z, 1, 37480, n, 500.0);
            const double fromStep2 = std::max(amplitudeAt(z, 0, 25480, n, 500.0), amplitudeAt(z, 1, 25480, n, 500.0));
            const double ratio = dB(echo2 / echo1), expected = dB(0.9 * 0.5);
            logMessage("  echo 1 (left) " + juce::String(dB(echo1 / 0.5), 2) + " dB, echo 2 (right) / echo 1 " + juce::String(ratio, 2)
                       + " dB (expected " + juce::String(expected, 2) + "), from the level-0 step " + juce::String(dB(fromStep2 / 0.25), 1) + " dB");
            expect(std::abs(dB(echo1 / 0.5)) <= 0.5, "echo 1 at the send level");
            expect(std::abs(ratio - expected) <= 0.5, juce::String(ratio));
            expect(dB(fromStep2 / 0.25) <= -90.0, "nothing sent from a level-0 step");
            expect(amplitudeAt(z, 0, 19480 + 18000, n, 500.0) < 1.0e-6, "echo 2 not on the left");
        }

        beginTest("TONAL: the spectrum peaks at NOTE (A = 220 Hz) within 1 %");
        {
            auto p = make();
            p->setParameterValue("throwATarget", rev::ThrowTonal);
            p->setParameterValue("throwANote", 9);
            p->setParameterValue("throwATail", 0.8);
            p->setRevolutionState(allSteps(kThrowA, 15));
            auto x = silence(2.0);
            x.setSample(0, 1000, 1.0f);
            x.setSample(1, 1000, 1.0f);
            const auto y = play(*p, x);
            constexpr int order = 16, size = 1 << order;
            juce::dsp::FFT fft(order);
            std::vector<float> data(2 * size, 0.0f);
            for (int i = 0; i < size; ++i)
                data[static_cast<size_t>(i)] = y.getSample(0, 1000 + i);
            fft.performFrequencyOnlyForwardTransform(data.data());
            int best = static_cast<int>(150.0 * size / kFs);
            for (int bin = best; bin < static_cast<int>(300.0 * size / kFs); ++bin)
                if (data[static_cast<size_t>(bin)] > data[static_cast<size_t>(best)])
                    best = bin;
            const double hz = best * kFs / size;
            logMessage("  peak " + juce::String(hz, 2) + " Hz (A3 = 220 Hz, " + juce::String(100.0 * (hz / 220.0 - 1.0), 2) + " %)");
            expect(std::abs(hz / 220.0 - 1.0) <= 0.01, juce::String(hz));
        }

        beginTest("REVERB: T30 within 25 % of the nominal decay (TAIL 50 % = 2.225 s)");
        {
            auto p = make();
            p->setParameterValue("throwATarget", rev::ThrowReverb);
            p->setParameterValue("throwATail", 0.5);
            p->setParameterValue("throwATone", 1.0);
            p->setRevolutionState(allSteps(kThrowA, 15));
            auto x = silence(7.0);
            x.setSample(0, 1000, 1.0f);
            x.setSample(1, 1000, 1.0f);
            const auto y = play(*p, x);
            // Schroeder: the energy still to come, from just after the impulse
            const int from = 1001, count = y.getNumSamples() - from;
            std::vector<double> edc(static_cast<size_t>(count));
            double sum = 0.0;
            for (int i = count - 1; i >= 0; --i)
            {
                const double l = y.getSample(0, from + i), r = y.getSample(1, from + i);
                sum += l * l + r * r;
                edc[static_cast<size_t>(i)] = sum;
            }
            const auto timeAt = [&](double db)
            {
                for (int i = 0; i < count; ++i)
                    if (10.0 * std::log10(edc[static_cast<size_t>(i)] / edc[0]) <= db)
                        return i / kFs;
                return -1.0;
            };
            const double t30 = 2.0 * (timeAt(-35.0) - timeAt(-5.0)), nominal = 0.3 + 7.7 * 0.25;
            logMessage("  T30 " + juce::String(t30, 3) + " s, nominal " + juce::String(nominal, 3) + " s (" + juce::String(100.0 * (t30 / nominal - 1.0), 1)
                       + " %)");
            expect(std::abs(t30 / nominal - 1.0) <= 0.25, juce::String(t30));
        }

        beginTest("PHASER and FLANGER move a 1 kHz sine's level by >= 10 dB over the LFO (1 bar)");
        for (int target : { static_cast<int>(rev::ThrowPhaser), static_cast<int>(rev::ThrowFlanger) })
        {
            auto p = make();
            p->setParameterValue("throwATarget", target);
            p->setParameterValue("throwATime", 11); // 1 BAR = 2 s
            p->setRevolutionState(allSteps(kThrowA, 15));
            const auto y = play(*p, tone(4.5, 1000.0, 0.3));
            double lo = 1.0e9, hi = -1.0e9;
            for (int from = 96000; from + 960 <= 192000; from += 480)
            {
                const double level = dB(amplitudeAt(y, 0, from, 960, 1000.0) / 0.3);
                lo = std::min(lo, level);
                hi = std::max(hi, level);
            }
            logMessage("  " + juce::String(rev::throwTargetName(target)) + ": 1 kHz moves " + juce::String(lo, 1) + " .. " + juce::String(hi, 1) + " dB");
            expect(hi - lo >= 10.0, juce::String(hi - lo));
        }

        beginTest("VINYL: wow moves 1 kHz by >= 0.3 %; BRAKE (TAIL 100 %) lowers it by >= 40 % in the step's last quarter");
        {
            auto p = make();
            p->setParameterValue("throwATarget", rev::ThrowVinyl);
            p->setParameterValue("throwATail", 0.0);
            p->setParameterValue("throwATone", 0.0);
            p->setParameterValue("throwATime", 8); // 1/4: a 0.5 s wow
            p->setRevolutionState(allSteps(kThrowA, 15));
            const auto y = play(*p, tone(2.5, 1000.0, 0.3));
            double lo = 1.0e9, hi = -1.0e9;
            for (int from = 48000; from + 1200 <= 96000; from += 600)
            {
                const double hz = crossingHz(y, 0, from, 1200);
                lo = std::min(lo, hz);
                hi = std::max(hi, hz);
            }
            const double wow = (hi - lo) / 1000.0;
            logMessage("  wow: " + juce::String(lo, 2) + " .. " + juce::String(hi, 2) + " Hz (" + juce::String(100.0 * wow, 2) + " %)");
            expect(wow >= 0.003, juce::String(wow));

            auto b = make();
            b->setParameterValue("throwATarget", rev::ThrowVinyl);
            b->setParameterValue("throwATail", 1.0);
            b->setParameterValue("throwATone", 0.0);
            b->setParameterValue("throwATime", 13); // 4 bars: the wow is slow
            b->setRevolutionState(withLevels(kThrowA, { 15 }));
            const auto z = play(*b, tone(2.5, 1000.0, 0.3));
            // step 1 of the second cycle (2 s): its last quarter, short of the next step's fade
            const int start = 96000 + 3 * kStep / 4;
            const double hz = crossingHz(z, 0, start, kStep / 4 - 200);
            logMessage("  brake: " + juce::String(hz, 1) + " Hz over the last quarter of the step (" + juce::String(100.0 * (1.0 - hz / 1000.0), 1)
                       + " % lower)");
            expect(hz > 0.0 && hz <= 600.0, juce::String(hz));
        }

        beginTest("DOWNSAMPLE at level 15: the fs / 32 alias of 1 kHz (500 Hz) >= -30 dB re the tone");
        {
            auto p = make();
            p->setParameterValue("throwATarget", rev::ThrowDownsample);
            p->setRevolutionState(allSteps(kThrowA, 15));
            const auto y = play(*p, tone(1.0, 1000.0, 0.3));
            const double alias = dB(amplitudeAt(y, 0, 24000, 9600, 500.0) / amplitudeAt(y, 0, 24000, 9600, 1000.0));
            logMessage("  500 Hz alias " + juce::String(alias, 1) + " dB re 1 kHz");
            expect(alias >= -30.0, juce::String(alias));
        }

        beginTest("An emptied DELAY lane keeps its tail, then goes idle: the output = the input exactly");
        {
            auto p = make();
            p->setParameterValue("throwATarget", rev::ThrowDelay);
            p->setParameterValue("throwATail", 0.2);
            p->setRevolutionState(withLevels(kThrowA, { 15 }));
            const auto x = drums(8.0);
            const int clearAt = 24064; // a block start, 0.5 s
            const auto y = play(*p, x, 512, 512, [clearAt](ModulePluginProcessor& q, int start)
            {
                if (start == clearAt)
                    q.setRevolutionState(State {});
            });
            const auto after = [&](double from, double to) { return segment(y, static_cast<int>(from * kFs), static_cast<int>((to - from) * kFs)); };
            const auto dry = [&](double from, double to) { return segment(x, static_cast<int>(from * kFs), static_cast<int>((to - from) * kFs)); };
            const double tailDb = differenceDb(after(0.7, 0.9), dry(0.7, 0.9));
            logMessage("  the echo after the lane was emptied (0.75 s): " + juce::String(tailDb, 1) + " dB re the input");
            expect(tailDb > -40.0, "the tail rings on");
            expect(bitExact(after(5.0, 8.0), dry(5.0, 8.0)), "idle: dry bit for bit");
        }

        beginTest("V4: a 100 Hz sine, a 1/16 gate (15 / 0): the fourth difference at step boundaries <= -40 dB or <= the steady effect's");
        for (int target = 0; target < rev::kThrowTargets; ++target)
        {
            const auto render = [target](const State& s)
            {
                auto p = make();
                p->setParameterValue("throwATarget", target);
                p->setParameterValue("throwATone", 0.0); // VINYL: no crackle (single-sample ticks by design)
                p->setRevolutionState(s);
                return play(*p, tone(2.0, 100.0, 0.5));
            };
            const auto gated = render(withLevels(kThrowA, { 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0 }));
            const auto steady = render(allSteps(kThrowA, 15));
            double atBoundary = 0.0, effect = 0.0;
            for (int i = 12000; i < gated.getNumSamples(); ++i)
            {
                const int fromBoundary = std::min(i % kStep, kStep - i % kStep);
                for (int ch = 0; ch < 2; ++ch)
                {
                    if (fromBoundary <= 144)
                        atBoundary = std::max(atBoundary, fourth(gated, ch, i));
                    effect = std::max(effect, fourth(steady, ch, i));
                }
            }
            const double boundaryDb = dB(atBoundary / 0.5), effectDb = dB(effect / 0.5);
            logMessage("  " + juce::String(rev::throwTargetName(target)) + ": at the boundaries " + juce::String(boundaryDb, 1) + " dB, the effect itself "
                       + juce::String(effectDb, 1) + " dB");
            expect(boundaryDb <= -40.0 || atBoundary <= effect, rev::throwTargetName(target));
        }

        beginTest("V3: blocks 1, 64, 127, 4096 after prepare(256) bit-identical with both THROW lanes on (every target)");
        {
            const int pairs[4][2] { { rev::ThrowDelay, rev::ThrowReverb }, { rev::ThrowTonal, rev::ThrowPhaser }, { rev::ThrowFlanger, rev::ThrowVinyl },
                                    { rev::ThrowDownsample, rev::ThrowDelay } };
            const auto x = drums(2.0);
            for (const auto& pair : pairs)
            {
                const auto render = [&](int block)
                {
                    auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
                    p->setCurrentProgram(2);
                    p->setParameterValue("throwATarget", pair[0]);
                    p->setParameterValue("throwBTarget", pair[1]);
                    p->setParameterValue("throwATail", 0.7);
                    auto s = p->getRevolutionState();
                    s.lanes[kThrowA].first = { 15, 0, 9, 0, 4, 12, 0, 7 };
                    s.lanes[kThrowA].length = 5;
                    s.lanes[kThrowB].first = { 2, 14, 6, 15, 0, 9 };
                    s.lanes[kThrowB].rate = 2;
                    s.lanes[kThrowB].band = rev::Mid;
                    p->setRevolutionState(s);
                    return play(*p, x, block, 256);
                };
                const auto expected = render(256);
                const juce::String pairName = juce::String(rev::throwTargetName(pair[0])) + " + " + rev::throwTargetName(pair[1]);
                expect(differenceDb(expected, x) > -40.0, pairName + ": the lanes do something");
                for (int block : { 1, 64, 127, 4096 })
                    expect(bitExact(render(block), expected), pairName + ", block " + juce::String(block));
            }
        }

        beginTest("V7: CPU, 48 kHz / 256, every lane type on, every pair of THROW targets: <= 15 % of the block time");
        {
            const auto x = drums(6.0);
            double worst = 0.0;
            juce::String worstName;
            for (int a = 0; a < rev::kThrowTargets; ++a)
                for (int b = a; b < rev::kThrowTargets; ++b)
                {
                    auto p = std::make_unique<ModulePluginProcessor>(ModuleKind::Revolution);
                    p->setParameterValue("timeStretch", 2); // CLEAN, the heaviest TIME renderer
                    p->setParameterValue("throwATarget", a);
                    p->setParameterValue("throwBTarget", b);
                    p->setParameterValue("shapeATarget", rev::ShapeBandPass);
                    p->setParameterValue("shapeBTarget", rev::ShapeDrive);
                    State s;
                    s.pattern.steps[rev::Time] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 5, 5, 0, 0 };
                    s.pattern.steps[rev::Glitch] = { 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 4, 0, 0, 0, 3, 3 };
                    s.pattern.steps[rev::Motion] = { 0, 6, 0, 9, 0, 12, 0, 3, 0, 6, 0, 9, 0, 12, 0, 15 };
                    s.lanes[rev::kFirstShape].first.fill(9);
                    s.lanes[rev::kFirstShape].band = rev::Mid;
                    s.lanes[rev::kFirstShape + 1].first = { 15, 0, 15, 0, 15, 0, 15, 0 };
                    s.lanes[rev::kFirstShape + 1].band = rev::Low;
                    s.lanes[kThrowA].first.fill(12);
                    s.lanes[kThrowB].first.fill(12);
                    s.lanes[kThrowB].band = rev::High;
                    p->setRevolutionState(s);
                    Head head;
                    p->setPlayHead(&head);
                    p->prepareToPlay(kFs, 256);
                    auto y = x;
                    juce::MidiBuffer midi;
                    double spent = 0.0;
                    for (int start = 0; start + 256 <= y.getNumSamples(); start += 256)
                    {
                        head.ppq = static_cast<double>(start) * 2.0 / kFs;
                        juce::AudioBuffer<float> view(y.getArrayOfWritePointers(), 2, start, 256);
                        const auto t0 = std::chrono::steady_clock::now();
                        p->processBlock(view, midi);
                        if (start >= 48000) // after the first second (the histories filled)
                            spent += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                    }
                    p->setPlayHead(nullptr);
                    const double share = spent / ((y.getNumSamples() - 48000) / kFs);
                    if (share > worst)
                    {
                        worst = share;
                        worstName = juce::String(rev::throwTargetName(a)) + " + " + rev::throwTargetName(b);
                    }
                }
            logMessage("  worst pair " + worstName + ": " + juce::String(100.0 * worst, 2) + " % of the block time"
#if JUCE_DEBUG
                       + " (a debug build: not checked)"
#endif
            );
#if !JUCE_DEBUG
            expect(worst <= 0.15, juce::String(worst));
#endif
        }

        beginTest("V6 latency 0; V13: THROW parameters, steps and scenes round trip");
        {
            auto p = make();
            p->prepareToPlay(kFs, 512);
            expectEquals(p->getLatencySamples(), 0);
            p->setParameterValue("throwBTarget", rev::ThrowFlanger);
            p->setParameterValue("throwBTime", 3);
            p->setParameterValue("throwANote", 5);
            p->setParameterValue("throwATail", 0.25);
            auto s = withLevels(kThrowB, { 1, 2, 3, 4, 5 });
            s.lanes[kThrowB].band = rev::Low;
            s.lanes[kThrowB].extra[2] = 7;
            s.lanes[kThrowA].first[6] = 11;
            p->setRevolutionState(s);
            juce::MemoryBlock saved;
            p->getStateInformation(saved);
            auto q = make();
            q->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            expect(q->getRevolutionState().lanes == s.lanes, "lanes");
            expectEquals(juce::roundToInt(q->parameterValue("throwBTarget")), static_cast<int>(rev::ThrowFlanger));
            expectEquals(juce::roundToInt(q->parameterValue("throwBTime")), 3);
            expectEquals(juce::roundToInt(q->parameterValue("throwANote")), 5);
            expect(q->currentSlot(true).values == p->currentSlot(true).values, "the slots");
            const auto slot = q->currentSlot(true);
            expectEquals(static_cast<int>(slot.values[hpeq::dsp::slot::laneBand(kThrowB)]), static_cast<int>(rev::Low));
        }

        beginTest("Page: THROW levels and BAND reach the processor");
        {
            auto p = make();
            std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(ed.get());
            auto* page = editor != nullptr ? dynamic_cast<hpeq::ui::HamloRevolutionPanel*>(editor->getPanel()) : nullptr;
            expect(page != nullptr);
            if (page == nullptr)
                return;
            page->setLevel(kThrowA, 2, 9);
            page->clickStep(kThrowB, 0);
            page->setLaneBand(kThrowB, rev::High);
            const auto s = p->getRevolutionState();
            expectEquals(static_cast<int>(s.lanes[kThrowA].first[2]), 9);
            expectEquals(static_cast<int>(s.lanes[kThrowB].first[0]), 1);
            expectEquals(s.lanes[kThrowB].band, static_cast<int>(rev::High));
        }
    }
};

static ThrowTests throwTests;
} // namespace hamlo::test
