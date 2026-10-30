#include "DSP/ReflectionPrototype.h"
#include <algorithm>
#include <cmath>

namespace hamlo::echo
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr int taps = 64, phases = 512, radius = taps / 2;
constexpr int attackTaps = 96, attackRadius = attackTaps / 2;
double bessel0(double x)
{
    double term = 1, sum = 1;
    for (int k = 1; k < 40; ++k)
    {
        term *= x * x / (4.0 * k * k);
        sum += term;
        if (term < 1e-16 * sum) break;
    }
    return sum;
}
struct Kernels
{
    std::array<double, 49> ratio;
    std::vector<float> coefficients;
    std::vector<float> attackCoefficients;
    Kernels() : coefficients(static_cast<size_t>(49 * (phases + 1) * taps)),
                attackCoefficients(static_cast<size_t>(49 * (phases + 1) * attackTaps))
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
                double attackSum = 0;
                auto* attackRow = attackCoefficients.data() + ((s + 24) * (phases + 1) + phase) * attackTaps;
                for (int k = 0; k < attackTaps; ++k)
                {
                    const double x = k - attackRadius + 1 - static_cast<double>(phase) / phases;
                    const double support = std::abs(x) / attackRadius;
                    const double sinc = std::abs(x) < 1e-12 ? 2 * cutoff : std::sin(2 * pi * cutoff * x) / (pi * x);
                    const double w = support <= 1 ? bessel0(9 * std::sqrt(std::max(0.0, 1 - support * support))) / bessel0(9) : 0;
                    attackRow[k] = static_cast<float>(sinc * w); attackSum += attackRow[k];
                }
                for (int k = 0; k < attackTaps; ++k) attackRow[k] = static_cast<float>(attackRow[k] / attackSum);
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
    grain = std::max(8, static_cast<int>(std::lround(.08 * rate)) / 4 * 4);
    search = static_cast<int>(std::lround(.02 * rate));
    capacity = static_cast<int>(std::ceil(6.5 * rate)) + taps + 8;
    for (auto& channel : history) channel.assign(static_cast<size_t>(2 * capacity), 0.0f);
    attackMask.assign(static_cast<size_t>(2 * capacity), 0.0f);
    activity.assign(static_cast<size_t>(2 * capacity), 0.0f);
    fullyMaskedRun.assign(static_cast<size_t>(capacity), 0);
    fullySilentRun.assign(static_cast<size_t>(capacity),0);silentRun=0;
    window.resize(static_cast<size_t>(grain));
    for (int i = 0; i < grain; ++i) window[static_cast<size_t>(i)] = .5 - .5 * std::cos(2 * pi * i / grain);
    voices = {};
    attackCount = 0;
    pending = false;
    lastDetection = lastSignal = -1000000000;
    maskFullEnd = maskEnd = -1;
    pendingPeak = slowPeak = envelope = 0;
    slowCoefficient = 1 - std::exp(-1 / (.02 * rate));
    release = std::exp(-1 / (.01 * rate));
    capture = static_cast<int>(std::ceil(.08 * rate));
    write = 0; clock = 0;
    validFrom = 0;
    setScore(score);
}

void ReflectionPrototype::resetEpoch() noexcept
{
    // No clearing of large audio buffers on the audio thread. Reads before this
    // epoch are explicitly zero, including partial interpolation windows.
    clock=0;write=0;validFrom=0;silentRun=0;
    voices = {};
    attackCount = 0; pending = false;
    maskFullEnd = maskEnd = -1;
    lastDetection = lastSignal = -1000000000;
    pendingPeak = slowPeak = envelope = 0;
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
        v.effectiveDelay = shift == 0 ? v.delay : std::max(v.delay, capture);
        const int length = v.effectiveDelay >= radius + 2 + std::abs(v.ratio - 1) * grain / 2 ? grain : grain / 2;
        if (length != v.windowLength) v.initialised = false;
        v.windowLength = length;
        v.windowOffset = static_cast<int>(i) * length / 8;
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
    // Worst additional wait at a short requested delay; the actual delay is max(D,80ms).
    return capture;
}

int ReflectionPrototype::reverseAccumulationSamples(int tap) const noexcept { return voices[static_cast<size_t>(std::clamp(tap, 0, 7))].reverseLength; }
size_t ReflectionPrototype::historyBytes() const noexcept { return static_cast<size_t>(8 * capacity) * sizeof(float) + static_cast<size_t>(capacity) * sizeof(uint16_t) + window.size() * sizeof(double); }
size_t ReflectionPrototype::sharedTableBytes() noexcept { return static_cast<size_t>(49 * (phases + 1) * (taps + attackTaps)) * sizeof(float); }

