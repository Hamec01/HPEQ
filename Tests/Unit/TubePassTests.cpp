// Tube pass (owner, 2026-10-26): BRING LENIN BACK (LOMO UP-25) and BRING STALIN BACK (LOMO UP-27) as the
// tube colour of the whole HPEQ output. The research's test bench (section 10): response, harmonics vs
// level, intermodulation, overload recovery, against the tanh control; plus the plug-in contract.
// Thresholds fixed before the runs.

#include <chrono>
#include <cmath>
#include <memory>

#include "DSP/TubePreamp.h"
#include "Harness/RtAllocationProbe.h"
#include "Plugin/FxPluginProcessor.h"
#include "UI/TubeButton.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;
constexpr double kPi = 3.14159265358979323846;

juce::AudioBuffer<float> sine(double hz, double dbfs, int samples, double hz2 = 0.0, double db2 = -200.0)
{
    juce::AudioBuffer<float> b(2, samples);
    const double a = std::pow(10.0, dbfs / 20.0), a2 = std::pow(10.0, db2 / 20.0);
    for (int i = 0; i < samples; ++i)
    {
        const auto v = static_cast<float>(a * std::sin(2.0 * kPi * hz * i / kFs) + a2 * std::sin(2.0 * kPi * hz2 * i / kFs));
        b.setSample(0, i, v);
        b.setSample(1, i, v);
    }
    return b;
}

juce::AudioBuffer<float> colour(const juce::AudioBuffer<float>& x, bool lenin, bool stalin, int block = 512, std::uint64_t* allocations = nullptr)
{
    TubeColour t;
    t.prepare(kFs, block);
    t.setTargets(lenin, stalin);
    t.reset(); // start at full colour (no fade-in)
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        const int n = juce::jmin(block, y.getNumSamples() - start);
        float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        ScopedAllocationProbe probe;
        t.process(c, 2, n);
        if (allocations != nullptr)
            *allocations += probe.counts().allocations;
    }
    return y;
}

// Amplitude of `hz` over [from, from + count) (a whole number of cycles of 1 kHz / 40 Hz windows).
double amplitude(const juce::AudioBuffer<float>& y, double hz, int from, int count)
{
    double s = 0.0, c = 0.0;
    for (int i = from; i < from + count; ++i)
    {
        const double w = 2.0 * kPi * hz * i / kFs;
        s += y.getSample(0, i) * std::sin(w);
        c += y.getSample(0, i) * std::cos(w);
    }
    return 2.0 * std::sqrt(s * s + c * c) / count;
}

struct Harmonics
{
    double fundamentalDb = 0.0, h2 = 0.0, h3 = 0.0, thd = 0.0; // h2, h3: dB below the fundamental; thd: %
};

Harmonics harmonics(const juce::AudioBuffer<float>& y, double hz)
{
    const int count = static_cast<int>(std::lround(kFs / hz * std::floor(0.5 * kFs / (kFs / hz)))); // ~0.5 s of whole cycles
    const int from = y.getNumSamples() - count;
    const double f = amplitude(y, hz, from, count);
    double sum = 0.0;
    Harmonics h;
    for (int k = 2; k <= 9; ++k)
    {
        const double a = amplitude(y, hz * k, from, count);
        sum += a * a;
        if (k == 2)
            h.h2 = 20.0 * std::log10(a / f + 1.0e-12);
        if (k == 3)
            h.h3 = 20.0 * std::log10(a / f + 1.0e-12);
    }
    h.fundamentalDb = 20.0 * std::log10(f + 1.0e-12);
    h.thd = 100.0 * std::sqrt(sum) / f;
    return h;
}

bool identical(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        if (std::memcmp(a.getReadPointer(ch), b.getReadPointer(ch), sizeof(float) * static_cast<size_t>(a.getNumSamples())) != 0)
            return false;
    return true;
}
} // namespace

class TubePassTests final : public juce::UnitTest
{
public:
    TubePassTests() : juce::UnitTest("Tubes: BRING LENIN / STALIN BACK", "HPEQ") {}

