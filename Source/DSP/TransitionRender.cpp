#include "DSP/TransitionRender.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace hpeq::dsp::transition
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

double unit(std::uint64_t seed, std::uint64_t salt) noexcept
{
    std::uint64_t z = seed * 0x9E3779B97F4A7C15ull + salt * 0xBF58476D1CE4E5B9ull + 0x94D049BB133111EBull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    return static_cast<double>(z >> 11) * (1.0 / 9007199254740992.0);
}

double raised(double x) noexcept { return 0.5 - 0.5 * std::cos(kPi * std::clamp(x, 0.0, 1.0)); }

// linear interpolation inside [0, size)
float at(const juce::AudioBuffer<float>& b, int ch, double pos) noexcept
{
    const int n = b.getNumSamples();
    if (n == 0)
        return 0.0f;
    pos = std::clamp(pos, 0.0, static_cast<double>(n - 1));
    const int i = static_cast<int>(pos);
    const auto f = static_cast<float>(pos - i);
    const float a = b.getSample(ch, i), c = b.getSample(ch, std::min(n - 1, i + 1));
    return a + f * (c - a);
}

// a grain's envelope: attack / release in samples, at local time v of a grain `length` long
double grain(double v, double length, double attack, double release) noexcept
{
    double g = 1.0;
    if (v < attack)
        g *= raised((v + 1.0) / (attack + 1.0));
    if (v > length - release)
        g *= raised((length - v) / (release + 1.0));
    return g;
}