float ReflectionPrototype::integerAt(int channel, int64_t position) const noexcept
{
    if (position < validFrom || position > clock || position <= clock - capacity) return 0.0f;
    return history[static_cast<size_t>(channel)][static_cast<size_t>(position % capacity)];
}

double ReflectionPrototype::interpolate(int channel, double position, int shift, bool attack, int64_t first, int64_t end) const noexcept
{
    const int r = attack ? attackRadius : radius;
    const int count = attack ? attackTaps : taps;
    if (position < static_cast<double>(validFrom - (attack ? r : 0)) || position > static_cast<double>(clock - r - 1)
        || position < static_cast<double>(clock - capacity + count)) return 0.0;
    const auto whole = static_cast<int64_t>(std::floor(position));
    const double phase = (position - static_cast<double>(whole)) * phases;
    const int p = std::clamp(static_cast<int>(phase), 0, phases - 1);
    const double fraction = phase - p;
    int index = static_cast<int>((whole - r + 1) % capacity);
    if (index < 0) index += capacity;
    if(silentRun>=count)
    {int last=index+count-1;if(last>=capacity)last-=capacity;if(fullySilentRun[static_cast<size_t>(last)]>=count)return 0;}
    const float* x = history[static_cast<size_t>(channel)].data() + index;
    const float* mask = attackMask.data() + index;
    const float* a = (attack ? kernels().attackCoefficients.data() : kernels().coefficients.data()) + ((shift + 24) * (phases + 1) + p) * count;
    const float* b = a + count;
    double sum = 0;
    const int epochBegin = static_cast<int>(std::clamp<int64_t>(validFrom - (whole - r + 1), 0, count));
    // Every nonzero mask interval lasts at least 14 ms, including retroactive
    // marking. At >=8 kHz it cannot fit between two zero endpoints of 64 taps.
    // Ordinary body reads can therefore use the original unmasked inner loop.
    if (!attack && epochBegin == 0 && rate >= 8000 && mask[0] == 0 && mask[count - 1] == 0)
    {
        for (int k = 0; k < count; ++k) sum += x[k] * (a[k] + fraction * (b[k] - a[k]));
        return sum;
    }
    if (!attack)
    {
        int last = index + count - 1;
        if (last >= capacity) last -= capacity;
        if (epochBegin == 0 && fullyMaskedRun[static_cast<size_t>(last)] >= count) return 0;
    }
    else
    {
        const auto begin = static_cast<int>(std::clamp<int64_t>(first - (whole - r + 1), 0, count));
        const auto finish = static_cast<int>(std::clamp<int64_t>(end - (whole - r + 1), 0, count));
        if (finish <= begin) return 0;
        int last = index + finish - 1;
        if (last >= capacity) last -= capacity;
        if (fullyMaskedRun[static_cast<size_t>(last)] >= finish - begin)
        {
            for (int k = begin; k < finish; ++k) sum += x[k] * (a[k] + fraction * (b[k] - a[k]));
            return sum;
        }
        for (int k = begin; k < finish; ++k) sum += x[k] * static_cast<double>(mask[k]) * (a[k] + fraction * (b[k] - a[k]));
        return sum;
    }
    for (int k = epochBegin; k < count; ++k)
    {
        const int64_t source = whole - r + 1 + k;
        const double weight = attack ? (source >= first && source < end ? mask[k] : 0.0) : 1.0 - mask[k];
        sum += x[k] * weight * (a[k] + fraction * (b[k] - a[k]));
    }
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
    if (base - search - length < validFrom || other - length < validFrom) return candidate;
    if(silentRun>=length&&other<=clock&&other>clock-capacity&&fullySilentRun[static_cast<size_t>(other%capacity)]>=length)
        return static_cast<double>(base)+continuing-static_cast<double>(other);
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
    const double base = (v.ratio - 1.0) * v.windowLength / 2;
    for (int h = 0; h < 2; ++h)
    {
        auto& head = v.heads[static_cast<size_t>(h)];
        // Spread splice searches over the sample clock instead of stacking eight
        // expensive restarts in the same host block. Block boundaries play no role.
        head.phase = (h * v.windowLength / 2 + v.windowOffset) % v.windowLength;
        head.read = static_cast<double>(clock - v.effectiveDelay) - base + (v.ratio - 1.0) * head.phase;
        head.active = true;
    }
    v.initialised = true;
}

