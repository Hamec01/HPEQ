#include "DSP/DrumPhraseFill.h"
#include "TestSignals.h"

#include <set>

// R17 FILL ENGINE (docs/R17_FILL_ENGINE.md): the targets F1 .. F8. They prove that each type does what its name says,
// not that it sounds good - the owner's listening decides that.
namespace hamlo::test
{
namespace
{
namespace tr = hpeq::dsp::transition;
namespace ph = hpeq::dsp::transition::phrase;

// a boom bap / trap style loop at any tempo: kick on 1, the "and" of 2 and 3, snare on 2 and 4, hats on the 8ths
juce::AudioBuffer<float> loop(double bpm, int bars, int seed = 5)
{
    const double beat = kFs * 60.0 / bpm;
    const int n = static_cast<int>(std::lround(bars * 4 * beat));
    juce::AudioBuffer<float> x(2, n);
    x.clear();
    juce::Random rng(seed);
    const auto add = [&](double at, int kind)
    {
        const int from = static_cast<int>(std::lround(at));
        for (int i = 0; i < static_cast<int>(0.35 * kFs) && from + i < n; ++i)
        {
            const double t = i / kFs, noise = 2.0 * rng.nextDouble() - 1.0;
            double v = 0;
            if (kind == 0)
                v = 0.75 * std::exp(-t / 0.10) * std::sin(2.0 * juce::MathConstants<double>::pi * (48.0 + 110.0 * std::exp(-t / 0.010)) * t);
            else if (kind == 1)
                v = 0.45 * std::exp(-t / 0.06) * noise + 0.3 * std::exp(-t / 0.05) * std::sin(2.0 * juce::MathConstants<double>::pi * 190.0 * t);
            else
                v = 0.12 * std::exp(-t / 0.012) * noise;
            x.addSample(0, from + i, static_cast<float>(v));
            x.addSample(1, from + i, static_cast<float>(v * (kind == 2 ? 0.6 : 1.0)));
        }
    };
    for (int b = 0; b < bars * 4; ++b)
    {
        const double at = b * beat;
        if (b % 4 == 0 || b % 4 == 2)
            add(at, 0);
        if (b % 4 == 1)
            add(at + 0.5 * beat, 0);
        if (b % 2 == 1)
            add(at, 1);
        add(at, 2);
        add(at + 0.5 * beat, 2);
    }
    return x;
}

tr::Settings settings(int recipe, double bpm, int bars, int beats, std::uint64_t seed)
{
    tr::Settings s;
    s.kind = tr::Fill;
    s.recipe = recipe;
    s.bpm = bpm;
    s.bars = bars;
    s.sampleRate = kFs;
    s.fillBeats = beats;
    s.seed = seed;
    s.knobs = tr::defaultKnobs(tr::Fill, recipe);
    return s;
}

double peakOf(const juce::AudioBuffer<float>& x, int from, int to)
{
    double p = 0;
    for (int c = 0; c < x.getNumChannels(); ++c)
        for (int n = std::max(0, from); n < std::min(to, x.getNumSamples()); ++n)
            p = std::max(p, static_cast<double>(std::abs(x.getSample(c, n))));
    return p;
}
} // namespace

class DrumFillR17Tests final : public juce::UnitTest
{
public:
    DrumFillR17Tests() : juce::UnitTest("Drum fill R17", "HamloRevolution") {}
    void runTest() override
    {
        beginTest("F7: every type, length, peak, untouched groove, determinism");
        {
            for (double bpm : { 90.0, 140.0, 174.0 })
            {
                const auto x = loop(bpm, 2);
                const double sourcePeak = peakOf(x, 0, x.getNumSamples());
                for (int recipe = tr::kR17Recipe; recipe <= tr::kR17Last; ++recipe)
                    for (int beats : { 1, 2, 4, 8 })
                        for (std::uint64_t seed : { 1ull, 2ull, 3ull, 4ull })
                        {
                            auto s = settings(recipe, bpm, 2, beats, seed);
                            tr::Plan p;
                            const auto y = tr::renderR17Fill(x, s, &p);
                            expectEquals(y.getNumSamples(), tr::lengthSamples(s));
                            bool finite = true;
                            for (int c = 0; c < 2; ++c)
                                for (int n = 0; n < y.getNumSamples(); ++n)
                                    finite = finite && std::isfinite(y.getSample(c, n));
                            expect(finite, "finite");
                            // the peak: no louder than the source or -1.8 dBTP (+ the sample-peak vs true-peak margin)
                            expect(peakOf(y, 0, y.getNumSamples()) <= std::max(std::min(sourcePeak, 0.98), std::pow(10.0, -1.8 / 20.0)) + 1e-3, "ceiling");
                            // the groove before the fill: bit-exact (50 ms margin for pre-roll and fades)
                            const double beat = kFs * 60.0 / bpm;
                            const int untouched = static_cast<int>(y.getNumSamples() - std::min(beats, 8) * beat - 0.05 * kFs);
                            bool exact = true;
                            for (int c = 0; c < 2 && untouched > 0; ++c)
                                for (int n = 0; n < untouched; ++n)
                                    exact = exact && y.getSample(c, n) == x.getSample(c, n);
                            expect(exact, "groove before the fill unchanged");
                            expect(p.status == ph::Generated, "generated: " + p.reason);
                            expect(bitExact(y, tr::renderR17Fill(x, s)), "deterministic");
                            if (seed == 1 && beats == 4)
                                expect(bitExact(y, tr::render(x, s)), "render() dispatch");
                        }
            }
        }

        const auto x = loop(95, 2);
        beginTest("F1: BUILD accelerates and grows");
        {
            for (std::uint64_t seed : { 1ull, 7ull, 9ull })
            {
                auto s = settings(tr::kR17Recipe + 1, 95, 2, 4, seed);
                s.knobs[3] = 0; // no drop: count the whole last beat
                const auto p = tr::makeR17FillPlan(x, s);
                const double beat = kFs * 60.0 / 95, start = x.getNumSamples() - 4 * beat;
                int first = 0, last = 0;
                double gFirst = 0, gLast = 0;
                for (const auto& e : p.events)
                {
                    if (e.role != ph::Snare)
                        continue;
                    const double b = (e.at - start) / beat;
                    if (b < 0.9) { ++first; gFirst = std::max(gFirst, e.velocity); }
                    if (b >= 2.9) { ++last; gLast = std::max(gLast, e.velocity); }
                }
                expect(first > 0 && last >= 4 * first, "strokes: first " + juce::String(first) + ", last " + juce::String(last));
                expect(gFirst > 0 && gLast >= 2.0 * gFirst * 0.999, "level: +" + juce::String(20 * std::log10(gLast / std::max(1e-9, gFirst)), 1) + " dB");
            }
        }
        beginTest("F2: AROUND THE KIT walks down three toms");
        {
            for (std::uint64_t seed : { 1ull, 2ull, 3ull, 4ull, 5ull })
            {
                const auto p = tr::makeR17FillPlan(x, settings(tr::kR17Recipe + 2, 95, 2, 4, seed));
                std::vector<double> rates;
                for (const auto& e : p.events)
                    if (e.role == ph::Tom)
                        rates.push_back(e.rate);
                std::set<double> distinct(rates.begin(), rates.end());
                expect(distinct.size() >= 3, "three toms (made from the snare)");
                bool falling = true;
                for (size_t i = 1; i < rates.size(); ++i)
                    falling = falling && rates[i] <= rates[i - 1] + 1e-12;
                expect(falling, "high -> low");
                expect(p.reason.contains("TOMS MADE"), p.reason);
            }
        }
        beginTest("F3: BIG SNARE lands loud with a long tail");
        {
            for (std::uint64_t seed : { 1ull, 2ull, 3ull })
            {
                auto s = settings(tr::kR17Recipe + 3, 95, 2, 4, seed);
                s.knobs[3] = 0;
                tr::Plan p;
                const auto y = tr::renderR17Fill(x, s, &p);
                double big = 0, other = 0;
                int bigAt = 0;
                for (const auto& e : p.events)
                    if (e.role == ph::Snare)
                    {
                        if (e.space >= 0.6) { big = std::max(big, e.velocity); bigAt = e.at; }
                        else other = std::max(other, e.velocity);
                    }
                expect(big >= std::pow(10.0, 3.0 / 20.0) * other, "landing +" + juce::String(20 * std::log10(big / std::max(1e-9, other)), 1) + " dB");
                // the tail: still ringing at the end of the clip (it is shorter than 0.8 s after the landing here)
                const int end = y.getNumSamples();
                expect(end - bigAt < static_cast<int>(0.8 * kFs + 0.6 * kFs));
                expect(peakOf(y, end - static_cast<int>(0.03 * kFs), end - static_cast<int>(0.004 * kFs)) > 0.003 * peakOf(y, bigAt, end), "tail rings to the end");
            }
        }
        beginTest("F4: GROOVE BREAK keeps the groove");
        {
            for (std::uint64_t seed : { 1ull, 2ull, 3ull })
            {
                auto s = settings(tr::kR17Recipe + 4, 95, 2, 4, seed);
                s.kitEnabled.fill(false); // no added strokes: what is left before the last beat is the groove itself
                const auto y = tr::renderR17Fill(x, s);
                const double beat = kFs * 60.0 / 95;
                const int lastBeat = static_cast<int>(x.getNumSamples() - beat - 0.02 * kFs);
                bool exact = true;
                for (int c = 0; c < 2; ++c)
                    for (int n = 0; n < lastBeat; ++n)
                        exact = exact && y.getSample(c, n) == x.getSample(c, n);
                expect(exact, "the groove plays on until the last beat");
            }
        }
        beginTest("F5: TRAP ROLL speeds up to 1/32");
        {
            for (std::uint64_t seed : { 1ull, 2ull })
            {
                auto s = settings(tr::kR17Recipe + 5, 140, 2, 4, seed);
                s.knobs[3] = 0;
                const auto p = tr::makeR17FillPlan(loop(140, 2), s);
                std::vector<int> at;
                for (const auto& e : p.events)
                    if (e.role == ph::Hat)
                        at.push_back(e.at);
                std::set<int> gaps;
                const double beat = kFs * 60.0 / 140;
                for (size_t i = 1; i < at.size(); ++i)
                    gaps.insert(static_cast<int>(std::lround(beat / (at[i] - at[i - 1]))));
                expect(gaps.size() >= 4, "stroke rates: " + juce::String(static_cast<int>(gaps.size())));
                expect(!gaps.empty() && *gaps.rbegin() >= 8, "fastest 1/32");
            }
        }
        beginTest("F6: a sustained sample is chopped");
        {
            juce::AudioBuffer<float> pad(2, tr::lengthSamples(settings(tr::kR17Recipe, 100, 2, 4, 1)));
            for (int n = 0; n < pad.getNumSamples(); ++n)
            {
                const double t = n / kFs, v = 0.3 * std::sin(2 * juce::MathConstants<double>::pi * 220 * t) + 0.2 * std::sin(2 * juce::MathConstants<double>::pi * 330 * t * (1 + 0.1 * std::sin(t)));
                pad.setSample(0, n, static_cast<float>(v));
                pad.setSample(1, n, static_cast<float>(v));
            }
            for (std::uint64_t seed : { 1ull, 2ull, 3ull, 4ull })
            {
                tr::Plan p;
                const auto y = tr::renderR17Fill(pad, settings(tr::kR17Recipe, 100, 2, 4, seed), &p);
                expect(p.family.startsWith("SAMPLE CHOP"), p.family);
                const int from = static_cast<int>(pad.getNumSamples() - 4 * kFs * 0.6);
                double diff = 0;
                for (int n = from; n < pad.getNumSamples(); ++n)
                    diff = std::max(diff, static_cast<double>(std::abs(y.getSample(0, n) - pad.getSample(0, n))));
                expect(diff > 0.05, "changed after the turn");
                expect(peakOf(y, from, pad.getNumSamples()) > 0.05, "not silent");
            }
            // a SAMPLE mark chooses the material, on a drum loop too
            auto s = settings(tr::kR17Recipe + 6, 95, 2, 4, 3);
            s.marks.push_back({ 0, static_cast<int>(kFs * 0.6), tr::kSampleMark });
            const auto p = tr::makeR17FillPlan(x, s);
            expect(p.family.contains("SAMPLE MARK"), p.family);
        }
        beginTest("F10 (R18): SAMPLE CHOP / WHOLE SAMPLE re-cuts every bar; BARS 8 and 16");
        {
            expect(tr::legalBars(8) == 8 && tr::legalBars(16) == 16 && tr::legalBars(3) == 2 && tr::legalBars(32) == 2);
            for (double bpm : { 90.0, 140.0 })
                for (int bars : { 1, 4, 16 })
                {
                    const auto src = loop(bpm, bars, 9);
                    const double sourcePeak = peakOf(src, 0, src.getNumSamples());
                    for (std::uint64_t seed : { 1ull, 2ull, 3ull, 4ull })
                    {
                        const auto s = settings(tr::kR17Chop, bpm, bars, tr::kWholeLoop, seed);
                        tr::Plan p;
                        const auto y = tr::renderR17Fill(src, s, &p);
                        expectEquals(y.getNumSamples(), tr::lengthSamples(s));
                        bool finite = true;
                        for (int c = 0; c < 2; ++c)
                            for (int n = 0; n < y.getNumSamples(); ++n)
                                finite = finite && std::isfinite(y.getSample(c, n));
                        expect(finite, "finite");
                        expect(peakOf(y, 0, y.getNumSamples()) <= std::max(std::min(sourcePeak, 0.98), std::pow(10.0, -1.8 / 20.0)) + 1e-3, "ceiling");
                        expect(p.status == ph::Generated, p.reason);
                        expect(bars == 1 || p.family.contains("WHOLE SAMPLE"), p.family);
                        expect(bitExact(y, tr::renderR17Fill(src, s)), "deterministic");
                        if (bars > 1) // the first bar is re-cut too (not only the end)
                        {
                            const int bar = static_cast<int>(4 * kFs * 60.0 / bpm);
                            double diff = 0;
                            for (int n = static_cast<int>(0.1 * kFs); n < bar; ++n)
                                diff = std::max(diff, static_cast<double>(std::abs(y.getSample(0, n) - src.getSample(0, n))));
                            expect(diff > 0.01, "the first bar changed");
                        }
                    }
                }
            // WHOLE SAMPLE is only for SAMPLE CHOP: the other types play the last 4 beats
            const auto src = loop(95, 4);
            const auto build = tr::makeR17FillPlan(src, settings(tr::kR17Recipe + 1, 95, 4, tr::kWholeLoop, 1));
            const double beat = kFs * 60.0 / 95;
            expect(build.phraseStart >= static_cast<int>(src.getNumSamples() - 4 * beat - 0.01 * kFs), "BUILD stays in the last 4 beats");
            // 1 / 2 / 4 / 8 beats of SAMPLE CHOP are the same as 0.5.0 (the whole-sample path is separate)
            const auto plain = settings(tr::kR17Chop, 95, 2, 4, 3);
            expect(!tr::makeR17FillPlan(loop(95, 2), plain).family.contains("WHOLE"));
        }
        beginTest("F8: recipes 0 .. 34 do not take the R17 path");
        {
            for (int recipe : { 0, 8, 21, 30, 34 })
                expect(!tr::isR17(tr::Fill, recipe));
            expect(tr::isR17(tr::Fill, tr::kR17Recipe) && !tr::isR17(tr::Rise, tr::kR17Recipe));
            expect(juce::String(tr::knobName(tr::Fill, 30, 0)) == "CHOP" && juce::String(tr::knobName(tr::Fill, tr::kR17Recipe, 0)) == "ENERGY");
            expect(tr::defaultKnobs(tr::Fill) == std::array<double, 4> { 0.6, 0.7, 0.4, 0.3 }, "the old defaults stay");
            auto zero = settings(tr::kR17Recipe, 95, 2, 4, 1);
            zero.knobs.fill(0);
            tr::Plan p;
            expect(bitExact(tr::renderR17Fill(x, zero, &p), x), "all knobs at zero: the source itself");
            expect(p.status == ph::Bypass);
        }
    }
};

static DrumFillR17Tests drumFillR17Tests;
} // namespace hamlo::test
