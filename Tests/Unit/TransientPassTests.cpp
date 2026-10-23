// Module pass 6: HamloUdarnik transient shaper (TRANSIENT_DESIGN.md §5).
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
#include "UI/HamloUdarnikPanel.h"

namespace hpeq::test
{
using namespace hpeq::dsp;

namespace
{
constexpr double kFs = 48000.0;
constexpr double kTwoPi = 2.0 * juce::MathConstants<double>::pi;

struct Tr
{
    float snap = 0.0f, body = 0.0f, speed = 0.5f;
    int style = 0, look = 0;
    float rise = 1.0f, decay = 1.0f, sensitivity = 1.0f, preserve = 0.0f;
    int listen = 0;
    float outputDb = 0.0f;
};

FxSlotParams tr(const Tr& t)
{
    FxSlotParams s;
    s.active = true;
    s.type = FxType::TransientShaper;
    s.values[slot::kAttackGainDb] = t.snap;
    s.values[slot::kSustainGainDb] = t.body;
    s.values[slot::kTransientSpeed] = t.speed;
    s.values[slot::kTrStyle] = static_cast<float>(t.style);
    s.values[slot::kTrLookahead] = static_cast<float>(t.look);
    s.values[slot::kTrRiseLog2] = std::log2(t.rise);
    s.values[slot::kTrDecayLog2] = std::log2(t.decay);
    s.values[slot::kTrStrictness] = 1.0f - t.sensitivity;
    s.values[slot::kTrPreserve] = t.preserve;
    s.values[slot::kTrListen] = static_cast<float>(t.listen);
    s.values[slot::kTrOutputDb] = t.outputDb;
    return s;
}

juce::AudioBuffer<float> run(const juce::AudioBuffer<float>& x, const FxSlotParams& p, int block = 256)
{
    TransientProcessor t;
    t.prepare(kFs);
    auto y = x;
    for (int start = 0; start < y.getNumSamples(); start += block)
    {
        float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
        t.process(c, 2, juce::jmin(block, y.getNumSamples() - start), p);
    }
    return y;
}

int latencyOf(const FxSlotParams& p) { return TransientProcessor::lookaheadSamples(p, kFs); }

// A(1 - e^{-t/ta}) e^{-t/td} sin(2 pi f t), added at `start` (both channels).
void addHit(juce::AudioBuffer<float>& b, int start, double amp, double hz, double taMs, double tdMs)
{
    const int length = juce::jmin(b.getNumSamples() - start, static_cast<int>(10.0 * tdMs * 1.0e-3 * kFs));
    for (int i = 0; i < length; ++i)
    {
        const double t = i / kFs;
        const double v = amp * (1.0 - std::exp(-t / (taMs * 1.0e-3))) * std::exp(-t / (tdMs * 1.0e-3)) * std::sin(kTwoPi * hz * t);
        for (int ch = 0; ch < 2; ++ch)
            b.addSample(ch, start + i, static_cast<float>(v));
    }
}

juce::AudioBuffer<float> silence(double seconds)
{
    juce::AudioBuffer<float> b(2, static_cast<int>(seconds * kFs));
    b.clear();
    return b;
}

// Energy of both channels over [from, from + length).
double energy(const juce::AudioBuffer<float>& b, int from, int length)
{
    double e = 0.0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = juce::jmax(0, from); i < juce::jmin(b.getNumSamples(), from + length); ++i)
            e += static_cast<double>(b.getSample(ch, i)) * b.getSample(ch, i);
    return e;
}

// Gain (dB) of the window [onset, onset + length) of y (delayed by L) against x.
double windowGainDb(const juce::AudioBuffer<float>& y, const juce::AudioBuffer<float>& x, int onset, int length, int L)
{
    return 10.0 * std::log10((energy(y, onset + L, length) + 1.0e-30) / (energy(x, onset, length) + 1.0e-30));
}

// Envelope: peak of |x| (channel 0) over a centred window of `half` samples each side.
std::vector<double> envelope(const juce::AudioBuffer<float>& b, int from, int length, int half)
{
    std::vector<double> e(static_cast<size_t>(length), 0.0);
    for (int i = 0; i < length; ++i)
        for (int k = -half; k <= half; ++k)
        {
            const int j = from + i + k;
            if (j >= 0 && j < b.getNumSamples())
                e[static_cast<size_t>(i)] = std::max(e[static_cast<size_t>(i)], static_cast<double>(std::abs(b.getSample(0, j))));
        }
    return e;
}

// 10 -> 90 % rise time (samples) of an envelope up to its peak.
double riseTime(const std::vector<double>& e)
{
    const auto peakIt = std::max_element(e.begin(), e.end());
    const double peak = *peakIt;
    const auto peakAt = static_cast<int>(peakIt - e.begin());
    int t10 = -1, t90 = -1;
    for (int i = 0; i <= peakAt; ++i)
    {
        if (t10 < 0 && e[static_cast<size_t>(i)] >= 0.1 * peak)
            t10 = i;
        if (t90 < 0 && e[static_cast<size_t>(i)] >= 0.9 * peak)
            t90 = i;
    }
    return static_cast<double>(t90 - t10);
}

// Decay time constant (ms): straight-line fit of the envelope in dB between loDb and hiDb under its peak.
double decayTauMs(const std::vector<double>& e, double hiDb, double loDb)
{
    const auto peakIt = std::max_element(e.begin(), e.end());
    const double peak = *peakIt;
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int n = 0;
    for (auto it = peakIt; it != e.end(); ++it)
    {
        const double d = 20.0 * std::log10(*it / peak + 1.0e-12);
        if (d > -hiDb)
            continue;
        if (d < -loDb)
            break;
        const double t = static_cast<double>(it - peakIt) / kFs * 1000.0;
        sx += t;
        sy += d;
        sxx += t * t;
        sxy += t * d;
        ++n;
    }
    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx); // dB per ms
    return -20.0 / std::log(10.0) / slope;
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

double interSamplePeakDb(const juce::AudioBuffer<float>& y)
{
    double peak = 0.0;
    constexpr int half = 32;
    for (int ch = 0; ch < y.getNumChannels(); ++ch)
        for (int n = half; n < y.getNumSamples() - half; ++n)
        {
            if (std::abs(y.getSample(ch, n)) < 0.25f * static_cast<float>(peak))
                continue; // cannot reach the peak between these samples (bound of the sinc sum is generous)
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
        }
    return 20.0 * std::log10(peak);
}

// A drum pattern with known onsets: kick (60 Hz falling to 45 Hz, 90 ms), snare (190 Hz + noise, 70 ms),
// eighth hats (7 kHz, 25 ms, -14 dB). 120 BPM, `bars` bars.
juce::AudioBuffer<float> drumPattern(int bars, std::vector<int>& onsets)
{
    auto b = silence(bars * 2.0 + 0.5);
    juce::Random random(1961);
    const int beat = static_cast<int>(0.5 * kFs);
    for (int k = 0; k < bars * 8; ++k)
    {
        const int start = static_cast<int>(0.05 * kFs) + k * beat / 2;
        if (k % 2 == 0)
        {
            const bool kick = (k / 2) % 2 == 0;
            onsets.push_back(start);
            const int length = static_cast<int>((kick ? 0.6 : 0.45) * kFs);
            double phase = 0.0;
            for (int i = 0; i < length && start + i < b.getNumSamples(); ++i)
            {
                const double t = i / kFs;
                double v;
                if (kick)
                {
                    phase += kTwoPi * (45.0 + 15.0 * std::exp(-t / 0.03)) / kFs;
                    v = 0.7 * (1.0 - std::exp(-t / 0.0005)) * std::exp(-t / 0.09) * std::sin(phase);
                }
                else
                    v = (1.0 - std::exp(-t / 0.0003)) * std::exp(-t / 0.07)
                        * (0.4 * std::sin(kTwoPi * 190.0 * t) + 0.3 * (random.nextFloat() * 2.0f - 1.0f));
                for (int ch = 0; ch < 2; ++ch)
                    b.addSample(ch, start + i, static_cast<float>(v));
            }
        }
        addHit(b, static_cast<int>(0.05 * kFs) + k * beat / 2 + beat / 4, 0.2, 7000.0, 0.1, 25.0);
    }
    return b;
}
} // namespace

