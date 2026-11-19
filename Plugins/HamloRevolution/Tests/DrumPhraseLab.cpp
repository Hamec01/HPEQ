// R15 PHRASE FILL laboratory: builds genre loops from real one-shot samples (ground truth known), measures the
// analyser (onset recall / precision, role accuracy, swing, genre) and renders listening files:
// <genre>_<kit>_source.wav, ..._phrase_<beats>b_s<seed>.wav (R15), ..._r14_<beats>b_s<seed>.wav (R14 AUTO COMPOSE).
// Each listening file: the 2-bar loop, then the 2-bar fill render, then the first beat again (the landing).
// Usage: HamloRevolution_DrumPhraseLab <kit folder with Kicks/ Snares/ Claps/ Hats/> <new output folder>
#include "DSP/DrumPhraseFill.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <map>
#include "TestSignals.h"

namespace tr = hpeq::dsp::transition;
namespace ph = hpeq::dsp::transition::phrase;
namespace
{
constexpr double fs = 48000;
juce::AudioBuffer<float> load(juce::AudioFormatManager& formats, const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader)
        return {};
    juce::AudioBuffer<float> raw(2, static_cast<int>(std::min<juce::int64>(reader->lengthInSamples, static_cast<juce::int64>(reader->sampleRate * 1.5))));
    reader->read(&raw, 0, raw.getNumSamples(), 0, true, true);
    if (reader->numChannels == 1)
        raw.copyFrom(1, 0, raw, 0, 0, raw.getNumSamples());
    const double ratio = reader->sampleRate / fs;
    juce::AudioBuffer<float> y(2, static_cast<int>(raw.getNumSamples() / ratio));
    for (int c = 0; c < 2; ++c)
        for (int n = 0; n < y.getNumSamples(); ++n)
        {
            const double p = n * ratio;
            const int i = static_cast<int>(p);
            const float f = static_cast<float>(p - i);
            y.setSample(c, n, raw.getSample(c, i) * (1 - f) + (i + 1 < raw.getNumSamples() ? raw.getSample(c, i + 1) : 0.0f) * f);
        }
    const float peak = y.getMagnitude(0, y.getNumSamples());
    if (peak > 0)
        y.applyGain(0.7f / peak);
    return y;
}
std::vector<juce::File> wavs(const juce::File& folder, bool skipLoops)
{
    std::vector<juce::File> files;
    for (const auto& f : folder.findChildFiles(juce::File::findFiles, false, "*.wav"))
        if (!skipLoops || !f.getFileName().containsIgnoreCase("loop"))
            files.push_back(f);
    std::sort(files.begin(), files.end(), [](const juce::File& a, const juce::File& b) { return a.getFileName() < b.getFileName(); });
    return files;
}
void wav(const juce::File& file, const juce::AudioBuffer<float>& x)
{
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    juce::WavAudioFormat f;
    auto w = f.createWriterFor(stream, juce::AudioFormatWriterOptions {}.withSampleRate(fs).withNumChannels(2).withBitsPerSample(24));
    if (!w || !w->writeFromAudioSampleBuffer(x, 0, x.getNumSamples()))
        throw std::runtime_error("WAV write");
}
struct Truth { int sample, role; };
// rendered-audio checks, identical for R14 and R15: band-energy cosine (tone faithfulness of the fill window vs the
// same window of the source), RMS level difference, onset grid deviation (16th / 32nd / triplet, after swing)
std::array<double, 4> bands(const juce::AudioBuffer<float>& x, int from, int to)
{
    std::array<double, 4> e {};
    const double edges[5] { 20, 120, 600, 4000, 20000 };
    for (int b = 0; b < 4; ++b)
    {
        juce::IIRFilter hp, lp;
        hp.setCoefficients(juce::IIRCoefficients::makeHighPass(fs, edges[b]));
        lp.setCoefficients(juce::IIRCoefficients::makeLowPass(fs, std::min(edges[b + 1], 0.45 * fs)));
        for (int n = 0; n < to; ++n)
        {
            const float v = lp.processSingleSampleRaw(hp.processSingleSampleRaw(0.5f * (x.getSample(0, n) + x.getSample(1, n))));
            if (n >= from)
                e[static_cast<size_t>(b)] += static_cast<double>(v) * v;
        }
    }
    return e;
}
struct Check { double cosine = 0, levelDb = 0, gridTicks = 0; };
Check check(const juce::AudioBuffer<float>& source, const juce::AudioBuffer<float>& y, double bpm, int beats)
{
    Check c;
    const int to = y.getNumSamples(), from = to - static_cast<int>(std::lround(beats * fs * 60 / bpm));
    const auto a = bands(source, from, to), b = bands(y, from, to);
    double dot = 0, aa = 0, bb = 0, ea = 0, eb = 0;
    for (int i = 0; i < 4; ++i)
    {
        const double la = std::sqrt(a[static_cast<size_t>(i)]), lb = std::sqrt(b[static_cast<size_t>(i)]);
        dot += la * lb; aa += la * la; bb += lb * lb; ea += a[static_cast<size_t>(i)]; eb += b[static_cast<size_t>(i)];
    }
    c.cosine = dot / std::sqrt(std::max(1e-30, aa * bb));
    c.levelDb = 10 * std::log10((eb + 1e-30) / (ea + 1e-30));
    const auto an = ph::analyse(y, fs, bpm);
    std::vector<double> dev;
    for (const auto& o : an.onsets)
        if (!o.secondary && o.attack >= from + static_cast<int>(0.01 * fs))
        {
            double best = 99;
            for (double grid : { 3.0, 4.0 }) // 32nds and 16th triplets (8ths / 16ths are multiples of 3)
            {
                const double t = o.tick - (std::fmod(std::round(o.tick / 6), 2) != 0 && grid == 3.0 ? an.swing : 0.0);
                best = std::min(best, std::abs(t - grid * std::round(t / grid)));
            }
            dev.push_back(best);
        }
    std::sort(dev.begin(), dev.end());
    c.gridTicks = dev.empty() ? 0 : dev[dev.size() / 2];
    return c;
}
std::map<int, std::vector<Check>> checks;
struct Pattern { const char* name; double bpm, swing; const char* k[2]; const char* s[2]; const char* h[2]; };
// one char per 16th: '.' none, 'x' hit, 'g' ghost, 'r' hat 32nd pair
const Pattern patterns[] {
    { "boombap", 90, 0.58, { "x.....x...x.....", "x.....x..x...x.." }, { "....x.......x...", "....x..g....x..g" }, { "x.x.x.x.x.x.x.x.", "x.x.x.x.x.x.x.x." } },
    { "trap", 140, 0.50, { "x.......x.....x.", "x.....x...x....." }, { "........x.......", "........x......." }, { "x.x.x.x.xrx.x.x.", "x.xxx.x.x.x.rrrr" } },
    { "dnb", 172, 0.50, { "x.........x.....", "x.x.......x....." }, { "....x.......x...", "....x.......x..g" }, { "x.x.x.x.x.x.x.x.", "x.x.x.x.x.x.x.x." } },
    { "techno", 128, 0.50, { "x...x...x...x...", "x...x...x...x..." }, { "....x.......x...", "....x.......x..." }, { "..x...x...x...x.", "..x...x...x...x." } } };
