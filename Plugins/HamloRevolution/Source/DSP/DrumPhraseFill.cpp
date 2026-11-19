#include "DSP/DrumPhraseFill.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <numeric>

// R15 PHRASE FILL. See docs/DRUM_FILL_R15.md for the design and its limits.
namespace hpeq::dsp::transition
{
namespace phrase
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr double ticksPerBeat = 24.0;

std::uint64_t hash(std::uint64_t z) noexcept
{
    z += 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}
double random(std::uint64_t seed, std::uint64_t salt) noexcept
{
    return static_cast<double>(hash(seed ^ hash(salt)) >> 11) / 9007199254740992.0;
}
double raised(double x) noexcept { return 0.5 - 0.5 * std::cos(pi * std::clamp(x, 0.0, 1.0)); }

struct Biquad
{
    double a1 = 0, a2 = 0, b0 = 1, b1 = 0, b2 = 0, z1 = 0, z2 = 0;
    Biquad(double fs, double hz, double q, bool high)
    {
        const double w = 2 * pi * std::clamp(hz, 10.0, fs * 0.45) / fs, c = std::cos(w), alpha = std::sin(w) / (2 * q), a0 = 1 + alpha;
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
// 4th-order Butterworth (two biquads), forward only
void filter4(std::vector<float>& x, double fs, double hz, bool high)
{
    Biquad a(fs, hz, 0.541196100146197, high), b(fs, hz, 1.306562964876377, high);
    for (auto& v : x)
        v = static_cast<float>(b.tick(a.tick(v)));
}
juce::AudioBuffer<float> filtered(const juce::AudioBuffer<float>& x, double fs, double hz, bool high)
{
    juce::AudioBuffer<float> y(x);
    for (int c = 0; c < y.getNumChannels(); ++c)
    {
        Biquad a(fs, hz, 0.541196100146197, high), b(fs, hz, 1.306562964876377, high);
        auto* d = y.getWritePointer(c);
        for (int n = 0; n < y.getNumSamples(); ++n)
            d[n] = static_cast<float>(b.tick(a.tick(d[n])));
    }
    return y;
}
double median(std::vector<double> v)
{
    if (v.empty())
        return 0;
    const auto mid = v.begin() + static_cast<std::ptrdiff_t>(v.size() / 2);
    std::nth_element(v.begin(), mid, v.end());
    return *mid;
}
double percentile(std::vector<double> v, double q)
{
    if (v.empty())
        return 0;
    std::sort(v.begin(), v.end());
    return v[static_cast<size_t>(std::clamp(q, 0.0, 1.0) * static_cast<double>(v.size() - 1))];
}
// nominal slice length per role, seconds (a hit longer than this is choked)
constexpr double nominal[kRoles] { 0.30, 0.24, 0.09, 0.22, 0.40 };
constexpr double needed[kRoles] { 0.16, 0.12, 0.045, 0.06, 0.12 };
} // namespace

const char* genreName(int genre) noexcept
{
    static constexpr const char* names[] { "BOOM BAP", "TRAP", "DNB", "TECHNO" };
    return names[std::clamp(genre, 0, 3)];
}
const char* roleName(int role) noexcept
{
    static constexpr const char* names[] { "KICK", "SNARE", "HAT", "PERC", "TOM" };
    return names[std::clamp(role, 0, kRoles - 1)];
}

Analysis analyse(const juce::AudioBuffer<float>& stereo, double fs, double bpm, const std::vector<Mark>& marks)
{
    Analysis a;
    const int length = stereo.getNumSamples();
    if (length < 64 || stereo.getNumChannels() < 1 || !(fs >= 8000) || !(bpm > 0))
        return a;
    a.beat = fs * 60.0 / bpm;
    std::vector<float> mono(static_cast<size_t>(length));
    for (int n = 0; n < length; ++n)
    {
        double v = 0;
        for (int c = 0; c < std::min(2, stereo.getNumChannels()); ++c)
            v += stereo.getSample(c, n);
        v /= std::min(2, stereo.getNumChannels());
        mono[static_cast<size_t>(n)] = std::isfinite(v) ? static_cast<float>(v) : 0.0f;
    }
    // four bands: sub < 120, low-mid 120..600, mid 600..4000, high > 4000 Hz
    std::array<std::vector<float>, 4> band;
    band[0] = mono; filter4(band[0], fs, 120, false);
    band[1] = mono; filter4(band[1], fs, 120, true); filter4(band[1], fs, 600, false);
    band[2] = mono; filter4(band[2], fs, 600, true); filter4(band[2], fs, 4000, false);
    band[3] = mono; filter4(band[3], fs, 4000, true);
    const int hop = std::max(1, static_cast<int>(std::lround(fs / 1000.0)));
    const int frames = length / hop;
    if (frames < 40)
        return a;
    std::array<std::vector<double>, 4> energy;
    for (int b = 0; b < 4; ++b)
    {
        energy[static_cast<size_t>(b)].assign(static_cast<size_t>(frames), 0.0);
        for (int f = 0; f < frames; ++f)
        {
            double e = 0;
            for (int n = f * hop; n < (f + 1) * hop; ++n)
                e += static_cast<double>(band[static_cast<size_t>(b)][static_cast<size_t>(n)]) * band[static_cast<size_t>(b)][static_cast<size_t>(n)];
            energy[static_cast<size_t>(b)][static_cast<size_t>(f)] = e / hop;
        }
    }
    // band-normalised positive amplitude flux: every band can trigger an onset, hats as well as kicks
    std::vector<double> flux(static_cast<size_t>(frames), 0.0);
    for (int b = 0; b < 4; ++b)
    {
        const auto& e = energy[static_cast<size_t>(b)];
        double top = 0;
        for (double v : e)
            top = std::max(top, std::sqrt(v));
        if (top < 1e-9)
            continue;
        for (int f = 2; f < frames; ++f)
            flux[static_cast<size_t>(f)] += std::max(0.0, std::sqrt(e[static_cast<size_t>(f)]) - std::sqrt(e[static_cast<size_t>(f - 2)])) / top;
    }
    double fluxTop = 0;
    for (double v : flux)
        fluxTop = std::max(fluxTop, v);
    if (fluxTop < 1e-9)
        return a;
    // adaptive threshold: local median (every 25 frames over +/-200 frames) + a fraction of the maximum
    std::vector<double> threshold(static_cast<size_t>(frames));
    for (int f0 = 0; f0 < frames; f0 += 25)
    {
        std::vector<double> local;
        for (int f = std::max(0, f0 - 200); f < std::min(frames, f0 + 200); ++f)
            local.push_back(flux[static_cast<size_t>(f)]);
        const double t = median(local) + 0.07 * fluxTop;
        for (int f = f0; f < std::min(frames, f0 + 25); ++f)
            threshold[static_cast<size_t>(f)] = t;
    }
    std::vector<int> peaks;
    for (int f = 3; f < frames - 3; ++f)
    {
        const double v = flux[static_cast<size_t>(f)];
        if (v < threshold[static_cast<size_t>(f)])
            continue;
        bool top = true;
        for (int j = std::max(0, f - 8); j <= std::min(frames - 1, f + 8) && top; ++j)
            if (j != f && (flux[static_cast<size_t>(j)] > v || (flux[static_cast<size_t>(j)] == v && j < f)))
                top = false;
        if (!top)
            continue;
        if (!peaks.empty() && f - peaks.back() < 30)
        {
            if (v > flux[static_cast<size_t>(peaks.back())])
                peaks.back() = f;
            continue;
        }
        peaks.push_back(f);
    }
    const auto mean = [&](int b, int from, int to)
    {
        from = std::clamp(from, 0, frames);
        to = std::clamp(to, 0, frames);
        if (to <= from)
            return 0.0;
        double s = 0;
        for (int f = from; f < to; ++f)
            s += energy[static_cast<size_t>(b)][static_cast<size_t>(f)];
        return s / (to - from);
    };
    constexpr int window[4] { 35, 25, 20, 12 };
    std::vector<double> masking;
    double previousTotal = 0;
    int previousAt = -1000, previousDominant = -1;
    for (int f : peaks)
    {
        Onset o;
        const int at = f - 2; // the flux compares f with f-2: the attack began near f-2
        std::array<double, 4> before {};
        double total = 0, pre = 0;
        for (int b = 0; b < 4; ++b)
        {
            before[static_cast<size_t>(b)] = mean(b, at - 22, at - 2);
            o.novelty[static_cast<size_t>(b)] = std::max(0.0, mean(b, at, at + window[b]) - before[static_cast<size_t>(b)]);
            total += o.novelty[static_cast<size_t>(b)];
            pre += before[static_cast<size_t>(b)];
        }
        // a real hit rises clearly over the tail it lands in; a ringing band inside a kick's tail does not
        // (DrumPhraseTests: a kick's 120..600 Hz ripple 52 ms after the attack was read as a confident snare)
        if (total <= 1e-14 || total < 0.12 * (total + pre))
            continue;
        std::array<double, 4> fr {};
        for (int b = 0; b < 4; ++b)
            fr[static_cast<size_t>(b)] = o.novelty[static_cast<size_t>(b)] / total;
        // a weak burst of the same band within 60 ms (clap flutter, flam texture) belongs to the previous hit
        // (first lab run counted them as hits: precision 0.74 and truncated slices)
        const int dominantBand = static_cast<int>(std::max_element(fr.begin(), fr.end()) - fr.begin());
        if (at - previousAt < 60 && dominantBand == previousDominant && total < 0.3 * previousTotal)
            continue;
        previousAt = at;
        previousTotal = total;
        previousDominant = dominantBand;
        // decay: frames until the mid+high energy falls 20 dB under its peak (hats are short, snares / claps long)
        double decay = 0;
        {
            double top = 0;
            int peakFrame = at;
            for (int g = at; g < std::min(frames, at + 12); ++g)
            {
                const double e = energy[2][static_cast<size_t>(g)] + energy[3][static_cast<size_t>(g)];
                if (e > top) { top = e; peakFrame = g; }
            }
            int g = peakFrame;
            while (g < std::min(frames, at + 300) && energy[2][static_cast<size_t>(g)] + energy[3][static_cast<size_t>(g)] > 0.01 * top)
                ++g;
            decay = g - at; // ms; a following onset can only make it look longer, never shorter
        }
        o.decayMs = decay;
        // role posterior: softmax over band-fraction evidence plus body (120..600 Hz present at all) and decay.
        // First run (DrumPhraseLab, 2026-11-19) used band fractions only: 77 % role accuracy, a mid-heavy hat read
        // as snare. Body and decay terms were added after that run; see docs/DRUM_FILL_R15.md.
        const double body = std::clamp(std::log10(fr[1] + 1e-5) + 3.0, -1.0, 1.0); // second run: 1e-4 / 1.5 still read a bodiless hat as snare
        const double ring = std::clamp((decay - 90.0) / 60.0, -1.0, 1.0);
        const double score[kAutoRoles] { 2.4 * fr[0] + 0.3 * fr[1] - 1.0 * fr[2] - 1.6 * fr[3],
                                     1.5 * fr[2] + 1.1 * fr[1] + 0.5 * fr[3] - 1.3 * fr[0] + 0.6 * body + 0.5 * ring,
                                     2.6 * fr[3] - 0.6 * fr[2] - 1.4 * fr[0] - 1.2 * fr[1] - 1.2 * body - 0.5 * ring };
        double sum = 0;
        for (int r = 0; r < kAutoRoles; ++r)
            sum += (o.p[static_cast<size_t>(r)] = std::exp(6.0 * score[r]));
        for (auto& v : o.p)
            v /= sum;
        o.role = static_cast<int>(std::max_element(o.p.begin(), o.p.end()) - o.p.begin());
        o.conf = o.p[static_cast<size_t>(o.role)];
        // the attack on the full-band waveform: first sample rising a quarter of the way from the tail level before
        // the hit to the hit's local peak. (First version searched the dominant filtered band: its group delay put
        // attacks 6..12 ms late, so slices started inside the hit and lost the transient. Found by DrumPhraseTests.)
        {
            const int ms = std::max(1, hop);
            const int search = std::max(0, (at - 10) * ms), tailFrom = std::max(0, (at - 20) * ms);
            float pre = 0, local = 0;
            for (int n = tailFrom; n < search; ++n)
                pre = std::max(pre, std::abs(mono[static_cast<size_t>(n)]));
            for (int n = search; n < std::min(length, (at + 12) * ms); ++n)
                local = std::max(local, std::abs(mono[static_cast<size_t>(n)]));
            const float threshold = pre + 0.25f * std::max(0.0f, local - pre);
            o.attack = std::min(length - 1, at * ms);
            for (int n = search; n < std::min(length, (at + 8) * ms); ++n)
                if (std::abs(mono[static_cast<size_t>(n)]) > threshold)
                {
                    o.attack = n;
                    break;
                }
        }
        o.start = std::max(0, o.attack - static_cast<int>(0.0015 * fs));
        masking.push_back(total / (total + pre));
        // own-band masking: sustained energy in the role's own bands before the attack (bass, pads, tails)
        static constexpr int ownBands[kRoles][2] { { 0, 1 }, { 1, 2 }, { 3, 3 }, { 2, 3 }, { 1, 2 } };
        double own = 0, ownPre = 0;
        for (int b : { ownBands[o.role][0], ownBands[o.role][1] })
        {
            own += o.novelty[static_cast<size_t>(b)];
            ownPre += before[static_cast<size_t>(b)];
            if (ownBands[o.role][0] == ownBands[o.role][1])
                break;
        }
        o.isolation = own / (own + 0.5 * ownPre + 1e-30);
        a.onsets.push_back(o);
    }
    if (a.onsets.empty())
        return a;
    a.drumness = median(masking);
    // free time to the next onset (any role): a full-band slice must not carry the next hit
    for (size_t i = 0; i < a.onsets.size(); ++i)
        a.onsets[i].free = (i + 1 < a.onsets.size() ? a.onsets[i + 1].start : length) - a.onsets[i].start;
    // contamination: another role's band also struck. Clean only that band, keep the rest full band.
    std::array<std::vector<double>, 4> roleBand;
    for (const auto& o : a.onsets)
        if (o.conf > 0.6)
        {
            if (o.role == Kick) roleBand[0].push_back(o.novelty[0]);
            if (o.role == Snare) roleBand[2].push_back(o.novelty[2]);
            if (o.role == Hat) roleBand[3].push_back(o.novelty[3]);
        }
    const double kickSub = median(roleBand[0]), snareMid = median(roleBand[2]), hatHigh = median(roleBand[3]);
    for (auto& o : a.onsets)
    {
        bool dirty = false;
        if (o.role == Kick && hatHigh > 0 && o.novelty[3] > 0.35 * hatHigh) { o.clean = LowPass; dirty = true; }
        if (o.role == Snare && kickSub > 0 && o.novelty[0] > 0.25 * kickSub) { o.clean = HighPassLow; dirty = true; }
        if (o.role == Hat && ((kickSub > 0 && o.novelty[0] > 0.08 * kickSub) || (snareMid > 0 && o.novelty[2] > 0.25 * snareMid))) { o.clean = HighPassHigh; dirty = true; }
        if (dirty)
            o.isolation *= 0.8;
        // drums decay: an attack that never falls 20 dB in 300 ms with nothing after it is a sustained tone
        if (o.decayMs >= 299 && o.free > 0.3 * fs)
            o.isolation *= 0.15;
        o.isolation *= std::sqrt(std::min(1.0, o.free / (needed[o.role] * fs)));
        o.isolation = std::clamp(o.isolation, 0.0, 1.0);
    }
    // secondary roles: a clap on the kick (Techno, Trap) or a hat on the kick is still a usable snare / hat source
    // once the kick band is filtered away. They get a lower isolation and never act as anchors or carried hits.
    {
        const size_t primaries = a.onsets.size();
        for (size_t i = 0; i < primaries; ++i)
        {
            const auto o = a.onsets[i];
            if (o.role != Kick)
                continue;
            const double total = o.novelty[0] + o.novelty[1] + o.novelty[2] + o.novelty[3];
            const double midShare = o.novelty[2] / total, highShare = o.novelty[3] / total;
            const double snareRef = snareMid > 0 ? o.novelty[2] / (0.6 * snareMid) : midShare / 0.12;
            const double hatRef = hatHigh > 0 ? o.novelty[3] / (0.6 * hatHigh) : highShare / 0.04;
            for (int role : { Snare, Hat })
            {
                const double strength = role == Snare ? snareRef : hatRef;
                // a snare inside a kick needs its mid to rival the highs: noise of a hat on the kick is not a snare
                if (strength < 0.5 || (role == Snare ? midShare < 0.06 || o.novelty[2] < 0.6 * o.novelty[3] : highShare < 0.01))
                    continue;
                Onset v = o;
                v.role = role;
                v.secondary = true;
                v.clean = role == Snare ? HighPassLow : HighPassHigh;
                v.conf = std::min(0.9, strength);
                v.isolation = 0.6 * std::sqrt(std::min(1.0, o.free / (needed[role] * fs)));
                a.onsets.push_back(v);
            }
        }
        std::stable_sort(a.onsets.begin(), a.onsets.end(), [](const Onset& p1, const Onset& p2) { return p1.start < p2.start; });
    }
    // the user's marks win: the strongest attack inside a marked region takes the mark's role, full confidence and
    // the region's end as its slice end; a region without a detected attack gets one at its first strong sample
    for (const auto& m : marks)
    {
        const int from = std::clamp(std::min(m.start, m.end), 0, length - 1), to = std::clamp(std::max(m.start, m.end), from + 1, length);
        if (to - from < static_cast<int>(0.004 * fs) || m.role < 0 || m.role >= kRoles)
            continue;
        const int slack = static_cast<int>(0.005 * fs);
        a.onsets.erase(std::remove_if(a.onsets.begin(), a.onsets.end(), [&](const Onset& o) { return o.secondary && o.attack >= from - slack && o.attack < to; }), a.onsets.end());
        int best = -1;
        double strongest = -1;
        for (int i = 0; i < static_cast<int>(a.onsets.size()); ++i)
        {
            const auto& o = a.onsets[static_cast<size_t>(i)];
            const double total = o.novelty[0] + o.novelty[1] + o.novelty[2] + o.novelty[3];
            if (!o.marked && o.attack >= from - slack && o.attack < to && total > strongest)
            {
                strongest = total;
                best = i;
            }
        }
        if (best < 0)
        {
            Onset o;
            float top = 0;
            for (int n = from; n < to; ++n)
                top = std::max(top, std::abs(mono[static_cast<size_t>(n)]));
            o.attack = from;
            for (int n = from; n < to; ++n)
                if (std::abs(mono[static_cast<size_t>(n)]) > 0.25f * top)
                {
                    o.attack = n;
                    break;
                }
            o.start = std::max(0, o.attack - static_cast<int>(0.0015 * fs));
            a.onsets.push_back(o);
            best = static_cast<int>(a.onsets.size()) - 1;
        }
        auto& o = a.onsets[static_cast<size_t>(best)];
        o.role = m.role;
        o.p = {};
        o.p[static_cast<size_t>(m.role)] = 1;
        o.conf = 1;
        o.isolation = 1;
        o.clean = Full;
        o.marked = true;
        o.secondary = false;
        o.start = std::max(0, std::min(o.start, to - 1));
        o.free = std::max(16, to - o.start);
    }
    std::stable_sort(a.onsets.begin(), a.onsets.end(), [](const Onset& p1, const Onset& p2) { return p1.start < p2.start; });
    // peaks on the slice that would be played (after cleaning)
    const juce::AudioBuffer<float> clean[4] { stereo, filtered(stereo, fs, 3500, false), filtered(stereo, fs, 200, true), filtered(stereo, fs, 5000, true) };
    for (auto& o : a.onsets)
    {
        const int n = std::min({ o.free, static_cast<int>(nominal[o.role] * fs), length - o.start });
        const auto& src = clean[o.clean];
        double peak = 0;
        for (int c = 0; c < src.getNumChannels(); ++c)
            peak = std::max(peak, static_cast<double>(src.getMagnitude(c, o.start, std::max(1, n))));
        o.peak = peak;
    }
    // grid phase from onsets near 8th notes (+/-12 ms), then swing and per-role microtiming
    {
        double best = std::numeric_limits<double>::max();
        for (double d = -0.012 * fs; d <= 0.012 * fs; d += 0.00025 * fs)
        {
            double cost = 0;
            for (const auto& o : a.onsets)
            {
                if (o.conf < 0.5 || o.secondary)
                    continue;
                const double eighth = (o.attack - d) / a.beat * 2.0, dist = eighth - std::round(eighth);
                cost += std::min(dist * dist, 0.0625) * std::sqrt(o.peak + 1e-9);
            }
            if (cost < best - 1e-12)
            {
                best = cost;
                a.phase = d;
            }
        }
    }
    std::vector<double> odd;
    for (auto& o : a.onsets)
    {
        o.tick = (o.attack - a.phase) / a.beat * ticksPerBeat;
        const double s16 = std::round(o.tick / 6.0), off = o.tick - 6.0 * s16;
        if (o.conf > 0.5 && !o.secondary && static_cast<long long>(s16) % 2 != 0 && std::abs(off) < 3.0)
            odd.push_back(off);
    }
    // swing only from a consistent set of late odd 16ths (>= 4, spread < 1 tick); a wrong swing would put every
    // generated off-beat late (first lab run: 1.6 ticks on a straight DnB loop)
    a.swing = 0.0;
    if (odd.size() >= 4)
    {
        const double m = median(odd);
        std::vector<double> spread;
        for (double v : odd)
            spread.push_back(std::abs(v - m));
        if (median(spread) < 1.0)
            a.swing = std::clamp(m, 0.0, 2.5);
    }
    int onGrid = 0, strong = 0;
    std::array<std::vector<double>, kRoles> offsets;
    for (const auto& o : a.onsets)
    {
        if (o.conf < 0.5 || o.secondary)
            continue;
        const double s16 = std::round((o.tick - a.swing * 0.5) / 6.0);
        const double grid = 6.0 * s16 + (static_cast<long long>(s16) % 2 != 0 ? a.swing : 0.0), off = o.tick - grid;
        ++strong;
        if (std::abs(off) <= 1.5)
            ++onGrid;
        if (std::abs(off) < 2.5)
            offsets[static_cast<size_t>(o.role)].push_back(off);
    }
    a.timing = strong > 0 ? static_cast<double>(onGrid) / strong : 0.0;
    for (int r = 0; r < kRoles; ++r)
        a.offset[static_cast<size_t>(r)] = offsets[static_cast<size_t>(r)].size() >= 2 ? std::clamp(median(offsets[static_cast<size_t>(r)]), -2.0, 2.0) : 0.0;
    std::array<std::vector<double>, kRoles> rolePeaks, secondaryPeaks;
    for (const auto& o : a.onsets)
    {
        if (quality(o) >= kUsable)
            ++a.usable[static_cast<size_t>(o.role)];
        if (o.conf >= 0.5)
            (o.secondary ? secondaryPeaks : rolePeaks)[static_cast<size_t>(o.role)].push_back(o.peak);
    }
    for (int r = 0; r < kRoles; ++r)
        a.typicalPeak[static_cast<size_t>(r)] = percentile(rolePeaks[static_cast<size_t>(r)].empty() ? secondaryPeaks[static_cast<size_t>(r)] : rolePeaks[static_cast<size_t>(r)], 0.75);
    // genre guess (a prior only): quarter-kick regularity, backbeat or half-time snare, hat density, tempo
    {
        const int beats = std::max(1, static_cast<int>(std::floor(length / a.beat + 0.01)));
        std::vector<std::array<bool, kRoles>> onBeat(static_cast<size_t>(beats));
        int hats = 0, fastHats = 0;
        double lastHat = -100;
        for (const auto& o : a.onsets)
        {
            if (o.conf < 0.45)
                continue;
            if (o.role == Hat)
            {
                ++hats;
                if (o.tick - lastHat < 3.6)
                    ++fastHats;
                lastHat = o.tick;
            }
            const double q = o.tick / ticksPerBeat;
            const int b = static_cast<int>(std::lround(q));
            if (std::abs(q - b) * ticksPerBeat <= 2.5 && b >= 0 && b < beats)
                onBeat[static_cast<size_t>(b)][static_cast<size_t>(o.role)] = true;
        }
        int kickBeats = 0, back = 0, backN = 0, three = 0, threeN = 0;
        for (int b = 0; b < beats; ++b)
        {
            kickBeats += onBeat[static_cast<size_t>(b)][Kick];
            if (b % 4 == 1 || b % 4 == 3) { back += onBeat[static_cast<size_t>(b)][Snare]; ++backN; }
            if (b % 4 == 2) { three += onBeat[static_cast<size_t>(b)][Snare]; ++threeN; }
        }
        const double kickQ = static_cast<double>(kickBeats) / beats, backQ = backN ? static_cast<double>(back) / backN : 0,
                     threeQ = threeN ? static_cast<double>(three) / threeN : 0, hatRate = static_cast<double>(hats) / beats;
        if (kickQ >= 0.85 && bpm >= 110 && bpm <= 155) { a.genre = Techno; a.genreConfidence = 0.5 + 0.5 * kickQ - 0.3 * threeQ; }
        else if (bpm >= 158 && backQ >= threeQ) { a.genre = DnB; a.genreConfidence = 0.5 + 0.4 * backQ; }
        else if ((threeQ > backQ && bpm >= 118) || fastHats >= 2 || hatRate >= 3.2) { a.genre = Trap; a.genreConfidence = 0.45 + 0.25 * (threeQ > backQ) + 0.2 * (fastHats >= 2); }
        else { a.genre = BoomBap; a.genreConfidence = 0.4 + 0.4 * backQ; }
        a.genreConfidence = std::clamp(a.genreConfidence, 0.0, 1.0);
    }
    return a;
}
} // namespace phrase

namespace
{
using namespace phrase;
constexpr double pi = 3.14159265358979323846;
std::uint64_t mix(std::uint64_t z) noexcept
{
    z += 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}
double rnd(std::uint64_t seed, std::uint64_t salt) noexcept { return static_cast<double>(mix(seed ^ mix(salt)) >> 11) / 9007199254740992.0; }
double fadeCurve(double x) noexcept { return 0.5 - 0.5 * std::cos(pi * std::clamp(x, 0.0, 1.0)); }

// One-beat cells. Strings per role, one char per subdivision (4 = 16ths, 6 = 16th triplets):
// '.' rest, 'g' ghost, 'x' normal, 'X' accent, 'f' flam (grace + accent), 'r' two 32nds, 'R' three 32nd
// triplets, 'q' four 64ths (only with a clean hat source and ROLL high).
struct Cell { int energy; const char* k; const char* s; const char* h; };
// R15c (acoustic reference, docs/reports/r15/ACOUSTIC_REFERENCE.md): more 16th / linear figures, RUN cells (even
// strokes carried over the bar line into the cadence), cadences that rest on the last 16th and land on the next
// downbeat, syncopation targets raised from the first guesses (BB 0.35, Trap 0.45, DnB 0.45, Techno 0.30).
struct Library { std::vector<Cell> figure, cadence, run, runCadence; bool carryHats; double syncopation, density; };
const Library& library(int genre)
{
    static const Library libs[kGenres] {
        { // BOOM BAP: backbeat, ghosts, double-ups, flams; drummers drop the hats in the fill
          { { 0, "X..x", "....", "...." }, { 0, "X...", "..g.", "...." }, { 0, "....", "X...", "...." },
            { 1, "X.x.", "...g", "...." }, { 1, "..x.", "X..g", "...." }, { 1, "X...", "..X.", "...." }, { 1, "x...", "g.X.", "...." },
            { 2, "....", "X.gX", "...." }, { 2, "X..x", "..X.", "...." }, { 2, "....", "gXgX", "...." }, { 2, "x...", ".XX.", "...." },
            { 3, "....", "XXgX", "...." }, { 3, "....", "rXrX", "...." }, { 3, "x.x.", ".X.X", "...." }, { 3, "....", "fXgX", "...." },
            { 1, "x..x", ".g..", "...." }, { 2, "x.x.", ".x.X", "...." }, { 2, "...x", "gX..", "...." }, { 3, "x..x", ".XX.", "...." } },
          { { 0, "....", "X.XX", "...." }, { 0, "X...", "..fX", "...." }, { 0, "x...", "..XX", "...." }, { 0, "....", "XgXX", "...." },
            { 0, "......", "XxXxXX", "......" }, { 0, "X...", ".X.X", "...." },
            { 0, "....", "XxX.", "...." }, { 0, "x...", ".XX.", "...." }, { 0, "....", "rXX.", "...." }, { 0, "x..x", ".X..", "...." },
            { 0, "....", "gXf.", "...." } },
          { { 2, "....", "xxxx", "...." }, { 2, "x.x.", ".x.x", "...." }, { 3, "....", "gxgX", "...." }, { 3, "x..x", ".xx.", "...." } },
          { { 0, "....", "xxX.", "...." }, { 0, "x...", ".xX.", "...." }, { 0, "x.x.", ".x..", "...." }, { 0, "....", "xxXX", "...." } },
          false, 0.55, 2.2 },
        { // TRAP: hats carry and accelerate, 32nd/triplet rolls, sparse kick answers, half-time snare
          { { 0, "X...", "....", "x.x." }, { 0, "..x.", "....", "xxxx" }, { 0, "X...", "....", "xxxx" },
            { 1, "X...", "....", "x.rr" }, { 1, "..X.", "....", "x.x." }, { 1, "X.....", "......", "xxxxxx" }, { 1, "....", "X...", "x.xx" },
            { 2, "....", "X...", "rrrr" }, { 2, "X..x", "....", "xRxR" }, { 2, "....", "....", "RRRR" }, { 2, "......", "......", "xxxxxx" },
            { 3, "....", "..XX", "rrrr" }, { 3, "....", "X...", "qqqq" }, { 3, "X...", "....", "RRqq" }, { 3, "....", "X.XX", "rr.." } },
          { { 0, "....", "X...", "rrRq" }, { 0, "....", "..XX", "RRRR" }, { 0, "x...", "X...", "qqqq" }, { 0, "....", "X.X.", "rrrr" },
            { 0, "......", "X..X..", "xxxxxx" }, { 0, "X...", "...X", "rRqq" },
            { 0, "....", "X...", "rrr." }, { 0, "....", "..X.", "RRR." } },
          { { 2, "...x", "..X.", "xxxx" }, { 2, "....", "....", "rrrr" }, { 3, "x...", "..X.", "RRRR" } },
          { { 0, "....", "X...", "rrr." }, { 0, "....", "..X.", "xxx." }, { 0, "x...", "X...", "RRqq" } },
          true, 0.50, 3.4 },
        { // DNB: break exchange, offset snares, short bursts on real hits
          { { 0, "X...", "....", "..x." }, { 0, "....", "X...", "..x." }, { 0, "X.x.", "....", "...." },
            { 1, "X.x.", "....", "..x." }, { 1, "..X.", "X...", "...." }, { 1, "X...", "..gX", "...." }, { 1, "..x.", "X..g", "...." },
            { 2, "....", "X.gX", "...." }, { 2, "..x.", "X..X", "...." }, { 2, "X..x", "..X.", "...." }, { 2, "....", "gXXg", "...." },
            { 3, "....", "XXgX", "...." }, { 3, "x...", ".XXX", "...." }, { 3, "....", "rrXX", "...." }, { 3, "X...", "rXrX", "...." } },
          { { 0, "....", "XXXX", "...." }, { 0, "X...", "..XX", "...." }, { 0, "....", "XgXX", "...." }, { 0, "....", "rrrX", "...." },
            { 0, "......", "XXxXXX", "......" },
            { 0, "....", "XXX.", "...." }, { 0, "x...", ".xX.", "...." }, { 0, "....", "rrX.", "...." }, { 0, "..x.", "X.X.", "...." } },
          { { 2, "....", "xxxx", "...." }, { 2, "x.x.", ".x.x", "...." }, { 3, "x..x", ".xx.", "...." }, { 3, "....", "gxxX", "...." } },
          { { 0, "....", "xxX.", "...." }, { 0, "x...", ".xX.", "...." }, { 0, "....", "xxXX", "...." } },
          false, 0.55, 2.8 },
        { // TECHNO: every quarter kick stays (hard rule, unless STOP drops it), snare/hat tension builds
          { { 0, "X...", "....", "..x." }, { 0, "X...", "..x.", "..x." },
            { 1, "X...", "....", ".xxx" }, { 1, "X...", "..x.", "x.x." }, { 1, "X...", "x.x.", "...." },
            { 2, "X...", "xxxx", "...." }, { 2, "X...", "x.xx", ".x.x" }, { 2, "X...", ".x.x", "x.x." },
            { 3, "X...", "rrrr", "...." }, { 3, "X...", "xxxx", "xxxx" }, { 3, "X...", "rrxx", "...." } },
          { { 0, "X...", "rrrr", "...." }, { 0, "X...", "xxrr", "...." }, { 0, "X...", "XXXX", "...." }, { 0, "X...", "xXxX", "x.x." },
            { 0, "X...", "xxX.", "...." }, { 0, "X...", ".xX.", "x..." } },
          { { 2, "X...", ".xxx", "...." }, { 3, "X...", "xxxx", "...." } },
          { { 0, "X...", "xxX.", "...." }, { 0, "X...", "xxxX", "...." } },
          true, 0.40, 3.0 } };
    return libs[std::clamp(genre, 0, kGenres - 1)];
}

struct Ev
{
    double tick = 0; // 24ths from the window start, straight grid (swing and microtiming added on scheduling)
    int role = Kick, source = -1; // onset index
    double velocity = 1;
    bool anchor = false, carried = false, reverse = false, roll = false, grace = false, run = false;
    int sample = 0; // anchors and carried hits: their exact original start
};
struct Candidate
{
    std::vector<Ev> events;
    int turn = 0, genre = BoomBap;
    std::array<int, 16> cells {};
    bool perc = false; // perc colour: user-marked perc answers ghost snares and some off-beat hats
    bool tom = false;  // tom colour: user-marked toms take the snare strokes of the second half and the runs
    bool drop = false, reverse = false, triplet = false, run = false;
    double score = -1e30, change = 0;
    int anchors = 0;
};
struct Scheduled { int at = 0, length = 0, source = 0, sourceEnd = 0, role = 0, clean = 0; double gain = 1; bool reverse = false, roll = false, anchor = false; };
struct Context
{
    const Analysis* a = nullptr;
    Settings s;
    int length = 0, genre = BoomBap, status = Generated, beats = 4;
    bool whole = false;
    double windowStart = 0, windowTicks = 0, gridOrigin = 0; // samples / ticks / samples
    std::array<std::vector<int>, kRoles> pool; // usable onsets per role, best first
    std::vector<int> toms;                     // marked toms, highest pitch first (a fill descends the kit)
    std::vector<int> original;                 // onsets inside the window
    double chop = 0.6, roll = 0.7, reverse = 0.4, stop = 0.3;
};

int pick(const Context& x, int role, char kind, std::uint64_t seed, int stroke)
{
    const auto& pool = x.pool[static_cast<size_t>(role)];
    if (pool.empty())
        return -1;
    const auto& on = x.a->onsets;
    const double typical = x.a->typicalPeak[static_cast<size_t>(role)];
    if (kind == 'r' || kind == 'R' || kind == 'q') // alternate hands: two best sources
        return pool[static_cast<size_t>(stroke % std::min<int>(2, static_cast<int>(pool.size())))];
    if (kind == 'g') // a real quieter hit sounds like a ghost note; louder ones are played softer otherwise
    {
        for (int i : pool)
            if (on[static_cast<size_t>(i)].peak <= 0.62 * typical && on[static_cast<size_t>(i)].peak >= 0.15 * typical)
                return i;
        return pool.front();
    }
    std::vector<int> loud;
    for (int i : pool)
        if (kind != 'X' || on[static_cast<size_t>(i)].peak >= 0.7 * typical)
            loud.push_back(i);
    if (loud.empty())
        loud = pool;
    const int top = std::min<int>(kind == 'X' ? 2 : 3, static_cast<int>(loud.size()));
    return loud[static_cast<size_t>(mix(seed + static_cast<std::uint64_t>(stroke) * 31) % static_cast<std::uint64_t>(top))];
}

void addCell(const Context& x, Candidate& c, const Cell& cell, double beatTick, double progress, std::uint64_t seed, bool rollsAllowed, bool upgradeRolls)
{
    const char* lanes[kAutoRoles] { cell.k, cell.s, cell.h };
    for (int lane = 0; lane < kAutoRoles; ++lane)
    {
        const int role = lane;
        const int div = static_cast<int>(std::strlen(lanes[lane]));
        if (div == 6)
            c.triplet = true;
        for (int i = 0; i < div; ++i)
        {
            char k = lanes[lane][i];
            if (k == '.')
                continue;
            if (!rollsAllowed && (k == 'r' || k == 'R' || k == 'q'))
                k = 'x';
            if (upgradeRolls && k == 'r')
                k = 'R';
            if (k == 'q' && (role != Hat || x.roll < 0.6))
                k = 'R';
            const double t0 = beatTick + i * ticksPerBeat / div, step = ticksPerBeat / div;
            const double base = (k == 'X' ? 1.0 : k == 'f' ? 1.0 : k == 'g' ? 0.42 : 0.8) * (0.86 + 0.14 * progress);
            const auto push = [&](double t, double v, char kind, bool roll, bool grace, int stroke)
            {
                // perc only from the user's marks: it plays the snare lane when there is no snare, answers ghost
                // snares and some off-beat hats when this candidate uses perc colour
                int use = role;
                const bool offBeat = std::abs(std::fmod(t, 12.0) - 6.0) < 0.01;
                if (!x.pool[Perc].empty() && ((role == Snare && x.pool[Snare].empty()) || (c.perc && role == Snare && kind == 'g')
                                              || (c.perc && role == Hat && !roll && offBeat && rnd(seed, static_cast<std::uint64_t>(t * 8) + 555) < 0.5)))
                    use = Perc;
                // toms only from the user's marks: in a tom candidate the normal / accented snare strokes of the
                // later figure (each bar's second half in WHOLE LOOP) move to the toms, high to low through it;
                // ghosts and rolls stay on the snare. Without snare and perc the toms play the snare lane.
                const double figureStart = c.turn * ticksPerBeat, span = std::max(1.0, x.windowTicks - figureStart);
                const double where = x.whole ? std::fmod(std::max(0.0, t - figureStart), 4 * ticksPerBeat) / (4 * ticksPerBeat) : std::clamp((t - figureStart) / span, 0.0, 0.999);
                const double zone = x.whole || x.windowTicks - figureStart >= 3 * ticksPerBeat ? 0.4 : 0.0;
                if (!x.toms.empty() && role == Snare && use == Snare
                    && ((c.tom && !roll && kind != 'g' && where >= zone) || (x.pool[Snare].empty() && x.pool[Perc].empty())))
                    use = Tom;
                int src = -1;
                if (use == Tom)
                {
                    const double local = zone > 0 && where >= zone ? (where - zone) / (1 - zone) : where;
                    src = x.toms[static_cast<size_t>(std::min<int>(static_cast<int>(local * static_cast<double>(x.toms.size())), static_cast<int>(x.toms.size()) - 1))];
                }
                else
                    src = pick(x, use, kind, seed + static_cast<std::uint64_t>(use) * 101 + static_cast<std::uint64_t>(t * 8), stroke);
                if (src < 0)
                    return;
                Ev e;
                e.tick = t; e.role = use; e.source = src; e.velocity = std::clamp(v, 0.1, 1.2); e.roll = roll; e.grace = grace;
                c.events.push_back(e);
            };
            if (k == 'r' || k == 'R' || k == 'q')
            {
                const int strokes = k == 'r' ? 2 : k == 'R' ? 3 : 4;
                for (int j = 0; j < strokes; ++j) // a ramp inside the roll, alternate hands slightly
                    push(t0 + j * step / strokes, (0.5 + 0.4 * (j + 1) / strokes) * (0.86 + 0.14 * progress) * (j % 2 ? 0.92 : 1.0), k, true, false, j);
            }
            else if (k == 'f')
            {
                push(t0 - 0.9, 0.42, 'g', false, true, 7); // grace ~25 ms ahead at 90 BPM (scaled by tempo)
                push(t0, base, 'X', false, false, 0);
            }
            else
                push(t0, base, k, false, false, 0);
        }
    }
}

double gaussCos(const std::vector<Ev>& a, const std::vector<Ev>& b, double from, double to)
{
    constexpr int cells = 96;
    std::array<std::array<double, cells>, kRoles> ma {}, mb {};
    const auto fill = [&](std::array<std::array<double, cells>, kRoles>& m, const std::vector<Ev>& ev)
    {
        for (const auto& e : ev)
        {
            if (e.tick < from || e.tick >= to || e.grace)
                continue;
            const double pos = (e.tick - from) / (to - from) * cells, sigma = std::max(0.6, cells / ((to - from) / 1.5));
            for (int n = std::max(0, static_cast<int>(pos - 4 * sigma)); n < std::min(cells, static_cast<int>(pos + 4 * sigma) + 1); ++n)
            {
                const double d = (n - pos) / sigma;
                m[static_cast<size_t>(e.role)][static_cast<size_t>(n)] += e.velocity * std::exp(-0.5 * d * d);
            }
        }
    };
    fill(ma, a);
    fill(mb, b);
    double dot = 0, aa = 0, bb = 0;
    for (int r = 0; r < kRoles; ++r)
        for (int n = 0; n < cells; ++n)
        {
            dot += ma[static_cast<size_t>(r)][static_cast<size_t>(n)] * mb[static_cast<size_t>(r)][static_cast<size_t>(n)];
            aa += ma[static_cast<size_t>(r)][static_cast<size_t>(n)] * ma[static_cast<size_t>(r)][static_cast<size_t>(n)];
            bb += mb[static_cast<size_t>(r)][static_cast<size_t>(n)] * mb[static_cast<size_t>(r)][static_cast<size_t>(n)];
        }
    if (aa <= 0 && bb <= 0)
        return 1;
    return dot / std::sqrt(std::max(1e-30, aa * bb));
}

Candidate build(const Context& x, int index)
{
    const auto& a = *x.a;
    const auto& lib = library(x.genre);
    const std::uint64_t seed = mix(x.s.seed * 0x9e3779b97f4a7c15ull + static_cast<std::uint64_t>(index) * 0x632be59bd9b4e019ull + 0x50485241ull);
    Candidate c;
    c.genre = x.genre;
    const int beats = x.beats;
    c.perc = !x.pool[Perc].empty() && rnd(seed, 3) < 0.7;
    c.tom = !x.toms.empty() && x.status != Conservative && rnd(seed, 4) < 0.75;
    // turn point: how many beats of the original pulse stay before the figure (WHOLE LOOP: none)
    if (x.status == Conservative)
        c.turn = beats - 1;
    else if (x.whole)
        c.turn = 0;
    else if (beats == 4)
        c.turn = rnd(seed, 1) < 0.25 + 0.5 * x.chop ? 1 : 2;
    else if (beats == 2)
        c.turn = rnd(seed, 1) < 0.35 + 0.55 * x.chop ? 0 : 1;
    else
        c.turn = 0;
    const int figureBeats = beats - c.turn;
    const double turnTick = c.turn * ticksPerBeat;
    const bool rolls = x.roll >= 0.2, upgrade = x.roll >= 0.8 && rnd(seed, 2) < 0.6;
    // energy contour: R15c flattened (first: 0.45 -> 1.0 of base); acoustic fills are not louder or denser at the
    // end (beat 4 / beat 1 = 0.72). CHOP lifts the whole contour.
    const double base = 0.4 + 2.4 * x.chop;
    // RUN: an even stroke stream over the last two beats into a cadence that rests on the last 16th (acoustic
    // players run 5..7 even strokes; R15 first measured 3)
    c.run = x.status != Conservative && figureBeats >= 2 && x.chop >= 0.25 && rnd(seed, 5) < 0.2 + 0.4 * x.chop;
    const auto choose = [&](const std::vector<Cell>& from, std::uint64_t salt)
    {
        const int n = static_cast<int>(from.size());
        return std::min<int>(static_cast<int>(rnd(seed, salt) * n), n - 1);
    };
    for (int j = 0; j < figureBeats; ++j)
    {
        const double beatTick = turnTick + j * ticksPerBeat, progress = figureBeats > 1 ? static_cast<double>(j) / (figureBeats - 1) : 1.0;
        const Cell* cell = nullptr;
        const bool runBeat = c.run && j >= figureBeats - 2;
        if (runBeat && j == figureBeats - 1)
        {
            const int choice = choose(lib.runCadence, 50);
            c.cells[static_cast<size_t>(j)] = 300 + choice;
            cell = &lib.runCadence[static_cast<size_t>(choice)];
        }
        else if (runBeat)
        {
            const int choice = choose(lib.run, 51);
            c.cells[static_cast<size_t>(j)] = 200 + choice;
            cell = &lib.run[static_cast<size_t>(choice)];
        }
        else if (j == figureBeats - 1)
        {
            const int choice = choose(lib.cadence, 10 + static_cast<std::uint64_t>(j));
            c.cells[static_cast<size_t>(j)] = 100 + choice;
            cell = &lib.cadence[static_cast<size_t>(choice)];
        }
        else
        {
            int energy = static_cast<int>(std::lround(base * (0.7 + 0.3 * progress) + (rnd(seed, 20 + static_cast<std::uint64_t>(j)) - 0.5) * 0.9));
            if (x.whole && j % 4 == 3)
                ++energy; // WHOLE LOOP: every bar answers on its last beat, the final bar resolves
            energy = std::clamp(energy, 0, x.status == Conservative ? 1 : 3);
            std::vector<int> options;
            for (int i = 0; i < static_cast<int>(lib.figure.size()); ++i)
                if (lib.figure[static_cast<size_t>(i)].energy == energy)
                    options.push_back(i);
            const int choice = options[static_cast<size_t>(std::min<int>(static_cast<int>(rnd(seed, 30 + static_cast<std::uint64_t>(j)) * static_cast<double>(options.size())), static_cast<int>(options.size()) - 1))];
            c.cells[static_cast<size_t>(j)] = choice;
            cell = &lib.figure[static_cast<size_t>(choice)];
        }
        const auto before = c.events.size();
        addCell(x, c, *cell, beatTick, progress, seed + static_cast<std::uint64_t>(j) * 977, rolls, upgrade && j == figureBeats - 1 && !runBeat);
        if (runBeat)
            for (auto i = before; i < c.events.size(); ++i)
                c.events[i].run = true;
        // hats of the source carry under calm cells (and always in Trap / Techno) if the cell has none
        bool cellHats = false;
        for (auto i = before; i < c.events.size(); ++i)
            cellHats = cellHats || c.events[i].role == Hat;
        if (!cellHats && (lib.carryHats || cell->energy <= 1) && j < figureBeats - 1)
            for (int o : x.original)
            {
                const auto& on = a.onsets[static_cast<size_t>(o)];
                const double t = on.tick - x.windowStart / a.beat * ticksPerBeat;
                if (on.role == Hat && quality(on) >= kUsable * 0.7 && t >= beatTick - 1 && t < beatTick + ticksPerBeat - 1)
                {
                    Ev e; e.tick = t; e.role = Hat; e.source = o; e.velocity = 1; e.carried = true; e.sample = on.start;
                    c.events.push_back(e);
                }
            }
    }
    // Conservative (mixed music / unsure timing): no added kicks over a bass, only clean snare / hat hits
    if (x.status == Conservative)
        c.events.erase(std::remove_if(c.events.begin(), c.events.end(), [&](const Ev& e) { return e.role == Kick && !e.anchor; }), c.events.end());
    // drop pause before the downbeat (STOP) and reverse pickup into the bar line (REVERSE)
    const double end = x.windowTicks;
    double dropTicks = 0;
    // drop and pickup are the user's macros, decided per seed for all candidates (the score must not cherry-pick them)
    const std::uint64_t shared = mix(x.s.seed ^ 0x44524f50ull);
    if (x.stop > 0.05 && rnd(shared, 40) < 0.6 * x.stop)
    {
        c.drop = true;
        dropTicks = x.stop > 0.7 ? 12 : 6;
        c.events.erase(std::remove_if(c.events.begin(), c.events.end(), [&](const Ev& e) { return e.tick >= end - dropTicks; }), c.events.end());
    }
    if (x.reverse > 0.05 && rnd(shared, 41) < 0.6 * x.reverse && x.status != Conservative)
    {
        const int role = !x.pool[Snare].empty() ? Snare : Hat;
        const int src = pick(x, role, 'X', seed + 4242, 0);
        if (src >= 0)
        {
            Ev e; e.tick = end - 12; e.role = role; e.source = src; e.velocity = 0.85; e.reverse = true;
            c.events.erase(std::remove_if(c.events.begin(), c.events.end(), [&](const Ev& o) { return o.role == role && o.tick >= end - 12; }), c.events.end());
            c.events.push_back(e);
            c.reverse = true;
        }
    }
    // hard rule: strong original kick / snare anchors in the figure stay where and what they are
    for (int o : x.original)
    {
        const auto& on = a.onsets[static_cast<size_t>(o)];
        const double t = on.tick - x.windowStart / a.beat * ticksPerBeat;
        const double eighth = t / 12.0;
        const bool onEighth = std::abs(eighth - std::round(eighth)) * 12.0 <= 2.0;
        const bool strong = on.peak >= 0.45 * a.typicalPeak[static_cast<size_t>(on.role)] && on.conf >= 0.55;
        if (t < turnTick - 0.5 || t >= end || (on.role != Kick && on.role != Snare) || !strong || !onEighth)
            continue;
        if (c.drop && t >= end - dropTicks)
            continue; // the owner asked for the drop with STOP
        if (c.reverse && on.role == Snare && t >= end - 12)
            continue; // the pickup replaces it explicitly (REVERSE)
        c.events.erase(std::remove_if(c.events.begin(), c.events.end(), [&](const Ev& e) { return e.role == on.role && !e.anchor && std::abs(e.tick - t) < 3.0; }), c.events.end());
        Ev e; e.tick = t; e.role = on.role; e.source = o; e.velocity = 1; e.anchor = true; e.sample = on.start;
        c.events.push_back(e);
        ++c.anchors;
    }
    // Techno: the quarter kick is the genre; keep it on every beat unless the drop removes it
    if (x.genre == Techno && !x.pool[Kick].empty())
        for (int b = c.turn; b < beats; ++b)
        {
            const double t = b * ticksPerBeat;
            if (c.drop && t >= end - dropTicks)
                continue;
            bool has = false;
            for (const auto& e : c.events)
                has = has || (e.role == Kick && std::abs(e.tick - t) < 2.0);
            if (!has)
            {
                Ev e; e.tick = t; e.role = Kick; e.source = x.pool[Kick].front(); e.velocity = 1;
                c.events.push_back(e);
            }
        }
    // roles switched off on the page: the same composition, those parts muted
    c.events.erase(std::remove_if(c.events.begin(), c.events.end(), [&](const Ev& e) { return !x.s.kitEnabled[static_cast<size_t>(e.role)]; }), c.events.end());
    std::stable_sort(c.events.begin(), c.events.end(), [](const Ev& p, const Ev& q) { return p.tick < q.tick || (p.tick == q.tick && p.role < q.role); });

    // ---- score: Q = 1.2 T + 0.8 V + 0.6 D - 1.5 X - 2.0 U - 1.2 R - 0.6 M + 0.5 F + 0.4 N (heuristic baseline) ----
    double early = 0, late = 0, weight = 0, syncopated = 0, uncertain = 0, risk = 0, collisions = 0;
    const double mid = turnTick + 0.5 * (end - turnTick);
    // landing: a strong hit on the last 16th, or a strong hit in the last 8th and then a rest, are both landings
    // (corrected acoustic reference: last 16th empty in 41 % acoustic, 12 % live boom bap, 8 % snare rolls).
    // R15c first run rewarded only the rest (+0.35, the hit +0.15) from a measurement that counted a slightly early
    // last-16th hit as a rest; R15 first: only the strong last-16th hit counted.
    bool landLast16 = false, strongLast8 = false, anyLast16 = false;
    const double finish = end - dropTicks;
    int counted = 0;
    for (size_t i = 0; i < c.events.size(); ++i)
    {
        const auto& e = c.events[i];
        const double v2 = e.velocity * e.velocity;
        (e.tick < mid ? early : late) += v2;
        if (e.tick >= finish - 6 + 0.5)
            anyLast16 = true;
        if (e.velocity >= 0.9 && !e.carried && e.tick >= finish - 7)
            landLast16 = true;
        else if (e.velocity >= 0.9 && !e.carried && e.tick >= finish - 13)
            strongLast8 = true;
        const double pos = std::fmod(e.tick, ticksPerBeat) / 6.0, near = std::round(pos);
        const int s16 = static_cast<int>(near) % 4;
        if (std::abs(pos - near) < 0.2)
            syncopated += e.velocity * (s16 == 1 || s16 == 3 ? 1.0 : s16 == 2 ? 0.5 : 0.0);
        else
            syncopated += e.velocity * 0.4; // triplet / roll positions: some off-grid motion
        weight += e.velocity;
        const auto& on = a.onsets[static_cast<size_t>(e.source)];
        uncertain += e.velocity * (1 - quality(on));
        // a tail that the source cannot provide (slice shorter than the space to the next same-role hit)
        double space = end - e.tick;
        for (size_t j = i + 1; j < c.events.size(); ++j)
            if (c.events[j].role == e.role) { space = c.events[j].tick - e.tick; break; }
        const double wantSamples = std::min(space * a.beat / ticksPerBeat, needed[e.role] * x.s.sampleRate);
        if (on.free < 0.8 * wantSamples)
            risk += 1;
        for (size_t j = i + 1; j < c.events.size() && c.events[j].tick - e.tick < 3.0; ++j)
        {
            const auto& f = c.events[j];
            const double d = f.tick - e.tick;
            if (f.role == e.role && !e.roll && !f.roll && !e.grace && !f.grace && d < (e.role == Kick ? 2.9 : 1.4))
                collisions += 1;
            if (f.role != e.role && d > 0.4 && d < 2.0 && !e.grace && !f.grace && !(e.roll || f.roll))
                collisions += 0.5; // near-but-not-together hits of different drums: mush, not a flam
        }
        if (e.role == Kick && !e.anchor && e.tick < turnTick + 3) // a kick stacked on the pulse's own low end
            risk += 0.5;
        ++counted;
    }
    const double n = std::max(1, counted);
    // T: R15c keeps a slight build only (target +0.05; acoustic full-bar fills measured -0.09, snare rolls +0.20);
    // first: T = tension, so every candidate was pushed to crescendo
    const double tension = (late - early) / (late + early + 1e-9);
    const double shape = std::clamp(1 - std::pow((tension - 0.05) / 0.35, 2.0), -1.0, 1.0);
    const double land = (c.drop && c.reverse) || landLast16 || (strongLast8 && !anyLast16) ? 0.35 : 0.0;
    const double T = 0.5 * shape + land;
    std::vector<Ev> original;
    for (int o : x.original)
    {
        Ev e; const auto& on = a.onsets[static_cast<size_t>(o)];
        e.tick = on.tick - x.windowStart / a.beat * ticksPerBeat; e.role = on.role; e.velocity = std::clamp(on.peak / std::max(1e-9, a.typicalPeak[static_cast<size_t>(on.role)]), 0.2, 1.2);
        original.push_back(e);
    }
    c.change = 1 - gaussCos(c.events, original, turnTick, end);
    const double V = std::clamp(1 - std::pow((c.change - 0.5) / 0.35, 2.0), -1.0, 1.0);
    const double sync = weight > 0 ? syncopated / weight : 0;
    const double D = std::clamp(1 - std::pow((sync - lib.syncopation) / 0.25, 2.0), -1.0, 1.0);
    const double X = 0.25 * collisions;
    const double U = weight > 0 ? uncertain / weight : 1;
    const double R = risk / n;
    double M = 0;
    for (int j = 1; j < figureBeats; ++j)
        if (c.cells[static_cast<size_t>(j)] == c.cells[static_cast<size_t>(j - 1)])
            M += 0.5;
    const double perBeat = n / std::max(1, figureBeats), target = lib.density * (0.55 + 0.9 * x.chop);
    const double F = 1 - std::abs(perBeat - target) / target;
    // L: a fill must not collapse under the groove it replaces (lab: a 1-beat boom bap fill 9 dB quieter)
    double fillEnergy = 0, originalEnergy = 0;
    for (const auto& e : c.events)
        if (e.tick >= turnTick - 0.5 && !e.grace)
        {
            const auto& on = a.onsets[static_cast<size_t>(e.source)];
            const double level = e.anchor || e.carried ? on.peak : e.velocity * a.typicalPeak[static_cast<size_t>(e.role)];
            fillEnergy += level * level;
        }
    for (int o : x.original)
    {
        const auto& on = a.onsets[static_cast<size_t>(o)];
        if (on.tick - x.windowStart / a.beat * ticksPerBeat >= turnTick - 0.5)
            originalEnergy += on.peak * on.peak;
    }
    const double levelDb = originalEnergy > 0 ? 10 * std::log10((fillEnergy + 1e-30) / originalEnergy) : 0.0;
    const double L = levelDb < -3 ? (-3 - levelDb) / 6 : levelDb > 9 ? (levelDb - 9) / 6 : 0.0;
    // N: longest stream of even strokes (16ths, 32nds or triplets) - acoustic median 5..7, R15 first 3
    int runLength = 1, longest = 1;
    double previous = -1e9;
    for (const auto& e : c.events)
    {
        if (e.grace)
            continue;
        const double d = e.tick - previous;
        if (d < 0.5)
            continue; // a stacked hit (kick + snare together) does not break or extend the stream
        runLength = std::abs(d - 6) <= 0.6 || std::abs(d - 3) <= 0.6 || std::abs(d - 4) <= 0.6 ? runLength + 1 : 1;
        longest = std::max(longest, runLength);
        previous = e.tick;
    }
    const double N = std::clamp((longest - 3) / 3.0, 0.0, 1.0);
    // a triplet run over a straight groove is a deliberate colour, not the default answer
    const double tripletPrior = c.triplet && a.swing < 1.5 ? 0.3 : 0.0;
    c.score = -tripletPrior - 0.8 * L + 1.2 * T + 0.8 * V + 0.6 * D - 1.5 * X - 2.0 * U - 1.2 * R - 0.6 * M + 0.5 * F + 0.4 * N + 1e-4 * rnd(seed, 99);
    if (c.events.empty())
        c.score = -1e9;
    return c;
}

juce::String familyName(const Context& x, const Candidate& c)
{
    int snareRoll = 0, hatRoll = 0, kicks = 0, snares = 0, ghosts = 0;
    for (const auto& e : c.events)
    {
        if (e.anchor || e.carried)
            continue;
        if (e.roll && e.role == Snare) ++snareRoll;
        if (e.roll && e.role == Hat) ++hatRoll;
        if (e.role == Kick) ++kicks;
        if (e.role == Snare) ++snares;
        if (e.velocity < 0.5 && !e.roll) ++ghosts;
    }
    juce::String name = c.triplet ? "TRIPLET RUN" : snareRoll >= 4 ? "SNARE ROLL" : hatRoll >= 4 ? "HAT ROLL" : kicks >= 2 && snares >= 2 ? "KICK / SNARE CALL"
                        : ghosts >= 2 ? "GHOST TURN" : snares >= 3 ? "SNARE DOUBLE-UP" : "SNARE TURN";
    if (x.genre == Techno && snareRoll + snares >= 4)
        name = "TECHNO BUILD";
    if (c.drop)
        name << " + DROP";
    if (c.reverse)
        name << " + REVERSE";
    int percs = 0;
    for (const auto& e : c.events)
        percs += e.role == Perc && !e.anchor ? 1 : 0;
    if (percs >= 2)
        name << " + PERC";
    int toms = 0;
    for (const auto& e : c.events)
        toms += e.role == Tom ? 1 : 0;
    if (toms >= 2)
        name << " + TOMS";
    if (x.whole)
        name = "WHOLE LOOP: " + name;
    return name;
}

struct Result
{
    Plan plan;
    std::vector<Scheduled> hits;
    int cut = 0;
    Analysis analysis;
};

bool sanitize(Settings& s)
{
    if (!std::isfinite(s.sampleRate) || s.sampleRate < 8000 || s.sampleRate > 192000 || !std::isfinite(s.bpm))
        return false;
    s.kind = Fill;
    s.bpm = std::clamp(s.bpm, 20.0, 999.0);
    s.bars = s.bars == 1 || s.bars == 4 ? s.bars : 2;
    s.fillBeats = s.fillBeats == 1 || s.fillBeats == 2 || s.fillBeats == kWholeLoop ? s.fillBeats : 4;
    s.recipe = std::clamp(s.recipe, kPhraseRecipe, kPhraseRecipe + 4);
    s.fillFx = std::isfinite(s.fillFx) ? std::clamp(s.fillFx, 0.0, 1.0) : 0.0;
    for (auto& k : s.knobs)
        k = std::isfinite(k) ? std::clamp(k, 0.0, 1.0) : 0.0;
    return true;
}
juce::AudioBuffer<float> sourceBuffer(const juce::AudioBuffer<float>& input, const Settings& s)
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

Result plan(const juce::AudioBuffer<float>& source, const Settings& s)
{
    Result r;
    Context x;
    x.s = s;
    x.length = source.getNumSamples();
    x.chop = s.knobs[0]; x.roll = s.knobs[1]; x.reverse = s.knobs[2]; x.stop = s.knobs[3];
    r.analysis = analyse(source, s.sampleRate, s.bpm, s.marks);
    x.whole = s.fillBeats == kWholeLoop;
    x.beats = x.whole ? s.bars * 4 : s.fillBeats;
    const auto& a = r.analysis;
    x.a = &r.analysis;
    auto& p = r.plan;
    p.recipe = s.recipe;
    p.phraseEnd = x.length;
    const double beat = s.sampleRate * 60.0 / s.bpm;
    x.windowStart = std::max(0.0, x.length - beat * x.beats);
    x.windowTicks = x.beats * ticksPerBeat;
    p.phraseStart = static_cast<int>(std::lround(x.windowStart));
    r.cut = p.phraseStart;
    for (int role = 0; role < kRoles; ++role)
    {
        // a role the user marked uses only the marked hits; other roles stay automatic
        const bool userRole = std::any_of(a.onsets.begin(), a.onsets.end(), [&](const Onset& o) { return o.marked && o.role == role; });
        for (int i = 0; i < static_cast<int>(a.onsets.size()); ++i)
        {
            const auto& o = a.onsets[static_cast<size_t>(i)];
            if (o.role == role && quality(o) >= kUsable && (!userRole || o.marked))
                x.pool[static_cast<size_t>(role)].push_back(i);
        }
        std::stable_sort(x.pool[static_cast<size_t>(role)].begin(), x.pool[static_cast<size_t>(role)].end(),
                         [&](int i, int j) { return quality(a.onsets[static_cast<size_t>(i)]) > quality(a.onsets[static_cast<size_t>(j)]); });
        p.kitCounts[static_cast<size_t>(role)] = static_cast<int>(x.pool[static_cast<size_t>(role)].size());
    }
    // toms high -> low by pitch: zero-crossing rate of the body 5..80 ms after the attack (a tom is a pitched,
    // ringing drum; the higher tom crosses zero more often)
    {
        std::vector<std::pair<double, int>> pitched;
        for (int i : x.pool[Tom])
        {
            const auto& o = a.onsets[static_cast<size_t>(i)];
            const int from = std::min(x.length - 1, o.attack + static_cast<int>(0.005 * s.sampleRate));
            const int to = std::min({ x.length, o.attack + static_cast<int>(0.080 * s.sampleRate), o.start + std::max(1, o.free) });
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
        for (const auto& entry : pitched)
            x.toms.push_back(entry.second);
    }
    for (int i = 0; i < static_cast<int>(a.onsets.size()); ++i)
        if (!a.onsets[static_cast<size_t>(i)].secondary && a.onsets[static_cast<size_t>(i)].attack >= a.phase + x.windowStart - 0.004 * s.sampleRate)
            x.original.push_back(i);
    for (const auto& o : a.onsets) // the page snaps marks to these hit regions
        if (!o.secondary)
        {
            Slice slice;
            slice.sample = o.start; slice.end = o.start + o.free; slice.role = o.role;
            p.slices.push_back(slice);
        }
    x.genre = s.recipe == kPhraseRecipe ? a.genre : s.recipe - kPhraseRecipe - 1;
    p.genre = x.genre;
    if (std::all_of(s.knobs.begin(), s.knobs.end(), [](double v) { return v == 0; }))
    {
        p.status = Bypass;
        p.reason = "ALL MACROS AT ZERO - SOURCE KEPT";
        return r;
    }
    if (x.pool[Kick].empty() && x.pool[Snare].empty())
    {
        p.status = Bypass;
        p.reason = a.onsets.empty() ? "NO DRUM HITS FOUND - SOURCE KEPT" : "NO CLEAN KICK OR SNARE SLICE - SOURCE KEPT, NOTHING INVENTED";
        return r;
    }
    x.status = a.drumness < 0.35 || a.timing < 0.45 ? Conservative : Generated;
    // 48 candidates; the seed chooses among the distinct ones scoring within 0.3 of the best, so NEW VARIATION gives
    // another good phrase instead of the same optimum every time
    std::vector<Candidate> all;
    for (int i = 0; i < 48; ++i)
        all.push_back(build(x, i));
    std::stable_sort(all.begin(), all.end(), [](const Candidate& p1, const Candidate& p2) { return p1.score > p2.score; });
    std::vector<size_t> near;
    std::vector<juce::String> seen;
    for (size_t i = 0; i < all.size() && near.size() < 5; ++i)
    {
        if (all[i].score < all.front().score - 0.3)
            break;
        juce::String signature;
        for (const auto& e : all[i].events)
            signature << juce::String(e.tick, 2) << ":" << e.role << ";";
        if (std::find(seen.begin(), seen.end(), signature) != seen.end())
            continue;
        seen.push_back(signature);
        near.push_back(i);
    }
    Candidate best = near.empty() ? Candidate {} : std::move(all[near[static_cast<size_t>(mix(x.s.seed ^ 0x5345ull) % near.size())]]);
    if (best.events.empty() || best.score <= -1e8)
    {
        p.status = Bypass;
        p.reason = "NO SAFE PHRASE WITH THE AVAILABLE SLICES - SOURCE KEPT";
        return r;
    }
    p.status = x.status;
    p.anchors = best.anchors;
    p.family = familyName(x, best);
    p.inputSimilarity = 1 - best.change;
    p.reason = x.status == Conservative ? (a.drumness < 0.35 ? "MIXED / SUSTAINED AUDIO - CAREFUL LAST-BEAT FILL" : "LOOSE TIMING - CAREFUL LAST-BEAT FILL") : "";
    // schedule: straight grid -> swing on odd 16ths -> role microtiming -> samples
    const double origin = a.phase + x.windowStart;
    // R15c systematic feel, deterministic (no random jitter): acoustic fills sit 9..11 ms from the grid, R15 first
    // measured 3.3 ms. Ghost notes late, backbeat accents laid back, added kicks a little ahead, off-beat hat 16ths
    // late, rolls and runs rushing into the cadence. Scaled by genre: played styles more than programmed ones.
    const double feelScale = (x.status == Conservative ? 0.5 : 1.0) * (x.genre == BoomBap ? 1.0 : x.genre == DnB ? 0.7 : 0.35);
    const double figureStart = best.turn * ticksPerBeat, figureLength = std::max(1.0, x.windowTicks - figureStart);
    const auto feelMs = [&](const Ev& e)
    {
        if (e.reverse || e.grace)
            return 0.0;
        const double progress = std::clamp((e.tick - figureStart) / figureLength, 0.0, 1.0);
        const double in16 = std::fmod(e.tick, ticksPerBeat) / 6.0;
        const bool on16 = std::abs(in16 - std::round(in16)) < 0.05, odd16 = on16 && static_cast<int>(std::round(in16)) % 2 != 0;
        double ms = 0;
        if (e.roll || e.run)
            ms = -(2.0 + 4.0 * progress); // rushing, more towards the bar line
        else if (e.velocity < 0.5)
            ms = 9.0; // ghost notes behind the beat
        else if (e.role == Snare && on16 && !odd16)
            ms = 5.0; // laid-back accents
        else if (e.role == Kick)
            ms = -3.0;
        else if (e.role == Hat)
            ms = odd16 ? 4.0 : -1.5;
        return ms * feelScale;
    };
    const auto toSample = [&](const Ev& e)
    {
        if (e.anchor || e.carried)
            return e.sample;
        double t = e.tick;
        const double pos16 = t / 6.0;
        if (std::abs(pos16 - std::round(pos16)) < 0.05 && static_cast<long long>(std::round(pos16)) % 2 != 0)
            t += a.swing;
        if (!e.reverse)
            t += a.offset[static_cast<size_t>(e.role)];
        return static_cast<int>(std::lround(origin + t * a.beat / ticksPerBeat + feelMs(e) * 0.001 * s.sampleRate)) - static_cast<int>(0.0015 * s.sampleRate);
    };
    const int turnSample = static_cast<int>(std::lround(origin + best.turn * ticksPerBeat * a.beat / ticksPerBeat)) - static_cast<int>(0.004 * s.sampleRate);
    r.cut = std::clamp(turnSample, 0, x.length - 1);
    double usedQuality = 0, usedWeight = 0;
    for (const auto& e : best.events)
    {
        const auto& on = a.onsets[static_cast<size_t>(e.source)];
        Scheduled h;
        h.role = e.role; h.anchor = e.anchor || e.carried; h.roll = e.roll; h.reverse = e.reverse;
        h.source = on.start; h.clean = h.anchor ? Full : on.clean;
        h.at = std::max(r.cut, toSample(e));
        h.length = std::min(on.free, static_cast<int>(nominal[e.role] * s.sampleRate));
        if (e.reverse)
        {
            h.length = std::min(h.length, static_cast<int>(12 * a.beat / ticksPerBeat));
            h.at = x.length - h.length;
        }
        h.sourceEnd = h.source + h.length;
        h.gain = h.anchor ? 1.0 : e.velocity * std::clamp(a.typicalPeak[static_cast<size_t>(e.role)] / std::max(1e-9, on.peak), 0.25, 1.6);
        if (h.at >= x.length - 8 || h.length < 16)
            continue;
        r.hits.push_back(h);
        usedQuality += e.velocity * quality(on);
        usedWeight += e.velocity;
    }
    std::stable_sort(r.hits.begin(), r.hits.end(), [](const Scheduled& p1, const Scheduled& p2) { return p1.at < p2.at; });
    // same-role choke: a new snare stroke stops the previous one (a drum, not a pile of copies); toms choke only
    // their own drum (different marked toms ring together)
    for (size_t i = 0; i < r.hits.size(); ++i)
        for (size_t j = i + 1; j < r.hits.size(); ++j)
            if (r.hits[j].role == r.hits[i].role && !r.hits[j].reverse && (r.hits[i].role != Tom || r.hits[j].source == r.hits[i].source))
            {
                r.hits[i].length = std::min(r.hits[i].length, r.hits[j].at - r.hits[i].at + static_cast<int>(0.002 * s.sampleRate));
                break;
            }
    for (auto& h : r.hits)
        h.length = std::max(0, std::min(h.length, x.length - h.at));
    p.confidence = (usedWeight > 0 ? usedQuality / usedWeight : 0) * (0.5 + 0.5 * a.timing);
    p.phraseStart = r.cut;
    for (const auto& h : r.hits)
    {
        Event e;
        e.at = h.at; e.length = h.length; e.source = h.source; e.sourceEnd = h.sourceEnd; e.role = h.role;
        e.velocity = h.gain; e.reverse = h.reverse; e.roll = h.roll;
        p.events.push_back(e);
    }
    p.attacks = static_cast<int>(p.events.size());
    return r;
}

void room(juce::AudioBuffer<float>& out, const juce::AudioBuffer<float>& send, int from, double amount, double fs)
{
    if (amount <= 0)
        return;
    constexpr double times[] { 0.0233, 0.0307, 0.0371, 0.0437 };
    std::array<std::array<std::vector<double>, 4>, 2> lines;
    int index[4] {}, size[4] {};
    double lp[2][4] {};
    for (int k = 0; k < 4; ++k)
    {
        size[k] = static_cast<int>(fs * times[k]);
        for (int c = 0; c < 2; ++c)
            lines[static_cast<size_t>(c)][static_cast<size_t>(k)].assign(static_cast<size_t>(size[k]), 0.0);
    }
    const double damping = 1 - std::exp(-2 * pi * 4800 / fs);
    for (int n = from; n < out.getNumSamples(); ++n)
    {
        for (int c = 0; c < 2; ++c)
        {
            double wet = 0;
            for (int j = 0; j < 4; ++j)
            {
                auto& line = lines[static_cast<size_t>(c)][static_cast<size_t>(j)];
                const double v = line[static_cast<size_t>(index[j])];
                lp[c][j] += damping * (v - lp[c][j]);
                line[static_cast<size_t>(index[j])] = send.getSample(c, n) + (0.78 + 0.1 * amount) * lp[c][j];
                wet += (c == 1 && j % 2 ? -0.7 : 1.0) * v * 0.25;
            }
            out.addSample(c, n, static_cast<float>(wet * amount * 0.8));
        }
        for (int j = 0; j < 4; ++j)
            if (++index[j] >= size[j])
                index[j] = 0;
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
            for (int k = 1; k < 4; ++k) // Catmull-Rom 4x: inter-sample overs
            {
                const double t = k / 4.0, p0 = d[n - 1], p1 = d[n], p2 = d[n + 1], p3 = d[n + 2];
                const double v = 0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
                peak = std::max(peak, std::abs(v));
            }
        }
    }
    return peak;
}
} // namespace

Plan makePhraseFillPlan(const juce::AudioBuffer<float>& input, const Settings& settings)
{
    Settings s = settings;
    if (!sanitize(s))
        return {};
    return plan(sourceBuffer(input, s), s).plan;
}

juce::AudioBuffer<float> renderPhraseFill(const juce::AudioBuffer<float>& input, const Settings& settings, Plan* info)
{
    Settings s = settings;
    if (!sanitize(s))
        return {};
    const auto source = sourceBuffer(input, s);
    auto result = plan(source, s);
    if (info)
        *info = result.plan;
    juce::AudioBuffer<float> out(source);
    const int length = source.getNumSamples();
    if (result.plan.status == Bypass || result.hits.empty())
        return out;
    const double fs = s.sampleRate;
    const juce::AudioBuffer<float> clean[4] { source, filtered(source, fs, 3500, false), filtered(source, fs, 200, true), filtered(source, fs, 5000, true) };
    // the original pulse plays up to the turn, then a 6 ms fade (the figure's first stroke has a 1.5 ms pre-roll)
    const int cut = result.cut, fade = std::max(1, static_cast<int>(0.006 * fs));
    for (int c = 0; c < 2; ++c)
    {
        for (int n = std::max(0, cut - fade); n < cut; ++n)
            out.setSample(c, n, out.getSample(c, n) * static_cast<float>(fadeCurve(static_cast<double>(cut - n) / fade)));
        out.clear(c, cut, length - cut);
    }
    juce::AudioBuffer<float> send(2, length);
    send.clear();
    const int cadence = length - static_cast<int>(std::lround(fs * 60.0 / s.bpm));
    for (const auto& h : result.hits)
    {
        const auto& src = clean[h.clean];
        const int count = std::min(h.length, length - h.at);
        if (count < 8)
            continue;
        const double attack = h.reverse ? count * 0.85 : 0.0006 * fs, release = std::min(count * 0.3, fs * (h.roll ? 0.002 : 0.004));
        for (int i = 0; i < count; ++i)
        {
            const int from = h.reverse ? h.source + count - 1 - i : h.source + i;
            if (from < 0 || from >= length)
                continue;
            const double env = (h.anchor ? 1.0 : fadeCurve((i + 1) / (attack + 1))) * fadeCurve((count - i) / (release + 1));
            for (int c = 0; c < 2; ++c)
            {
                const float v = static_cast<float>(src.getSample(c, from) * env * h.gain);
                out.addSample(c, h.at + i, v);
                if (h.at >= cadence && h.role == Snare)
                    send.addSample(c, h.at + i, v);
            }
        }
    }
    room(out, send, cut, s.fillFx, fs);
    // land on the bar line: a 3 ms fade at the very end
    const int endFade = std::max(1, static_cast<int>(0.003 * fs));
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < endFade; ++i)
            out.setSample(c, length - 1 - i, out.getSample(c, length - 1 - i) * static_cast<float>(fadeCurve(static_cast<double>(i) / endFade)));
    // the edit is never louder than the source's own peak or -1.8 dBTP, whichever is higher; the prefix is untouched
    const double prefix = truePeak(source, 0, length), tail = truePeak(out, cut, length);
    const double ceiling = std::max(std::min(prefix, 0.98), std::pow(10.0, -1.8 / 20.0));
    if (tail > ceiling)
    {
        const double g = ceiling / tail;
        const int ramp = std::max(1, static_cast<int>(0.002 * fs));
        for (int c = 0; c < 2; ++c)
            for (int n = cut; n < length; ++n)
            {
                const double k = n - cut < ramp ? 1 + (g - 1) * (n - cut) / ramp : g;
                out.setSample(c, n, static_cast<float>(out.getSample(c, n) * k));
            }
    }
    return out;
}
} // namespace hpeq::dsp::transition