// an 8-line feedback delay network, wet only (the reverse swell's reverb)
void reverb(const juce::AudioBuffer<float>& in, juce::AudioBuffer<float>& out, double sampleRate, double decaySeconds)
{
    static constexpr double ms[8] { 29.7, 37.1, 41.1, 43.7, 53.3, 59.9, 67.1, 73.9 };
    std::array<std::vector<float>, 8> lines;
    std::array<int, 8> pos {};
    std::array<double, 8> gain {}, low {};
    for (int i = 0; i < 8; ++i)
    {
        lines[static_cast<size_t>(i)].assign(static_cast<size_t>(std::max(8.0, ms[i] * 1.0e-3 * sampleRate)), 0.0f);
        gain[static_cast<size_t>(i)] = std::pow(10.0, -3.0 * static_cast<double>(lines[static_cast<size_t>(i)].size()) / (decaySeconds * sampleRate));
    }
    const double damp = std::exp(-2.0 * kPi * 6000.0 / sampleRate);
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

double peakOf(const juce::AudioBuffer<float>& b) { return std::max(b.getMagnitude(0, 0, b.getNumSamples()), b.getMagnitude(1, 0, b.getNumSamples())); }

juce::AudioBuffer<float> renderRise(const juce::AudioBuffer<float>& src, const Settings& s, int length)
{
    const double sr = s.sampleRate, beat = 60.0 / s.bpm * sr;
    const double pitch = s.knobs[0], rollAmount = s.knobs[1], swellAmount = s.knobs[2], sweep = s.knobs[3];
    juce::AudioBuffer<float> out(2, length);
    out.clear();

    // ROLL: the source's last beat (or the one before, by the seed), repeated 1/4 -> 1/8 -> 1/16 -> 1/32 (the loop-roll
    // halving) or on a smooth geometric curve, the pitch climbing to +12 x PITCH semitones
    // the grab: two beats of source (the last two, or the two before), so a grain sped up to 2x never runs out of it
    const double window = 2.0 * beat;
    const double grabStart = std::max(0.0, length - window - (unit(s.seed, 1) < 0.7 ? 0.0 : beat));
    const bool smooth = unit(s.seed, 2) < 0.4;
    const auto period = [&](double x)
    {
        if (smooth)
            return beat * std::pow(2.0, -4.0 * x); // 1/4 at the start, 1/64 at the end
        return x < 0.5 ? beat : x < 0.75 ? beat / 2.0 : x < 0.875 ? beat / 4.0 : beat / 8.0;
    };
    double start = 0.0;
    while (start < length)
    {
        const double x0 = start / length, len = std::max(0.004 * sr, period(x0));
        for (int n = static_cast<int>(std::ceil(start)); n < std::min(static_cast<double>(length), start + len); ++n)
        {
            const double x = static_cast<double>(n) / length, v = n - start;
            const double ratio = std::pow(2.0, 12.0 * pitch * std::pow(x, 1.5) / 12.0);
            const double from = std::min(v * ratio, window - 2.0); // no wrap inside a grain (a wrap is a click)
            const double g = grain(v, len, 0.002 * sr, std::min(0.003 * sr, 0.25 * len));
            for (int ch = 0; ch < 2; ++ch)
                out.setSample(ch, n, static_cast<float>(rollAmount * g * at(src, ch, grabStart + from)));
        }
        start += len;
    }

    // SWELL: the last bar reversed, a long reverb on it (wet), reversed back: it swells into the end
    if (swellAmount > 0.0)
    {
        const int bar = static_cast<int>(std::lround(4.0 * beat));
        juce::AudioBuffer<float> rev(2, std::min(bar, src.getNumSamples()));
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < rev.getNumSamples(); ++i)
                rev.setSample(ch, i, src.getSample(ch, src.getNumSamples() - 1 - i));
        juce::AudioBuffer<float> wet(2, length);
        reverb(rev, wet, sr, std::max(0.5, 0.8 * length / sr));
        const double norm = peakOf(src) / std::max(1.0e-9, peakOf(wet));
        for (int ch = 0; ch < 2; ++ch)
            for (int n = 0; n < length; ++n)
                out.addSample(ch, n, static_cast<float>(swellAmount * norm * wet.getSample(ch, length - 1 - n)));
    }

    // SWEEP: a resonant high pass rising 30 Hz -> 30 .. 1500 Hz, the level rising from -24 dB, the width opening.
    // The filtered build is first levelled (short-term RMS, +-18 dB): a kick in the early grains and thin 1/32 grains
    // at the end would otherwise make the "rise" fall; the ramp alone then sets how it grows.
    {
        const double top = 30.0 + 1470.0 * sweep, q = 0.7 + 1.5 * sweep;
        std::array<double, 2> z1 {}, z2 {};
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        for (int n = 0; n < length; ++n)
        {
            const double x = static_cast<double>(n) / length;
            if (n % 32 == 0)
            {
                const double f = 30.0 * std::pow(top / 30.0, x * x), w = 2.0 * kPi * f / sr, alpha = std::sin(w) / (2.0 * q), c = std::cos(w);
                const double a0 = 1.0 + alpha;
                b0 = (1.0 + c) / 2.0 / a0;
                b1 = -(1.0 + c) / a0;
                b2 = b0;
                a1 = -2.0 * c / a0;
                a2 = (1.0 - alpha) / a0;
            }
            const double level = 1.0;
            float y[2] {};
            for (int ch = 0; ch < 2; ++ch)
            {
                const double in = out.getSample(ch, n);
                const double o = b0 * in + z1[static_cast<size_t>(ch)];
                z1[static_cast<size_t>(ch)] = b1 * in - a1 * o + z2[static_cast<size_t>(ch)];
                z2[static_cast<size_t>(ch)] = b2 * in - a2 * o;
                y[ch] = static_cast<float>(level * o);
            }
            out.setSample(0, n, static_cast<float>(y[0] / level)); // the ramp after the levelling below
            out.setSample(1, n, static_cast<float>(y[1] / level));
        }
        // the levelling: RMS per 1024 samples, smoothed, the gain to the whole clip's RMS (clamped to +-18 dB)
        constexpr int window = 1024;
        const int windows = (length + window - 1) / window;
        std::vector<double> rms(static_cast<size_t>(windows), 0.0);
        double total = 0.0;
        for (int w = 0; w < windows; ++w)
        {
            double e = 0.0;
            int count = 0;
            for (int n = w * window; n < std::min(length, (w + 1) * window); ++n, ++count)
                for (int ch = 0; ch < 2; ++ch)
                    e += static_cast<double>(out.getSample(ch, n)) * out.getSample(ch, n);
            rms[static_cast<size_t>(w)] = e / std::max(1, 2 * count);
            total += e;
        }
        const double target = std::sqrt(total / (2.0 * length) + 1.0e-20);
        std::vector<double> gain(static_cast<size_t>(windows), 1.0);
        for (int w = 0; w < windows; ++w)
        {
            double e = 0.0;
            int count = 0;
            for (int k = std::max(0, w - 4); k <= std::min(windows - 1, w + 4); ++k, ++count) // ~0.2 s around
                e += rms[static_cast<size_t>(k)];
            gain[static_cast<size_t>(w)] = std::clamp(target / std::sqrt(e / count + 1.0e-20), 0.125, 8.0);
        }
        for (int n = 0; n < length; ++n)
        {
            const double x = static_cast<double>(n) / length, pos = (n + 0.5) / window - 0.5;
            const int w0 = std::clamp(static_cast<int>(std::floor(pos)), 0, windows - 1), w1 = std::min(windows - 1, w0 + 1);
            const double f = std::clamp(pos - w0, 0.0, 1.0), g = gain[static_cast<size_t>(w0)] + f * (gain[static_cast<size_t>(w1)] - gain[static_cast<size_t>(w0)]);
            const double level = std::pow(10.0, -24.0 * std::max(sweep, 0.4) * std::pow(1.0 - x, 1.2) / 20.0); // -24 dB -> 0 dB
            const float l = static_cast<float>(g * level * out.getSample(0, n)), r = static_cast<float>(g * level * out.getSample(1, n));
            const float mid = 0.5f * (l + r), side = 0.5f * (l - r) * static_cast<float>(0.4 + 0.6 * x);
            out.setSample(0, n, mid + side);
            out.setSample(1, n, mid - side);
        }
    }
    return out;
}