const int genreOf[] { ph::BoomBap, ph::Trap, ph::DnB, ph::Techno };

juce::AudioBuffer<float> makeLoop(const Pattern& p, const juce::AudioBuffer<float>* kit[3], int length, std::vector<Truth>& truth, int seed)
{
    juce::AudioBuffer<float> y(2, length);
    y.clear();
    juce::Random rng(seed);
    const double beat = fs * 60 / p.bpm, sixteenth = beat / 4;
    const auto add = [&](int role, double at, double gain)
    {
        const auto& x = *kit[role];
        const int start = static_cast<int>(std::lround(at));
        for (int c = 0; c < 2; ++c)
            for (int n = 0; n < x.getNumSamples() && start + n < length; ++n)
                if (start + n >= 0)
                    y.addSample(c, start + n, static_cast<float>(x.getSample(c, n) * gain));
        int lead = 0; // the one-shot's own leading silence: truth is where its sound begins
        const float top = x.getMagnitude(0, x.getNumSamples());
        while (lead < x.getNumSamples() - 1 && std::abs(x.getSample(0, lead)) < 0.05f * top && std::abs(x.getSample(1, lead)) < 0.05f * top)
            ++lead;
        truth.push_back({ start + lead, role });
    };
    for (int bar = 0; bar < 2; ++bar)
        for (int step = 0; step < 16; ++step)
        {
            const double swingShift = (step % 2) ? (p.swing - 0.5) * 2 * sixteenth : 0.0;
            const double at = (bar * 16 + step) * sixteenth + swingShift + (rng.nextDouble() - 0.5) * 0.004 * fs;
            const char* lanes[3] { p.k[bar], p.s[bar], p.h[bar] };
            const double levels[3] { 1.0, 0.85, 0.32 };
            for (int role = 0; role < 3; ++role)
            {
                const char ch = lanes[role][step];
                if (ch == 'x') add(role, at, levels[role] * (0.85 + 0.15 * rng.nextDouble()));
                if (ch == 'g') add(role, at, levels[role] * 0.35);
                if (ch == 'r') { add(role, at, levels[role] * 0.8); add(role, at + sixteenth / 2, levels[role] * 0.6); }
            }
        }
    return y;
}
} // namespace

