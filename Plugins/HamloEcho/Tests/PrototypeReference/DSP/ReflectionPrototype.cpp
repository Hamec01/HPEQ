#include "DSP/ReflectionPrototype.h"
#include <algorithm>
#include <cmath>

namespace hamlo::echo
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr int taps = 64, phases = 512, radius = taps / 2;
struct Kernels
{
    std::array<double, 49> ratio;
    std::vector<float> coefficients;
    Kernels() : coefficients(static_cast<size_t>(49 * (phases + 1) * taps))
    {
        for (int s = -24; s <= 24; ++s)
        {
            ratio[static_cast<size_t>(s + 24)] = std::pow(2.0, s / 12.0);
            const double cutoff = .45 / std::max(1.0, ratio[static_cast<size_t>(s + 24)]);
            for (int phase = 0; phase <= phases; ++phase)
            {
                double sum = 0;
                auto* row = coefficients.data() + ((s + 24) * (phases + 1) + phase) * taps;
                for (int k = 0; k < taps; ++k)
                {
                    const double x = k - radius + 1 - static_cast<double>(phase) / phases;
                    const double sinc = std::abs(x) < 1e-12 ? 2 * cutoff : std::sin(2 * pi * cutoff * x) / (pi * x);
                    const double w = .42 - .5 * std::cos(2 * pi * k / (taps - 1)) + .08 * std::cos(4 * pi * k / (taps - 1));
                    row[k] = static_cast<float>(sinc * w); sum += row[k];
                }
                for (int k = 0; k < taps; ++k) row[k] = static_cast<float>(row[k] / sum);
            }
        }
    }
};
const Kernels& kernels() { static const Kernels data; return data; }
}

void ReflectionPrototype::prepare(double sampleRate, int)
{
    rate = sampleRate;
    (void) kernels(); // Complete every table allocation before the audio callback.
    grain = std::max(4, static_cast<int>(std::lround(.08 * rate)) / 2 * 2);
    search = static_cast<int>(std::lround(.02 * rate));
    capacity = static_cast<int>(std::ceil(6.5 * rate)) + taps + 8;
    for (auto& channel : history) channel.assign(static_cast<size_t>(2 * capacity), 0.0f);
    window.resize(static_cast<size_t>(grain));
    for (int i = 0; i < grain; ++i) window[static_cast<size_t>(i)] = .5 - .5 * std::cos(2 * pi * i / grain);
    voices = {};
    write = 0; clock = 0;
    setScore(score);
}

void ReflectionPrototype::setScore(const Score& incoming) noexcept
{
    const auto previous = score;
    score = incoming;
    score.sourceNote = std::clamp(score.sourceNote, 0, 127);
    score.bpm = std::clamp(std::isfinite(score.bpm) ? score.bpm : 120.0, 20.0, 999.0);
    for (size_t i = 0; i < voices.size(); ++i)
    {
        auto& config = score.taps[i];
        auto& v = voices[i];
        v.windowOffset = static_cast<int>(i) * grain / 8;
        config.targetNote = std::clamp(config.targetNote, 0, 127);
        const int shift = std::clamp(config.targetNote - score.sourceNote, -24, 24);
        if (shift != v.shift || config.reverse != previous.taps[i].reverse || config.enabled != previous.taps[i].enabled)
            v.initialised = false;
        v.shift = shift;
        // Values are from prepared immutable tables; no exp/pow/table construction on automation.
        v.ratio = capacity > 0 ? kernels().ratio[static_cast<size_t>(shift + 24)] : 1.0;
        const double ms = config.sync ? std::clamp(config.beats, .03125, 16.0) * 60000.0 / score.bpm : config.timeMs;
        v.delay = static_cast<int>(std::lround(std::clamp(std::isfinite(ms) ? ms : 125.0, 1.0, 4000.0) * rate / 1000.0));
        const double reverseMs = std::isfinite(config.reverseMs) ? config.reverseMs : 125.0;
        v.reverseLength = std::max(4, static_cast<int>(std::lround(std::clamp(reverseMs, 50.0, 2000.0) * rate / 1000.0)) / 2 * 2);
        const double gain = std::clamp(std::isfinite(config.gain) ? config.gain : 0.0, 0.0, 1.0);
        const double pan = std::clamp(std::isfinite(config.pan) ? config.pan : 0.0, -1.0, 1.0);
        v.left = gain * (pan > 0 ? 1.0 - pan : 1.0);
        v.right = gain * (pan < 0 ? 1.0 + pan : 1.0);
    }
}