    void runTest() override
    {
        beginTest("Off: both buttons off leave the signal untouched (bit-exact)");
        {
            const auto x = sine(1000.0, -6.0, 24000);
            expect(identical(colour(x, false, false), x));
        }

        beginTest("Small signals pass at unity (-50 dBFS, 1 kHz: 0 +-0.5 dB, THD < 0.3 %)");
        {
            const auto x = sine(1000.0, -50.0, 48000);
            for (const auto [l, s] : { std::pair { true, false }, std::pair { false, true } })
            {
                const auto h = harmonics(colour(x, l, s), 1000.0);
                const double g = h.fundamentalDb - harmonics(x, 1000.0).fundamentalDb;
                logMessage(juce::String(l ? "  LENIN" : "  STALIN") + ": gain " + juce::String(g, 3) + " dB, THD " + juce::String(h.thd, 4) + " %");
                expect(std::abs(g) <= 0.5 && h.thd < 0.3);
            }
        }

        beginTest("Response at -40 dBFS, 40 Hz - 15 kHz: LENIN +-1 dB, STALIN +-1.5 dB");
        {
            for (const auto [l, s] : { std::pair { true, false }, std::pair { false, true } })
            {
                double worst = 0.0;
                juce::String line;
                for (double hz : { 40.0, 100.0, 1000.0, 5000.0, 10000.0, 15000.0 })
                {
                    const auto x = sine(hz, -40.0, 48000);
                    const auto y = colour(x, l, s);
                    const double g = 20.0 * std::log10(amplitude(y, hz, 24000, 24000) / amplitude(x, hz, 24000, 24000));
                    worst = std::max(worst, std::abs(g));
                    line << juce::String(hz, 0) << " Hz " << juce::String(g, 2) << "  ";
                }
                logMessage(juce::String(l ? "  LENIN: " : "  STALIN: ") + line);
                expect(worst <= (l ? 1.0 : 1.5));
            }
        }

        beginTest("Harmonics vs level (1 kHz): THD rises with level; at -6 dBFS LENIN 0.3-10 %, STALIN above LENIN; LENIN H2 over H3 at -12 dBFS");
        {
            double lenin6 = 0.0, stalin6 = 0.0;
            for (const auto [l, s] : { std::pair { true, false }, std::pair { false, true } })
            {
                double previous = -1.0;
                bool rising = true;
                juce::String line;
                for (double db : { -30.0, -18.0, -12.0, -6.0, 0.0 })
                {
                    const auto h = harmonics(colour(sine(1000.0, db, 48000), l, s), 1000.0);
                    line << juce::String(db, 0) << " dBFS: THD " << juce::String(h.thd, 3) << " % H2 " << juce::String(h.h2, 1) << " H3 " << juce::String(h.h3, 1)
                         << " | ";
                    rising = rising && h.thd > previous;
                    previous = h.thd;
                    if (db == -6.0)
                        (l ? lenin6 : stalin6) = h.thd;
                    if (l && db == -12.0)
                        expect(h.h2 > h.h3, "LENIN: H2 not above H3 at -12 dBFS");
                }
                logMessage(juce::String(l ? "  LENIN " : "  STALIN ") + line);
                expect(rising, "THD does not rise with level");
            }
            expect(lenin6 >= 0.3 && lenin6 <= 10.0 && stalin6 > lenin6);
        }

        beginTest("Control: tanh has no even harmonics; LENIN does (H2 at -12 dBFS above -80 dB)");
        {
            auto x = sine(1000.0, -12.0, 48000), t = x;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < t.getNumSamples(); ++i)
                    t.setSample(ch, i, std::tanh(4.0f * t.getSample(ch, i)) / 4.0f);
            const auto tanhH = harmonics(t, 1000.0), leninH = harmonics(colour(x, true, false), 1000.0);
            logMessage("  tanh: H2 " + juce::String(tanhH.h2, 1) + " H3 " + juce::String(tanhH.h3, 1) + " dB;  LENIN: H2 " + juce::String(leninH.h2, 1) + " H3 "
                       + juce::String(leninH.h3, 1) + " dB");
            expect(tanhH.h2 < -100.0 && leninH.h2 > -80.0);
        }

