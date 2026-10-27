#include "DSP/Compressor.h"

#include <algorithm>
#include <cmath>

namespace hamlo::dsp
{
void CompressorProcessor::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    reset();
}

void CompressorProcessor::reset() noexcept
{
    scSource = -1;
    fastDb = {};
    slowDb = {};
    memory = {};
    meanSquare30 = {};
    evenMean = {};
    lastOut = {};
    smoothedReductionDb = {};
    meanSquare = {};
    hold = {};
    autoMakeupDb = {};
    meterGainReductionDb.store(0.0f, std::memory_order_relaxed);
    meterInputDb.store(-120.0f, std::memory_order_relaxed);
    meterAutoMakeupDb.store(0.0f, std::memory_order_relaxed);
}

double CompressorProcessor::staticGainReductionDb(double x, double t, double ratio, double kneeDb) noexcept
{
    const double slope = 1.0 / ratio - 1.0; // <= 0
    const double over = x - t;
    if (kneeDb > 0.0 && std::abs(over) <= kneeDb * 0.5)
    {
        const double a = over + kneeDb * 0.5;
        return -slope * a * a / (2.0 * kneeDb);
    }
    return over > 0.0 ? -slope * over : 0.0;
}

namespace
{
// RBJ cookbook biquads (normalised by a0)
void highPass(double fs, double f, double q, double& b0, double& b1, double& b2, double& a1, double& a2) noexcept
{
    const double w = 2.0 * 3.14159265358979323846 * f / fs, c = std::cos(w), alpha = std::sin(w) / (2.0 * q), a0 = 1.0 + alpha;
    b0 = (1.0 + c) / 2.0 / a0;
    b1 = -(1.0 + c) / a0;
    b2 = b0;
    a1 = -2.0 * c / a0;
    a2 = (1.0 - alpha) / a0;
}

void bell(double fs, double f, double gainDb, double q, double& b0, double& b1, double& b2, double& a1, double& a2) noexcept
{
    const double a = std::pow(10.0, gainDb / 40.0), w = 2.0 * 3.14159265358979323846 * f / fs, c = std::cos(w), alpha = std::sin(w) / (2.0 * q);
    const double a0 = 1.0 + alpha / a;
    b0 = (1.0 + alpha * a) / a0;
    b1 = -2.0 * c / a0;
    b2 = (1.0 - alpha * a) / a0;
    a1 = -2.0 * c / a0;
    a2 = (1.0 - alpha / a) / a0;
}
} // namespace

void CompressorProcessor::setSource(int source) noexcept
{
    if (source == scSource)
        return;
    scSource = source;
    for (auto& stages : sc)
        for (int k = 0; k < kScStages; ++k)
        {
            auto& b = stages[static_cast<size_t>(k)];
            b = Biquad {}; // identity, state cleared
            if (source == 1 && k == 0)
                highPass(sampleRate, 100.0, 0.7071, b.b0, b.b1, b.b2, b.a1, b.a2);
            else if (source == 2 && k == 0)
                highPass(sampleRate, 50.0, 0.7071, b.b0, b.b1, b.b2, b.a1, b.a2);
            else if (source == 3 && k < 2)
                highPass(sampleRate, 120.0, 0.7071, b.b0, b.b1, b.b2, b.a1, b.a2);
            else if (source == 3 && k == 2)
                bell(sampleRate, 3000.0, 4.0, 1.0, b.b0, b.b1, b.b2, b.a1, b.a2);
        }
}

float CompressorProcessor::sidechain(int channel, float x) noexcept
{
    auto& stages = sc[static_cast<size_t>(channel)];
    for (auto& b : stages)
        x = b.run(x);
    return x;
}

