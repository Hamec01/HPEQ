#pragma once

#include "DSP/HamloUdarnik.h"
#include "DSP/TransientSettings.h"

namespace hamlo::dsp
{
// Transient shaper (insert). Linked peak envelopes: fast (attack 0.5 ms, release 20 ms * s) and slow
// (attack 20 ms * s, release 250 ms * s), s = 0.5 + speed. d = 20 log10(fast / slow) in dB (> 0 on
// attacks, < 0 in decays). gain dB = attack * clamp(d / 6, 0, 1) + sustain * clamp(-d / 6, 0, 1).
// Attack = sustain = 0 dB is an exact identity.
// With any HamloUdarnik option set (HamloUdarnik.h) that core runs instead; latency = its look-ahead.
class TransientProcessor
{
public:
    void prepare(double sampleRate) { sampleRate_ = sampleRate; core.prepare(sampleRate); reset(); } // allocates (core rings)
    void reset() noexcept { fast = slow = 0.0; core.reset(); }
    void process(float* const* channels, int numChannels, int numSamples, const TransientSettings&) noexcept;

    static bool usesUdarnik(const TransientSettings&) noexcept;
    static int lookaheadSamples(const TransientSettings&, double sampleRate) noexcept;
    const UdarnikCore& udarnik() const noexcept { return core; }
    bool udarnikActive() const noexcept { return coreActive; }

private:
    double sampleRate_ = 48000.0, fast = 0.0, slow = 0.0;
    UdarnikCore core;
    bool coreActive = false;
};
} // namespace hamlo::dsp