        beginTest("STALIN's transformer: at -6 dBFS a 40 Hz tone distorts more than twice a 1 kHz one");
        {
            const auto low = harmonics(colour(sine(40.0, -6.0, 96000), false, true), 40.0);
            const auto mid = harmonics(colour(sine(1000.0, -6.0, 96000), false, true), 1000.0);
            logMessage("  STALIN THD: 40 Hz " + juce::String(low.thd, 3) + " %, 1 kHz " + juce::String(mid.thd, 3) + " %");
            expect(low.thd > 2.0 * mid.thd);
        }

        beginTest("Intermodulation (SMPTE 60 Hz + 7 kHz, 4:1, -6 dBFS peak): STALIN above LENIN");
        {
            const auto x = sine(60.0, -7.94, 96000, 7000.0, -19.98);
            double imd[2] {};
            int k = 0;
            for (const auto [l, s] : { std::pair { true, false }, std::pair { false, true } })
            {
                const auto y = colour(x, l, s);
                const double carrier = amplitude(y, 7000.0, 48000, 48000);
                double side = 0.0;
                for (double d : { 60.0, 120.0 })
                    for (double sgn : { -1.0, 1.0 })
                    {
                        const double a = amplitude(y, 7000.0 + sgn * d, 48000, 48000);
                        side += a * a;
                    }
                imd[k++] = 100.0 * std::sqrt(side) / carrier;
            }
            logMessage("  IMD: LENIN " + juce::String(imd[0], 3) + " %, STALIN " + juce::String(imd[1], 3) + " %");
            expect(imd[1] > imd[0]);
        }

        beginTest("Overload recovery (LENIN): after a +6 dBFS burst the -40 dBFS tone is changed for a moment (> 0.05 dB), back within 0.5 dB after 0.4 s");
        {
            juce::AudioBuffer<float> x(2, 96000);
            for (int i = 0; i < x.getNumSamples(); ++i)
            {
                const double a = i >= 24000 && i < 26400 ? 2.0 : 0.01;
                const auto v = static_cast<float>(a * std::sin(2.0 * kPi * 1000.0 * i / kFs));
                x.setSample(0, i, v);
                x.setSample(1, i, v);
            }
            const auto y = colour(x, true, false);
            const double before = amplitude(y, 1000.0, 14400, 9600);
            const double right = amplitude(y, 1000.0, 26880, 960);   // 10 - 30 ms after the burst
            const double later = amplitude(y, 1000.0, 45600, 4800);  // 0.4 s after
            const double moved = 20.0 * std::log10(right / before), back = 20.0 * std::log10(later / before);
            logMessage("  right after: " + juce::String(moved, 3) + " dB, 0.4 s later: " + juce::String(back, 3) + " dB");
            expect(std::abs(moved) > 0.05 && std::abs(back) <= 0.5);
        }

        beginTest("Block sizes 1 / 64 / 512 give the same output; no allocation; large input stays finite");
        {
            const auto x = sine(220.0, -3.0, 24000);
            const auto reference = colour(x, true, true, 512);
            std::uint64_t allocations = 0;
            expect(identical(colour(x, true, true, 64, &allocations), reference));
            expect(identical(colour(x, true, true, 1), reference));
            expectEquals(static_cast<int>(allocations), 0);
            auto loud = x;
            loud.applyGain(100.0f);
            const auto z = colour(loud, true, true);
            bool finite = true;
            for (int i = 0; i < z.getNumSamples(); ++i)
                finite = finite && std::isfinite(z.getSample(0, i));
            expect(finite);
        }

