#include "DSP/DrumPhraseFill.h"
#include "Standalone/ModulePluginProcessor.h"
#include "Standalone/ModulePluginEditor.h"
#include "UI/HamloRevolutionTransitionPage.h"
#include "TestSignals.h"
#include <limits>
#include <set>

// R15 PHRASE FILL (docs/DRUM_FILL_R15.md): legality, anchors, window, determinism, bypass, kit switches, edge cases.
// These prove rules, not taste; listening material comes from Tests/DrumPhraseLab.cpp.
namespace hamlo::test
{
namespace tr = hpeq::dsp::transition;
namespace ph = hpeq::dsp::transition::phrase;
namespace
{
bool same(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    if (a.getNumSamples() != b.getNumSamples() || a.getNumChannels() != b.getNumChannels())
        return false;
    for (int c = 0; c < a.getNumChannels(); ++c)
        for (int n = 0; n < a.getNumSamples(); ++n)
            if (a.getSample(c, n) != b.getSample(c, n))
                return false;
    return true;
}
// kick on every beat and hats on the eighths, no snare at all (120 BPM, 48 kHz)
juce::AudioBuffer<float> kickAndHats(double seconds)
{
    const int n = static_cast<int>(seconds * kFs), beat = static_cast<int>(kFs / 2);
    juce::AudioBuffer<float> x(2, n);
    juce::Random rng(3);
    for (int i = 0; i < n; ++i)
    {
        const double tb = (i % beat) / kFs, te = (i % (beat / 2)) / kFs;
        double v = 0.6 * std::exp(-tb / 0.09) * std::sin(2.0 * juce::MathConstants<double>::pi * (50.0 + 90.0 * std::exp(-tb / 0.012)) * tb);
        v += 0.12 * std::exp(-te / 0.012) * (2.0 * rng.nextDouble() - 1.0);
        x.setSample(0, i, static_cast<float>(v));
        x.setSample(1, i, static_cast<float>(v));
    }
    return x;
}
std::set<int> starts(const ph::Analysis& a)
{
    std::set<int> s;
    for (const auto& o : a.onsets)
        s.insert(o.start);
    return s;
}
} // namespace

class DrumPhraseTests final : public juce::UnitTest
{
public:
    DrumPhraseTests() : juce::UnitTest("Drum phrase fill R15", "HamloRevolution") {}
    void runTest() override
    {
        auto source = drums(8);
        source.applyGain(0.35f);
        tr::Settings s;
        s.kind = tr::Fill; s.recipe = tr::kPhraseRecipe; s.bars = 1; s.bpm = 120; s.sampleRate = 48000; s.knobs = tr::defaultKnobs(tr::Fill);
        const auto cut = [&](const tr::Settings& t) { juce::AudioBuffer<float> y(2, tr::lengthSamples(t)); for (int c = 0; c < 2; ++c) y.copyFrom(c, 0, source, c, 0, y.getNumSamples()); return y; };

        beginTest("Analysis finds the synthetic kit, its grid and genre");
        {
            const auto a = ph::analyse(cut(s), 48000, 120);
            int kicks = 0;
            for (const auto& o : a.onsets)
                kicks += !o.secondary && o.role == ph::Kick;
            expect(kicks >= 3, "kicks " + juce::String(kicks));
            expect(a.usable[ph::Kick] > 0 && a.usable[ph::Hat] > 0);
            expect(std::abs(a.phase) < 0.003 * 48000, "phase " + juce::String(a.phase));
            expectEquals(a.genre, static_cast<int>(ph::Techno)); // kick on every beat at 120 BPM
            expect(a.drumness >= 0.35 && a.timing > 0.8, "drumness " + juce::String(a.drumness) + " timing " + juce::String(a.timing)); // 0.35 = the Generated threshold (first draft guessed 0.5)
        }
        beginTest("Every slice is a real analysed onset; events stay inside the window; prefix is exact");
        for (int recipe = tr::kPhraseRecipe; recipe <= tr::kPhraseRecipe + 4; ++recipe)
            for (int beats : { 1, 2, 4 })
                for (int seed = 1; seed <= 6; ++seed)
                {
                    auto t = s;
                    t.recipe = recipe; t.fillBeats = beats; t.seed = static_cast<std::uint64_t>(seed);
                    const auto x = cut(t);
                    const auto onsets = starts(ph::analyse(x, 48000, 120));
                    tr::Plan p;
                    const auto y = tr::renderPhraseFill(x, t, &p);
                    expectEquals(p.status, 2);
                    expect(!p.events.empty());
                    const int window = y.getNumSamples() - beats * 24000 - static_cast<int>(0.016 * 48000);
                    bool legal = true, inside = true;
                    for (const auto& e : p.events)
                    {
                        legal = legal && onsets.count(e.source) == 1 && e.role >= 0 && e.role < 3;
                        inside = inside && e.at >= p.phraseStart && e.at >= window && e.at + e.length <= y.getNumSamples();
                    }
                    expect(legal, "illegal slice, recipe " + juce::String(recipe));
                    expect(inside, "event outside the fill window");
                    bool prefix = true;
                    for (int c = 0; c < 2; ++c)
                        for (int n = 0; n < p.phraseStart - static_cast<int>(0.006 * 48000); ++n)
                            prefix = prefix && y.getSample(c, n) == x.getSample(c, n);
                    expect(prefix, "groove before the turn must stay bit-exact");
                    expect(same(y, tr::render(x, t)), "render() dispatch = renderPhraseFill");
                    expect(same(y, tr::renderPhraseFill(x, t)), "deterministic");
                }
        beginTest("Strong original kick / snare anchors stay where and what they are");
        for (int beats : { 2, 4 })
        {
            auto t = s;
            t.fillBeats = beats; t.knobs[2] = 0; t.knobs[3] = 0; // no reverse pickup / drop: nothing may replace an anchor
            const auto x = cut(t);
            const auto a = ph::analyse(x, 48000, 120);
            for (int seed = 1; seed <= 8; ++seed)
            {
                t.seed = static_cast<std::uint64_t>(seed);
                const auto p = tr::makePhraseFillPlan(x, t);
                int expected = 0, kept = 0;
                for (const auto& o : a.onsets)
                {
                    const double eighth = o.tick / 12.0;
                    if (o.secondary || (o.role != ph::Kick && o.role != ph::Snare) || o.conf < 0.55 || o.peak < 0.45 * a.typicalPeak[static_cast<size_t>(o.role)]
                        || std::abs(eighth - std::round(eighth)) * 12 > 2 || o.attack < p.phraseStart + 0.005 * 48000)
                        continue;
                    ++expected;
                    for (const auto& e : p.events)
                        if (e.source == o.start && e.role == o.role && e.at == o.start) { ++kept; break; }
                }
                expectEquals(kept, expected, "anchors kept, beats " + juce::String(beats) + " seed " + juce::String(seed));
                expect(p.anchors >= expected);
            }
        }
        beginTest("No invented drums: a loop without a snare never gets a snare");
        {
            auto x = kickAndHats(2.0);
            x.applyGain(0.5f);
            for (int seed = 1; seed <= 8; ++seed)
                for (int recipe = tr::kPhraseRecipe; recipe <= tr::kPhraseRecipe + 4; ++recipe)
                {
                    auto t = s;
                    t.seed = static_cast<std::uint64_t>(seed); t.recipe = recipe;
                    const auto p = tr::makePhraseFillPlan(x, t);
                    bool snare = false;
                    for (const auto& e : p.events)
                        snare = snare || e.role == ph::Snare;
                    expect(!snare, "snare invented, recipe " + juce::String(recipe));
                }
        }
        beginTest("Techno keeps every quarter kick in the figure unless STOP drops it");
        {
            auto t = s;
            t.recipe = tr::kPhraseRecipe + 4; t.fillBeats = 4; t.knobs[3] = 0; t.knobs[2] = 0;
            for (int seed = 1; seed <= 8; ++seed)
            {
                t.seed = static_cast<std::uint64_t>(seed);
                const auto p = tr::makePhraseFillPlan(cut(t), t);
                for (int b = 0; b < 4; ++b)
                {
                    const int at = b * 24000;
                    if (at < p.phraseStart)
                        continue;
                    bool kick = false;
                    for (const auto& e : p.events)
                        kick = kick || (e.role == ph::Kick && std::abs(e.at - at) < 0.03 * 48000);
                    expect(kick, "quarter kick missing at beat " + juce::String(b) + " seed " + juce::String(seed));
                }
            }
        }
        beginTest("NEW VARIATION gives different phrases; genres differ");
        {
            std::set<juce::String> plans, genres;
            auto t = s;
            t.fillBeats = 4;
            for (int seed = 1; seed <= 16; ++seed)
            {
                t.seed = static_cast<std::uint64_t>(seed);
                juce::String signature;
                for (const auto& e : tr::makePhraseFillPlan(cut(t), t).events)
                    signature << e.at << ":" << e.role << ";";
                plans.insert(signature);
            }
            logMessage("distinct 4-beat plans over 16 seeds: " + juce::String(static_cast<int>(plans.size())));
            expect(plans.size() >= 6);
            for (int recipe = tr::kPhraseRecipe + 1; recipe <= tr::kPhraseRecipe + 4; ++recipe)
            {
                t.recipe = recipe; t.seed = 3;
                const auto p = tr::makePhraseFillPlan(cut(t), t);
                expectEquals(p.genre, recipe - tr::kPhraseRecipe - 1);
                juce::String signature;
                for (const auto& e : p.events)
                    signature << e.at << ":" << e.role << ";";
                genres.insert(signature);
            }
            expect(genres.size() >= 3, "genre profiles must give different phrases");
        }
        beginTest("Bypass and neutral cases keep the source exactly");
        {
            juce::AudioBuffer<float> silent(2, 96000);
            silent.clear();
            tr::Plan p;
            auto y = tr::renderPhraseFill(silent, s, &p);
            expectEquals(p.status, 0);
            expectEquals(y.getMagnitude(0, y.getNumSamples()), 0.0f);
            const auto tone = sine(2.0, 220.0, -12.0); // no drums: nothing to slice
            y = tr::renderPhraseFill(tone, s, &p);
            expect(p.status == 0 || p.events.empty() || p.kitCounts[0] + p.kitCounts[1] == 0 || p.status == 1, "a sustained tone is not a drum kit");
            auto neutral = s;
            neutral.knobs.fill(0);
            const auto x = cut(s);
            expect(same(tr::renderPhraseFill(x, neutral, &p), x));
            expectEquals(p.status, 0);
            auto muted = s;
            muted.kitEnabled = { false, false, false };
            const auto m = tr::renderPhraseFill(x, muted, &p);
            expect(p.events.empty());
            expect(same(m, x) || p.status == 0, "nothing enabled: source kept");
        }
        beginTest("Kit switches mute parts of the same composition");
        {
            const auto x = cut(s);
            const auto full = tr::makePhraseFillPlan(x, s);
            for (int role = 0; role < 3; ++role)
            {
                auto solo = s;
                solo.kitEnabled = { false, false, false };
                solo.kitEnabled[static_cast<size_t>(role)] = true;
                const auto p = tr::makePhraseFillPlan(x, solo);
                bool only = true;
                for (const auto& e : p.events)
                    only = only && e.role == role;
                expect(only);
            }
            expect(!full.events.empty());
        }
        beginTest("Finite, exact length and bounded peaks across rates, tempi, bars and lengths");
        for (double fs : { 44100.0, 48000.0, 96000.0 })
            for (double bpm : { 70.0, 120.0, 175.0 })
                for (int bars : { 1, 2, 4 })
                    for (int beats : { 1, 4 })
                    {
                        auto t = s;
                        t.sampleRate = fs; t.bpm = bpm; t.bars = bars; t.fillBeats = beats;
                        const auto y = tr::renderPhraseFill(source, t);
                        expectEquals(y.getNumSamples(), tr::lengthSamples(t));
                        bool finite = true;
                        for (int c = 0; c < 2; ++c)
                            for (int n = 0; n < y.getNumSamples(); ++n)
                                finite = finite && std::isfinite(y.getSample(c, n));
                        expect(finite);
                        expect(y.getMagnitude(0, y.getNumSamples()) <= std::max(0.82f, source.getMagnitude(0, source.getNumSamples()) * 1.02f));
                    }
        {
            auto hot = cut(s);
            hot.applyGain(6.0f);
            const auto y = tr::renderPhraseFill(hot, s);
            expect(y.getMagnitude(0, y.getNumSamples()) <= hot.getMagnitude(0, hot.getNumSamples()) * 1.02f, "never louder than a hot source");
            auto t = s;
            t.bpm = std::numeric_limits<double>::quiet_NaN();
            expectEquals(tr::renderPhraseFill(source, t).getNumSamples(), 0);
            juce::AudioBuffer<float> nan(2, 48000);
            nan.clear();
            nan.setSample(0, 100, std::numeric_limits<float>::quiet_NaN());
            const auto z = tr::renderPhraseFill(nan, s);
            bool finite = true;
            for (int c = 0; c < 2; ++c)
                for (int n = 0; n < z.getNumSamples(); ++n)
                    finite = finite && std::isfinite(z.getSample(c, n));
            expect(finite);
        }
        beginTest("Recipe names, ranges and legacy recipes are unchanged");
        {
            expectEquals(tr::maximumRecipe(tr::Fill), 34);
            expectEquals(tr::maximumRecipe(tr::Rise), 7);
            expectEquals(juce::String(tr::recipeName(tr::Fill, 30)), juce::String("PHRASE AUTO"));
            expectEquals(juce::String(tr::recipeName(tr::Fill, 34)), juce::String("PHRASE TECHNO"));
            expectEquals(juce::String(tr::recipeName(tr::Fill, 21)), juce::String("AUTO COMPOSE"));
            auto old = s;
            old.recipe = 21; old.seed = 5;
            expect(same(tr::render(cut(old), old), tr::renderComposedFill(cut(old), old)), "R14 path untouched by dispatch");
        }
        beginTest("User marks win: a marked role uses only the marked hits");
        {
            const auto x = cut(s);
            const std::vector<tr::Mark> marks { { 11900, 16000, ph::Snare } }; // an off-beat hat, called a snare
            const auto a = ph::analyse(x, 48000, 120, marks);
            int markedStart = -1, markedCount = 0;
            for (const auto& o : a.onsets)
                if (o.marked)
                {
                    ++markedCount;
                    markedStart = o.start;
                    expectEquals(o.role, static_cast<int>(ph::Snare));
                    expect(o.start >= 11900 - 300 && o.start < 16000 && o.start + o.free == 16000);
                }
            expectEquals(markedCount, 1);
            auto t = s;
            t.marks = marks;
            t.recipe = tr::kPhraseRecipe + 1; // BOOM BAP: snare-led cells
            bool snare = false, onlyMarked = true;
            for (int seed = 1; seed <= 8; ++seed)
            {
                t.seed = static_cast<std::uint64_t>(seed);
                const auto p = tr::makePhraseFillPlan(x, t);
                expectEquals(p.kitCounts[ph::Snare], 1);
                for (const auto& e : p.events)
                    if (e.role == ph::Snare)
                    {
                        snare = true;
                        onlyMarked = onlyMarked && (e.source == markedStart || e.at == e.source); // anchors are original audio
                    }
            }
            expect(snare && onlyMarked, "snare events must come from the marked hit");
            const auto y = tr::renderPhraseFill(x, t);
            expect(same(y, tr::renderPhraseFill(x, t)), "deterministic with marks");
        }
        beginTest("PERC exists only from a mark; it plays the snare lane when there is no snare");
        {
            auto x = kickAndHats(2.0);
            x.applyGain(0.5f);
            auto t = s;
            t.recipe = tr::kPhraseRecipe + 1;
            expectEquals(tr::makePhraseFillPlan(x, t).kitCounts[ph::Perc], 0);
            t.marks = { { 11900, 16000, ph::Perc } };
            int perc = 0, snare = 0;
            for (int seed = 1; seed <= 8; ++seed)
            {
                t.seed = static_cast<std::uint64_t>(seed);
                const auto p = tr::makePhraseFillPlan(x, t);
                expectEquals(p.kitCounts[ph::Perc], 1);
                for (const auto& e : p.events)
                {
                    perc += e.role == ph::Perc;
                    snare += e.role == ph::Snare;
                }
            }
            expect(perc > 0, "marked perc is used");
            expectEquals(snare, 0);
            auto off = t;
            off.kitEnabled[ph::Perc] = false;
            for (const auto& e : tr::makePhraseFillPlan(x, off).events)
                expect(e.role != ph::Perc, "PERC switch mutes it");
        }
        beginTest("TOM exists only from marks; tom fills run down from the highest marked tom to the lowest");
        {
            auto x = cut(s);
            const auto addTom = [&](int at, double hz)
            {
                for (int n = 0; n < 4800 && at + n < x.getNumSamples(); ++n)
                {
                    const double t = n / 48000.0;
                    const auto v = static_cast<float>(0.7 * std::exp(-t / 0.12) * std::sin(2.0 * juce::MathConstants<double>::pi * hz * (1 + 0.3 * std::exp(-t / 0.02)) * t));
                    for (int c = 0; c < 2; ++c)
                        x.addSample(c, at + n, v);
                }
            };
            addTom(3000, 95.0);   // the low (floor) tom is marked first on purpose
            addTom(27000, 240.0); // the high tom
            auto t = s;
            t.recipe = tr::kPhraseRecipe + 1; // BOOM BAP
            t.knobs[2] = 0; t.knobs[3] = 0;   // no reverse pickup / drop
            expectEquals(tr::makePhraseFillPlan(x, t).kitCounts[ph::Tom], 0, "never invented");
            t.marks = { { 2950, 5900, ph::Tom }, { 26950, 29900, ph::Tom } };
            const auto a = ph::analyse(x, 48000, 120, t.marks);
            int low = -1, high = -1;
            for (const auto& o : a.onsets)
                if (o.marked && o.role == ph::Tom)
                    (o.start < 10000 ? low : high) = o.start;
            expect(low >= 0 && high >= 0, "both marked toms analysed");
            int tomPlans = 0, tomEvents = 0;
            bool onlyMarked = true, descending = true;
            for (int seed = 1; seed <= 12; ++seed)
            {
                t.seed = static_cast<std::uint64_t>(seed);
                const auto p = tr::makePhraseFillPlan(x, t);
                expectEquals(p.kitCounts[ph::Tom], 2);
                expectEquals(p.status, 2);
                int lastHigh = -1, firstLow = std::numeric_limits<int>::max(), count = 0;
                for (const auto& e : p.events)
                    if (e.role == ph::Tom)
                    {
                        ++count;
                        onlyMarked = onlyMarked && (e.source == low || e.source == high);
                        if (e.source == high) lastHigh = std::max(lastHigh, e.at);
                        if (e.source == low) firstLow = std::min(firstLow, e.at);
                    }
                descending = descending && (lastHigh < 0 || firstLow == std::numeric_limits<int>::max() || lastHigh <= firstLow);
                tomEvents += count;
                tomPlans += count > 0;
                if (count >= 2)
                    expect(p.family.contains("TOMS"), p.family);
            }
            expect(tomPlans >= 6, "toms used in " + juce::String(tomPlans) + " / 12 plans");
            expect(onlyMarked, "tom strokes come only from the marked toms");
            expect(descending, "high tom before low tom");
            logMessage("tom strokes over 12 seeds: " + juce::String(tomEvents));
            auto off = t;
            off.kitEnabled[ph::Tom] = false;
            for (const auto& e : tr::makePhraseFillPlan(x, off).events)
                expect(e.role != ph::Tom, "TOM switch mutes it");
            const auto y = tr::renderPhraseFill(x, t);
            expect(same(y, tr::renderPhraseFill(x, t)), "deterministic with toms");
            expect(y.getMagnitude(0, y.getNumSamples()) <= 1.0f);
        }
        beginTest("WHOLE LOOP re-makes every bar and keeps the strong hits as its skeleton");
        {
            auto t = s;
            t.bars = 2; t.fillBeats = tr::kWholeLoop; t.knobs[2] = 0; t.knobs[3] = 0;
            juce::AudioBuffer<float> x(2, tr::lengthSamples(t));
            for (int c = 0; c < 2; ++c)
                x.copyFrom(c, 0, source, c, 0, x.getNumSamples());
            for (int seed = 1; seed <= 6; ++seed)
            {
                t.seed = static_cast<std::uint64_t>(seed);
                tr::Plan p;
                const auto y = tr::renderPhraseFill(x, t, &p);
                expectEquals(p.status, 2);
                expect(p.family.startsWith("WHOLE LOOP"), p.family);
                expect(p.phraseStart < 0.02 * 48000, "the edit starts at the loop start");
                int firstBar = 0, added = 0;
                for (const auto& e : p.events)
                {
                    firstBar += e.at < x.getNumSamples() / 2;
                    added += e.at != e.source;
                }
                expect(firstBar > 0 && added > 4, "every bar changes");
                expectEquals(y.getNumSamples(), x.getNumSamples());
                bool finite = true;
                for (int c = 0; c < 2; ++c)
                    for (int n = 0; n < y.getNumSamples(); ++n)
                        finite = finite && std::isfinite(y.getSample(c, n));
                expect(finite);
                expect(same(y, tr::renderPhraseFill(x, t)));
            }
            auto old = t;
            old.recipe = 21; // the R14 composer has no whole-loop mode: it falls back to its own 4-beat edit
            expect(tr::render(x, old).getNumSamples() == x.getNumSamples());
        }
        beginTest("Owner scenario: sample -> FILL -> mark / fix marks -> variation -> whole loop -> reopen -> new capture");
        {
            using P = hpeq::standalone::ModulePluginProcessor;
            auto p = std::make_unique<P>(hpeq::standalone::ModuleKind::Revolution);
            p->prepareToPlay(48000, 256);
            const auto folder = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("HamloR15Scenario");
            folder.deleteRecursively();
            folder.createDirectory();
            p->setRendersFolder(folder);
            // 1. a 2-bar loop dropped on the page as a WAV
            const auto wavFile = folder.getChildFile("loop.wav");
            {
                auto loop = drums(4.0);
                loop.applyGain(0.35f);
                std::unique_ptr<juce::OutputStream> stream = wavFile.createOutputStream();
                juce::WavAudioFormat format;
                auto writer = format.createWriterFor(stream, juce::AudioFormatWriterOptions {}.withSampleRate(48000).withNumChannels(2).withBitsPerSample(24));
                expect(writer != nullptr && writer->writeFromAudioSampleBuffer(loop, 0, loop.getNumSamples()));
            }
            expect(p->useSampleForTransition(wavFile));
            std::unique_ptr<juce::AudioProcessorEditor> owner(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owner.get());
            auto* page = editor->getTransitionPage();
            const auto checkRender = [&](const juce::String& step)
            {
                const auto t = p->lastTransition();
                expect(t.valid && t.file.existsAsFile(), step + ": WAV written");
                const auto clip = p->transitionClip();
                expectEquals(clip.getNumSamples(), tr::lengthSamples(t.settings), step + ": length");
                bool finite = true;
                for (int c = 0; c < clip.getNumChannels(); ++c)
                    for (int n = 0; n < clip.getNumSamples(); ++n)
                        finite = finite && std::isfinite(clip.getSample(c, n));
                expect(finite && clip.getMagnitude(0, clip.getNumSamples()) > 0.01f && clip.getMagnitude(0, clip.getNumSamples()) <= 1.0f, step + ": audible, bounded");
                expect(t.fillInfo.isNotEmpty(), step + ": diagnostics");
                logMessage(step + ": " + t.fillInfo);
            };
            // 2. DRUM FILL opens on PHRASE AUTO, no marks needed
            page->chooseKind(tr::Fill);
            expectEquals(page->getValues().recipe, tr::kPhraseRecipe);
            checkRender("auto, unmarked");
            const auto hits = page->getValues().hits;
            expect(hits.size() >= 6, "hit regions to snap marks to");
            const auto autoCounts = p->lastTransition().kitCounts;
            // 3. mark a kick, then mark a hat wrongly as SNARE
            const auto kick = hits[0], hat = hits[1];
            page->markRegion(kick.getStart(), kick.getEnd(), ph::Kick);
            page->markRegion(hat.getStart(), hat.getEnd(), ph::Snare);
            expectEquals(p->lastTransition().kitCounts[ph::Kick], 1, "kick: only the marked hit");
            expectEquals(p->lastTransition().kitCounts[ph::Snare], 1, "snare: only the (wrong) marked hit");
            checkRender("kick + wrong snare marked");
            // 4. fix it: the same region again relabels it (no duplicate)
            page->markRegion(hat.getStart(), hat.getEnd(), ph::Hat);
            expectEquals(static_cast<int>(page->getValues().marks.size()), 2);
            expectEquals(p->lastTransition().kitCounts[ph::Snare], autoCounts[ph::Snare], "snare back to automatic");
            expectEquals(p->lastTransition().kitCounts[ph::Hat], 1);
            // 5. undo the fix, then remove that mark, then undo twice more and clear
            page->undoMark();
            expectEquals(p->lastTransition().kitCounts[ph::Snare], 1, "undo restores the previous label");
            expect(page->markAt(hat.getStart() + 1) >= 0);
            page->markRegion(hat.getStart(), hat.getEnd(), -1);
            expect(page->markAt(hat.getStart() + 1) < 0, "REMOVE MARK");
            expectEquals(static_cast<int>(p->lastTransition().settings.marks.size()), 1);
            page->clearMarks();
            expect(p->lastTransition().settings.marks.empty(), "CLEAR ALL MARKS");
            expect(p->lastTransition().kitCounts == autoCounts, "no marks = the automatic result again");
            page->undoMark();
            expectEquals(static_cast<int>(p->lastTransition().settings.marks.size()), 1, "undo after clear");
            // 6. a perc mark, NEW VARIATION keeps marks and never rewrites a dragged WAV
            page->markRegion(hits[3].getStart(), hits[3].getEnd(), ph::Perc);
            checkRender("kick + perc marked");
            page->markRegion(hits[4].getStart(), hits[4].getEnd(), ph::Tom);
            expectEquals(p->lastTransition().kitCounts[ph::Tom], 1, "a TOM mark from the page");
            checkRender("kick + perc + tom marked");
            const auto before = p->lastTransition();
            juce::MemoryBlock dragged;
            before.file.loadFileAsData(dragged);
            page->pressVariation();
            expectEquals(p->lastTransition().settings.seed, before.settings.seed + 1);
            expect(p->lastTransition().settings.marks == before.settings.marks, "marks survive a variation");
            juce::MemoryBlock after;
            before.file.loadFileAsData(after);
            expect(dragged == after && p->lastTransition().file != before.file, "the earlier WAV is untouched");
            checkRender("variation");
            // 7. WHOLE LOOP: every bar re-made with the same marks
            page->chooseFillBeats(tr::kWholeLoop);
            expectEquals(p->lastTransition().settings.fillBeats, tr::kWholeLoop);
            expect(p->lastTransition().fillInfo.contains("WHOLE LOOP"), p->lastTransition().fillInfo);
            checkRender("whole loop");
            // 8. PERC switch, editor reopen keeps marks and options
            page->toggleKit(ph::Perc);
            expect(!p->lastTransition().settings.kitEnabled[ph::Perc]);
            owner.reset();
            owner.reset(p->createEditor());
            page = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owner.get())->getTransitionPage();
            expect(page->getValues().marks == p->lastTransition().settings.marks && !page->getValues().marks.empty(), "marks after reopening");
            expectEquals(page->getValues().fillBeats, tr::kWholeLoop);
            // 9. GENERATE takes a new capture: no marks, no undo into the old source
            page->pressGenerate();
            expect(p->lastTransition().settings.marks.empty() && page->getValues().marks.empty());
            page->undoMark();
            expect(p->lastTransition().settings.marks.empty(), "undo cannot bring old-source marks back");
            checkRender("new capture");
            folder.deleteRecursively();
        }
        beginTest("PHRASE AUTO is the FILL default and reports its diagnostics");
        {
            using P = hpeq::standalone::ModulePluginProcessor;
            auto p = std::make_unique<P>(hpeq::standalone::ModuleKind::Revolution);
            p->prepareToPlay(48000, 256);
            const auto folder = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("HamloR15Tests");
            folder.createDirectory();
            p->setRendersFolder(folder);
            juce::MidiBuffer midi;
            for (int n = 0; n < 192000; n += 256)
            {
                auto block = segment(source, n, 256);
                p->processBlock(block, midi);
            }
            std::unique_ptr<juce::AudioProcessorEditor> owner(p->createEditor());
            auto* editor = dynamic_cast<hpeq::standalone::ModulePluginEditor*>(owner.get());
            auto* page = editor->getTransitionPage();
            page->chooseKind(tr::Fill);
            expectEquals(page->getValues().recipe, tr::kPhraseRecipe);
            const auto t = p->lastTransition();
            expect(t.valid);
            expect(t.fillInfo.isNotEmpty(), "diagnostics line");
            logMessage("fill info: " + t.fillInfo);
            const auto seed = t.settings.seed;
            page->pressVariation();
            expectEquals(p->lastTransition().settings.seed, seed + 1);
            // marks reach the renderer, survive NEW VARIATION and are cleared by a new capture
            page->setSource(p->transitionSourceCopy());
            page->markRegion(11900, 16000, ph::Perc);
            expectEquals(static_cast<int>(p->lastTransition().settings.marks.size()), 1);
            expectEquals(p->lastTransition().kitCounts[ph::Perc], 1);
            expect(!p->lastTransition().hits.empty(), "hit regions for snapping");
            page->pressVariation();
            expectEquals(static_cast<int>(p->lastTransition().settings.marks.size()), 1);
            page->markRegion(11000, 17000, -1);
            expect(p->lastTransition().settings.marks.empty(), "REMOVE MARK");
            page->markRegion(11900, 16000, ph::Snare);
            page->pressGenerate();
            expect(p->lastTransition().settings.marks.empty() && page->getValues().marks.empty(), "a new capture starts unmarked");
            page->chooseFillBeats(tr::kWholeLoop);
            expectEquals(p->lastTransition().settings.fillBeats, tr::kWholeLoop);
            page->chooseRecipe(21);
            expectEquals(p->lastTransition().settings.fillBeats, 4); // R14 has no whole-loop mode
            folder.deleteRecursively();
        }
    }
};
static DrumPhraseTests drumPhraseTests;
} // namespace hamlo::test
