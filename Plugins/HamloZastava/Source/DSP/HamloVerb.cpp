#include "DSP/HamloVerb.h"

#include <algorithm>
#include <cmath>
#include <complex>

#include "DSP/DspConfig.h"

namespace hpeq::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kSpeedOfSound = 343.0;
constexpr double kGainSmoothing = 0.05;  // s: decay, damping, levels, freeze
constexpr double kSizeSmoothing = 0.20;  // s: size glides the delay lengths
constexpr double kFilterSmoothing = 0.03;

// Spaces: room for the early field, tank, diffusion, balance, modulation (REVERB_DESIGN.md §2).
//                                          room X  Y     Z     refl  dmin  dmax   ap    inDiff early late  e->l  mod   out
constexpr std::array<verb::SpaceDef, verb::NumSpaces> kSpaces { {
    { 0.0, 0.0, 0.0, 0.0, 31.3, 73.7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 },               // Classic (M4 engine, unused here)
    { 6.0, 4.5, 3.0, 0.78, 9.0, 28.0, 0.11, 0.62, 0.95, 0.70, 0.30, 0.45, 0.68 },        // Room
    { 9.0, 6.0, 4.0, 0.88, 14.0, 41.0, 0.13, 0.72, 0.75, 0.85, 0.40, 0.55, 0.95 },       // Chamber
    { 32.0, 22.0, 13.0, 0.84, 28.0, 83.0, 0.09, 0.66, 0.55, 1.00, 0.35, 0.80, 1.63 },    // Hall
    { 0.0, 0.0, 0.0, 0.0, 8.0, 37.0, 0.17, 0.78, 0.0, 1.00, 0.0, 0.50, 1.25 },           // Plate
    { 60.0, 28.0, 30.0, 0.90, 41.0, 127.0, 0.08, 0.70, 0.45, 1.00, 0.30, 0.90, 2.00 },   // Cathedral
    { 8.0, 6.0, 3.5, 0.74, 6.0, 19.0, 0.12, 0.60, 1.10, 0.45, 0.25, 0.35, 0.79 },        // Ambience
} };

// Small fixed irregularities so no two lines share a ratio; the order interleaves short and long
// lines between the left (even) and right (odd) inputs.
constexpr std::array<double, verb::kLines> kJitter { 0.013, -0.021, 0.027, -0.008, 0.019, -0.026, 0.004, -0.017,
                                                     0.023, -0.011, 0.009, -0.029, 0.016, -0.004, 0.025, -0.014 };
constexpr std::array<int, verb::kLines> kOrder { 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15 };
constexpr std::array<double, verb::kLines> kRateSpread { 0.63, 1.31, 0.87, 1.12, 0.71, 1.37, 0.94, 1.05,
                                                         0.79, 1.24, 0.67, 1.18, 0.99, 0.83, 1.27, 0.74 };
// Rows 1 and 2 of a 16 x 16 Sylvester-Hadamard matrix: two orthogonal output patterns.
constexpr std::array<double, verb::kLines> kOutL { 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1 };
constexpr std::array<double, verb::kLines> kOutR { 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1, 1, -1, -1 };
constexpr std::array<double, verb::kLines> kInSign { 1, 1, -1, 1, 1, -1, -1, -1, 1, -1, 1, 1, -1, 1, -1, -1 };

constexpr std::array<double, 4> kInputDiffMs { 1.42, 2.13, 3.27, 4.91 };
constexpr std::array<double, 4> kEarlyDiffMs { 0.73, 1.19, 1.87, 2.69 };
constexpr std::array<double, 8> kAttackShare { 0.0, 0.09, 0.21, 0.34, 0.48, 0.63, 0.80, 1.0 };

constexpr std::array<double, verb::kSyncDivisions> kSyncBeats { 0.0, 0.0625, 0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.375, 0.5,
                                                                2.0 / 3.0, 0.75, 1.0, 2.0 };

double onePole(double hz, double fs) noexcept
{
    return 1.0 - std::exp(-2.0 * kPi * std::clamp(hz, 1.0, 0.49 * fs) / fs);
}

double smoothing(double seconds, double fs) noexcept
{
    return 1.0 - std::exp(-1.0 / (seconds * fs));
}

// Four-point Hermite read `delay` samples back from the slot about to be written (delay >= 2).
inline double readCubic(const float* line, int size, int writePos, double delay) noexcept
{
    const int whole = static_cast<int>(delay);
    const double f = delay - whole;
    int i0 = writePos - whole;
    if (i0 < 0)
        i0 += size;
    if (f == 0.0)
        return line[i0];
    int im1 = i0 + 1;
    if (im1 >= size)
        im1 -= size;
    int i1 = i0 - 1;
    if (i1 < 0)
        i1 += size;
    int i2 = i1 - 1;
    if (i2 < 0)
        i2 += size;
    const double xm1 = line[im1], x0 = line[i0], x1 = line[i1], x2 = line[i2];
    const double c1 = 0.5 * (x1 - xm1);
    const double c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
    const double c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}

inline double readLinear(const float* line, int size, int writePos, double delay) noexcept
{
    const int whole = static_cast<int>(delay);
    const double f = delay - whole;
    int i0 = writePos - whole;
    if (i0 < 0)
        i0 += size;
    int i1 = i0 - 1;
    if (i1 < 0)
        i1 += size;
    return line[i0] + f * (line[i1] - line[i0]);
}

