#pragma once

#include <array>
#include <atomic>
#include <vector>

namespace hpeq
{
// ITU-R BS.1770-4 / EBU R128 loudness and true peak, computed on the audio thread (fixed memory,
// allocation-free process()), read lock-free by the UI.
//
// Loudness: K-weighting (stage 1 high shelf + stage 2 high pass, analog prototypes re-derived for any
// rate; identical to the standard's 48 kHz coefficients), channel weights 1 (L, R), mean square over
// 100 ms hops. Momentary = last 4 hops (400 ms), short-term = last 30 hops (3 s),
// L = -0.691 + 10 log10(sum of channel mean squares). Integrated: 400 ms blocks every 100 ms, absolute
// gate -70 LUFS, relative gate -10 LU, from a 0.1 LU histogram of block energies (-70 ... +10 LUFS).
//
// True peak: oversampled to >= 192 kHz (4x below 96 kHz, 2x below 192 kHz, 1x above) with a polyphase
// windowed-sinc interpolator (16 taps per phase, Kaiser beta 9, cutoff 0.48 of the input rate); the
// reported value is max(interpolated peak, sample peak) in dBTP.
class LoudnessMeter
{
public:
    static constexpr double kSilence = -200.0;

    void prepare(double sampleRate, bool measureTruePeak);
    void reset() noexcept;               // audio thread, or before processing starts
    void requestReset() noexcept { resetRequested.store(true, std::memory_order_relaxed); } // any thread

    void process(const float* const* channels, int numChannels, int numSamples) noexcept;

    double momentaryLufs() const noexcept { return momentary.load(std::memory_order_relaxed); }
    double shortTermLufs() const noexcept { return shortTerm.load(std::memory_order_relaxed); }
    double integratedLufs() const noexcept { return integrated.load(std::memory_order_relaxed); }
    double truePeakDbtp() const noexcept { return truePeak.load(std::memory_order_relaxed); }      // max since reset
    double samplePeakDbfs() const noexcept { return samplePeak.load(std::memory_order_relaxed); }  // max since reset

    int oversampling() const noexcept { return factor; }

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        std::array<std::array<double, 2>, 2> z {};
        double process(int ch, double x) noexcept
        {
            auto& s = z[static_cast<size_t>(ch)];
            const double y = b0 * x + s[0];
            s[0] = b1 * x - a1 * y + s[1];
            s[1] = b2 * x - a2 * y;
            return y;
        }
    };

    void endHop() noexcept;
    void updateIntegrated() noexcept;

    static constexpr int kTapsPerPhase = 16;
    static constexpr int kShortHops = 30, kMomentaryHops = 4;
    static constexpr int kHistogramBins = 800; // 0.1 LU from -70 LUFS

    double sampleRate = 48000.0;
    bool truePeakOn = false;
    int factor = 4;
    Biquad shelf, highPass;
    int hopLength = 4800, hopFill = 0;
    std::array<double, 2> hopSum {};
    std::array<double, kShortHops> hops {}; // mean square (summed over channels) of the last hops
    int hopWrite = 0, hopsSeen = 0;
    std::array<long long, kHistogramBins> histCount {};
    std::array<double, kHistogramBins> histEnergy {};

    std::vector<float> phaseTaps;                                      // factor x kTapsPerPhase
    std::array<std::array<float, 2 * kTapsPerPhase>, 2> history {};    // doubled ring per channel
    int historyPos = 0;
    double peakTrue = 0.0, peakSample = 0.0;

    std::atomic<bool> resetRequested { false };
    std::atomic<double> momentary { kSilence }, shortTerm { kSilence }, integrated { kSilence };
    std::atomic<double> truePeak { kSilence }, samplePeak { kSilence };
};
} // namespace hpeq