// copy `count` samples of the source from `from` to `to`, with 1.5 ms fades at both ends (a block of a cut)
void block(const juce::AudioBuffer<float>& src, juce::AudioBuffer<float>& out, int from, int to, int count, double sr)
{
    const double edge = 0.0015 * sr;
    for (int i = 0; i < count && to + i < out.getNumSamples(); ++i)
    {
        const double g = grain(i, count, edge, edge);
        for (int ch = 0; ch < 2; ++ch)
            out.setSample(ch, to + i, static_cast<float>(g * src.getSample(ch, std::clamp(from + i, 0, src.getNumSamples() - 1))));
    }
}

juce::AudioBuffer<float> renderFill(const juce::AudioBuffer<float>& src, const Settings& s, int length)
{
    const double sr = s.sampleRate;
    const double chop = s.knobs[0], roll = s.knobs[1], reverse = s.knobs[2], stop = s.knobs[3];
    juce::AudioBuffer<float> out(src);
    const int bar = length / std::max(1, s.bars), beat = bar / 4, eighth = bar / 8;
    // the bars before the last: at most one beat edited each (a repeat of the beat before, or a jump in the bar)
    for (int b = 0; b + 1 < s.bars; ++b)
        if (unit(s.seed, 10 + static_cast<std::uint64_t>(b)) < 0.6 * chop)
        {
            const int at4 = b * bar + 3 * beat;
            const int from = unit(s.seed, 20 + static_cast<std::uint64_t>(b)) < 0.5 ? at4 - beat : b * bar + beat;
            block(src, out, from, at4, beat, sr);
        }
    // the last bar: blocks of eighths from seeded offsets (3 + 3 + 2 or a rotation), then the roll on the last beat
    const int last = (s.bars - 1) * bar;
    static constexpr int patterns[3][3] { { 3, 3, 2 }, { 2, 3, 3 }, { 3, 2, 3 } };
    const auto& pattern = patterns[std::min(2, static_cast<int>(unit(s.seed, 30) * 3.0))];
    int position = 0;
    for (int i = 0; i < 3; ++i)
    {
        const int size = pattern[i];
        const bool rollHere = i == 2 && roll >= 0.25; // the last block's last beat is the roll's
        int offset = position; // in eighths
        if (chop >= 0.2 && i > 0)
            offset = static_cast<int>(unit(s.seed, 40 + static_cast<std::uint64_t>(i)) * (8 - size + 1)); // another place in the bar
        const int count = rollHere ? (size - 2) * eighth : size * eighth;
        if (count > 0 && (offset != position || chop >= 0.2))
            block(src, out, last + offset * eighth, last + position * eighth, count, sr);
        position += size;
    }
    if (roll >= 0.25)
    {
        // the last beat: its first 1/16 rolled - 1/16 x 2, 1/32 x 2, 1/64 x 2 - released on the downbeat
        const int start = last + 3 * beat;
        const int grabFrom = start;
        int t = 0;
        const int segments[3][2] { { beat / 2, beat / 4 }, { beat / 4, beat / 8 }, { beat / 4, beat / 16 } };
        for (const auto& seg : segments)
        {
            const int span = seg[0], grainLength = std::max(16, seg[1]);
            for (int g = 0; g * grainLength < span; ++g)
                block(src, out, grabFrom, start + t + g * grainLength, std::min(grainLength, span - g * grainLength), sr);
            t += span;
        }
    }
    // the ending: REVERSE of the last eighth, or a TAPE STOP over it, weighted by the knobs and the seed
    const double pick = unit(s.seed, 50) * (reverse + stop + 0.4);
    const int endStart = length - eighth;
    if (pick < reverse && reverse > 0.05)
    {
        juce::AudioBuffer<float> tail(2, eighth);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < eighth; ++i)
                tail.setSample(ch, i, out.getSample(ch, endStart + eighth - 1 - i));
        block(tail, out, 0, endStart, eighth, sr);
    }
    else if (pick < reverse + stop && stop > 0.05)
    {
        juce::AudioBuffer<float> held(out);
        for (int i = 0; i < eighth; ++i)
        {
            const double x = static_cast<double>(i) / eighth, moved = eighth * (x - 0.5 * x * x); // speed 1 -> 0
            const double g = 1.0 - x;
            for (int ch = 0; ch < 2; ++ch)
                out.setSample(ch, endStart + i, static_cast<float>(g * at(held, ch, endStart + moved)));
        }
    }
    return out;
}
} // namespace