        beginTest("In the plug-in: the buttons are saved with the project, the latency is reported while on and the output lines up with it");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            plugin->setRateAndBufferSizeDetails(kFs, 512);
            plugin->prepareToPlay(kFs, 512);
            expectEquals(plugin->getLatencySamples(), 0);
            auto p = plugin->getProject();
            p.lenin = true;
            plugin->setProject(p);
            const int latency = plugin->getLatencySamples();
            logMessage("  latency with LENIN: " + juce::String(latency) + " samples");
            expect(latency > 0 && latency < 64);
            // an impulse: the peak lands at the reported latency (+-1)
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;
            buffer.clear();
            buffer.setSample(0, 0, 0.05f);
            buffer.setSample(1, 0, 0.05f);
            for (int k = 0; k < 8; ++k) // past the 20 ms fade-in
            {
                juce::AudioBuffer<float> silence(2, 512);
                silence.clear();
                plugin->processBlock(silence, midi);
            }
            plugin->processBlock(buffer, midi);
            int peak = 0;
            for (int i = 1; i < 512; ++i)
                if (std::abs(buffer.getSample(0, i)) > std::abs(buffer.getSample(0, peak)))
                    peak = i;
            logMessage("  impulse peak at " + juce::String(peak));
            expect(std::abs(peak - latency) <= 1);
            const auto xml = toValueTree(plugin->getProject()).createXml();
            const auto back = fromValueTree(juce::ValueTree::fromXml(*xml));
            expect(back.has_value() && back->lenin && !back->stalin);
            FxProjectState plain;
            expect(!toValueTree(plain).hasProperty("lenin") && !toValueTree(plain).hasProperty("stalin"));
        }

        beginTest("MIX: at 50 % the colour's H2 is 6 dB lower (+-1); MIX round-trips; the portrait buttons edit the project");
        {
            const auto x = sine(1000.0, -6.0, 48000);
            const auto full = harmonics(colour(x, true, false), 1000.0);
            TubeColour t;
            t.prepare(kFs, 512);
            t.setTargets(true, false, 0.5, 1.0);
            t.reset();
            auto y = x;
            for (int start = 0; start < y.getNumSamples(); start += 512)
            {
                float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                t.process(c, 2, juce::jmin(512, y.getNumSamples() - start));
            }
            const auto half = harmonics(y, 1000.0);
            logMessage("  H2 full " + juce::String(full.h2, 2) + " dB, MIX 50 % " + juce::String(half.h2, 2) + " dB");
            expect(std::abs((full.h2 - half.h2) - 6.02) <= 1.0);

            FxProjectState p;
            p.lenin = true;
            p.leninMix = 0.35;
            const auto back = fromValueTree(juce::ValueTree::fromXml(*toValueTree(p).createXml()));
            expect(back.has_value() && back->lenin && std::abs(back->leninMix - 0.35) < 1.0e-12 && back->stalinMix == 1.0);

            auto plugin = std::make_unique<FxPluginProcessor>();
            auto editor = std::unique_ptr<juce::AudioProcessorEditor>(plugin->createEditor());
            editor->setSize(1280, 840);
            ui::TubeButton* lenin = nullptr;
            ui::TubeButton* stalin = nullptr;
            for (auto* c : editor->getChildren())
                if (auto* b = dynamic_cast<ui::TubeButton*>(c))
                    (lenin == nullptr ? lenin : stalin) = b;
            expect(lenin != nullptr && stalin != nullptr);
            if (lenin != nullptr && stalin != nullptr)
            {
                stalin->onToggle(true);
                stalin->onMix(0.4);
                const auto now = plugin->getProject();
                expect(now.stalin && std::abs(now.stalinMix - 0.4) < 1.0e-12 && !now.lenin);
            }
            plugin->editorBeingDeleted(editor.get());
        }

        beginTest("CPU (48 kHz, 64-sample blocks, both on): p99 <= 25 % of the deadline");
        {
            TubeColour t;
            t.prepare(kFs, 64);
            t.setTargets(true, true);
            t.reset();
            auto x = sine(220.0, -6.0, 96000);
            std::vector<double> ms;
            for (int start = 0; start + 64 <= x.getNumSamples(); start += 64)
            {
                float* c[2] { x.getWritePointer(0) + start, x.getWritePointer(1) + start };
                const auto t0 = std::chrono::steady_clock::now();
                t.process(c, 2, 64);
                ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
            }
            std::sort(ms.begin(), ms.end());
            const double deadline = 64.0 / kFs * 1000.0, p50 = ms[ms.size() / 2], p99 = ms[ms.size() * 99 / 100];
            logMessage("  p50 " + juce::String(100.0 * p50 / deadline, 1) + " %, p99 " + juce::String(100.0 * p99 / deadline, 1) + " %, max "
                       + juce::String(100.0 * ms.back() / deadline, 1) + " %");
            expect(p99 <= 0.25 * deadline);
        }
    }
};

static TubePassTests tubePassTests;
} // namespace hpeq::test