// In-place fast Walsh-Hadamard transform, scaled to be orthogonal.
inline void hadamard16(std::array<double, verb::kLines>& v) noexcept
{
    for (int h = 1; h < verb::kLines; h <<= 1)
        for (int i = 0; i < verb::kLines; i += h << 1)
            for (int j = i; j < i + h; ++j)
            {
                const double a = v[static_cast<size_t>(j)], b = v[static_cast<size_t>(j + h)];
                v[static_cast<size_t>(j)] = a + b;
                v[static_cast<size_t>(j + h)] = a - b;
            }
    for (auto& x : v)
        x *= 0.25;
}

void designButterworth(double hz, double fs, bool highPass, double& b0, double& b1, double& b2, double& a1, double& a2) noexcept
{
    const double w = 2.0 * kPi * std::clamp(hz, 5.0, 0.45 * fs) / fs;
    const double cw = std::cos(w), alpha = std::sin(w) / (2.0 * 0.7071067811865476);
    const double a0 = 1.0 + alpha;
    if (highPass)
    {
        b0 = (1.0 + cw) * 0.5 / a0;
        b1 = -(1.0 + cw) / a0;
    }
    else
    {
        b0 = (1.0 - cw) * 0.5 / a0;
        b1 = (1.0 - cw) / a0;
    }
    b2 = b0;
    a1 = -2.0 * cw / a0;
    a2 = (1.0 - alpha) / a0;
}

// RBJ shelves with slope S = 1 (monotonic magnitude): `k` = linear gain at DC (low) or Nyquist (high),
// half of it (in dB) at the corner. k = 1 gives b == a exactly, i.e. an exact identity.
struct ShelfCoefficients
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
};

ShelfCoefficients shelf(bool high, double cosW, double sinW, double k) noexcept
{
    const double A = std::sqrt(k), sqrtA = std::sqrt(A);
    const double alpha = sinW * 0.7071067811865476; // sin(w) / 2 * sqrt(2) for S = 1
    const double ap = A + 1.0, am = A - 1.0;
    double b0, b1, b2, a0, a1, a2;
    if (high)
    {
        b0 = A * (ap + am * cosW + 2.0 * sqrtA * alpha);
        b1 = -2.0 * A * (am + ap * cosW);
        b2 = A * (ap + am * cosW - 2.0 * sqrtA * alpha);
        a0 = ap - am * cosW + 2.0 * sqrtA * alpha;
        a1 = 2.0 * (am - ap * cosW);
        a2 = ap - am * cosW - 2.0 * sqrtA * alpha;
    }
    else
    {
        b0 = A * (ap - am * cosW + 2.0 * sqrtA * alpha);
        b1 = 2.0 * A * (am - ap * cosW);
        b2 = A * (ap - am * cosW - 2.0 * sqrtA * alpha);
        a0 = ap + am * cosW + 2.0 * sqrtA * alpha;
        a1 = -2.0 * (am + ap * cosW);
        a2 = ap + am * cosW - 2.0 * sqrtA * alpha;
    }
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

std::complex<double> response(const ShelfCoefficients& c, double w) noexcept
{
    const auto z1 = std::polar(1.0, -w), z2 = std::polar(1.0, -2.0 * w);
    return (c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2);
}

int spaceOf(const FxSlotParams& p) noexcept
{
    return std::clamp(static_cast<int>(std::lround(p.values[slot::kRevSpace])), 1, verb::NumSpaces - 1);
}
} // namespace