const char* knobName(int kind, int knob)
{
    static constexpr const char* rise[4] { "PITCH", "ROLL", "SWELL", "SWEEP" };
    static constexpr const char* fill[4] { "CHOP", "ROLL", "REVERSE", "STOP" };
    return (kind == Rise ? rise : fill)[std::clamp(knob, 0, 3)];
}

std::array<double, 4> defaultKnobs(int kind) noexcept
{
    return kind == Rise ? std::array<double, 4> { 0.6, 0.8, 0.5, 0.7 } : std::array<double, 4> { 0.6, 0.7, 0.4, 0.3 };
}

int lengthSamples(const Settings& s) noexcept
{
    return static_cast<int>(std::lround(s.bars * 4.0 * 60.0 / std::max(20.0, s.bpm) * s.sampleRate));
}

juce::AudioBuffer<float> render(const juce::AudioBuffer<float>& source, const Settings& s)
{
    if(s.recipe>0)return renderEvolve(source,s);
    const int length = lengthSamples(s);
    juce::AudioBuffer<float> src(2, length);
    src.clear();
    for (int ch = 0; ch < 2; ++ch)
        src.copyFrom(ch, 0, source, std::min(ch, source.getNumChannels() - 1), 0, std::min(length, source.getNumSamples()));
    auto out = s.kind == Rise ? renderRise(src, s, length) : renderFill(src, s, length);
    // land on the bar line: a 5 ms fade at the very end
    const int fade = std::max(1, static_cast<int>(0.005 * s.sampleRate));
    for (int i = 0; i < fade && i < length; ++i)
        for (int ch = 0; ch < 2; ++ch)
            out.setSample(ch, length - 1 - i, out.getSample(ch, length - 1 - i) * static_cast<float>(raised(static_cast<double>(i) / fade)));
    // no louder than the source's peak, and never over -0.1 dBFS (a WAV clips at 1.0)
    const double peak = peakOf(out), ref = std::min(0.98855, std::max(1.0e-6, peakOf(src)));
    if (peak > ref)
        out.applyGain(static_cast<float>(ref / peak));
    return out;
}
} // namespace hpeq::dsp::transition