class TransientPassTests final : public juce::UnitTest
{
public:
    TransientPassTests() : juce::UnitTest("Transient pass", "HPEQ") {}

    void runTest() override
    {
        std::vector<int> drumOnsets;
        const auto drums = drumPattern(4, drumOnsets);

        beginTest("T1. Keys absent = M6 (no HamloUdarnik, latency 0); neutral settings in every mode / look-ahead = input delayed by L, bit-exact");
        {
            expect(!TransientProcessor::usesUdarnik(tr({})));
            expectEquals(latencyOf(tr({})), 0);
            Tr look;
            look.look = 1;
            expectEquals(latencyOf(tr(look)), 96);
            look.look = 2;
            expectEquals(latencyOf(tr(look)), 240);
            for (int style : { 0, 1, 2 })
                for (int lk : { 0, 1, 2 })
                {
                    Tr t;
                    t.style = style;
                    t.look = lk;
                    t.preserve = 0.5f;
                    t.sensitivity = style == 0 ? 1.0f : 0.7f;
                    const auto p = tr(t);
                    expect(TransientProcessor::usesUdarnik(p));
                    const auto y = run(drums, p);
                    const auto r = compareAligned(y, drums, latencyOf(p));
                    expect(r.maxAbsError == 0.0, "style " + juce::String(style) + " look " + juce::String(lk));
                }
        }

        beginTest("T2. Latency reported = measured; blocks 1 / 64 / 512 identical; 0 allocations");
        {
            const auto quiet = makeWhiteNoise({ kFs, 2, 0.5, kFixtureSeed }, -30.0);
            for (int lk : { 0, 1, 2 })
            {
                Tr t;
                t.look = lk;
                t.style = 1;
                const auto p = tr(t);
                expectEquals(estimateDelay(run(quiet, p), quiet, 600), latencyOf(p));
            }
            for (int style : { 0, 1, 2 })
            {
                Tr t;
                t.style = style;
                t.look = 2;
                t.snap = 6.0f;
                t.body = -4.0f;
                t.rise = 0.7f;
                t.decay = 1.5f;
                t.sensitivity = 0.8f;
                t.preserve = 0.5f;
                t.outputDb = -1.0f;
                const auto p = tr(t);
                const auto ref = run(drums, p, 1);
                for (int block : { 64, 512 })
                    expect(compareAligned(run(drums, p, block), ref).bitExact, "style " + juce::String(style) + " block " + juce::String(block));
                TransientProcessor proc;
                proc.prepare(kFs);
                auto y = drums;
                std::uint64_t allocations = 0;
                for (int start = 0; start < y.getNumSamples(); start += 512)
                {
                    float* c[2] { y.getWritePointer(0) + start, y.getWritePointer(1) + start };
                    ScopedAllocationProbe probe;
                    proc.process(c, 2, juce::jmin(512, y.getNumSamples() - start), p);
                    allocations += probe.counts().allocations;
                }
                expectEquals(static_cast<int>(allocations), 0);
            }
        }

        beginTest("T3. Look-ahead: PRAVDA SNAP +12 dB, gain on the first sample of a hit >= 90 % (10.8 dB) with VOSTOK and SOYUZ");
        {
            auto hits = silence(3.0);
            std::vector<int> onsets;
            for (int k = 0; k < 10; ++k)
            {
                onsets.push_back(static_cast<int>((0.1 + 0.27 * k) * kFs));
                addHit(hits, onsets.back(), 0.5, 3000.0 + 170.0 * k, 0.2, 50.0);
            }
            for (int lk : { 0, 1, 2 })
            {
                Tr t;
                t.snap = 12.0f;
                t.look = lk;
                const auto p = tr(t);
                const int L = latencyOf(p);
                const auto y = run(hits, p);
                double sum = 0.0;
                for (int onset : onsets)
                {
                    int first = onset;
                    while (std::abs(hits.getSample(0, first)) < 0.005f)
                        ++first;
                    sum += 20.0 * std::log10(std::abs(y.getSample(0, first + L) / hits.getSample(0, first)));
                }
                const double mean = sum / static_cast<double>(onsets.size());
                logMessage("  " + juce::String(lk == 0 ? "SPUTNIK" : lk == 1 ? "VOSTOK" : "SOYUZ") + ": gain on the first sample " + juce::String(mean, 2) + " dB");
                if (lk > 0)
                    expect(mean >= 10.8);
            }
        }

        beginTest("T4. Confidence: noise without hits changes <= 1 dB (50 ms windows); hits over noise get >= 11 dB; bleed 30 dB under (sensitivity 0.5) <= 1 dB");
        {
            const auto noise = makePinkNoise({ kFs, 2, 4.0, kFixtureSeed }, -20.0);
            const auto worstWindow = [&](const juce::AudioBuffer<float>& y, const juce::AudioBuffer<float>& x, int L)
            {
                const int w = static_cast<int>(0.05 * kFs);
                double worst = 0.0;
                for (int s = static_cast<int>(0.5 * kFs); s + w + L < x.getNumSamples(); s += w)
                    worst = std::max(worst, std::abs(windowGainDb(y, x, s, w, L)));
                return worst;
            };
            for (const auto& [style, sens] : { std::pair { 1, 1.0f }, std::pair { 2, 1.0f }, std::pair { 0, 0.5f } })
            {
                Tr t;
                t.style = style;
                t.look = 1;
                t.snap = 12.0f;
                t.body = -12.0f;
                t.sensitivity = sens;
                const auto p = tr(t);
                const double worst = worstWindow(run(noise, p), noise, latencyOf(p));
                logMessage("  noise, style " + juce::String(style) + " sensitivity " + juce::String(sens) + ": worst 50 ms change " + juce::String(worst, 2) + " dB");
                expect(worst <= 1.0);
            }

            auto over = makePinkNoise({ kFs, 2, 4.0, kFixtureSeed }, -26.0);
            std::vector<int> onsets;
            for (int k = 0; k < 12; ++k)
            {
                onsets.push_back(static_cast<int>((0.3 + 0.3 * k) * kFs));
                addHit(over, onsets.back(), 0.5, 2500.0, 0.3, 40.0);
            }
            {
                Tr t;
                t.style = 2;
                t.look = 1;
                t.snap = 12.0f;
                const auto p = tr(t);
                const auto y = run(over, p);
                double mean = 0.0;
                for (int onset : onsets)
                    mean += windowGainDb(y, over, onset, static_cast<int>(0.01 * kFs), latencyOf(p)) / static_cast<double>(onsets.size());
                logMessage("  hits over noise (PERESTROIKA, SNAP +12): attack-window gain " + juce::String(mean, 2) + " dB");
                expect(mean >= 11.0);
            }

            auto bleed = silence(4.0);
            std::vector<int> accents, quiet;
            for (int k = 0; k < 7; ++k)
            {
                accents.push_back(static_cast<int>((0.2 + 0.5 * k) * kFs));
                quiet.push_back(accents.back() + static_cast<int>(0.25 * kFs));
                addHit(bleed, accents.back(), 0.5, 2000.0, 0.3, 40.0);
                addHit(bleed, quiet.back(), 0.5 * std::pow(10.0, -30.0 / 20.0), 6000.0, 0.1, 20.0);
            }
            for (int style : { 0, 2 })
            {
                Tr t;
                t.style = style;
                t.look = 1;
                t.snap = 12.0f;
                t.sensitivity = 0.5f;
                const auto p = tr(t);
                const auto y = run(bleed, p);
                const int L = latencyOf(p), w = static_cast<int>(0.01 * kFs);
                double accentGain = 0.0, bleedWorst = 0.0;
                for (size_t k = 1; k < accents.size(); ++k) // the first accent sets the reference
                {
                    accentGain += windowGainDb(y, bleed, accents[k], w, L) / static_cast<double>(accents.size() - 1);
                    bleedWorst = std::max(bleedWorst, std::abs(windowGainDb(y, bleed, quiet[k], w, L)));
                }
                logMessage("  bleed, style " + juce::String(style) + ": accents " + juce::String(accentGain, 2) + " dB, bleed worst " + juce::String(bleedWorst, 2) + " dB");
                expect(accentGain >= 11.0);
                expect(bleedWorst <= 1.0);
            }
        }

        beginTest("T5. UDARNIK SNAP +6 SOYUZ on drums: contrast >= +3 dB; sample peak <= input; true peak <= input + 0.1 dB; loudness +-0.5 LU");
        {
            Tr t;
            t.style = 1;
            t.look = 2;
            t.snap = 6.0f;
            const auto p = tr(t);
            const int L = latencyOf(p);
            const auto y = run(drums, p);
            const int a = static_cast<int>(0.03 * kFs), tail = static_cast<int>(0.27 * kFs);
            double contrastIn = 0.0, contrastOut = 0.0;
            for (int onset : drumOnsets)
            {
                contrastIn += 10.0 * std::log10(energy(drums, onset, a) / energy(drums, onset + a, tail)) / static_cast<double>(drumOnsets.size());
                contrastOut += 10.0 * std::log10(energy(y, onset + L, a) / energy(y, onset + L + a, tail)) / static_cast<double>(drumOnsets.size());
            }
            const double peakIn = measureAll(drums).peakDbfs, peakOut = measureAll(y).peakDbfs;
            const double tpIn = interSamplePeakDb(drums), tpOut = interSamplePeakDb(y);
            const double lufsIn = integratedLufs(drums), lufsOut = integratedLufs(y);
            logMessage("  contrast " + juce::String(contrastIn, 2) + " -> " + juce::String(contrastOut, 2) + " dB (" + juce::String(contrastOut - contrastIn, 2)
                       + "); sample peak " + juce::String(peakIn, 3) + " -> " + juce::String(peakOut, 3) + " dBFS; true peak " + juce::String(tpIn, 3) + " -> "
                       + juce::String(tpOut, 3) + " dBTP; loudness " + juce::String(lufsIn, 2) + " -> " + juce::String(lufsOut, 2) + " LUFS");
            expect(contrastOut - contrastIn >= 3.0);
            expect(peakOut <= peakIn + 1.0e-5);
            expect(tpOut <= tpIn + 0.1);
            expect(std::abs(lufsOut - lufsIn) <= 0.5);

            Tr plain;
            plain.snap = 6.0f;
            plain.look = 2;
            const auto classic = run(drums, tr(plain)); // PRAVDA for comparison (reported)
            double contrastClassic = 0.0;
            for (int onset : drumOnsets)
                contrastClassic += 10.0 * std::log10(energy(classic, onset + L, a) / energy(classic, onset + L + a, tail)) / static_cast<double>(drumOnsets.size());
            logMessage("  for comparison PRAVDA SNAP +6: contrast " + juce::String(contrastClassic - contrastIn, 2) + " dB, sample peak "
                       + juce::String(measureAll(classic).peakDbfs, 3) + " dBFS, loudness " + juce::String(integratedLufs(classic), 2) + " LUFS");
        }

        beginTest("T6. PERESTROIKA SOYUZ: DECAY x0.5 / x2 -> tau x K (+-10 %); RISE x0.5 / x2 -> 10-90 % rise x r (+-15 %)");
        {
            for (const double hz : { 4000.0, 200.0 })
            {
                auto hits = silence(4.0);
                std::vector<int> onsets;
                for (int k = 0; k < 5; ++k)
                {
                    onsets.push_back(static_cast<int>((0.2 + 0.75 * k) * kFs));
                    addHit(hits, onsets.back(), 0.5, hz, 1.0, 60.0);
                }
                const int half = static_cast<int>(kFs / hz); // one carrier period each side
                const int length = static_cast<int>(0.7 * kFs);
                const double tauIn = decayTauMs(envelope(hits, onsets[2], length, half), 6.0, 30.0);
                for (const float k : { 0.5f, 2.0f })
                {
                    Tr t;
                    t.style = 2;
                    t.look = 2;
                    t.decay = k;
                    const auto p = tr(t);
                    const auto y = run(hits, p);
                    double sum = 0.0;
                    for (size_t h = 1; h < onsets.size(); ++h)
                        sum += decayTauMs(envelope(y, onsets[h] + latencyOf(p), length, half), 6.0, 30.0) / static_cast<double>(onsets.size() - 1);
                    const double ratio = sum / tauIn;
                    logMessage("  " + juce::String(hz, 0) + " Hz DECAY x" + juce::String(k) + ": tau " + juce::String(tauIn, 1) + " -> " + juce::String(sum, 1)
                               + " ms (x" + juce::String(ratio, 3) + ")");
                    expect(std::abs(ratio / k - 1.0) <= 0.10);
                }
                if (hz < 1000.0)
                    continue; // RISE is meant for bands above ~200 Hz (design note 3.5)
                const int riseLength = static_cast<int>(0.03 * kFs), lead = static_cast<int>(0.002 * kFs); // start before the onset: t10 is not cut off
                const double riseIn = riseTime(envelope(hits, onsets[2] - lead, riseLength, half));
                for (const float r : { 0.5f, 2.0f })
                {
                    Tr t;
                    t.style = 2;
                    t.look = 2;
                    t.rise = r;
                    const auto p = tr(t);
                    const auto y = run(hits, p);
                    double sum = 0.0;
                    for (size_t h = 1; h < onsets.size(); ++h)
                        sum += riseTime(envelope(y, onsets[h] + latencyOf(p) - lead, riseLength, half)) / static_cast<double>(onsets.size() - 1);
                    const double ratio = sum / riseIn;
                    logMessage("  " + juce::String(hz, 0) + " Hz RISE x" + juce::String(r) + ": 10-90 % rise " + juce::String(riseIn / kFs * 1000.0, 2) + " -> "
                               + juce::String(sum / kFs * 1000.0, 2) + " ms (x" + juce::String(ratio, 3) + ")");
                    expect(std::abs(ratio / r - 1.0) <= 0.15);
                }
            }
        }

        beginTest("T7. PRESERVE 1: ghost/accent attack-energy ratio changes <= 0.5 dB (PRAVDA, UDARNIK; PRESERVE 0 reported)");
        {
            auto groove = silence(4.0);
            std::vector<int> accents, ghosts;
            for (int k = 0; k < 7; ++k)
            {
                accents.push_back(static_cast<int>((0.2 + 0.5 * k) * kFs));
                ghosts.push_back(accents.back() + static_cast<int>(0.1 * kFs));
                addHit(groove, accents.back(), 0.5, 1000.0, 0.3, 15.0);
                addHit(groove, ghosts.back(), 0.5 * std::pow(10.0, -12.0 / 20.0), 1000.0, 0.3, 15.0);
            }
            for (int style : { 0, 1 })
                for (float preserve : { 0.0f, 1.0f })
                {
                    Tr t;
                    t.style = style;
                    t.look = style == 0 ? 1 : 2;
                    t.snap = 6.0f;
                    t.preserve = preserve;
                    const auto p = tr(t);
                    const auto y = run(groove, p);
                    const int L = latencyOf(p), w = static_cast<int>(0.02 * kFs);
                    double change = 0.0;
                    for (size_t k = 1; k < accents.size(); ++k)
                        change += (windowGainDb(y, groove, ghosts[k], w, L) - windowGainDb(y, groove, accents[k], w, L)) / static_cast<double>(accents.size() - 1);
                    logMessage("  " + juce::String(style == 0 ? "PRAVDA" : "UDARNIK") + " PRESERVE " + juce::String(preserve) + ": ghost/accent ratio change "
                               + juce::String(change, 2) + " dB");
                    if (preserve >= 1.0f)
                        expect(std::abs(change) <= 0.5);
                }
        }

        beginTest("T8. LISTEN: ATTACK + BODY = FULL (<= 1e-6), every mode");
        {
            for (int style : { 0, 1, 2 })
            {
                Tr t;
                t.style = style;
                t.look = 1;
                t.snap = 6.0f;
                t.body = -6.0f;
                const auto full = run(drums, tr(t));
                t.listen = 1;
                const auto attack = run(drums, tr(t));
                t.listen = 2;
                const auto bodyPart = run(drums, tr(t));
                double worst = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < drums.getNumSamples(); ++i)
                        worst = std::max(worst, static_cast<double>(std::abs(attack.getSample(ch, i) + bodyPart.getSample(ch, i) - full.getSample(ch, i))));
                logMessage("  style " + juce::String(style) + ": worst |attack + body - full| " + juce::String(worst));
                expect(worst <= 1.0e-6);
            }
        }