namespace verb
{
const char* spaceName(int s) noexcept
{
    static constexpr const char* names[] { "Classic", "Room", "Chamber", "Hall", "Plate", "Cathedral", "Ambience" };
    return names[std::clamp(s, 0, NumSpaces - 1)];
}

const char* colorName(int c) noexcept
{
    static constexpr const char* names[] { "1970", "1980", "NOW" };
    return names[std::clamp(c, 0, 2)];
}

const char* syncName(int d) noexcept
{
    static constexpr const char* names[] { "Free", "1/64", "1/32", "1/16T", "1/16", "1/8T", "1/16D", "1/8", "1/4T", "1/8D", "1/4", "1/2" };
    return names[std::clamp(d, 0, kSyncDivisions - 1)];
}

double syncBeats(int d) noexcept
{
    return kSyncBeats[static_cast<size_t>(std::clamp(d, 0, kSyncDivisions - 1))];
}

const SpaceDef& spaceDef(int s) noexcept
{
    return kSpaces[static_cast<size_t>(std::clamp(s, 0, NumSpaces - 1))];
}

EarlyTaps earlyTaps(int s, int mic)
{
    EarlyTaps out;
    const auto& d = spaceDef(s);
    if (d.roomX <= 0.0)
        return out;
    // Source a little off-centre at the front, microphones in the room behind it, spaced by a fifth of
    // the width (at least 1 m): the two image patterns differ, which is the early field's stereo.
    const double sx = 0.47 * d.roomX, sy = 0.28 * d.roomY, sz = 0.42 * d.roomZ;
    const double spacing = std::max(1.0, 0.2 * d.roomX);
    const double mx = 0.5 * d.roomX + (mic == 0 ? -0.5 : 0.5) * spacing, my = 0.63 * d.roomY, mz = 0.38 * d.roomZ;
    const double direct = std::sqrt((sx - mx) * (sx - mx) + (sy - my) * (sy - my) + (sz - mz) * (sz - mz));
    const auto image = [](int n, double length, double pos) { return n * length + ((n & 1) == 0 ? pos : length - pos); };

    struct Tap { double t, g; };
    std::array<Tap, 128> all {};
    int count = 0;
    for (int nx = -3; nx <= 3; ++nx)
        for (int ny = -3; ny <= 3; ++ny)
            for (int nz = -3; nz <= 3; ++nz)
            {
                const int order = std::abs(nx) + std::abs(ny) + std::abs(nz);
                if (order < 1 || order > 3)
                    continue;
                const double x = image(nx, d.roomX, sx) - mx, y = image(ny, d.roomY, sy) - my, z = image(nz, d.roomZ, sz) - mz;
                const double r = std::sqrt(x * x + y * y + z * z);
                const Tap tap { (r - direct) / kSpeedOfSound, std::pow(d.reflection, order) * direct / r };
                if (count < static_cast<int>(all.size()))
                    all[static_cast<size_t>(count++)] = tap;
                else
                {
                    auto weakest = std::min_element(all.begin(), all.end(), [](const Tap& a, const Tap& b) { return a.g < b.g; });
                    if (tap.g > weakest->g)
                        *weakest = tap;
                }
            }
    std::sort(all.begin(), all.begin() + count, [](const Tap& a, const Tap& b) { return a.g > b.g; });
    out.count = std::min(count, kMaxEarlyTaps);
    double energy = 0.0;
    for (int i = 0; i < out.count; ++i)
        energy += all[static_cast<size_t>(i)].g * all[static_cast<size_t>(i)].g;
    const double norm = energy > 0.0 ? 1.0 / std::sqrt(energy) : 0.0;
    for (int i = 0; i < out.count; ++i)
    {
        out.seconds[static_cast<size_t>(i)] = all[static_cast<size_t>(i)].t;
        out.gain[static_cast<size_t>(i)] = all[static_cast<size_t>(i)].g * norm;
    }
    return out;
}

double decayAt(double hz, const FxSlotParams& p) noexcept
{
    // The magnitude of the loop filter of a typical line (50 ms), turned back into a decay time.
    const double fs = 48000.0;
    const double t60 = p.values[slot::kDecaySeconds];
    const double low = t60 * std::max(0.05, static_cast<double>(p.values[slot::kRevBassMult]));
    const double high = t60 * std::max(0.05, static_cast<double>(p.values[slot::kHfRatio]));
    const double dSamples = 0.05 * fs;
    const double gM = std::pow(10.0, -3.0 * dSamples / (t60 * fs));
    const double kL = std::pow(10.0, -3.0 * dSamples / (low * fs)) / gM, kH = std::pow(10.0, -3.0 * dSamples / (high * fs)) / gM;
    const double wB = 2.0 * kPi * std::clamp(static_cast<double>(p.values[slot::kRevBassXover]), 20.0, 0.45 * fs) / fs;
    const double wH = 2.0 * kPi * std::clamp(static_cast<double>(p.values[slot::kRevHighXover]), 20.0, 0.45 * fs) / fs;
    const double w = 2.0 * kPi * std::clamp(hz, 1.0, 0.49 * fs) / fs;
    const double g = gM * std::abs(response(shelf(false, std::cos(wB), std::sin(wB), kL), w) * response(shelf(true, std::cos(wH), std::sin(wH), kH), w));
    return g >= 1.0 ? 1.0e3 : -3.0 * dSamples / (fs * std::log10(g));
}
} // namespace verb

float HamloVerb::Allpass::process(float x, float g) noexcept
{
    int r = write - length;
    if (r < 0)
        r += static_cast<int>(buffer.size());
    const float d = buffer[static_cast<size_t>(r)];
    const float w = x + g * d;
    buffer[static_cast<size_t>(write)] = w;
    if (++write == static_cast<int>(buffer.size()))
        write = 0;
    return d - g * w;
}

double HamloVerb::Biquad::process(double x, int ch) noexcept
{
    const double y = b0 * x + z1[static_cast<size_t>(ch)];
    z1[static_cast<size_t>(ch)] = b1 * x - a1 * y + z2[static_cast<size_t>(ch)];
    z2[static_cast<size_t>(ch)] = b2 * x - a2 * y;
    return y;
}

void HamloVerb::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    double maxLineMs = 0.0, maxEarly = 0.0;
    for (int s = 1; s < verb::NumSpaces; ++s)
    {
        maxLineMs = std::max(maxLineMs, verb::spaceDef(s).maxDelayMs * 1.04);
        for (int mic = 0; mic < 2; ++mic)
        {
            const auto t = verb::earlyTaps(s, mic);
            for (int i = 0; i < t.count; ++i)
                maxEarly = std::max(maxEarly, t.seconds[static_cast<size_t>(i)]);
        }
    }
    const auto lineSize = static_cast<size_t>(std::ceil((maxLineMs * verb::kMaxSize + verb::kMaxModMs) * 1.0e-3 * sampleRate)) + 8;
    double maxAllpassMs = 0.0;
    for (int s = 1; s < verb::NumSpaces; ++s)
        maxAllpassMs = std::max(maxAllpassMs, verb::spaceDef(s).maxDelayMs * 1.04 * verb::spaceDef(s).allpassShare);
    for (auto& l : lines)
    {
        l.buffer.assign(lineSize, 0.0f);
        l.allpass.assign(static_cast<size_t>(std::ceil(maxAllpassMs * 1.0e-3 * sampleRate)) + 4, 0.0f);
    }
    for (auto& p : predelay)
        p.assign(static_cast<size_t>(std::ceil((verb::kMaxPredelayMs * 1.0e-3 + verb::kMaxAttackSeconds * verb::kMaxSize) * sampleRate)) + 8, 0.0f);
    for (auto& e : early)
        e.assign(static_cast<size_t>(std::ceil(maxEarly * verb::kMaxSize * sampleRate)) + 8, 0.0f);
    for (int ch = 0; ch < 2; ++ch)
        for (size_t i = 0; i < 4; ++i)
        {
            auto& in = inputDiffusers[static_cast<size_t>(ch)][i];
            in.length = std::max(1, static_cast<int>(std::lround(kInputDiffMs[i] * (ch == 0 ? 1.0 : 1.07) * 1.0e-3 * sampleRate)));
            in.buffer.assign(static_cast<size_t>(in.length) + 2, 0.0f);
            auto& ed = earlyDiffusers[static_cast<size_t>(ch)][i];
            ed.length = std::max(1, static_cast<int>(std::lround(kEarlyDiffMs[i] * (ch == 0 ? 1.0 : 1.11) * 1.0e-3 * sampleRate)));
            ed.buffer.assign(static_cast<size_t>(ed.length) + 2, 0.0f);
        }
    reset();
}