int main(int argc, char** argv)
{
    if (argc == 2) // debug: analyse a synthetic test signal ("drums" or "kickhats")
    {
        auto x = hamlo::test::drums(8.0);
        if (juce::String(argv[1]) == "kickhats")
        {
            const int n = 96000, beat = 24000;
            juce::Random rng(3);
            for (int i = 0; i < n; ++i)
            {
                const double tb = (i % beat) / 48000.0, te = (i % (beat / 2)) / 48000.0;
                double v = 0.6 * std::exp(-tb / 0.09) * std::sin(2.0 * juce::MathConstants<double>::pi * (50.0 + 90.0 * std::exp(-tb / 0.012)) * tb);
                v += 0.12 * std::exp(-te / 0.012) * (2.0 * rng.nextDouble() - 1.0);
                x.setSample(0, i, static_cast<float>(v)); x.setSample(1, i, static_cast<float>(v));
            }
        }
        x.applyGain(0.35f);
        const auto a = ph::analyse(x, fs, 120);
        std::cout << "phase " << a.phase << " swing " << a.swing << " genre " << a.genre << " drumness " << a.drumness << " timing " << a.timing << std::endl;
        for (const auto& o : a.onsets)
        {
            const double t = o.novelty[0] + o.novelty[1] + o.novelty[2] + o.novelty[3];
            std::cout << o.attack << " tick " << o.tick << " role " << o.role << (o.secondary ? " SEC" : "") << " conf " << o.conf << " iso " << o.isolation
                      << " fr " << o.novelty[0] / t << " " << o.novelty[1] / t << " " << o.novelty[2] / t << " " << o.novelty[3] / t << " decay " << o.decayMs << std::endl;
        }
        return 0;
    }
    if (argc != 3)
        return 2;
    const juce::File kitFolder(juce::String::fromUTF8(argv[1])), out(juce::String::fromUTF8(argv[2]));
    if (out.exists() || out.createDirectory().failed())
        return 3;
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    const auto kicks = wavs(kitFolder.getChildFile("Kicks"), true), snares = wavs(kitFolder.getChildFile("Snares"), true),
               claps = wavs(kitFolder.getChildFile("Claps"), true), hats = wavs(kitFolder.getChildFile("Hats"), true);
    if (kicks.empty() || snares.empty() || hats.empty())
        return 4;
    int truthTotal = 0, found = 0, detected = 0, roleRight = 0;
    std::map<int, std::array<int, 3>> confusion;
    int genreRight = 0, loops = 0;
    std::vector<double> attackErrors;
    for (int kitIndex = 0; kitIndex < 3; ++kitIndex)
    {
        const auto kick = load(formats, kicks[static_cast<size_t>(kitIndex * 3 % kicks.size())]);
        const auto snare = load(formats, kitIndex == 1 && !claps.empty() ? claps[0] : snares[static_cast<size_t>(kitIndex * 5 % snares.size())]);
        const auto hat = load(formats, hats[static_cast<size_t>(kitIndex * 2 % hats.size())]);
        const juce::AudioBuffer<float>* kit[3] { &kick, &snare, &hat };
        for (int g = 0; g < 4; ++g)
        {
            const auto& p = patterns[g];
            tr::Settings s;
            s.kind = tr::Fill; s.bars = 2; s.bpm = p.bpm; s.sampleRate = fs; s.knobs = tr::defaultKnobs(tr::Fill);
            const int length = tr::lengthSamples(s);
            std::vector<Truth> truth;
            auto loop = makeLoop(p, kit, length, truth, 11 + kitIndex);
            loop.applyGain(0.5f);
            const auto a = ph::analyse(loop, fs, p.bpm);
            ++loops;
            genreRight += a.genre == genreOf[g];
            // match: a truth hit is found if an onset attack lies within 15 ms; when hits coincide the expected role is
            // the strongest (kick > snare > hat)
            std::vector<bool> used(a.onsets.size());
            std::map<int, int> expected;
            for (const auto& t : truth)
            {
                auto it = expected.find(t.sample);
                if (it == expected.end() || t.role < it->second)
                    expected[t.sample] = t.role;
            }
            std::vector<std::pair<int, int>> merged;
            for (const auto& [at, role] : expected)
                if (merged.empty() || at - merged.back().first > static_cast<int>(0.012 * fs))
                    merged.push_back({ at, role });
                else
                    merged.back().second = std::min(merged.back().second, role);
            for (const auto& [at, role] : merged)
            {
                ++truthTotal;
                int best = -1, distance = static_cast<int>(0.015 * fs);
                for (size_t i = 0; i < a.onsets.size(); ++i)
                    if (!used[i] && !a.onsets[i].secondary && std::abs(a.onsets[i].attack - at) <= distance)
                    {
                        distance = std::abs(a.onsets[i].attack - at);
                        best = static_cast<int>(i);
                    }
                if (best >= 0)
                {
                    used[static_cast<size_t>(best)] = true;
                    attackErrors.push_back((a.onsets[static_cast<size_t>(best)].attack - at) / fs * 1000);
                    ++found;
                    confusion[role][static_cast<size_t>(a.onsets[static_cast<size_t>(best)].role)]++;
                    roleRight += a.onsets[static_cast<size_t>(best)].role == role;
                    if (juce::SystemStats::getEnvironmentVariable("PHRASE_FEATURES", {}).isNotEmpty())
                    {
                        const auto& o = a.onsets[static_cast<size_t>(best)];
                        const double t = o.novelty[0] + o.novelty[1] + o.novelty[2] + o.novelty[3];
                        std::cout << "F " << role << " " << o.role << " " << o.novelty[0] / t << " " << o.novelty[1] / t << " " << o.novelty[2] / t << " " << o.novelty[3] / t << " iso " << o.isolation << " free " << o.free / fs * 1000 << " decay " << o.decayMs << std::endl;
                    }
                }
            }
            for (const auto& o : a.onsets)
                detected += o.secondary ? 0 : 1;
            const juce::String key = juce::String(p.name) + "_kit" + juce::String(kitIndex);
            std::cout << key << ": onsets " << a.onsets.size() << " truth " << merged.size() << " genre " << ph::genreName(a.genre)
                      << " swing " << a.swing << " (true " << (p.swing - 0.5) * 12 << ") phase " << a.phase / fs * 1000 << "ms drumness " << a.drumness
                      << " timing " << a.timing << " usable " << a.usable[0] << "/" << a.usable[1] << "/" << a.usable[2] << "\n";
            wav(out.getChildFile(key + "_source.wav"), loop);
            const int beat = static_cast<int>(fs * 60 / p.bpm);
            for (int recipe : { tr::kPhraseRecipe, 21 })
                for (int beats : { 1, 2, 4 })
                    for (int seed = 1; seed <= 3; ++seed)
                    {
                        auto t = s;
                        t.recipe = recipe; t.fillBeats = beats; t.seed = static_cast<std::uint64_t>(seed);
                        tr::Plan plan;
                        const auto y = recipe == 21 ? tr::renderComposedFill(loop, t, &plan) : tr::renderPhraseFill(loop, t, &plan);
                        juce::AudioBuffer<float> listen(2, 2 * length + beat);
                        for (int c = 0; c < 2; ++c)
                        {
                            listen.copyFrom(c, 0, loop, c, 0, length);
                            listen.copyFrom(c, length, y, c, 0, length);
                            listen.copyFrom(c, 2 * length, loop, c, 0, beat);
                        }
                        const juce::String name = key + (recipe == 21 ? "_r14_" : "_phrase_") + juce::String(beats) + "b_s" + juce::String(seed);
                        wav(out.getChildFile(name + ".wav"), listen);
                        const auto checked = check(loop, y, p.bpm, beats);
                        checks[recipe].push_back(checked);
                        std::cout << "  CHECK " << name << " cos " << checked.cosine << " level " << checked.levelDb << " grid " << checked.gridTicks << std::endl;
                        if (recipe != 21)
                        {
                            bool finite = true;
                            for (int c = 0; c < 2; ++c)
                                for (int n = 0; n < y.getNumSamples(); ++n)
                                    finite = finite && std::isfinite(y.getSample(c, n));
                            std::cout << "  " << name << ": status " << plan.status << " " << plan.family << " events " << plan.events.size()
                                      << " anchors " << plan.anchors << " conf " << plan.confidence << " change " << 1 - plan.inputSimilarity
                                      << " peak " << juce::Decibels::gainToDecibels(y.getMagnitude(0, y.getNumSamples())) << "dB" << (finite ? "" : " NONFINITE")
                                      << (plan.reason.isNotEmpty() ? " / " + plan.reason : juce::String()) << "\n";
                        }
                    }
        }
    }
    std::sort(attackErrors.begin(), attackErrors.end());
    if (!attackErrors.empty())
        std::cout << "attack error ms (detected - true sound start): p10 " << attackErrors[attackErrors.size() / 10] << " median " << attackErrors[attackErrors.size() / 2] << " p90 " << attackErrors[attackErrors.size() * 9 / 10] << std::endl;
    std::cout << "\nonset recall " << static_cast<double>(found) / truthTotal << " precision " << static_cast<double>(found) / std::max(1, detected)
              << " role accuracy " << static_cast<double>(roleRight) / std::max(1, found) << " genre " << genreRight << "/" << loops << "\n";
    for (auto& [recipe, list] : checks)
    {
        std::vector<double> cosine, level, grid;
        for (const auto& c : list) { cosine.push_back(c.cosine); level.push_back(std::abs(c.levelDb)); grid.push_back(c.gridTicks); }
        const auto median = [](std::vector<double> v) { std::sort(v.begin(), v.end()); return v.empty() ? 0.0 : v[v.size() / 2]; };
        const auto worst = [](const std::vector<double>& v, bool high) { return v.empty() ? 0.0 : high ? *std::max_element(v.begin(), v.end()) : *std::min_element(v.begin(), v.end()); };
        std::cout << (recipe == 21 ? "R14 AUTO COMPOSE" : "R15 PHRASE") << " renders " << list.size() << ": band-tone cosine median " << median(cosine) << " worst " << worst(cosine, false)
                  << " | |level diff| dB median " << median(level) << " worst " << worst(level, true) << " | onset grid deviation ticks median " << median(grid) << " worst " << worst(grid, true) << std::endl;
    }
    for (const auto& [role, row] : confusion)
        std::cout << "  truth " << ph::roleName(role) << " -> kick " << row[0] << " snare " << row[1] << " hat " << row[2] << "\n";
    return 0;
}
