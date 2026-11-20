#include "DSP/DrumPhraseFill.h"

#include <algorithm>
#include <cmath>
#include <numeric>

// R17 FILL ENGINE (docs/R17_FILL_ENGINE.md): six recognisable fill types (BUILD, AROUND THE KIT, BIG SNARE, GROOVE
// BREAK, TRAP ROLL, SAMPLE CHOP) and AUTO, played with the best slices of the source. Offline, message thread only.
// The analysis (roles, slices, swing, the user's marks) is the R15 one; the rhythm, the mix and the bed are new.
namespace hpeq::dsp::transition
{
namespace
{
namespace ph = phrase;
constexpr double kPi = 3.14159265358979323846;

std::uint64_t scramble(std::uint64_t z) noexcept
{
    z += 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}
double rnd(std::uint64_t seed, std::uint64_t salt) noexcept
{
    return static_cast<double>(scramble(seed ^ scramble(salt + 0x52313700ull)) >> 11) / 9007199254740992.0;
}
double cosFade(double x) noexcept { return 0.5 - 0.5 * std::cos(kPi * std::clamp(x, 0.0, 1.0)); }
double dB(double d) noexcept { return std::pow(10.0, d / 20.0); }
double semitones(double s) noexcept { return std::pow(2.0, s / 12.0); }

enum Type { TAuto, TBuild, TAround, TBig, TGroove, TTrap, TChop, kTypes };
constexpr const char* kTypeNames[kTypes] { "FILL AUTO", "BUILD", "AROUND THE KIT", "BIG SNARE", "GROOVE BREAK", "TRAP ROLL", "SAMPLE CHOP" };
enum Group { GKick, GSnare, GHat, GPerc, GChop = 100 };

struct Biquad
{
    double a1 = 0, a2 = 0, b0 = 1, b1 = 0, b2 = 0, z1 = 0, z2 = 0;
    void set(double fs, double hz, double q, bool high) noexcept
    {
        const double w = 2 * kPi * std::clamp(hz, 10.0, fs * 0.45) / fs, c = std::cos(w), alpha = std::sin(w) / (2 * q), a0 = 1 + alpha;
        b0 = (high ? 1 + c : 1 - c) * 0.5 / a0;
        b1 = (high ? -(1 + c) : 1 - c) / a0;
        b2 = b0;
        a1 = -2 * c / a0;
        a2 = (1 - alpha) / a0;
    }
    double tick(double x) noexcept
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};
juce::AudioBuffer<float> filtered(const juce::AudioBuffer<float>& x, double fs, double hz, bool high)
{
    juce::AudioBuffer<float> y(x);
    for (int c = 0; c < y.getNumChannels(); ++c)
    {
        Biquad a, b;
        a.set(fs, hz, 0.541196100146197, high);
        b.set(fs, hz, 1.306562964876377, high);
        auto* d = y.getWritePointer(c);
        for (int n = 0; n < y.getNumSamples(); ++n)
            d[n] = static_cast<float>(b.tick(a.tick(d[n])));
    }
    return y;
}

// an 8-line feedback delay network, wet only: the fill's room and the landing's tail
void reverb(const juce::AudioBuffer<float>& in, juce::AudioBuffer<float>& out, double fs, double decaySeconds)
{
    static constexpr double ms[8] { 29.7, 37.1, 41.1, 43.7, 53.3, 59.9, 67.1, 73.9 };
    std::array<std::vector<float>, 8> lines;
    std::array<int, 8> pos {};
    std::array<double, 8> gain {}, low {};
    for (size_t i = 0; i < 8; ++i)
    {
        lines[i].assign(static_cast<size_t>(std::max(8.0, ms[i] * 1.0e-3 * fs)), 0.0f);
        gain[i] = std::pow(10.0, -3.0 * static_cast<double>(lines[i].size()) / (decaySeconds * fs));
    }
    const double damp = std::exp(-2.0 * kPi * 5500.0 / fs);
    for (int n = 0; n < out.getNumSamples(); ++n)
    {
        const double x = n < in.getNumSamples() ? 0.5 * (in.getSample(0, n) + in.getSample(1, n)) : 0.0;
        std::array<double, 8> o {};
        double sum = 0.0;
        for (size_t i = 0; i < 8; ++i)
        {
            o[i] = lines[i][static_cast<size_t>(pos[i])];
            low[i] = o[i] + damp * (low[i] - o[i]);
            sum += gain[i] * low[i];
        }
        sum *= 0.25;
        for (size_t i = 0; i < 8; ++i)
        {
            lines[i][static_cast<size_t>(pos[i])] = static_cast<float>(gain[i] * low[i] - sum + (i % 2 == 0 ? x : -x));
            pos[i] = pos[i] + 1 == static_cast<int>(lines[i].size()) ? 0 : pos[i] + 1;
        }
        out.setSample(0, n, static_cast<float>(0.35 * (o[0] - o[2] + o[4] - o[6])));
        out.setSample(1, n, static_cast<float>(0.35 * (o[1] - o[3] + o[5] - o[7])));
    }
}

double truePeak(const juce::AudioBuffer<float>& x, int from, int to)
{
    double peak = 0;
    for (int c = 0; c < x.getNumChannels(); ++c)
    {
        const float* d = x.getReadPointer(c);
        for (int n = std::max(1, from); n < std::min(to, x.getNumSamples() - 2); ++n)
        {
            peak = std::max(peak, static_cast<double>(std::abs(d[n])));
            for (int k = 1; k < 4; ++k)
            {
                const double t = k / 4.0, p0 = d[n - 1], p1 = d[n], p2 = d[n + 1], p3 = d[n + 2];
                const double v = 0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
                peak = std::max(peak, std::abs(v));
            }
        }
    }
    return peak;
}

// a playable source slice
struct Voice
{
    int start = 0, length = 0, preRoll = 0, clean = ph::Full, role = ph::Snare, onset = -1;
    double norm = 1;
};
// a stroke on the fill's grid (beats from the window start)
struct Hit
{
    double beat = 0;
    int voice = 0, role = ph::Snare, group = GSnare;
    double rate = 1, rateEnd = -1, gain = 1, pan = 0, send = 0, holdBeats = -1;
    bool reverse = false, chop = false, swing = true, keep = false, roll = false;
};
struct Segment { double from = 0, to = 0; int div = 1; };

struct Ctx
{
    const ph::Analysis* a = nullptr;
    const juce::AudioBuffer<float>* source = nullptr;
    Settings s;
    double fs = 48000, beat = 24000, origin = 0, windowStart = 0, swingBeats = 0;
    int length = 0, W = 4, type = TBuild;
    double energy = 0.6, space = 0.4, pitch = 0.5, drop = 0.25, dropBeats = 0;
    std::vector<Voice> voices;
    int kick = -1, kick2 = -1, snare = -1, snare2 = -1, hat = -1, perc = -1;
    std::array<int, 3> toms { -1, -1, -1 };
    std::array<double, 3> tomRate { 1, 1, 1 };
    bool madeToms = false, drums = false;
    std::uint64_t seed = 1;
    juce::String detail;
};

int addVoice(Ctx& x, int onset, int clean)
{
    const auto& o = x.a->onsets[static_cast<size_t>(onset)];
    static constexpr double nominal[ph::kRoles] { 0.34, 0.30, 0.10, 0.24, 0.45 };
    Voice v;
    v.onset = onset;
    v.start = o.start;
    v.preRoll = std::max(0, o.attack - o.start);
    v.role = o.role;
    v.clean = clean;
    const double longest = nominal[o.role] * (0.8 + 0.7 * x.space) * x.fs;
    v.length = std::max(32, std::min(o.free > 0 ? o.free : static_cast<int>(longest), static_cast<int>(longest)));
    v.length = std::min(v.length, x.length - v.start);
    v.norm = std::clamp(x.a->typicalPeak[static_cast<size_t>(o.role)] / std::max(1e-9, o.peak), 0.25, 1.6);
    x.voices.push_back(v);
    return static_cast<int>(x.voices.size()) - 1;
}

// the beat's segments: the last `count` divisions of `divs`, each a whole number of half beats, the earlier ones longer
std::vector<Segment> stages(double W, std::vector<int> divs)
{
    const int halves = std::max(1, static_cast<int>(std::lround(2 * W)));
    const int n = std::min<int>(static_cast<int>(divs.size()), halves);
    divs.erase(divs.begin(), divs.begin() + (static_cast<int>(divs.size()) - n));
    std::vector<int> len(static_cast<size_t>(n), halves / n);
    for (int extra = halves - (halves / n) * n, i = 0; extra > 0; --extra, ++i)
        ++len[static_cast<size_t>(i)];
    std::vector<Segment> out;
    double at = 0;
    for (int i = 0; i < n; ++i)
    {
        Segment g;
        g.from = at;
        g.to = at + 0.5 * len[static_cast<size_t>(i)];
        g.div = divs[static_cast<size_t>(i)];
        out.push_back(g);
        at = g.to;
    }
    return out;
}

Hit stroke(int voice, int role, int group, double beat, double gain, double rate = 1, double pan = 0)
{
    Hit h;
    h.voice = voice; h.role = role; h.group = group; h.beat = beat; h.gain = gain; h.rate = rate; h.pan = pan;
    return h;
}

int snareVoice(const Ctx& x, int i) { return (i % 2 == 1 && x.snare2 >= 0) ? x.snare2 : x.snare; }

// ---------------------------------------------------------------------------------------------------------------
// the six types

void buildBuild(Ctx& x, std::vector<Hit>& h, double from, double to, bool kickDrive)
{
    const double W = to - from, e = x.energy;
    const int fast = x.s.bpm <= 110 ? 16 : 12;
    std::vector<int> divs = e < 0.35 ? std::vector<int> { 1, 2, 4 } : e < 0.7 ? std::vector<int> { 1, 2, 4, 8 } : std::vector<int> { 2, 4, 8, fast };
    if (rnd(x.seed, 11) < 0.35) // triplet colour in the middle of the build
        for (size_t i = 0; i + 1 < divs.size(); ++i)
            if (divs[i] == 4)
                divs[i] = 6;
    const bool flams = rnd(x.seed, 12) < 0.4;
    const double cresc = 4 + 8 * e;
    int count = 0;
    for (const auto& g : stages(W, divs))
    {
        const double step = 1.0 / g.div;
        for (double t = g.from; t < g.to - 1e-9; t += step, ++count)
        {
            const double xx = (t + step * 0.5) / W;
            const double gain = dB(-cresc * (1 - xx)) * (t == g.from ? 1.12 : 1.0);
            auto hit = stroke(snareVoice(x, count), ph::Snare, GSnare, from + t, gain, semitones(x.pitch * 7 * std::pow(xx, 1.5)));
            hit.roll = g.div >= 8;
            h.push_back(hit);
            if (flams && t == g.from && g.from > 0)
            {
                auto grace = stroke(snareVoice(x, count + 1), ph::Snare, GSnare, from + t - 0.06, gain * 0.38, hit.rate);
                grace.swing = false;
                h.push_back(grace);
            }
        }
    }
    if (kickDrive && x.kick >= 0 && e > 0.3)
    {
        for (int b = 0; b < static_cast<int>(std::floor(W + 1e-9)); ++b)
            if (b < W - 1 || e <= 0.7)
                h.push_back(stroke(x.kick, ph::Kick, GKick, from + b, 0.85 * dB(-4 * (1 - b / W))));
        if (e > 0.7) // the last beat drives on 8ths
            for (double t = std::max(0.0, W - 1); t < W - 1e-9; t += 0.5)
                h.push_back(stroke(x.kick, ph::Kick, GKick, from + t, 0.9));
    }
}

void buildAround(Ctx& x, std::vector<Hit>& h)
{
    const double W = x.W;
    const double aroundBeats = W <= 2 ? W : W >= 8 ? 4 : 2, pre = W - aroundBeats;
    if (pre > 0)
        buildBuild(x, h, 0, pre, true);
    const int strokes = static_cast<int>(std::lround(aroundBeats * 4));
    const int variant = static_cast<int>(rnd(x.seed, 21) * 3);
    static constexpr double pans[4] { 0.0, 0.35, 0.0, -0.35 };
    const auto drum = [&](int i)
    {
        if (variant == 1) // snare on every beat, the toms walk down between
            return i % 4 == 0 ? 0 : 1 + std::min(2, i * 3 / strokes);
        return std::min(3, i * 4 / strokes);
    };
    int previous = -1;
    for (int i = 0; i < strokes; ++i)
    {
        const double t = pre + i * 0.25, xx = static_cast<double>(i) / std::max(1, strokes - 1);
        const int d = drum(i);
        const bool accent = d != previous || i % 4 == 0;
        previous = d;
        const double gain = dB(-3 * (1 - xx)) * (accent ? 1.0 : 0.78);
        const bool doubled = x.energy > 0.7 && t >= W - 1 && d == 3; // the floor tom doubles to 32nds at the end
        const auto put = [&](double at, double g)
        {
            if (d == 0)
                h.push_back(stroke(snareVoice(x, i), ph::Snare, GSnare, at, g, 1, pans[0]));
            else
            {
                const int k = d - 1;
                auto hit = stroke(x.toms[static_cast<size_t>(k)], ph::Tom, 10 + k, at, g * (x.madeToms ? 1.15 : 1.0), x.tomRate[static_cast<size_t>(k)], pans[d]);
                h.push_back(hit);
            }
        };
        put(t, gain);
        if (doubled)
            put(t + 0.125, gain * 0.85);
        if (variant == 2 && i >= strokes - 2 && x.kick >= 0) // floor tom + kick unison at the end
            h.push_back(stroke(x.kick, ph::Kick, GKick, t, 0.85));
    }
    if (x.kick >= 0)
        for (double b = pre; b < W - 1e-9; b += 1)
            h.push_back(stroke(x.kick, ph::Kick, GKick, b, 0.8));
}

double buildBig(Ctx& x, std::vector<Hit>& h)
{
    const double W = x.W, e = x.energy;
    const double big = W >= 2 ? (rnd(x.seed, 31) < 0.4 ? W - 0.5 : W - 1) : 0.5;
    const double half = big >= 1 ? big * 0.5 : 0;
    int count = 0;
    for (double t = 0; t < big - 1e-9; ++count)
    {
        const bool fast32 = e > 0.7 && t >= big - 0.25;
        const double step = fast32 ? 0.125 : t < half ? 0.5 : 0.25;
        const double xx = t / std::max(0.25, big);
        auto hit = stroke(snareVoice(x, count), ph::Snare, GSnare, t, dB(-10 * (1 - xx) - 2), semitones(x.pitch * 3 * xx));
        hit.roll = fast32;
        h.push_back(hit);
        t += step;
    }
    if (x.kick >= 0 && e > 0.4)
        for (double b = 0; b < big - 1e-9; b += 1)
            h.push_back(stroke(x.kick, ph::Kick, GKick, b, 0.8));
    // the landing: snare + the snare an octave down + the kick, all into a long tail
    auto main = stroke(x.snare, ph::Snare, 50, big, dB(2));
    main.send = 0.6 + 0.4 * x.space;
    main.keep = true;
    h.push_back(main);
    auto octave = stroke(x.snare, ph::Snare, 51, big, 0.55, 0.5);
    octave.send = 0.5;
    octave.keep = true;
    h.push_back(octave);
    if (x.snare2 >= 0)
    {
        auto thick = stroke(x.snare2, ph::Snare, 52, big, 0.5, 1, 0.2);
        thick.send = 0.4;
        thick.keep = true;
        h.push_back(thick);
    }
    if (x.kick >= 0)
    {
        auto k = stroke(x.kick, ph::Kick, 53, big, 0.9);
        k.keep = true;
        h.push_back(k);
    }
    return big;
}

double buildGroove(Ctx& x, std::vector<Hit>& h)
{
    const double W = x.W, last = W <= 1 ? 0.5 : 1.0, edit = W - last;
    // ghost snares in the groove's gaps, on the e / a 16ths only
    for (int p = 1; p < static_cast<int>(std::lround(edit * 4)); p += 2)
    {
        const double t = p * 0.25;
        bool busy = false;
        for (const auto& o : x.a->onsets)
        {
            if (o.secondary)
                continue;
            const double at = (o.attack - x.origin) / x.beat;
            busy = busy || std::abs(at - t) < 1.0 / 12.0;
        }
        if (!busy && rnd(x.seed, 100 + static_cast<std::uint64_t>(p)) < 0.25 + 0.35 * x.energy)
            h.push_back(stroke(snareVoice(x, p), ph::Snare, GSnare, t, 0.30));
    }
    const int variant = static_cast<int>(rnd(x.seed, 41) * (x.toms[0] >= 0 ? 4 : 3));
    const auto snareAt = [&](double t, double g, int i) { h.push_back(stroke(snareVoice(x, i), ph::Snare, GSnare, edit + t, g)); };
    const auto kickAt = [&](double t, double g) { if (x.kick >= 0) h.push_back(stroke(x.kick, ph::Kick, GKick, edit + t, g)); };
    const auto flamAt = [&](double t, double g)
    {
        auto grace = stroke(snareVoice(x, 1), ph::Snare, GSnare, edit + t - 0.06, g * 0.38);
        grace.swing = false;
        h.push_back(grace);
        snareAt(t, g, 0);
    };
    if (last < 1) // a half-beat answer
    {
        kickAt(0, 0.9);
        flamAt(0.25, 1.0);
        return edit;
    }
    switch (variant)
    {
        case 0: kickAt(0, 0.95); snareAt(0.25, 0.35, 1); flamAt(0.5, 1.0); snareAt(0.75, 0.85, 1); break;
        case 1:
        {
            snareAt(0, 0.95, 0); kickAt(0.25, 0.9); snareAt(0.5, 0.9, 1);
            auto r1 = stroke(snareVoice(x, 0), ph::Snare, GSnare, edit + 0.75, 0.75); r1.roll = true; h.push_back(r1);
            auto r2 = stroke(snareVoice(x, 1), ph::Snare, GSnare, edit + 0.875, 0.95); r2.roll = true; r2.swing = false; h.push_back(r2);
            break;
        }
        case 2: kickAt(0, 0.95); kickAt(0.25, 0.85); flamAt(0.5, 1.0); break; // and a rest: the breath before the one
        default:
            snareAt(0, 0.95, 0);
            for (int k = 0; k < 3; ++k)
                h.push_back(stroke(x.toms[static_cast<size_t>(k)], ph::Tom, 10 + k, edit + 0.25 * (k + 1), 0.95, x.tomRate[static_cast<size_t>(k)], k == 0 ? 0.35 : k == 2 ? -0.35 : 0.0));
            kickAt(0.75, 0.8);
            break;
    }
    return edit;
}

void buildTrap(Ctx& x, std::vector<Hit>& h)
{
    const double W = x.W, e = x.energy;
    const int fast = x.s.bpm <= 100 ? 16 : 12;
    const std::vector<int> divs = e < 0.35 ? std::vector<int> { 2, 4, 6 } : e < 0.7 ? std::vector<int> { 2, 4, 6, 8 } : std::vector<int> { 4, 6, 8, fast };
    const int voice = x.hat >= 0 ? x.hat : x.snare;
    const int role = x.hat >= 0 ? ph::Hat : ph::Snare;
    int count = 0;
    for (const auto& g : stages(W, divs))
    {
        const double step = 1.0 / g.div;
        const int n = static_cast<int>(std::lround((g.to - g.from) * g.div));
        for (int k = 0; k < n; ++k, ++count)
        {
            const double t = g.from + k * step, xx = t / W;
            auto hit = stroke(voice, role, GHat, t, dB(-9 * (1 - static_cast<double>(k) / std::max(1, n - 1))) * (0.85 + 0.15 * xx),
                              semitones(x.pitch * 7 * xx * xx), (count % 2 ? 0.12 : -0.12) * x.space);
            hit.roll = g.div >= 8;
            hit.swing = g.div == 4;
            h.push_back(hit);
        }
    }
}

// SAMPLE CHOP: any audio. Material: a SAMPLE mark, else the window that the fill replaces.
void buildChop(Ctx& x, std::vector<Hit>& h)
{
    int m0 = static_cast<int>(std::lround(std::max(0.0, x.origin))), m1 = x.length;
    for (const auto& m : x.s.marks)
        if (m.role == kSampleMark && m.end - m.start > static_cast<int>(0.05 * x.fs))
        {
            m0 = std::clamp(m.start, 0, x.length - 1);
            m1 = std::clamp(m.end, m0 + 1, x.length);
            x.detail = "SAMPLE MARK";
            break;
        }
    std::vector<int> cuts;
    for (const auto& o : x.a->onsets)
        if (!o.secondary && o.start >= m0 && o.start < m1 - static_cast<int>(0.03 * x.fs))
            cuts.push_back(o.start);
    if (cuts.size() < 3) // no attacks: 1/8 grid
    {
        cuts.clear();
        for (double p = m0; p < m1 - 0.25 * x.beat; p += 0.5 * x.beat)
            cuts.push_back(static_cast<int>(std::lround(p)));
    }
    if (cuts.empty())
        cuts.push_back(m0);
    std::vector<int> slice;
    for (size_t i = 0; i < cuts.size(); ++i)
    {
        Voice v;
        v.start = cuts[i];
        v.length = std::max(64, (i + 1 < cuts.size() ? cuts[i + 1] : m1) - cuts[i]);
        v.length = std::min(v.length, x.length - v.start);
        v.role = ph::Perc;
        x.voices.push_back(v);
        slice.push_back(static_cast<int>(x.voices.size()) - 1);
    }
    const bool marked = x.detail.startsWith("SAMPLE MARK");
    // a long voice from a point of the material (half speed and tape stop read through it)
    const auto longVoice = [&](int from)
    {
        Voice v;
        v.start = std::clamp(from, 0, x.length - 1);
        v.length = std::max(64, m1 - v.start);
        v.length = std::min(v.length, x.length - v.start);
        v.role = ph::Perc;
        x.voices.push_back(v);
        return static_cast<int>(x.voices.size()) - 1;
    };
    // the stutter's piece: one of the two loudest of the first four slices (a quiet hat repeated is no fill)
    const auto loudestOf = [&](const std::vector<int>& pieces, std::uint64_t salt)
    {
        std::vector<std::pair<double, int>> loudest;
        for (size_t i = 0; i < std::min<size_t>(4, pieces.size()); ++i)
        {
            const auto& v = x.voices[static_cast<size_t>(pieces[i])];
            double peak = 0;
            for (int c = 0; c < x.source->getNumChannels(); ++c)
                peak = std::max(peak, static_cast<double>(x.source->getMagnitude(c, v.start, std::min(v.length, static_cast<int>(0.1 * x.fs)))));
            loudest.push_back({ peak, pieces[i] });
        }
        std::stable_sort(loudest.begin(), loudest.end(), [](const auto& p1, const auto& p2) { return p1.first > p2.first; });
        return loudest[static_cast<size_t>(loudest.size() > 1 && rnd(x.seed, 62 + salt) < 0.35 ? 1 : 0)].second;
    };
    const auto chop = [&](int voice, double t, double hold, double gain, double rate = 1, bool reverse = false)
    {
        Hit c;
        c.voice = voice; c.role = ph::Perc; c.group = GChop; c.beat = t; c.holdBeats = hold; c.gain = gain; c.rate = rate; c.reverse = reverse;
        c.chop = true; c.swing = false; c.send = 0.3 * x.space;
        h.push_back(c);
    };
    static constexpr const char* kPatternNames[] { "STUTTER", "REARRANGE", "HALF SPEED", "TAPE STOP" };
    // one pattern over [from, from + W) beats; salt 0 = the plain fill (the same random draws as 0.5.0)
    const auto segment = [&](int pattern, double from, double W, const std::vector<int>& pieces, int first, int all, std::uint64_t salt)
    {
        if (pattern == 0) // STUTTER: one slice, faster and higher
        {
            const std::vector<int> divs = x.energy < 0.5 ? std::vector<int> { 2, 4, 8 } : std::vector<int> { 2, 4, 8, 16 };
            for (const auto& g : stages(W, divs))
            {
                const double step = 1.0 / g.div;
                for (double t = g.from; t < g.to - 1e-9; t += step)
                    chop(first, from + t, step * 0.85, dB(-4 * (1 - t / W)), semitones(x.pitch * 7 * std::pow(t / W, 1.5)));
            }
        }
        else if (pattern == 1) // REARRANGE: 1/8 pieces in another order, the last one backwards
        {
            const int eighths = static_cast<int>(std::lround(W * 2));
            std::vector<int> order(pieces.size());
            std::iota(order.begin(), order.end(), 0);
            for (size_t i = order.size(); i > 1; --i)
                std::swap(order[i - 1], order[static_cast<size_t>(rnd(x.seed, 70 + i + salt) * static_cast<double>(i))]);
            const int doubled = static_cast<int>(rnd(x.seed, 64 + salt) * eighths);
            for (int k = 0; k < eighths; ++k)
            {
                const int v = pieces[static_cast<size_t>(order[static_cast<size_t>(k) % order.size()])];
                const double pitchUp = x.pitch > 0.6 && k % 4 == 3 ? 12 : 0;
                if (k == doubled)
                {
                    chop(v, from + k * 0.5, 0.22, 0.95, semitones(pitchUp));
                    chop(v, from + k * 0.5 + 0.25, 0.22, 0.95, semitones(pitchUp));
                }
                else if (k == eighths - 1 && rnd(x.seed, 65 + salt) < 0.6)
                    chop(v, from + k * 0.5, 0.5, 0.9, 1, true);
                else
                    chop(v, from + k * 0.5, 0.48, 0.95, semitones(pitchUp));
            }
        }
        else if (pattern == 2) // HALF SPEED: the material, then an octave down, then a 1/32 stutter
        {
            const double half = std::floor(W) >= 2 ? std::floor(W / 2) : 0.5;
            for (double t = 0; t < half - 1e-9; t += 0.5)
                chop(pieces[static_cast<size_t>(static_cast<int>(t * 2) % static_cast<int>(pieces.size()))], from + t, 0.48, 0.95);
            chop(all, from + half, W - half - 0.25, 0.95, 0.5);
            for (int k = 0; k < 2; ++k)
                chop(first, from + W - 0.25 + k * 0.125, 0.11, 0.9, semitones(x.pitch * 5));
        }
        else // TAPE STOP: a 1/16 stutter, then the material slowing down to a stop
        {
            const double half = W >= 2 ? W / 2 : 0.5;
            for (double t = 0; t < half - 1e-9; t += 0.25)
                chop(first, from + t, 0.2, dB(-3 * (1 - t / half)));
            Hit stop;
            stop.voice = all; stop.role = ph::Perc; stop.group = GChop; stop.beat = from + half; stop.holdBeats = W - half;
            stop.rate = 1; stop.rateEnd = 0.06; stop.chop = true; stop.swing = false; stop.send = 0.3 * x.space;
            h.push_back(stop);
        }
    };
    // the pieces that start inside [b0, b1) source samples; too few: the material's pieces, turned by `turn`
    const auto piecesIn = [&](double b0, double b1, int turn)
    {
        std::vector<int> pieces;
        if (!marked)
            for (int v : slice)
            {
                const int at = x.voices[static_cast<size_t>(v)].start;
                if (at >= b0 - 0.01 * x.fs && at < b1 - 0.01 * x.fs)
                    pieces.push_back(v);
            }
        if (pieces.size() < 2)
        {
            pieces = slice;
            std::rotate(pieces.begin(), pieces.begin() + static_cast<std::ptrdiff_t>(static_cast<size_t>(turn) % pieces.size()), pieces.end());
        }
        return pieces;
    };
    const double W = x.W;
    const int pattern = static_cast<int>(rnd(x.seed, 61) * 4);
    int first = loudestOf(slice, 0);
    const juce::String name = kPatternNames[pattern];
    if (x.s.fillBeats == kWholeLoop && W > 4) // R18 WHOLE SAMPLE: every bar re-cut from its own audio, the last one into the downbeat
    {
        const int bars = static_cast<int>(std::floor(W / 4 + 1e-9));
        for (int bar = 0; bar + 1 < bars; ++bar)
        {
            const std::uint64_t salt = 1000ull * static_cast<std::uint64_t>(bar + 1);
            const double b0 = x.origin + bar * 4 * x.beat;
            const auto pieces = piecesIn(b0, b0 + 4 * x.beat, bar);
            const double pick = rnd(x.seed, 61 + salt); // mid-phrase: no tape stop (it would sound broken)
            if (pick < 0.5)
                segment(1, bar * 4.0, 4.0, pieces, 0, 0, salt);
            else if (pick < 0.8) // three beats re-arranged, a stutter on the last
            {
                segment(1, bar * 4.0, 3.0, pieces, 0, 0, salt);
                segment(0, bar * 4.0 + 3.0, 1.0, pieces, loudestOf(pieces, salt), 0, salt);
            }
            else
                segment(2, bar * 4.0, 4.0, pieces, loudestOf(pieces, salt), longVoice(static_cast<int>(std::lround(marked ? m0 : b0))), salt);
        }
        const double lastFrom = (bars - 1) * 4.0, b0 = x.origin + lastFrom * x.beat;
        const auto pieces = piecesIn(b0, static_cast<double>(x.length) + x.fs, bars - 1);
        first = loudestOf(pieces, 9000);
        segment(pattern, lastFrom, W - lastFrom, pieces, first, longVoice(static_cast<int>(std::lround(marked ? m0 : b0))), 9000);
        x.detail = x.detail.isEmpty() ? "WHOLE SAMPLE / " + name : x.detail + " / WHOLE SAMPLE / " + name;
    }
    else
    {
        x.detail = x.detail.isEmpty() ? name : x.detail + " / " + name;
        segment(pattern, 0, W, slice, first, longVoice(m0), 0);
    }
    if (pattern == 0 && rnd(x.seed, 63) < 0.4) // the last 1/8 backwards into the downbeat
    {
        h.erase(std::remove_if(h.begin(), h.end(), [&](const Hit& c) { return c.beat >= W - 0.5 - 1e-9; }), h.end());
        chop(first, W - 0.5, 0.5, 0.9, 1, true);
    }
}

// ---------------------------------------------------------------------------------------------------------------

struct Rendered
{
    int at = 0, length = 0, voice = 0;
    Hit hit;
    int release = 0;
};

Plan plan(const juce::AudioBuffer<float>& source, Settings s, juce::AudioBuffer<float>* render)
{
    Plan p;
    p.recipe = s.recipe;
    Ctx x;
    x.s = s;
    x.fs = s.sampleRate;
    x.length = source.getNumSamples();
    x.energy = s.knobs[0]; x.space = s.knobs[1]; x.pitch = s.knobs[2]; x.drop = s.knobs[3];
    x.seed = scramble(s.seed * 0x9e3779b97f4a7c15ull + 0x52313737ull);
    const auto a = ph::analyse(source, s.sampleRate, s.bpm, s.marks);
    x.a = &a;
    x.source = &source;
    x.beat = s.sampleRate * 60.0 / s.bpm;
    x.W = std::min(s.fillBeats, s.bars * 4);
    x.windowStart = std::max(0.0, x.length - x.W * x.beat);
    x.origin = a.phase + x.windowStart;
    x.swingBeats = a.swing / 24.0;
    p.genre = a.genre;
    p.phraseEnd = x.length;
    for (const auto& o : a.onsets) // the page snaps marks to these hit regions
        if (!o.secondary)
        {
            Slice slice;
            slice.sample = o.start; slice.end = o.start + o.free; slice.role = o.role;
            p.slices.push_back(slice);
        }
    if (render)
        *render = source;
    if (truePeak(source, 0, x.length) < 1.0e-5)
    {
        p.status = ph::Bypass;
        p.reason = "SILENT SOURCE - NOTHING TO CUT";
        return p;
    }

    if (std::all_of(s.knobs.begin(), s.knobs.end(), [](double v) { return v == 0; }))
    {
        p.status = ph::Bypass;
        p.reason = "ALL KNOBS AT ZERO - SOURCE KEPT";
        return p;
    }
    // the kit: per role the usable hits, best first (a role the user marked uses only the marked hits)
    std::array<std::vector<int>, ph::kRoles> pool;
    for (int role = 0; role < ph::kRoles; ++role)
    {
        const bool userRole = std::any_of(a.onsets.begin(), a.onsets.end(), [&](const ph::Onset& o) { return o.marked && o.role == role; });
        for (int i = 0; i < static_cast<int>(a.onsets.size()); ++i)
        {
            const auto& o = a.onsets[static_cast<size_t>(i)];
            if (o.role == role && (ph::quality(o) >= ph::kUsable || o.marked) && (!userRole || o.marked))
                pool[static_cast<size_t>(role)].push_back(i);
        }
        std::stable_sort(pool[static_cast<size_t>(role)].begin(), pool[static_cast<size_t>(role)].end(),
                         [&](int i, int j) { return ph::quality(a.onsets[static_cast<size_t>(i)]) > ph::quality(a.onsets[static_cast<size_t>(j)]); });
        p.kitCounts[static_cast<size_t>(role)] = static_cast<int>(pool[static_cast<size_t>(role)].size());
    }
    const auto take = [&](int role, size_t k, int clean) { return pool[static_cast<size_t>(role)].size() > k ? addVoice(x, pool[static_cast<size_t>(role)][k], clean) : -1; };
    const auto cleanOf = [&](int role, size_t k) { return pool[static_cast<size_t>(role)].size() > k ? a.onsets[static_cast<size_t>(pool[static_cast<size_t>(role)][k])].clean : static_cast<int>(ph::Full); };
    x.kick = take(ph::Kick, 0, cleanOf(ph::Kick, 0));
    x.kick2 = take(ph::Kick, 1, cleanOf(ph::Kick, 1));
    x.snare = take(ph::Snare, 0, cleanOf(ph::Snare, 0));
    x.snare2 = take(ph::Snare, 1, cleanOf(ph::Snare, 1));
    x.hat = take(ph::Hat, 0, cleanOf(ph::Hat, 0));
    x.perc = take(ph::Perc, 0, ph::Full);
    juce::StringArray made;
    if (x.snare < 0 && x.perc >= 0) { x.snare = x.perc; made.add("PERC AS SNARE"); }
    if (x.snare < 0 && x.hat >= 0 && x.kick >= 0) // a clap-less loop: the kick's attack pitched up stands in
    {
        const Voice kickCopy = x.voices[static_cast<size_t>(x.kick)];
        x.voices.push_back(kickCopy);
        x.voices.back().clean = ph::HighPassLow;
        x.voices.back().role = ph::Snare;
        x.snare = static_cast<int>(x.voices.size()) - 1;
        made.add("SNARE MADE FROM THE KICK");
    }
    // toms: the user's (high -> low by the body's zero-crossing rate), else made from the snare
    {
        std::vector<std::pair<double, int>> pitched;
        for (int i : pool[ph::Tom])
        {
            const auto& o = a.onsets[static_cast<size_t>(i)];
            const int from = std::min(x.length - 1, o.attack + static_cast<int>(0.005 * x.fs));
            const int to = std::min({ x.length, o.attack + static_cast<int>(0.080 * x.fs), o.start + std::max(1, o.free) });
            int crossings = 0;
            float previous = 0;
            for (int n = from; n < to; ++n)
            {
                float v = 0;
                for (int ch = 0; ch < source.getNumChannels(); ++ch)
                    v += source.getSample(ch, n);
                if ((v > 0) != (previous > 0) && n > from)
                    ++crossings;
                previous = v;
            }
            pitched.push_back({ to > from ? crossings / static_cast<double>(to - from) : 0.0, i });
        }
        std::stable_sort(pitched.begin(), pitched.end(), [](const auto& p1, const auto& p2) { return p1.first > p2.first; });
        if (pitched.size() >= 3)
        {
            const size_t picks[3] { 0, pitched.size() / 2, pitched.size() - 1 };
            for (int k = 0; k < 3; ++k)
                x.toms[static_cast<size_t>(k)] = addVoice(x, pitched[picks[k]].second, ph::Full);
        }
        else if (x.snare >= 0)
        {
            // made toms: the snare's body, low-passed, a third / more down per drum (PITCH spreads them)
            x.madeToms = true;
            Voice body = x.voices[static_cast<size_t>(x.snare)];
            body.clean = ph::LowPass;
            body.role = ph::Tom;
            body.length = std::min(x.length - body.start, std::max(body.length, static_cast<int>(0.22 * x.fs)));
            x.voices.push_back(body);
            const int v = static_cast<int>(x.voices.size()) - 1;
            const double step = 3 + 3 * x.pitch;
            for (int k = 0; k < 3; ++k)
            {
                x.toms[static_cast<size_t>(k)] = v;
                x.tomRate[static_cast<size_t>(k)] = semitones(-step * (k + 1));
            }
            for (int k = 0; k < static_cast<int>(pitched.size()) && k < 3; ++k) // one or two marked toms lead, made ones follow
            {
                x.toms[static_cast<size_t>(k)] = addVoice(x, pitched[static_cast<size_t>(k)].second, ph::Full);
                x.tomRate[static_cast<size_t>(k)] = 1;
            }
            if (!pitched.empty())
                for (int k = static_cast<int>(pitched.size()); k < 3; ++k)
                    x.tomRate[static_cast<size_t>(k)] = semitones(-step * (k - static_cast<int>(pitched.size()) + 1)) * 0.94;
            made.add("TOMS MADE FROM THE SNARE");
        }
    }
    x.drums = x.snare >= 0 && (x.kick >= 0 || x.hat >= 0 || made.isEmpty()) && a.drumness >= 0.35;

    // the type: chosen, or AUTO by length, the kit and the seed
    int type = std::clamp(s.recipe - kR17Recipe, 0, kTypes - 1);
    const bool automatic = type == TAuto;
    if (automatic)
    {
        if (!x.drums)
            type = TChop;
        else
        {
            const bool trapish = a.genre == ph::Trap && x.hat >= 0;
            double w[kTypes] {};
            if (x.W <= 1) { w[TGroove] = 0.35; w[TBig] = 0.25; w[TBuild] = 0.2; w[TTrap] = trapish ? 0.3 : 0.05; }
            else if (x.W <= 2) { w[TBuild] = 0.3; w[TBig] = 0.25; w[TAround] = 0.25; w[TGroove] = 0.1; w[TTrap] = trapish ? 0.3 : 0.08; }
            else { w[TBuild] = 0.35; w[TAround] = 0.3; w[TBig] = 0.2; w[TTrap] = trapish ? 0.35 : 0.1; }
            double total = 0;
            for (double v : w)
                total += v;
            double pick = rnd(x.seed, 1) * total;
            type = TBuild;
            for (int t = 1; t < kTypes; ++t)
                if ((pick -= w[t]) < 0) { type = t; break; }
        }
    }
    else if (type != TChop && x.snare < 0)
        type = TChop; // nothing to play a drum fill with: cut the audio instead
    x.type = type;

    // DROP: silence before the downbeat (the tail of the room / the landing still rings through it)
    x.dropBeats = x.drop < 0.1 ? 0.0 : x.drop < 0.4 ? 0.25 : x.drop < 0.75 ? 0.5 : 1.0;
    x.dropBeats = std::min(x.dropBeats, type == TGroove ? 0.25 : x.W * 0.5);

    std::vector<Hit> hits;
    double turn = 0, bedMute = 0; // beats from the window start: where the bed changes / stops
    switch (type)
    {
        case TBuild: buildBuild(x, hits, 0, x.W, true); bedMute = x.W; break;
        case TAround: buildAround(x, hits); bedMute = x.W <= 2 ? 0 : x.W >= 8 ? x.W - 4 : x.W - 2; break;
        case TBig: bedMute = buildBig(x, hits); break;
        case TGroove: turn = bedMute = buildGroove(x, hits); break;
        case TTrap: buildTrap(x, hits); bedMute = x.W; break;
        default: buildChop(x, hits); bedMute = 0; break;
    }
    // the drop removes strokes (not the landing) from the last beats
    if (x.dropBeats > 0)
        hits.erase(std::remove_if(hits.begin(), hits.end(), [&](const Hit& h) { return !h.keep && h.beat >= x.W - x.dropBeats - 1e-9; }), hits.end());
    // kit switches: a muted role plays nothing (chops are the audio itself)
    hits.erase(std::remove_if(hits.begin(), hits.end(), [&](const Hit& h) { return !h.chop && (h.voice < 0 || !s.kitEnabled[static_cast<size_t>(std::clamp(h.role, 0, kKitRoles - 1))]); }), hits.end());

    // grid -> samples: straight 1/48 grid, the loop's swing on the off 16ths, the attack on the grid point
    std::vector<Rendered> list;
    for (const auto& h : hits)
    {
        if (h.voice < 0 || h.voice >= static_cast<int>(x.voices.size()))
            continue;
        const auto& v = x.voices[static_cast<size_t>(h.voice)];
        double t = h.beat;
        const double q = t * 4;
        if (h.swing && std::abs(q - std::round(q)) < 1e-6 && static_cast<long long>(std::round(q)) % 2 != 0)
            t += x.swingBeats;
        Rendered r;
        r.hit = h;
        r.voice = h.voice;
        r.at = static_cast<int>(std::lround(x.origin + t * x.beat - v.preRoll / std::max(0.05, h.rate)));
        const double rateAvg = h.rateEnd >= 0 ? 0.5 * (h.rate + h.rateEnd) : h.rate;
        r.length = static_cast<int>(v.length / std::max(0.05, rateAvg));
        if (h.holdBeats > 0)
            r.length = std::min(r.length, static_cast<int>(h.holdBeats * x.beat));
        if (h.rateEnd >= 0) // a tape stop reads as far as it gets
            r.length = static_cast<int>(h.holdBeats * x.beat);
        r.release = static_cast<int>(0.004 * x.fs);
        if (r.at >= x.length - 8 || r.length < 16 || r.at + r.length <= 0)
            continue;
        list.push_back(r);
    }
    std::stable_sort(list.begin(), list.end(), [](const Rendered& p1, const Rendered& p2) { return p1.at < p2.at; });
    // choke: the next stroke of the same drum ends the last one with a short fade (SPACE lets it ring longer)
    const int chokeFade = static_cast<int>((0.003 + 0.03 * x.space) * x.fs);
    for (size_t i = 0; i < list.size(); ++i)
        for (size_t j = i + 1; j < list.size(); ++j)
            if (list[j].hit.group == list[i].hit.group)
            {
                const int cap = list[j].at - list[i].at + (list[i].hit.chop ? static_cast<int>(0.002 * x.fs) : chokeFade);
                if (cap < list[i].length)
                {
                    list[i].length = std::max(16, cap);
                    list[i].release = list[i].hit.chop ? static_cast<int>(0.002 * x.fs) : chokeFade;
                }
                break;
            }

    p.status = ph::Generated;
    p.family = juce::String(kTypeNames[type]) + (automatic ? " (AUTO)" : "");
    if (x.detail.isNotEmpty())
        p.family << " / " << x.detail;
    if (type != TChop && !made.isEmpty())
        p.reason = made.joinIntoString(" / ");
    if (automatic && type == TChop)
        p.reason = "NO DRUM KIT FOUND - THE AUDIO IS CHOPPED";
    p.confidence = a.drumness;
    const int gridStart = static_cast<int>(std::lround(x.origin));
    const int fade6 = std::max(1, static_cast<int>(0.006 * x.fs));
    const int cut = std::clamp(gridStart + static_cast<int>(std::lround(turn * x.beat)) - static_cast<int>(0.004 * x.fs), 0, x.length);
    p.phraseStart = cut;
    for (const auto& r : list)
    {
        const auto& v = x.voices[static_cast<size_t>(r.voice)];
        Event e;
        e.at = std::max(0, r.at); e.length = std::min(r.length, x.length - r.at) - (e.at - r.at); e.source = v.start; e.sourceEnd = v.start + v.length;
        e.rate = r.hit.rate; e.velocity = r.hit.gain * v.norm; e.pan = r.hit.pan; e.reverse = r.hit.reverse;
        e.role = std::clamp(r.hit.role, 0, kKitRoles - 1); e.space = r.hit.send; e.roll = r.hit.roll;
        p.events.push_back(e);
    }
    p.attacks = static_cast<int>(p.events.size());
    if (!render)
        return p;

    // ---- render ----
    const double fs = x.fs;
    auto& out = *render;
    const juce::AudioBuffer<float> clean[4] { source, filtered(source, fs, 3500, false), filtered(source, fs, 200, true), filtered(source, fs, 5000, true) };
    const int muteAt = std::clamp(gridStart + static_cast<int>(std::lround(bedMute * x.beat)) - static_cast<int>(0.004 * x.fs), 0, x.length);
    const int dropAt = x.dropBeats > 0 ? std::clamp(static_cast<int>(std::lround(x.origin + (x.W - x.dropBeats) * x.beat)) - static_cast<int>(0.004 * x.fs), 0, x.length) : x.length;
    const int fillFrom = std::clamp(gridStart - static_cast<int>(0.004 * x.fs), 0, x.length);
    // the bed: what the groove does under the fill
    if (type == TBuild || type == TTrap || type == TBig || (type == TAround && muteAt > fillFrom))
    {
        juce::AudioBuffer<float> lowPassed;
        if (type == TTrap)
            lowPassed = filtered(source, fs, 5500, false); // the roll takes over the hats
        std::array<Biquad, 2> hp;
        const double top = 200 + 1300 * x.energy + 800 * x.drop;
        for (int n = fillFrom; n < x.length; ++n)
        {
            const double xx = std::clamp((n - fillFrom) / std::max(1.0, x.W * x.beat), 0.0, 1.0);
            double gain = 1;
            if (type == TBuild)
            {
                gain = 1 - (0.45 + 0.35 * x.energy) * xx;
                if ((n - fillFrom) % 32 == 0)
                    for (auto& f : hp)
                        f.set(fs, 30 * std::pow(top / 30, xx * xx), 0.8, true);
            }
            else if (type == TBig || type == TAround)
                gain = 1 - 0.4 * std::clamp((n - fillFrom) / std::max(1.0, static_cast<double>(muteAt - fillFrom)), 0.0, 1.0);
            for (int c = 0; c < 2; ++c)
            {
                double v = out.getSample(c, n);
                if (type == TBuild)
                    v = hp[static_cast<size_t>(c)].tick(v);
                else if (type == TTrap)
                {
                    const double k = std::clamp((n - fillFrom) / (0.01 * fs), 0.0, 1.0);
                    v = (1 - k) * v + k * lowPassed.getSample(c, n);
                }
                out.setSample(c, n, static_cast<float>(v * gain));
            }
        }
    }
    const int silentFrom = std::min(type == TBuild || type == TTrap ? x.length : muteAt, dropAt);
    for (int c = 0; c < 2; ++c)
    {
        for (int n = std::max(0, silentFrom - fade6); n < silentFrom; ++n)
            out.setSample(c, n, out.getSample(c, n) * static_cast<float>(cosFade(static_cast<double>(silentFrom - n) / fade6)));
        if (silentFrom < x.length)
            out.clear(c, silentFrom, x.length - silentFrom);
    }
    // the strokes
    juce::AudioBuffer<float> send(2, x.length);
    send.clear();
    for (const auto& r : list)
    {
        const auto& v = x.voices[static_cast<size_t>(r.voice)];
        const auto& src = clean[std::clamp(v.clean, 0, 3)];
        const auto& h = r.hit;
        const int count = std::min(r.length, x.length - std::max(0, r.at));
        const double gL = h.gain * v.norm * std::min(1.0, 1 - h.pan), gR = h.gain * v.norm * std::min(1.0, 1 + h.pan);
        const double attack = h.reverse ? 0.7 * count : (h.chop ? 0.0015 : 0.0006) * fs, release = std::max(1, r.release);
        double pos = 0;
        for (int i = std::max(0, -r.at); i < count; ++i)
        {
            if (pos >= v.length - 1)
                break;
            const double read = h.reverse ? v.length - 1 - pos : pos;
            const int i0 = static_cast<int>(read);
            const double f = read - i0;
            const int at0 = std::clamp(v.start + i0, 0, x.length - 1), at1 = std::clamp(v.start + i0 + 1, 0, x.length - 1);
            const double env = cosFade((i + 1) / (attack + 1)) * cosFade((count - i) / release);
            for (int c = 0; c < 2; ++c)
            {
                const double s0 = src.getSample(c, at0), s1 = src.getSample(c, at1);
                const double val = (s0 + f * (s1 - s0)) * env * (c == 0 ? gL : gR);
                out.addSample(c, r.at + i, static_cast<float>(val));
                if (h.send > 0)
                    send.addSample(c, r.at + i, static_cast<float>(val * h.send));
            }
            const double rate = h.rateEnd >= 0 ? h.rate + (h.rateEnd - h.rate) * std::pow(static_cast<double>(i) / std::max(1, count), 0.7) : h.rate;
            pos += rate;
        }
    }
    // the room and the landing's tail (SPACE): everything sent rings on, also through the drop
    {
        const double decay = 0.5 + 1.5 * x.space + (type == TBig ? 1.0 : 0.0);
        bool any = false;
        for (int c = 0; c < 2 && !any; ++c)
            any = send.getMagnitude(c, 0, x.length) > 0;
        if (any)
        {
            juce::AudioBuffer<float> wet(2, x.length);
            reverb(send, wet, fs, decay);
            const double level = 0.6 + 0.8 * x.space;
            for (int c = 0; c < 2; ++c)
                out.addFrom(c, fillFrom, wet, c, fillFrom, x.length - fillFrom, static_cast<float>(level));
        }
    }
    // land on the bar line: a 3 ms fade at the very end
    const int endFade = std::max(1, static_cast<int>(0.003 * fs));
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < endFade && i < x.length; ++i)
            out.setSample(c, x.length - 1 - i, out.getSample(c, x.length - 1 - i) * static_cast<float>(cosFade(static_cast<double>(i) / endFade)));
    // never louder than the source's own peak or -1.8 dBTP, whichever is higher; the audio before the fill untouched
    const int edited = std::min(cut, fillFrom);
    const double prefix = truePeak(source, 0, x.length), tail = truePeak(out, edited, x.length);
    const double ceiling = std::max(std::min(prefix, 0.98), std::pow(10.0, -1.8 / 20.0));
    if (tail > ceiling)
    {
        const double g = ceiling / tail;
        const int ramp = std::max(1, static_cast<int>(0.002 * fs));
        for (int c = 0; c < 2; ++c)
            for (int n = edited; n < x.length; ++n)
            {
                const double k = n - edited < ramp ? 1 + (g - 1) * (n - edited) / ramp : g;
                out.setSample(c, n, static_cast<float>(out.getSample(c, n) * k));
            }
    }
    p.phraseStart = edited;
    return p;
}

bool sanitize(Settings& s)
{
    if (!std::isfinite(s.sampleRate) || s.sampleRate < 8000 || s.sampleRate > 192000 || !std::isfinite(s.bpm))
        return false;
    s.kind = Fill;
    s.bpm = std::clamp(s.bpm, 20.0, 999.0);
    s.recipe = std::clamp(s.recipe, kR17Recipe, kR17Last);
    s.bars = legalBars(s.bars);
    s.fillBeats = s.fillBeats == 1 || s.fillBeats == 2 || s.fillBeats == 8 || (s.fillBeats == kWholeLoop && s.recipe == kR17Chop) ? s.fillBeats : 4; // R18: SAMPLE CHOP may take the whole source
    for (auto& k : s.knobs)
        k = std::isfinite(k) ? std::clamp(k, 0.0, 1.0) : 0.0;
    return true;
}
juce::AudioBuffer<float> sourceOf(const juce::AudioBuffer<float>& input, const Settings& s)
{
    juce::AudioBuffer<float> source(2, lengthSamples(s));
    source.clear();
    if (input.getNumChannels() > 0)
        for (int c = 0; c < 2; ++c)
            for (int n = 0; n < std::min(input.getNumSamples(), source.getNumSamples()); ++n)
            {
                const float v = input.getSample(std::min(c, input.getNumChannels() - 1), n);
                source.setSample(c, n, std::isfinite(v) ? v : 0.0f);
            }
    return source;
}
} // namespace

Plan makeR17FillPlan(const juce::AudioBuffer<float>& input, const Settings& settings)
{
    Settings s = settings;
    if (!sanitize(s))
        return {};
    return plan(sourceOf(input, s), s, nullptr);
}

juce::AudioBuffer<float> renderR17Fill(const juce::AudioBuffer<float>& input, const Settings& settings, Plan* info)
{
    Settings s = settings;
    if (!sanitize(s))
        return {};
    juce::AudioBuffer<float> out;
    auto p = plan(sourceOf(input, s), s, &out);
    if (info)
        *info = std::move(p);
    return out;
}
} // namespace hpeq::dsp::transition