void HamloVerb::reset() noexcept
{
    for (auto& l : lines)
    {
        std::fill(l.buffer.begin(), l.buffer.end(), 0.0f);
        std::fill(l.allpass.begin(), l.allpass.end(), 0.0f);
    }
    for (auto& p : predelay)
        std::fill(p.begin(), p.end(), 0.0f);
    for (auto& e : early)
        std::fill(e.begin(), e.end(), 0.0f);
    for (auto* set : { &inputDiffusers, &earlyDiffusers })
        for (auto& ch : *set)
            for (auto& a : ch)
            {
                std::fill(a.buffer.begin(), a.buffer.end(), 0.0f);
                a.write = 0;
            }
    predelayWrite = earlyWrite = lineWrite = allpassWrite = 0;
    lz1 = lz2 = hz1 = hz2 = {};
    colorState = {};
    earlyDampState = {};
    lowCut.z1 = lowCut.z2 = highCut.z1 = highCut.z2 = {};
    duckEnvelope = 0.0;
    duckGain = 1.0;
    controlCounter = 0;
    blockPeak = 0.0f;
    meterOutDb.store(-120.0f, std::memory_order_relaxed);
    meterDuckDb.store(0.0f, std::memory_order_relaxed);
    primed = false;
}

void HamloVerb::configureSpace(int newSpace) noexcept
{
    space = newSpace;
    const auto& d = verb::spaceDef(space);
    for (int m = 0; m < 2; ++m)
    {
        taps[static_cast<size_t>(m)] = verb::earlyTaps(space, m);
        for (int k = 0; k < verb::kMaxEarlyTaps; ++k)
            tapGain[static_cast<size_t>(m)][static_cast<size_t>(k)] = static_cast<float>(taps[static_cast<size_t>(m)].gain[static_cast<size_t>(k)]);
    }
    tapsDirty = true;
    for (int i = 0; i < verb::kLines; ++i)
    {
        auto& l = lines[static_cast<size_t>(i)];
        const double rank = static_cast<double>(kOrder[static_cast<size_t>(i)]) / (verb::kLines - 1);
        const double ms = d.minDelayMs * std::pow(d.maxDelayMs / d.minDelayMs, rank) * (1.0 + kJitter[static_cast<size_t>(i)]);
        l.allpassLength = std::clamp(static_cast<int>(std::lround(ms * d.allpassShare * 1.0e-3 * sampleRate)), 1,
                                     static_cast<int>(l.allpass.size()) - 2);
        const double phase = 2.0 * kPi * std::fmod(0.6180339887 * i, 1.0);
        l.lfoCos = std::cos(phase);
        l.lfoSin = std::sin(phase);
    }
}

double HamloVerb::predelaySeconds(const FxSlotParams& p, double bpm) noexcept
{
    const int sync = std::clamp(static_cast<int>(std::lround(p.values[slot::kRevSync])), 0, verb::kSyncDivisions - 1);
    const double seconds = sync == 0 ? p.values[slot::kPredelayMs] * 1.0e-3 : verb::syncBeats(sync) * 60.0 / std::clamp(bpm, 20.0, 999.0);
    return std::clamp(seconds, 0.0, verb::kMaxPredelayMs * 1.0e-3);
}

double HamloVerb::tailSeconds(const FxSlotParams& p, double bpm) noexcept
{
    const double size = std::clamp(static_cast<double>(p.values[slot::kSize]), verb::kMinSize, verb::kMaxSize);
    const double t60 = p.values[slot::kDecaySeconds] * std::max(1.0, static_cast<double>(p.values[slot::kRevBassMult]));
    const double t = predelaySeconds(p, bpm) + p.values[slot::kRevAttack] * verb::kMaxAttackSeconds * size + t60 + 0.1;
    return p.values[slot::kRevFreeze] >= 0.5f ? std::max(t, 30.0) : t;
}

