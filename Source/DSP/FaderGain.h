#pragma once
#include <array>
#include <atomic>
#include <vector>
#include "DSP/DspConfig.h"
#include "DSP/FaderSettings.h"
namespace hpeq::dsp
{
class GainProcessor
{
public:
    static constexpr double kMaxDelayMs = 20.0;

    void prepare(double sampleRate); // allocates the delay lines
    void reset() noexcept;
    void process(float* const* channels, int numChannels, int numSamples, const FxSlotParams&) noexcept;

    float inputDb() const noexcept { return meterIn.load(std::memory_order_relaxed); }
    float inputLongDb() const noexcept { return meterInLong.load(std::memory_order_relaxed); }
    float outputDb() const noexcept { return meterOut.load(std::memory_order_relaxed); }

private:
    double sampleRate = 48000.0;
    std::array<std::vector<float>, kMaxChannels> lines;
    int writePos = 0;
    bool primed = false;
    double gain = 1.0, delay = 0.0, muteGain = 1.0;
    std::array<double, kMaxChannels> polarity { 1.0, 1.0 };
    double msIn = 0.0, msInLong = 0.0, msOut = 0.0;
    std::atomic<float> meterIn { -120.0f }, meterInLong { -120.0f }, meterOut { -120.0f };
};

}
