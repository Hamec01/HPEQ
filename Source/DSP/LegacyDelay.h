#pragma once

#include "DSP/HamloEcho.h"

namespace hpeq::dsp
{
class ClassicDelay
{
public:
    static constexpr double kMaxSeconds = 2.0;

    void prepare(double sampleRate);
    void reset() noexcept;
    void process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams&, const Transport&) noexcept;

    static double delaySeconds(const FxSlotParams&, double bpm) noexcept;
    static double tailSeconds(const FxSlotParams&, double bpm) noexcept;
    float lastBlockPeak() const noexcept { return blockPeak; }

private:
    double sampleRate = 48000.0;
    std::array<std::vector<float>, 2> lines;
    int writePos = 0;
    double delay = -1.0;
    std::array<double, 2> lowState {}, highState {};
    float blockPeak = 0.0f;
};

// The Delay module: Classic (the M6 delay above) or a HamloEcho character (DELAY_DESIGN.md). A change
// between the two engines fades the old one out (10 ms), clears the new one and fades it in.
class DelayProcessor
{
public:
    static constexpr double kMaxSeconds = ClassicDelay::kMaxSeconds;

    void prepare(double sampleRate);
    void reset() noexcept;
    void process(const float* const* in, float* const* wet, int numChannels, int numSamples, const FxSlotParams&, const Transport&) noexcept;

    // The Classic delay's time (also used by the M6 tests).
    static double delaySeconds(const FxSlotParams& p, double bpm) noexcept { return ClassicDelay::delaySeconds(p, bpm); }
    static double tailSeconds(const FxSlotParams&, double bpm) noexcept;
    float lastBlockPeak() const noexcept { return blockPeak; }
    float outputLevelDb() const noexcept { return meterOutDb.load(std::memory_order_relaxed); }
    float duckingDb() const noexcept { return engine > 0 ? echo.duckingDb() : 0.0f; }

private:
    ClassicDelay classic;
    EchoProcessor echo;
    int engine = -1; // -1 none yet, 0 Classic, 1 HamloEcho
    double fade = 1.0;
    bool fadingOut = false;
    int fadeSamples = 1;
    float blockPeak = 0.0f;
    std::atomic<float> meterOutDb { -120.0f };
};

}