void HamloVerb::updateTargets(const FxSlotParams& p, const Transport& transport) noexcept
{
    const int wanted = spaceOf(p);
    if (!primed || wanted != space)
        configureSpace(wanted);
    const auto& d = verb::spaceDef(space);
    const double fs = sampleRate;
    const bool freeze = p.values[slot::kRevFreeze] >= 0.5f;
    const int color = std::clamp(static_cast<int>(std::lround(p.values[slot::kRevColor])), 0, 2);

    sizeTarget = std::clamp(static_cast<double>(p.values[slot::kSize]), verb::kMinSize, verb::kMaxSize);
    const double t60 = std::clamp(static_cast<double>(p.values[slot::kDecaySeconds]), 0.1, 30.0);
    const double low = t60 * std::clamp(static_cast<double>(p.values[slot::kRevBassMult]), 0.25, 4.0);
    const double high = t60 * std::clamp(static_cast<double>(p.values[slot::kHfRatio]), 0.05, 1.0);
    for (int i = 0; i < verb::kLines; ++i)
    {
        auto& l = lines[static_cast<size_t>(i)];
        const double rank = static_cast<double>(kOrder[static_cast<size_t>(i)]) / (verb::kLines - 1);
        const double ms = d.minDelayMs * std::pow(d.maxDelayMs / d.minDelayMs, rank) * (1.0 + kJitter[static_cast<size_t>(i)]);
        l.lengthTarget = std::round(ms * sizeTarget * 1.0e-3 * fs);
        const double loop = l.lengthTarget + l.allpassLength; // mean group delay of the allpass = its length
        const double gM = std::pow(10.0, -3.0 * loop / (t60 * fs));
        l.gainMidTarget = freeze ? 1.0 : gM;
        l.lowRatioTarget = freeze ? 1.0 : std::pow(10.0, -3.0 * loop / (low * fs)) / gM;
        l.highRatioTarget = freeze ? 1.0 : std::pow(10.0, -3.0 * loop / (high * fs)) / gM;
        const double rate = std::clamp(static_cast<double>(p.values[slot::kRevModRate]), 0.01, 10.0) * kRateSpread[static_cast<size_t>(i)]
                            * (color == verb::Color1970 ? 0.5 : 1.0);
        l.lfoRotCos = std::cos(2.0 * kPi * rate * kControlInterval / fs);
        l.lfoRotSin = std::sin(2.0 * kPi * rate * kControlInterval / fs);
    }
    const double wB = 2.0 * kPi * std::clamp(static_cast<double>(p.values[slot::kRevBassXover]), 50.0, 2000.0) / fs;
    const double wH = 2.0 * kPi * std::min(std::clamp(static_cast<double>(p.values[slot::kRevHighXover]), 1000.0, 16000.0), 0.45 * fs) / fs;
    if (std::cos(wB) != bassCos || std::cos(wH) != highCos)
    {
        bassCos = std::cos(wB);
        bassSin = std::sin(wB);
        highCos = std::cos(wH);
        highSin = std::sin(wH);
        shelvesDirty = true;
    }

    predelayTarget = HamloVerb::predelaySeconds(p, transport.bpm) * fs;
    attackTarget = std::clamp(static_cast<double>(p.values[slot::kRevAttack]), 0.0, 1.0);
    earlyDiffTarget = std::clamp(static_cast<double>(p.values[slot::kRevEarlyDiff]), 0.0, 1.0);
    lateDiffTarget = std::clamp(static_cast<double>(p.values[slot::kRevLateDiff]), 0.0, 1.0);
    const double colorMod = color == verb::Color1970 ? 1.5 : color == verb::Color1980 ? 1.25 : 1.0;
    // Freeze: the loop must be lossless, and a modulated (interpolated) read low-passes every pass.
    modDepthTarget = freeze ? 0.0 : std::clamp(static_cast<double>(p.values[slot::kRevModDepth]), 0.0, 1.0) * d.modMs * colorMod * 1.0e-3 * fs;
    const double distance = std::clamp(static_cast<double>(p.values[slot::kRevDistance]), 0.0, 1.0);
    earlyGainTarget = d.earlyLevel * (1.6 - 1.2 * distance);
    lateGainTarget = d.lateLevel * (0.6 + 0.8 * distance);
    earlyDamp = onePole(16000.0 * std::pow(0.25, distance), fs);
    widthTarget = std::clamp(static_cast<double>(p.values[slot::kWidth]), 0.0, 1.5);
    inputGateTarget = freeze ? 0.0 : 1.0;
    lowCutTarget = std::clamp(static_cast<double>(p.values[slot::kLowCutHz]), 10.0, 1000.0);
    highCutTarget = std::clamp(static_cast<double>(p.values[slot::kRevHighCut]), 1000.0, 20000.0);
    colorCoefficient = color == verb::Color1970 ? onePole(6500.0, fs) : color == verb::Color1980 ? onePole(10500.0, fs) : 1.0;
    saturation = color == verb::Color1970 && !freeze ? 0.12 : 0.0;
    duckAmount = std::clamp(static_cast<double>(p.values[slot::kRevDuck]), 0.0, 1.0);

    if (!primed)
    {
        size = sizeTarget;
        for (auto& l : lines)
        {
            l.length = l.lengthTarget;
            l.gainMid = l.gainMidTarget;
            l.lowRatio = l.lowRatioTarget;
            l.highRatio = l.highRatioTarget;
        }
        predelaySamples = predelayTarget;
        attack = attackTarget;
        earlyDiff = earlyDiffTarget;
        lateDiff = lateDiffTarget;
        modDepthSamples = modDepthTarget;
        earlyGain = earlyGainTarget;
        lateGain = lateGainTarget;
        width = widthTarget;
        inputGate = inputGateTarget;
        lowCutHz = lowCutTarget;
        highCutHz = highCutTarget;
        updateFilters();
        updateShelves();
        for (int i = 0; i < verb::kLines; ++i)
        {
            const auto& l = lines[static_cast<size_t>(i)];
            delayNext[static_cast<size_t>(i)] = l.length - modDepthSamples * (1.0 + l.lfoSin);
            gMid[static_cast<size_t>(i)] = l.gainMid;
        }
        tapsDirty = true;
        controlCounter = 0;
        primed = true;
    }
}

