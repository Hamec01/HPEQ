#pragma once

#include <array>
#include <atomic>

#include "DSP/DspConfig.h"
#include "DSP/EqDesign.h"
#include "DSP/LayerSnapshot.h"
#include "DSP/Ramp.h"

namespace hpeq::dsp
{
// PRE-EQ of one layer (decision B: always before the FX nodes), M3 scope: all shapes, Low/High Cut
// 6...96 dB/oct, channel modes Stereo / Left / Right / Mid / Side (Linked Stereo and Dual Mono equal
// Stereo for a linear EQ). Mid/Side use M = (L+R)/2, S = (L-R)/2, L = M+S, R = M-S.
//
// All sections are TPT filters (stable under fast modulation). Frequency, gain and Q glide on the
// control clock. A band never switches abruptly:
//  - gain shapes (Bell, shelves, Tilt) fade their gain from/to 0 dB and are skipped at exactly 0 dB;
//  - Low Cut and All Pass enter from / retire to 1 Hz, where they are numerically an identity;
//  - High Cut, Notch and Band Pass crossfade wet/dry over kMixRampSeconds;
//  - a change of shape, slope or channel mode crossfades the old and the new filter over
//    kStructureCrossfadeSeconds.
// No running band at all -> bit-exact pass-through (T01).
//
// EQ pass: DYNAMIC points (gain shapes) add an offset to their gain from a detector on their own region
// (band pass at the point for Bell, low / high pass at f for the shelves, broadband for Tilt; channels
// linked by the larger level): level over threshold, 6 dB soft knee, (1 - 1/ratio), limited to |range|,
// down for a negative range, up for a positive one; attack / release on the detector. The offset rides
// on the control clock like the gain itself. DETECT VOICE (a de-esser on the point): the detector level is
// the region's level over the voice body's (band pass 1 kHz, Q 0.6: 300 Hz - 3 kHz) minus 20 dB, so the
// threshold reads the same way (-30 dB = the region 10 dB under the voice), with a floor at -60 dBFS so
// quiet hiss never triggers it. SOLO: after all bands the signal goes through a filter on
// the soloed point (band pass at f and Q; low pass for a low shelf / low cut, high pass for a high shelf /
// high cut), crossfaded in and out over 10 ms. Without either, the processing is unchanged.
class PreEqProcessor
{
public:
    static constexpr double kStructureCrossfadeSeconds = 0.030;
    static constexpr double kParkingHz = 1.0;
    static constexpr double kBirthSeconds = 0.060;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setTargets(const std::array<EqBandParams, kMaxEqNodesPerLayer>&) noexcept;
    void process(float* const* channels, int numChannels, int numSamples, int controlPhase) noexcept;
    bool isNeutral() const noexcept;

    // Exact response of the band as currently configured (for tests and the M5 display).
    BandDesign currentDesign(int band) const noexcept;
    // The gain a DYNAMIC point adds right now (dB, 0 when static), for the page's ring (UI thread).
    float dynamicOffsetDb(int band) const noexcept { return shownOffset[static_cast<size_t>(band)].load(std::memory_order_relaxed); }
    bool isSoloing() const noexcept { return soloBand >= 0 || soloMix > 0.0; }

private:
    enum class Entry { Gain, Park, Wet };

    // Coefficients are recomputed on the control clock and interpolated linearly per sample in
    // between (g, k, m0..m2): stepping them every 32 samples leaks zipper products straight into an
    // SVF's output (M3 lab: -72.9 dB stepped vs -150.6 dB per sample).
    struct RuntimeSection
    {
        SectionSpec spec, step, target;
        int remaining = 0;
        double a1 = 1.0, a2 = 0.0, a3 = 0.0, onePoleG = 0.0;
        std::array<std::array<double, 2>, kMaxChannels> state {};

        void setSpec(const SectionSpec&, bool interpolate) noexcept;
        void advance() noexcept;
        void derive() noexcept;
        double process(int ch, double x) noexcept;
    };

    struct FilterSet
    {
        EqShape shape = EqShape::Bell;
        int slope = 12;
        ChannelMode mode = ChannelMode::Stereo;
        int count = 0;
        bool fresh = true; // next coefficient update jumps instead of interpolating
        std::array<RuntimeSection, kMaxEqSections> sections;

        void reset() noexcept;
        void advance() noexcept;
        double run(int ch, double x) noexcept;
        void apply(double& l, double& r, int numChannels) noexcept; // channel-mode routing
    };

    struct Band
    {
        bool wanted = false, running = false, crossfading = false;
        EqBandParams target;
        int active = 0;
        std::array<FilterSet, 2> sets;
        LinearRamp logFrequency, gainDb, q; // control rate
        LinearRamp wet, crossfade;          // sample rate
        // DYNAMIC: detector filter (TPT SVF) and envelope, the gain offset in dB
        double env = 0.0, dynOffset = 0.0, detG = 0.0, detK = 1.0, detF = -1.0, detQ = -1.0, attackCoef = 0.0, releaseCoef = 0.0;
        double voiceEnv = 0.0; // VOICE detection: the voice body's level (300 Hz - 3 kHz)
        std::array<std::array<double, 2>, kMaxChannels> voiceState {};
        int detType = 0; // 0 band pass, 1 low pass, 2 high pass, 3 broadband
        std::array<std::array<double, 2>, kMaxChannels> detState {};
    };

    static Entry entryOf(EqShape) noexcept;
    static bool sameStructure(const FilterSet&, const EqBandParams&) noexcept;
    void configure(FilterSet&, const EqBandParams&) noexcept;
    void updateCoefficients(Band&) noexcept;
    void startStructureChange(Band&) noexcept;
    void advanceControl(Band&, size_t index) noexcept;
    void detect(Band&, double l, double r, int numChannels) noexcept;
    static double svf(std::array<double, 2>& state, double g, double k, int type, double x) noexcept;

    double sampleRate = 48000.0;
    int rampSteps = 1, birthSteps = 1, mixSteps = 1, crossfadeSteps = 1;
    std::array<Band, kMaxEqNodesPerLayer> bands;
    std::array<std::atomic<float>, kMaxEqNodesPerLayer> shownOffset {};

    // SOLO (listen)
    int soloBand = -1, soloType = 0;
    double soloMix = 0.0, soloG = 0.0, soloK = 1.0, voiceG = 0.0;
    std::array<std::array<double, 2>, kMaxChannels> soloState {};
};
} // namespace hpeq::dsp