void CompressorProcessor::process(float* const* channels, int numChannels, int numSamples, const CompressorSettings& p, bool linked) noexcept
{
    const int model = static_cast<int>(std::lround(std::clamp(p.model, 0.0f, 4.0f)));
    const int source = static_cast<int>(std::lround(std::clamp(p.source, 0.0f, 3.0f)));
    setSource(source);
    if (model != 0)
    {
        processModel(channels, numChannels, numSamples, p, linked, model);
        return;
    }

    // Same law and ballistics as documented in the header; the work is split into loops the compiler
    // can vectorise (level, dB conversion, gain computer, dB -> gain), around the one inherently
    // serial step (the attack/release recursion, kept in double).
    const float threshold = p.thresholdDb;
    const float ratio = std::max(1.0f, p.ratio);
    const float knee = std::max(0.0f, p.kneeDb);
    const double makeup = p.makeupDb;
    const double attack = std::exp(-1.0 / (std::max(0.05f, p.attackMs) * 1.0e-3 * sampleRate));
    const double release = std::exp(-1.0 / (std::max(1.0f, p.releaseMs) * 1.0e-3 * sampleRate));
    const float kneeFloorLinear = std::pow(10.0f, (threshold - knee * 0.5f) / 20.0f);
    const float slope = 1.0f / ratio - 1.0f;
    const float halfKnee = knee * 0.5f;
    const float kneeScale = knee > 0.0f ? -slope / (2.0f * knee) : 0.0f;
    constexpr float kDbToLog2 = 0.16609640474436813f; // log2(10) / 20
    const int detectors = linked ? 1 : numChannels;
    const bool rms = p.detector > 0.5f;
    const int character = static_cast<int>(std::lround(std::clamp(p.character, 0.0f, 2.0f)));
    const bool autoMakeup = p.autoMakeup > 0.5f;
    const double rmsCoefficient = std::exp(-1.0 / (0.010 * sampleRate));
    const double holdCoefficient = std::exp(-1.0 / (1.0 * sampleRate));
    const double autoCoefficient = std::exp(-1.0 / (2.0 * sampleRate));
    const double releaseFast = std::exp(-1.0 / (std::max(1.0f, p.releaseMs) * 0.25e-3 * sampleRate));
    const double releaseSlow = std::exp(-1.0 / (std::max(1.0f, p.releaseMs) * 2.0e-3 * sampleRate));

    double peakReduction = 0.0;
    float peakLevel = 0.0f;
    const float punchScale = std::pow(10.0f, threshold / 20.0f);
    for (int start = 0; start < numSamples; start += kChunk)
    {
        const int n = std::min(kChunk, numSamples - start);
        if (source != 0) // the detector hears the shaped copy (docs/COMPRESSOR_MODELS.md); ALL reads the audio
            for (int ch = 0; ch < numChannels; ++ch)
                for (int i = 0; i < n; ++i)
                    scBuffer[static_cast<size_t>(ch)][static_cast<size_t>(i)] = sidechain(ch, channels[ch][start + i]);
        for (int d = 0; d < detectors; ++d)
        {
            // 1. detector level (sample peak; linked: max over channels)
            if (linked)
            {
                std::fill(level.begin(), level.begin() + n, 0.0f);
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const float* x = source != 0 ? scBuffer[static_cast<size_t>(ch)].data() : channels[ch] + start;
                    for (int i = 0; i < n; ++i)
                        level[static_cast<size_t>(i)] = std::max(level[static_cast<size_t>(i)], std::abs(x[i]));
                }
            }
            else
            {
                const float* x = source != 0 ? scBuffer[static_cast<size_t>(d)].data() : channels[d] + start;
                for (int i = 0; i < n; ++i)
                    level[static_cast<size_t>(i)] = std::abs(x[i]);
            }
            if (rms)
            {
                // RMS: one-pole mean square of the (linked) peak square, serial.
                auto& m = meanSquare[static_cast<size_t>(d)];
                for (int i = 0; i < n; ++i)
                {
                    const double l = level[static_cast<size_t>(i)];
                    m = l * l + rmsCoefficient * (m - l * l);
                    level[static_cast<size_t>(i)] = static_cast<float>(std::sqrt(m));
                }
            }
            for (int i = 0; i < n; ++i)
                peakLevel = std::max(peakLevel, level[static_cast<size_t>(i)]);

            // 2. static gain computer in dB (0 below the knee, exactly)
            for (int i = 0; i < n; ++i)
            {
                const float l = level[static_cast<size_t>(i)];
                const float xDb = 20.0f * std::log10(std::max(l, 1.0e-30f));
                const float over = xDb - threshold;
                const float a = over + halfKnee;
                const float inKnee = kneeScale * a * a;
                const float above = -slope * over;
                const float reduction = (knee > 0.0f && std::abs(over) <= halfKnee) ? inKnee : (over > 0.0f ? above : 0.0f);
                target[static_cast<size_t>(i)] = l > kneeFloorLinear ? reduction : 0.0f;
            }

            // 3. branching attack/release recursion in dB (serial)
            auto& smoothed = smoothedReductionDb[static_cast<size_t>(d)];
            auto& held = hold[static_cast<size_t>(d)];
            auto& autoDb = autoMakeupDb[static_cast<size_t>(d)];
            for (int i = 0; i < n; ++i)
            {
                const double t = target[static_cast<size_t>(i)];
                double releaseNow = release;
                if (character == 2)
                {
                    // Glue: program-dependent release (see the header).
                    held = (smoothed > 1.0 ? 1.0 : 0.0) + holdCoefficient * (held - (smoothed > 1.0 ? 1.0 : 0.0));
                    releaseNow = releaseFast + (releaseSlow - releaseFast) * held;
                }
                smoothed = t + (t > smoothed ? attack : releaseNow) * (smoothed - t);
                if (smoothed < 1.0e-9)
                    smoothed = 0.0; // settle exactly, so a recovered compressor is an identity again
                peakReduction = std::max(peakReduction, smoothed);
                reductionDb[static_cast<size_t>(i)] = static_cast<float>(smoothed);
                if (autoMakeup)
                {
                    autoDb = smoothed + autoCoefficient * (autoDb - smoothed);
                    gainDb[static_cast<size_t>(i)] = static_cast<float>(makeup + autoDb - smoothed);
                }
                else
                    gainDb[static_cast<size_t>(i)] = static_cast<float>(makeup - smoothed);
            }

            // 4. dB -> linear gain; 0 dB stays an exact 1
            for (int i = 0; i < n; ++i)
            {
                const float g = gainDb[static_cast<size_t>(i)];
                gain[static_cast<size_t>(i)] = g == 0.0f ? 1.0f : std::exp2(g * kDbToLog2);
            }

            // 5. apply
            for (int ch = linked ? 0 : d; ch < (linked ? numChannels : d + 1); ++ch)
            {
                float* x = channels[ch] + start;
                if (character == 1)
                    for (int i = 0; i < n; ++i)
                    {
                        // Punch: saturation relative to the threshold, before the gain, growing with the
                        // gain reduction (see the header).
                        const float m = 0.35f * std::min(1.0f, reductionDb[static_cast<size_t>(i)] * (1.0f / 12.0f));
                        if (m > 0.0f)
                            x[i] += m * (punchScale * std::tanh(x[i] / punchScale) - x[i]);
                    }
                for (int i = 0; i < n; ++i)
                    x[i] *= gain[static_cast<size_t>(i)];
            }
        }
    }
    meterGainReductionDb.store(static_cast<float>(peakReduction), std::memory_order_relaxed);
    meterInputDb.store(peakLevel > 0.0f ? 20.0f * std::log10(peakLevel) : -120.0f, std::memory_order_relaxed);
    meterAutoMakeupDb.store(static_cast<float>(autoMakeup ? autoMakeupDb[0] : 0.0), std::memory_order_relaxed);
}

