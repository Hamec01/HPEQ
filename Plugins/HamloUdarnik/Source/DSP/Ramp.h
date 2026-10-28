#pragma once

#include <cmath>

namespace hamlo::dsp
{
// Linear ramp that lands exactly on its target after `steps` advances (no float drift at the end),
// so a parameter returning to a neutral value becomes bit-exactly neutral again.
class LinearRamp
{
public:
    void reset(double value) noexcept
    {
        current = target = value;
        remaining = 0;
    }

    void setTarget(double newTarget, int steps) noexcept
    {
        if (newTarget == target)
            return;
        target = newTarget;
        if (steps <= 0)
        {
            current = target;
            remaining = 0;
            return;
        }
        remaining = steps;
        increment = (target - current) / steps;
    }

    double next() noexcept
    {
        if (remaining > 0)
        {
            current = --remaining == 0 ? target : current + increment;
        }
        return current;
    }

    bool isRamping() const noexcept { return remaining > 0; }
    double value() const noexcept { return current; }
    double targetValue() const noexcept { return target; }

private:
    double current = 0.0, target = 0.0, increment = 0.0;
    int remaining = 0;
};

inline int stepsFor(double seconds, double ratePerSecond) noexcept
{
    return static_cast<int>(std::ceil(seconds * ratePerSecond));
}
} // namespace hamlo::dsp
