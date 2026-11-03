#pragma once
#include <cstdint>
#include <memory>

namespace hamlo::fader
{
struct MeterReading
{
    double rmsM=-120,rmsS=-120,rmsI=-120,lufsM=-120,lufsS=-120,lufsI=-120;
    double samplePeak=-120,truePeak=-120;
    bool readyM=false,readyS=false,readyI=false,full=false;
    std::uint64_t samples=0,blocks=0;
};
// Single audio-thread meter. Allocation only in constructor/prepare.
class Meter
{
public:
    Meter();~Meter();
    void prepare(double rate,int channels);
    void reset() noexcept;
    void push(double left,double right=0) noexcept;
    void finishPeak() noexcept;
    MeterReading reading() const noexcept;
private:
    struct Impl;std::unique_ptr<Impl> p;
};
}