        beginTest("T9. Onset not moved (SNAP +6, every mode, RISE 1): output onset = input onset + L within 0.1 ms");
        {
            auto hits = silence(2.0);
            std::vector<int> onsets;
            for (int k = 0; k < 6; ++k)
            {
                onsets.push_back(static_cast<int>((0.1 + 0.3 * k) * kFs));
                addHit(hits, onsets.back(), 0.5, 2500.0, 0.3, 40.0);
            }
            const auto onsetOf = [](const juce::AudioBuffer<float>& b, int from)
            {
                float peak = 0.0f;
                for (int i = from; i < from + static_cast<int>(0.05 * kFs); ++i)
                    peak = std::max(peak, std::abs(b.getSample(0, i)));
                int i = from;
                while (std::abs(b.getSample(0, i)) < 0.05f * peak)
                    ++i;
                return i;
            };
            for (int style : { 0, 1, 2 })
            {
                Tr t;
                t.style = style;
                t.look = 1;
                t.snap = 6.0f;
                const auto p = tr(t);
                const auto y = run(hits, p);
                double worst = 0.0;
                for (int onset : onsets)
                {
                    const int from = onset - static_cast<int>(0.002 * kFs);
                    worst = std::max(worst, std::abs(static_cast<double>(onsetOf(y, from + latencyOf(p)) - latencyOf(p) - onsetOf(hits, from))) / kFs * 1000.0);
                }
                logMessage("  style " + juce::String(style) + ": worst onset shift " + juce::String(worst, 3) + " ms");
                expect(worst <= 0.1);
            }
        }