void HamloVerb::updateFilters() noexcept
{
    designButterworth(lowCutHz, sampleRate, true, lowCut.b0, lowCut.b1, lowCut.b2, lowCut.a1, lowCut.a2);
    highCutOpen = highCutHz >= std::min(19900.0, 0.45 * sampleRate);
    if (!highCutOpen)
        designButterworth(highCutHz, sampleRate, false, highCut.b0, highCut.b1, highCut.b2, highCut.a1, highCut.a2);
}

void HamloVerb::updateShelves() noexcept
{
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const auto lo = shelf(false, bassCos, bassSin, lines[i].lowRatio), hi = shelf(true, highCos, highSin, lines[i].highRatio);
        lb0[i] = lo.b0;
        lb1[i] = lo.b1;
        lb2[i] = lo.b2;
        la1[i] = lo.a1;
        la2[i] = lo.a2;
        hb0[i] = hi.b0;
        hb1[i] = hi.b1;
        hb2[i] = hi.b2;
        ha1[i] = hi.a1;
        ha2[i] = hi.a2;
    }
}

// Control rate (every kControlInterval samples on the running sample clock, so a render does not depend
// on the host block size): the per-line gains, shelves and lengths, size, diffusion, modulation depth,
// levels and output filters glide here, and the early-tap positions are precomputed here.
void HamloVerb::controlTick() noexcept
{
    const auto glide = [](double& v, double target, double k, double snap)
    {
        if (v == target)
            return false;
        v += k * (target - v);
        if (std::abs(target - v) < snap)
            v = target; // freeze reaches exactly 1: a lossless loop
        return true;
    };
    bool shelvesMoved = shelvesDirty;
    glide(modDepthSamples, modDepthTarget, tickGainK, 1.0e-7);
    const int lineSize = static_cast<int>(lines[0].buffer.size());
    for (size_t i = 0; i < lines.size(); ++i)
    {
        auto& l = lines[i];
        glide(l.gainMid, l.gainMidTarget, tickGainK, 1.0e-12);
        gMid[i] = l.gainMid;
        shelvesMoved |= glide(l.lowRatio, l.lowRatioTarget, tickGainK, 1.0e-12);
        shelvesMoved |= glide(l.highRatio, l.highRatioTarget, tickGainK, 1.0e-12);
        glide(l.length, l.lengthTarget, tickSizeK, 1.0e-4);
        // the read delay moves linearly from this tick's value to the next tick's (sine LFO, one
        // rotation per tick); unmodulated and settled it is the integer length exactly (step 0)
        delayNow[i] = delayNext[i];
        double next = l.length;
        if (modDepthSamples > 0.0)
        {
            const double c = l.lfoCos * l.lfoRotCos - l.lfoSin * l.lfoRotSin;
            const double sn = l.lfoSin * l.lfoRotCos + l.lfoCos * l.lfoRotSin;
            const double k = 1.5 - 0.5 * (c * c + sn * sn); // renormalisation: no drift
            l.lfoCos = c * k;
            l.lfoSin = sn * k;
            next -= modDepthSamples * (1.0 + l.lfoSin);
        }
        delayNext[i] = std::clamp(next, 2.0, static_cast<double>(lineSize - 3 - kControlInterval));
        delayStep[i] = (delayNext[i] - delayNow[i]) / kControlInterval;
    }
    if (shelvesMoved)
        updateShelves();
    shelvesDirty = false;

    const bool sizeMoved = glide(size, sizeTarget, tickSizeK, 1.0e-9);
    glide(attack, attackTarget, tickGainK, 1.0e-9);
    glide(earlyDiff, earlyDiffTarget, tickGainK, 1.0e-9);
    glide(lateDiff, lateDiffTarget, tickGainK, 1.0e-9);
    glide(earlyGain, earlyGainTarget, tickGainK, 1.0e-9);
    glide(lateGain, lateGainTarget, tickGainK, 1.0e-9);
    glide(width, widthTarget, tickGainK, 1.0e-9);
    const bool lowMoved = glide(lowCutHz, lowCutTarget, tickFilterK, 1.0e-3);
    const bool highMoved = glide(highCutHz, highCutTarget, tickFilterK, 1.0e-3);
    if (lowMoved || highMoved || filtersDirty)
        updateFilters();
    filtersDirty = false;

    if (sizeMoved || tapsDirty)
    {
        const int earlySize = static_cast<int>(early[0].size());
        for (int m = 0; m < 2; ++m)
            for (int k = 0; k < taps[static_cast<size_t>(m)].count; ++k)
            {
                const double at = std::clamp(taps[static_cast<size_t>(m)].seconds[static_cast<size_t>(k)] * size * sampleRate, 1.0,
                                             static_cast<double>(earlySize - 2));
                tapWhole[static_cast<size_t>(m)][static_cast<size_t>(k)] = static_cast<int>(at);
                tapFrac[static_cast<size_t>(m)][static_cast<size_t>(k)] = static_cast<float>(at - std::floor(at));
            }
    }
    tapsDirty = false;
    attackSpread = attack * verb::kMaxAttackSeconds * size * sampleRate;
}

