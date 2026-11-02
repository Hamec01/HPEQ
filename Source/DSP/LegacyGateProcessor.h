#pragma once
#include "DSP/HamloZastava.h"
namespace hpeq::dsp
{
class LegacyGateProcessor
{
public:
    static constexpr double kDetectorReleaseMs = 10.0;

    void prepare(double sampleRate) { sampleRate_ = sampleRate; core.prepare(sampleRate); reset(); } // allocates (core rings)
    void reset() noexcept { open = false; holdLeft = 0; attenuationDb = 0.0; detector = 0.0; transitions = 0; core.reset(); }
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;
    long long stateChanges() const noexcept { return transitions; }

    // HamloZastava (HamloZastava.h): runs instead of the M6 law when any of its options is set.
    static bool usesZastava(const FxSlotParams&) noexcept;
    static int lookaheadSamples(const FxSlotParams&, double sampleRate) noexcept;
    const ZastavaCore& zastava() const noexcept { return core; }
    bool zastavaActive() const noexcept { return coreActive; }

private:
    double sampleRate_ = 48000.0, attenuationDb = 0.0, detector = 0.0;
    bool open = false;
    int holdLeft = 0;
    long long transitions = 0;
    ZastavaCore core;
    bool coreActive = false;
};
}