        beginTest("T10. OUTPUT -6 / +6 dB = output x 10^(O/20) (1e-6 relative)");
        {
            Tr t;
            t.style = 1;
            t.look = 1;
            t.snap = 6.0f;
            const auto ref = run(drums, tr(t));
            for (float o : { -6.0f, 6.0f })
            {
                t.outputDb = o;
                const auto y = run(drums, tr(t));
                const double g = std::pow(10.0, o / 20.0);
                double worst = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < y.getNumSamples(); ++i)
                        worst = std::max(worst, std::abs(y.getSample(ch, i) - ref.getSample(ch, i) * g));
                expect(worst <= 1.0e-6 * std::max(1.0, g));
            }
        }

        beginTest("T11. Page in the real editor: fills the body; knobs, MIX and levers edit the project; an older transient shows the M6 defaults; latency reaches the engine");
        {
            auto plugin = std::make_unique<FxPluginProcessor>();
            FxProjectState p;
            FxNodeState n;
            n.id = 1;
            FxSlotState old;
            old.type = FxType::TransientShaper; // saved before this pass: no HamloUdarnik keys
            old.parameters.set("attackDb", 4.0);
            n.chain = { old };
            p.layers[0].fxNodes = { n };
            p.nextNodeId = 2;
            plugin->setProject(p);
            auto editor = std::unique_ptr<FxPluginEditor>(dynamic_cast<FxPluginEditor*>(plugin->createEditor()));
            editor->setSize(1400, 900);
            editor->selectNode(true, 1, 0);
            ui::HamloUdarnikPanel* panel = nullptr;
            juce::Array<juce::Component*> stack { editor.get() };
            while (!stack.isEmpty())
            {
                auto* c = stack.removeAndReturn(stack.size() - 1);
                for (auto* child : c->getChildren())
                    stack.add(child);
                if (auto* w = dynamic_cast<ui::HamloUdarnikPanel*>(c))
                    panel = w;
            }
            expect(panel != nullptr, "no HamloUdarnik panel on the transient page");
            if (panel != nullptr)
            {
                expect(panel->getWidth() > 1000 && panel->getHeight() > 500);
                const auto& v = panel->getValues();
                expect(v.knobs[ui::HamloUdarnikPanel::Snap] == 4.0 && v.knobs[ui::HamloUdarnikPanel::Sensitivity] == 1.0
                       && v.knobs[ui::HamloUdarnikPanel::Preserve] == 0.0 && v.switches[ui::HamloUdarnikPanel::Lookahead] == 0
                       && v.knobs[ui::HamloUdarnikPanel::Mode] == 0.0);
                expect(!TransientProcessor::usesUdarnik(makeSnapshot(plugin->getProject().layers[0]).fx[0].slots[0]));
                panel->onKnobChange(ui::HamloUdarnikPanel::Mode, 1.0);
                panel->onKnobChange(ui::HamloUdarnikPanel::Mix, 0.5);
                panel->onKnobChange(ui::HamloUdarnikPanel::Decay, 1.5);
                panel->onSwitchChange(ui::HamloUdarnikPanel::Lookahead, 2);
                const auto project = plugin->getProject(); // a copy: keep it alive while reading from it
                const auto& m = project.layers[0].fxNodes[0].chain[0];
                expectEquals(static_cast<double>(m.parameters["style"]), 1.0);
                expectEquals(static_cast<double>(m.mix), 0.5);
                expectEquals(static_cast<double>(m.parameters["decay"]), 1.5);
                expectEquals(static_cast<int>(m.parameters["lookahead"]), 2);
                const auto snap = makeSnapshot(project.layers[0]);
                expect(TransientProcessor::usesUdarnik(snap.fx[0].slots[0]));
                expectEquals(ProjectEngine::latencyFor(project, kFs), 240);
            }
            FxProjectState q;
            q.layers[0].mixChain = { defaultModule(FxType::TransientShaper) };
            expectEquals(ProjectEngine::latencyFor(q, kFs), 96); // a module added from the UI: VOSTOK
        }
    }
};

static TransientPassTests transientPassTests;
} // namespace hpeq::test
