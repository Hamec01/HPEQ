#pragma once

#include <atomic>
#include <memory>
#include "DSP/VerbV2Settings.h"

namespace hpeq::dsp
{
// Wet-only standalone reverb. All delay memory and the two handover paths are
// allocated before processing. Musical wet delays never delay the dry signal.
class VerbV2Processor
{
public:
    VerbV2Processor();
    ~VerbV2Processor();
    VerbV2Processor(const VerbV2Processor&) = delete;
    VerbV2Processor& operator=(const VerbV2Processor&) = delete;

    void prepare(double sampleRate, int maximumBlockSize);
    void reset() noexcept;
    void process(const float* const* in, float* const* wet, int channels, int count,
                 const VerbV2Settings&, const Transport&) noexcept;
    float outputLevelDb() const noexcept { return outputDb.load(std::memory_order_relaxed); }
    float duckingDb() const noexcept { return duckDb.load(std::memory_order_relaxed); }
    double energy() const noexcept;
    static double tailSeconds(const VerbV2Settings&, double bpm = 120.0) noexcept;
    static double decaySecondsAt(const VerbV2Settings&, double hz) noexcept;
    static double postEqMagnitude(const VerbV2Settings&, double hz, double sampleRate) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    std::atomic<float> outputDb { -120.0f }, duckDb { 0.0f };
};
} // namespace hpeq::dsp