void HamloVerb::process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams& p,
                        const Transport& transport) noexcept
{
    updateTargets(p, transport);
    const double fs = sampleRate;
    const double sizeK = smoothing(kSizeSmoothing, fs), gainK = smoothing(kGainSmoothing, fs);
    tickGainK = 1.0 - std::pow(1.0 - gainK, kControlInterval);
    tickSizeK = 1.0 - std::pow(1.0 - sizeK, kControlInterval);
    tickFilterK = 1.0 - std::pow(1.0 - smoothing(kFilterSmoothing, fs), kControlInterval);
    const double duckAttack = smoothing(0.002, fs), duckDetectorRelease = smoothing(0.05, fs);
    const double duckGainAttack = smoothing(0.010, fs), duckRelease = smoothing(0.150, fs);
    const auto& d = verb::spaceDef(space);
    const int predelaySize = static_cast<int>(predelay[0].size()), earlySize = static_cast<int>(early[0].size());
    const double outGain = d.outputGain;
    float peak = 0.0f;
    double energy = 0.0, duckSum = 0.0;

    // Attack taps (equal energy, rising): computed per block from the target.
    std::array<double, 8> attackWeight {};
    {
        double e = 0.0;
        for (size_t k = 0; k < attackWeight.size(); ++k)
        {
            const double ramp = std::pow(static_cast<double>(k) / 7.0, 1.5);
            attackWeight[k] = k == 0 ? (1.0 - attackTarget) * (1.0 - attackTarget) + 1.0e-12 : attackTarget * (0.25 + ramp);
            e += attackWeight[k] * attackWeight[k];
        }
        for (auto& w : attackWeight)
            w /= std::sqrt(e);
    }

    for (int n = 0; n < numSamples; ++n)
    {
        if (controlCounter == 0)
            controlTick();
        if (++controlCounter == kControlInterval)
            controlCounter = 0;

        // per-sample controls: the pre-delay glide (a moving read) and the freeze gate
        if (predelaySamples != predelayTarget)
        {
            predelaySamples += sizeK * (predelayTarget - predelaySamples);
            if (std::abs(predelayTarget - predelaySamples) < 1.0e-6)
                predelaySamples = predelayTarget;
        }
        if (inputGate != inputGateTarget)
        {
            inputGate += gainK * (inputGateTarget - inputGate);
            if (std::abs(inputGateTarget - inputGate) < 1.0e-9)
                inputGate = inputGateTarget;
        }

        const double dryL = in[0][n], dryR = numChannels > 1 ? in[1][n] : in[0][n];

        // ducking: a fast peak detector on the dry input; the reduction (full above -6 dBFS, none below
        // -40 dBFS, scaled by the amount) attacks in 10 ms and releases in 150 ms
        const double level = std::max(std::abs(dryL), std::abs(dryR));
        duckEnvelope += (level > duckEnvelope ? duckAttack : duckDetectorRelease) * (level - duckEnvelope);
        double duckTarget = 1.0;
        if (duckAmount > 0.0)
        {
            const double envDb = 20.0 * std::log10(duckEnvelope + 1.0e-9);
            duckTarget = std::pow(10.0, -duckAmount * 30.0 * std::clamp((envDb + 40.0) / 34.0, 0.0, 1.0) / 20.0);
        }
        if (duckGain != duckTarget)
            duckGain += (duckTarget < duckGain ? duckGainAttack : duckRelease) * (duckTarget - duckGain);

        // pre-delay (and the attack taps behind it)
        predelay[0][static_cast<size_t>(predelayWrite)] = static_cast<float>(inputGate * dryL);
        predelay[1][static_cast<size_t>(predelayWrite)] = static_cast<float>(inputGate * dryR);
        const double pre = std::max(0.0, predelaySamples);
        const double pl = readLinear(predelay[0].data(), predelaySize, predelayWrite, pre);
        const double pr = readLinear(predelay[1].data(), predelaySize, predelayWrite, pre);
        double al = attackWeight[0] * pl, ar = attackWeight[0] * pr;
        if (attackTarget > 0.0)
            for (size_t k = 1; k < attackWeight.size(); ++k)
            {
                const double at = std::min(pre + kAttackShare[k] * attackSpread, static_cast<double>(predelaySize - 2));
                al += attackWeight[k] * readLinear(predelay[0].data(), predelaySize, predelayWrite, at);
                ar += attackWeight[k] * readLinear(predelay[1].data(), predelaySize, predelayWrite, at);
            }
        if (++predelayWrite == predelaySize)
            predelayWrite = 0;

        // early field: tap positions precomputed at control rate
        early[0][static_cast<size_t>(earlyWrite)] = static_cast<float>(pl);
        early[1][static_cast<size_t>(earlyWrite)] = static_cast<float>(pr);
        std::array<double, 2> er {};
        const auto ge = static_cast<float>(0.7 * earlyDiff);
        for (int m = 0; m < 2; ++m)
        {
            const auto& t = taps[static_cast<size_t>(m)];
            const float* line = early[static_cast<size_t>(m)].data();
            const auto& whole = tapWhole[static_cast<size_t>(m)];
            const auto& frac = tapFrac[static_cast<size_t>(m)];
            const auto& gain = tapGain[static_cast<size_t>(m)];
            float sum = 0.0f;
            for (int k = 0; k < t.count; ++k)
            {
                int i0 = earlyWrite - whole[static_cast<size_t>(k)];
                if (i0 < 0)
                    i0 += earlySize;
                const int i1 = i0 == 0 ? earlySize - 1 : i0 - 1;
                const float x0 = line[i0];
                sum += gain[static_cast<size_t>(k)] * (x0 + frac[static_cast<size_t>(k)] * (line[i1] - x0));
            }
            float v = sum;
            for (auto& a : earlyDiffusers[static_cast<size_t>(m)])
                v = a.process(v, ge);
            earlyDampState[static_cast<size_t>(m)] += earlyDamp * (v - earlyDampState[static_cast<size_t>(m)]);
            er[static_cast<size_t>(m)] = earlyDampState[static_cast<size_t>(m)];
        }
        if (++earlyWrite == earlySize)
            earlyWrite = 0;

        // tank input: build-up taps plus part of the early field, colour bandwidth, input diffusion
        std::array<double, 2> tankIn { al + d.earlyToLate * er[0], ar + d.earlyToLate * er[1] };
        const auto gi = static_cast<float>(d.inputDiffusion * lateDiff);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto& c = colorState[static_cast<size_t>(ch)];
            c += colorCoefficient * (tankIn[static_cast<size_t>(ch)] - c);
            float v = static_cast<float>(c);
            for (auto& a : inputDiffusers[static_cast<size_t>(ch)])
                v = a.process(v, gi);
            tankIn[static_cast<size_t>(ch)] = v;
        }

        // late field: 16-line FDN (every line buffer has the same size: one write index for all).
        // Reads and allpass taps are gathers; the per-line arithmetic runs over arrays.
        LineArray y {}, apd {}, w {}, z {};
        const double gap = 0.5 * lateDiff;
        const int lineSize = static_cast<int>(lines[0].buffer.size()), apSize = static_cast<int>(lines[0].allpass.size());
        for (size_t i = 0; i < y.size(); ++i)
        {
            auto& l = lines[i];
            y[i] = readCubic(l.buffer.data(), lineSize, lineWrite, delayNow[i]);
            delayNow[i] += delayStep[i];
            int r = allpassWrite - l.allpassLength;
            if (r < 0)
                r += apSize;
            apd[i] = l.allpass[static_cast<size_t>(r)];
        }
        for (size_t i = 0; i < y.size(); ++i)
        {
            // late diffusion (Schroeder allpass in the loop), then the three-band decay: low shelf (DC gain
            // lowRatio), high shelf (Nyquist gain highRatio), mid gain
            w[i] = y[i] + gap * apd[i];
            const double v = apd[i] - gap * w[i];
            const double u = lb0[i] * v + lz1[i];
            lz1[i] = lb1[i] * v - la1[i] * u + lz2[i];
            lz2[i] = lb2[i] * v - la2[i] * u;
            const double h = hb0[i] * u + hz1[i];
            hz1[i] = hb1[i] * u - ha1[i] * h + hz2[i];
            hz2[i] = hb2[i] * u - ha2[i] * h;
            z[i] = gMid[i] * h;
        }
        if (saturation > 0.0)
            for (auto& o : z)
                o /= 1.0 + saturation * o * o;
        double lateL = 0.0, lateR = 0.0;
        for (size_t i = 0; i < z.size(); ++i)
        {
            lines[i].allpass[static_cast<size_t>(allpassWrite)] = static_cast<float>(w[i]);
            lateL += kOutL[i] * z[i];
            lateR += kOutR[i] * z[i];
        }
        hadamard16(z);
        for (size_t i = 0; i < z.size(); ++i)
            lines[i].buffer[static_cast<size_t>(lineWrite)] = static_cast<float>(z[i] + 0.5 * kInSign[i] * tankIn[i & 1]);
        if (++lineWrite == lineSize)
            lineWrite = 0;
        if (++allpassWrite == apSize)
            allpassWrite = 0;
        lateL *= 0.25;
        lateR *= 0.25;

        // output: balance, width, filters, ducking
        double outL = earlyGain * er[0] + lateGain * lateL;
        double outR = earlyGain * er[1] + lateGain * lateR;
        const double mid = 0.5 * (outL + outR), side = 0.5 * (outL - outR) * width;
        outL = mid + side;
        outR = mid - side;
        outL = lowCut.process(outL, 0);
        outR = lowCut.process(outR, 1);
        if (!highCutOpen)
        {
            outL = highCut.process(outL, 0);
            outR = highCut.process(outR, 1);
        }
        const double g = outGain * duckGain;
        outL *= g;
        outR *= g;

        if (numChannels > 1)
        {
            wet[0][n] = static_cast<float>(outL);
            wet[1][n] = static_cast<float>(outR);
        }
        else
            wet[0][n] = static_cast<float>(0.5 * (outL + outR));
        peak = std::max({ peak, std::abs(static_cast<float>(outL)), std::abs(static_cast<float>(outR)) });
        energy += 0.5 * (outL * outL + outR * outR);
        duckSum += duckGain;
    }
    blockPeak = peak;
    if (numSamples > 0)
    {
        meterOutDb.store(static_cast<float>(10.0 * std::log10(energy / numSamples + 1.0e-12)), std::memory_order_relaxed);
        meterDuckDb.store(static_cast<float>(-20.0 * std::log10(std::max(1.0e-6, duckSum / numSamples))), std::memory_order_relaxed);
    }
}
} // namespace hpeq::dsp