double ReflectionPrototype::forward(Voice& v, int channel) const noexcept
{
    if (v.shift == 0) return integerAt(channel, clock - v.delay);
    double out = 0;
    for (const auto& head : v.heads)
        out += window[static_cast<size_t>(head.phase * (grain / v.windowLength))] * interpolate(channel, head.read, v.shift);
    return out * bodyGain(v) + attackOutput(v, channel);
}

void ReflectionPrototype::detectAttack(double peak) noexcept
{
    const bool quiet = envelope < 1e-6 || clock - lastSignal >= static_cast<int64_t>(std::ceil(.002 * rate));
    const bool onset = peak > 1e-7 && (quiet || (peak > .005 && peak > 4 * slowPeak && peak > 1.8 * envelope));
    if (!pending && onset && clock - lastDetection >= static_cast<int64_t>(std::lround(.008 * rate)))
    {
        pending = true;
        pendingAttack.first = std::max<int64_t>(validFrom, clock - attackRadius);
        // Seal the preceding fragment before a nearby new attack, avoiding reuse
        // of the next transient by two different pitched event readers.
        if (attackCount != 0)
        {
            auto& previous = attackEvents[static_cast<size_t>((attackCount - 1) % attackEvents.size())];
            previous.end = std::min(previous.end, pendingAttack.first);
        }
        pendingAttack.peak = clock;
        pendingAttack.end = clock + static_cast<int64_t>(std::lround(.014 * rate));
        pendingEnd = clock + static_cast<int64_t>(std::lround(.006 * rate));
        pendingPeak = peak;
        lastDetection = clock;
        maskFullEnd = clock + static_cast<int64_t>(std::lround(.010 * rate));
        maskEnd = pendingAttack.end;
        // The capture budget lets us split the few samples before detection too.
        for (int j = 1; j <= attackRadius && clock - j >= validFrom; ++j)
        {
            int index = write - j;
            if (index < 0) index += capacity;
            attackMask[static_cast<size_t>(index)] = attackMask[static_cast<size_t>(index + capacity)] = 1;
        }
        // Repair the short retroactive range in chronological order. Queries can
        // then skip a wholly extracted body window with one exact run-length read.
        for (int j = static_cast<int>(std::min<int64_t>(attackRadius, clock - validFrom)); j >= 1; --j)
        {
            int index = write - j;
            if (index < 0) index += capacity;
            const int before = index == 0 ? capacity - 1 : index - 1;
            fullyMaskedRun[static_cast<size_t>(index)] = static_cast<uint16_t>(std::min(65535, 1 + fullyMaskedRun[static_cast<size_t>(before)]));
        }
    }
    if (pending)
    {
        if (peak > pendingPeak) { pendingPeak = peak; pendingAttack.peak = clock; }
        if (clock >= pendingEnd)
        {
            attackEvents[static_cast<size_t>(attackCount % attackEvents.size())] = pendingAttack;
            ++attackCount;
            pending = false;
        }
    }
    double weight = 0;
    if (clock < maskFullEnd) weight = 1;
    else if (clock < maskEnd) weight = .5 + .5 * std::cos(pi * static_cast<double>(clock - maskFullEnd) / static_cast<double>(maskEnd - maskFullEnd));
    attackMask[static_cast<size_t>(write)] = attackMask[static_cast<size_t>(write + capacity)] = static_cast<float>(weight);
    const int before = write == 0 ? capacity - 1 : write - 1;
    fullyMaskedRun[static_cast<size_t>(write)] = weight == 1 ? static_cast<uint16_t>(std::min(65535, 1 + fullyMaskedRun[static_cast<size_t>(before)])) : 0;
    envelope = std::max(peak, envelope * release);
    if (peak > 1e-7) lastSignal = clock;
    slowPeak += slowCoefficient * (peak - slowPeak);
    const float active = envelope > 1e-6 ? 1.0f : 0.0f;
    activity[static_cast<size_t>(write)] = activity[static_cast<size_t>(write + capacity)] = active;
}

void ReflectionPrototype::updateAttacks(Voice& v) noexcept
{
    if (v.nextAttack + attackEvents.size() < attackCount) v.nextAttack = attackCount - attackEvents.size();
    while (v.nextAttack < attackCount)
    {
        const auto& event = attackEvents[static_cast<size_t>(v.nextAttack % attackEvents.size())];
        const int64_t due = event.peak + v.effectiveDelay;
        const int lead = std::max(static_cast<int>(std::ceil((event.peak - event.first + attackRadius) / v.ratio)), static_cast<int>(std::ceil(.002 * rate)));
        if (clock < due - lead) break;
        ++v.nextAttack;
        v.heardAttack = true;
        const int64_t finish = due + static_cast<int64_t>(std::ceil((event.end - event.peak + attackRadius) / v.ratio));
        if (clock > finish) continue;
        for (auto& reader : v.attacks)
            if (!reader.active)
            {
                reader = { event, due, finish, v.ratio, v.shift, true };
                ++v.activeAttacks;
                v.heardAttack = true;
                break;
            }
    }
    if (v.activeAttacks != 0)
        for (auto& reader : v.attacks)
            if (reader.active && clock > reader.finish)
            {
                reader.active = false;
                --v.activeAttacks;
            }
}