int ReflectionPrototype::pitchAccumulationSamples(int semitones) const noexcept
{
    if (semitones == 0) return 0;
    const double ratio = kernels().ratio[static_cast<size_t>(std::clamp(semitones, -24, 24) + 24)];
    // Oldest reader over a full window + optional search; a bound, not constant PDC.
    return radius + 2 + static_cast<int>(std::ceil(std::abs(ratio - 1.0) * grain)) + search;
}

int ReflectionPrototype::reverseAccumulationSamples(int tap) const noexcept { return voices[static_cast<size_t>(std::clamp(tap, 0, 7))].reverseLength; }
size_t ReflectionPrototype::historyBytes() const noexcept { return static_cast<size_t>(4 * capacity) * sizeof(float) + window.size() * sizeof(double); }
size_t ReflectionPrototype::sharedTableBytes() noexcept { return static_cast<size_t>(49 * (phases + 1) * taps) * sizeof(float); }

float ReflectionPrototype::integerAt(int channel, int64_t position) const noexcept
{
    if (position < 0 || position > clock || position <= clock - capacity) return 0.0f;
    return history[static_cast<size_t>(channel)][static_cast<size_t>(position % capacity)];
}

double ReflectionPrototype::interpolate(int channel, double position, int shift) const noexcept
{
    if (position < 0 || position > static_cast<double>(clock - radius - 1)
        || position < static_cast<double>(clock - capacity + taps)) return 0.0;
    const auto whole = static_cast<int64_t>(std::floor(position));
    const double phase = (position - static_cast<double>(whole)) * phases;
    const int p = std::clamp(static_cast<int>(phase), 0, phases - 1);
    const double fraction = phase - p;
    int index = static_cast<int>((whole - radius + 1) % capacity);
    if (index < 0) index += capacity;
    const float* x = history[static_cast<size_t>(channel)].data() + index;
    const float* a = kernels().coefficients.data() + ((shift + 24) * (phases + 1) + p) * taps;
    const float* b = a + taps;
    double sum = 0;
    for (int k = 0; k < taps; ++k) sum += x[k] * (a[k] + fraction * (b[k] - a[k]));
    return sum;
}

double ReflectionPrototype::matchedStart(double candidate, double continuing, int channels) const noexcept
{
    const int64_t base = static_cast<int64_t>(std::lround(candidate));
    const int64_t other = static_cast<int64_t>(std::lround(continuing));
    double bestScore = -1e300;
    int best = 0;
    const int step = std::max(1, static_cast<int>(rate / 6000));
    const int length = std::max(8, static_cast<int>(.006 * rate));
    if (base - search - length < 0 || other - length < 0) return candidate;
    // Mirrored history makes the entire backwards search contiguous. Compute the
    // ring indices once per splice, rather than taking a modulo in every dot product.
    std::array<const float*, 2> aPointers {}, bPointers {};
    for (int ch = 0; ch < channels; ++ch)
    {
        aPointers[static_cast<size_t>(ch)] = history[static_cast<size_t>(ch)].data() + base % capacity + capacity;
        bPointers[static_cast<size_t>(ch)] = history[static_cast<size_t>(ch)].data() + other % capacity + capacity;
    }
    const auto evaluate = [&](int offset)
    {
        double dot = 0, energy = 1e-20;
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto* a = aPointers[static_cast<size_t>(ch)] - offset;
            const auto* b = bPointers[static_cast<size_t>(ch)];
            for (int j = 0; j < length; j += step)
            {
                const double x = a[-j], y = b[-j];
                dot += x * y; energy += x * x;
            }
        }
        return dot / std::sqrt(energy);
    };
    const int stride = 2 * step;
    for (int offset = 0; offset <= search; offset += stride)
    {
        const double value = evaluate(offset);
        if (value > bestScore) { bestScore = value; best = offset; }
    }
    const int coarse = best;
    for (int offset = std::max(0, coarse - stride); offset <= std::min(search, coarse + stride); ++offset)
    {
        const double value = evaluate(offset);
        if (value > bestScore) { bestScore = value; best = offset; }
    }
    double fractionalOffset = 0;
    if (best > 0 && best < search)
    {
        const double left = evaluate(best - 1), right = evaluate(best + 1);
        const double denominator = left - 2 * bestScore + right;
        if (denominator < -1e-20) fractionalOffset = std::clamp(.5 * (left - right) / denominator, -.5, .5);
    }
    // Include the continuing reader's fractional position. Integer-only matching
    // can leave a fresh phase error at every splice even for a steady sine.
    return static_cast<double>(base - best) - fractionalOffset + continuing - static_cast<double>(other);
}

