#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include "DSP/ScoreSettings.h"

namespace hamlo::echo
{
class EchoSpace
{
public:
    void prepare(double);
    void reset() noexcept;
    void configure(const ScoreSettings&) noexcept;
    std::array<double,2> process(std::array<double,2>) noexcept;
private:
    struct Line { std::vector<double> data; int pos=0, length=1, written=0; double loss=0, gain=0,coupling=0; };
    std::array<Line,8> lines;
    double rate=48000, damping=.5;
    int type=0;
    std::array<double,2> inputLow {};
    std::array<double,2> directGain{1,1};
};

class TextureGlitch
{
public:
    void prepare(double);
    void reset() noexcept;
    std::array<double,2> process(std::array<double,2>, int mode, double ms, double amount) noexcept;
private:
    std::array<std::vector<float>,2> history;
    int capacity=1, write=0;
    int64_t clock=0, epoch=0;
    double rate=48000;
    double at(int ch, int64_t sample) const noexcept;
};
}