void CompressorProcessor::processModel(float* const* channels, int numChannels, int numSamples, const CompressorSettings& p, bool linked, int model) noexcept
{
    // Per sample (the FET feedback needs the previous output). See the header for every law.
    constexpr int kVca = 1, kFet = 2, kOpto = 3, kTube = 4;
    const double fs = sampleRate;
    const auto coefficient = [fs](double seconds) { return std::exp(-1.0 / (std::max(1.0e-6, seconds) * fs)); };
    const double threshold = p.thresholdDb;
    const double ratio = std::max(1.0f, p.ratio);
    const double knee = model == kOpto ? std::max(12.0, static_cast<double>(p.kneeDb)) : std::max(0.0, static_cast<double>(p.kneeDb));
    const double width = std::max(18.0, static_cast<double>(p.kneeDb)); // TUBE: the span the ratio rises over
    const double releaseSeconds = std::max(1.0f, p.releaseMs) * 1.0e-3;
    const double attack = coefficient(std::max(0.05f, p.attackMs) * 1.0e-3);
    const double release = coefficient(releaseSeconds);
    const double slowCharge = coefficient(model == kVca ? 0.3 : 0.5);
    const double slowReleaseVca = coefficient(5.0 * releaseSeconds);
    const double memoryCoefficient = coefficient(5.0), holdCoefficient = coefficient(1.0), autoCoefficient = coefficient(2.0);
    const double rmsCoefficient = coefficient(model == kOpto ? 0.005 : 0.010), rms30 = coefficient(0.030);
    const double meanCoefficient = std::exp(-2.0 * 3.14159265358979323846 * 20.0 / fs);
    const bool rms = model == kOpto || model == kTube || p.detector > 0.5f;
    const bool samples = scSource == 2;
    const bool autoMakeup = p.autoMakeup > 0.5f;
    const double makeup = p.makeupDb;
    const double k2 = model == kFet ? 0.03 : model == kOpto ? 0.01 : model == kTube ? 0.04 : 0.0;
    const double k3 = model == kFet ? 0.08 : model == kTube ? 0.02 : 0.0;
    const double scale = std::pow(10.0, threshold / 20.0);
    const int detectors = linked ? 1 : numChannels;

    // static gain reduction (dB) for a detector level (dB)
    const auto computer = [&](double xDb)
    {
        const double over = xDb - threshold;
        if (model == kTube)
        {
            if (over <= 0.0)
                return 0.0;
            const double r1 = ratio - 1.0;
            if (r1 <= 0.0)
                return 0.0;
            const double u = std::min(over, width);
            double y = width / r1 * std::log1p(r1 * u / width); // integral of the slope 1 / r(u)
            if (over > width)
                y += (over - width) / ratio;
            return over - y;
        }
        if (model == kFet)
        {
            // written on the output level: c = (y - T)(R - 1) above the knee (soft knee as the law's shape)
            if (knee > 0.0 && std::abs(over) <= knee * 0.5)
            {
                const double a = over + knee * 0.5;
                return (ratio - 1.0) * a * a / (2.0 * knee);
            }
            return over > 0.0 ? (ratio - 1.0) * over : 0.0;
        }
        return staticGainReductionDb(xDb, threshold, ratio, knee);
    };

    double peakReduction = 0.0;
    float peakLevel = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        for (int d = 0; d < detectors; ++d)
        {
            const int first = linked ? 0 : d, last = linked ? numChannels : d + 1;
            // 1. what the detector hears: the input (FET: the previous output), through the source shaping
            double s = 0.0;
            for (int ch = first; ch < last; ++ch)
            {
                const float heard = model == kFet ? static_cast<float>(lastOut[static_cast<size_t>(ch)]) : channels[ch][i];
                const float shaped = scSource != 0 ? sidechain(ch, heard) : heard;
                s = std::max(s, static_cast<double>(std::abs(shaped)));
            }
            const auto di = static_cast<size_t>(d);
            double levelLinear = s;
            if (rms)
            {
                auto& m = meanSquare[di];
                m = s * s + rmsCoefficient * (m - s * s);
                levelLinear = std::sqrt(m);
            }
            if (samples)
            {
                auto& m = meanSquare30[di];
                m = s * s + rms30 * (m - s * s);
                levelLinear = 0.5 * (levelLinear + std::sqrt(m));
            }
            peakLevel = std::max(peakLevel, static_cast<float>(levelLinear));
            const double want = levelLinear > 0.0 ? computer(20.0 * std::log10(levelLinear)) : 0.0;

            // 2. ballistics
            auto& c = smoothedReductionDb[di];
            if (model == kVca)
            {
                auto& f = fastDb[di];
                auto& sl = slowDb[di];
                f = want + (want > f ? attack : release) * (f - want);
                sl = want + (want > sl ? slowCharge : slowReleaseVca) * (sl - want);
                if (f < 1.0e-9)
                    f = 0.0;
                if (sl < 1.0e-9)
                    sl = 0.0;
                c = std::max(f, sl);
            }
            else if (model == kFet)
            {
                const double releaseNow = want > c ? attack : coefficient(releaseSeconds / (1.0 + c / 10.0));
                c = want + releaseNow * (c - want);
            }
            else if (model == kOpto)
            {
                auto& f = fastDb[di];
                auto& sl = slowDb[di];
                auto& mem = memory[di];
                f = want + (want > f ? attack : release) * (f - want);
                const double held = f > 1.0 ? 1.0 : 0.0;
                mem = held + memoryCoefficient * (mem - held);
                sl = f + (f > sl ? slowCharge : coefficient(2.0 * (1.0 + 2.0 * mem))) * (sl - f);
                if (f < 1.0e-9)
                    f = 0.0;
                if (sl < 1.0e-9)
                    sl = 0.0;
                c = std::max(f, 0.5 * sl);
            }
            else // kTube
            {
                auto& h = hold[di];
                const double held = c > 1.0 ? 1.0 : 0.0;
                h = held + holdCoefficient * (h - held);
                c = want + (want > c ? attack : coefficient(releaseSeconds * (1.0 + 7.0 * h))) * (c - want);
            }
            if (c < 1.0e-9)
                c = 0.0;
            peakReduction = std::max(peakReduction, c);

            double gainNowDb = makeup - c;
            if (autoMakeup)
            {
                auto& a = autoMakeupDb[di];
                a = c + autoCoefficient * (a - c);
                gainNowDb += a;
            }
            const double g = gainNowDb == 0.0 ? 1.0 : std::pow(10.0, gainNowDb / 20.0);

            // 3. gain, then the model's colour (an identity without gain reduction)
            const double m = std::min(1.0, c / 12.0);
            for (int ch = first; ch < last; ++ch)
            {
                const auto ci = static_cast<size_t>(ch);
                double y = channels[ch][i] * g;
                const double u = y / scale;
                const double e = u * u / (1.0 + std::abs(u));
                const double mean = evenMean[ci];
                evenMean[ci] = e + meanCoefficient * (mean - e);
                if (m > 0.0 && (k2 > 0.0 || k3 > 0.0))
                    y += scale * m * (k2 * (e - mean) + k3 * (std::tanh(u) - u));
                channels[ch][i] = static_cast<float>(y);
                lastOut[ci] = y;
            }
        }
    }
    meterGainReductionDb.store(static_cast<float>(peakReduction), std::memory_order_relaxed);
    meterInputDb.store(peakLevel > 0.0f ? 20.0f * std::log10(peakLevel) : -120.0f, std::memory_order_relaxed);
    meterAutoMakeupDb.store(static_cast<float>(autoMakeup ? autoMakeupDb[0] : 0.0), std::memory_order_relaxed);
}
} // namespace hamlo::dsp