void ReflectionPrototype::initialise(Voice& v) noexcept
{
    const double base = radius + 2 + std::max(0.0, v.ratio - 1.0) * grain;
    for (int h = 0; h < 2; ++h)
    {
        auto& head = v.heads[static_cast<size_t>(h)];
        // Spread splice searches over the sample clock instead of stacking eight
        // expensive restarts in the same host block. Block boundaries play no role.
        head.phase = (h * grain / 2 + v.windowOffset) % grain;
        head.read = static_cast<double>(clock - v.delay) - base + (v.ratio - 1.0) * head.phase;
        head.active = true;
    }
    v.initialised = true;
}

double ReflectionPrototype::forward(Voice& v, int channel) const noexcept
{
    if (v.shift == 0) return integerAt(channel, clock - v.delay);
    double out = 0;
    for (const auto& head : v.heads)
        out += window[static_cast<size_t>(head.phase)] * interpolate(channel, head.read, v.shift);
    return out;
}

double ReflectionPrototype::backwards(Voice& v, int channel) const noexcept
{
    double out = 0;
    const int length = v.reverseLength;
    const int64_t elapsed = clock - v.delay - length;
    if (elapsed < 0) return 0;
    const auto frame = elapsed / (length / 2);
    for (int ago = 0; ago < 2; ++ago)
    {
        const auto launch = frame - ago;
        if (launch < 0) continue;
        const int64_t start = v.delay + length + launch * (length / 2);
        const int age = static_cast<int>(clock - start);
        if (age >= length) continue;
        const double w = .5 - .5 * std::cos(2 * pi * age / length);
        out += w * integerAt(channel, start - v.delay - 1 - age);
    }
    return out;
}

void ReflectionPrototype::process(const float* const* input, float* const* wet, int channels, int samples) noexcept
{
    channels = std::clamp(channels, 1, 2);
    for (int sample = 0; sample < samples; ++sample)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            auto& ring = history[static_cast<size_t>(ch)];
            ring[static_cast<size_t>(write)] = ring[static_cast<size_t>(write + capacity)] = input[ch < channels ? ch : 0][sample];
        }
        std::array<double, 2> sum {};
        for (size_t tap = 0; tap < voices.size(); ++tap)
        {
            const auto& config = score.taps[tap];
            if (!config.enabled) continue;
            auto& v = voices[tap];
            if (!config.reverse && v.shift != 0)
            {
                if (!v.initialised) initialise(v);
                for (int h = 0; h < 2; ++h)
                {
                    auto& head = v.heads[static_cast<size_t>(h)];
                    if (head.phase == grain)
                    {
                        head.phase = 0;
                        const double base = static_cast<double>(clock - v.delay) - radius - 2 - std::max(0.0, v.ratio - 1.0) * grain;
                        head.read = matchedStart(base, v.heads[static_cast<size_t>(1 - h)].read, channels);
                    }
                }
            }
            for (int ch = 0; ch < channels; ++ch)
                sum[static_cast<size_t>(ch)] += (config.reverse ? backwards(v, ch) : forward(v, ch)) * (ch == 0 ? v.left : v.right);
            if (!config.reverse && v.shift != 0)
                for (auto& head : v.heads) { head.read += v.ratio; ++head.phase; }
        }
        for (int ch = 0; ch < channels; ++ch) wet[ch][sample] = static_cast<float>(sum[static_cast<size_t>(ch)]);
        if (++write == capacity) write = 0;
        ++clock;
    }
}
}