double ReflectionPrototype::attackOutput(const Voice& v, int channel) const noexcept
{
    double out = 0;
    if (v.activeAttacks != 0)
        for (const auto& reader : v.attacks)
            if (reader.active)
            {
                const double position = reader.event.peak + reader.ratio * static_cast<double>(clock - reader.due);
                out += interpolate(channel, position, reader.shift, true, reader.event.first, reader.event.end);
            }
    return out;
}

double ReflectionPrototype::bodyGain(const Voice& v) const noexcept
{
    const int64_t sourceTime = clock - v.effectiveDelay;
    if (!v.heardAttack || sourceTime < validFrom || sourceTime <= clock - capacity) return 0;
    double gain = activity[static_cast<size_t>(sourceTime % capacity)];
    if (v.activeAttacks != 0)
        for (const auto& reader : v.attacks)
            if (reader.active)
            {
                const double end = reader.due + std::max(0.0, (reader.event.end - .004 * rate - reader.event.peak) / reader.ratio);
                double weight = 0;
                if (clock < reader.due) weight = std::clamp((reader.due - clock) / (.002 * rate), 0.0, 1.0);
                else weight = std::clamp((clock - end) / (.005 * rate), 0.0, 1.0);
                gain = std::min(gain, weight);
            }
    return gain;
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
    processInternal(input, wet, nullptr, channels, samples);
}

void ReflectionPrototype::processVoices(const float* const* input, float* const* const* separate, int channels, int samples) noexcept
{
    processInternal(input, nullptr, separate, channels, samples);
}

void ReflectionPrototype::processInternal(const float* const* input, float* const* wet, float* const* const* separate, int channels, int samples) noexcept
{
    channels = std::clamp(channels, 1, 2);
    for (int sample = 0; sample < samples; ++sample)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            auto& ring = history[static_cast<size_t>(ch)];
            ring[static_cast<size_t>(write)] = ring[static_cast<size_t>(write + capacity)] = input[ch < channels ? ch : 0][sample];
        }
        double peak = 0;bool silent=true;
        for (int ch = 0; ch < channels; ++ch){const float value=input[ch][sample];peak=std::max(peak,std::abs(static_cast<double>(value)));silent&=value==0;}
        silentRun=silent?std::min(capacity,silentRun+1):0;fullySilentRun[static_cast<size_t>(write)]=silentRun;
        detectAttack(peak);
        std::array<double, 2> sum {};
        if (separate != nullptr)
            for (int tap = 0; tap < 8; ++tap)
                for (int ch = 0; ch < channels; ++ch) separate[tap][ch][sample] = 0;
        for (size_t tap = 0; tap < voices.size(); ++tap)
        {
            const auto& config = score.taps[tap];
            if (!config.enabled) continue;
            auto& v = voices[tap];
            if (!config.reverse && v.shift != 0)
            {
                updateAttacks(v);
                if (!v.initialised) initialise(v);
                for (int h = 0; h < 2; ++h)
                {
                    auto& head = v.heads[static_cast<size_t>(h)];
                    if (head.phase == v.windowLength)
                    {
                        head.phase = 0;
                        const double base = static_cast<double>(clock - v.effectiveDelay) - (v.ratio - 1.0) * v.windowLength / 2;
                        head.read = matchedStart(base, v.heads[static_cast<size_t>(1 - h)].read, channels);
                    }
                }
            }
            for (int ch = 0; ch < channels; ++ch)
            {
                const double value = (config.reverse ? backwards(v, ch) : forward(v, ch)) * (ch == 0 ? v.left : v.right);
                sum[static_cast<size_t>(ch)] += value;
                if (separate != nullptr) separate[tap][ch][sample] = static_cast<float>(value);
            }
            if (!config.reverse && v.shift != 0)
                for (auto& head : v.heads) { head.read += v.ratio; ++head.phase; }
        }
        if (wet != nullptr)
            for (int ch = 0; ch < channels; ++ch) wet[ch][sample] = static_cast<float>(sum[static_cast<size_t>(ch)]);
        if (++write == capacity) write = 0;
        ++clock;
    }
}
}
