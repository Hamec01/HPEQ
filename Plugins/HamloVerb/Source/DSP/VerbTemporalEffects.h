#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace hpeq::dsp::detail
{
using Pair = std::array<double, 2>;
constexpr double pi = 3.14159265358979323846;

struct Delay
{
    std::vector<double> data;
    int write = 0;
    void prepare(int capacity) { data.assign(static_cast<size_t>(std::max(8, capacity)), 0.0); write = 0; }
    void clear() noexcept { std::fill(data.begin(), data.end(), 0.0); write = 0; }
    void push(double x) noexcept
    {
        data[static_cast<size_t>(write)] = x;
        if (++write == static_cast<int>(data.size())) write = 0;
    }
    double at(int offset) const noexcept
    {
        int i = write - 1 - offset;
        const int size = static_cast<int>(data.size());
        if (i < 0) i += size;
        return data[static_cast<size_t>(i)];
    }
    // Delay is relative to the last written sample. Capacity is guaranteed in prepare.
    double linear(double delay) const noexcept
    {
        delay = std::clamp(delay, 0.0, static_cast<double>(data.size() - 3));
        const int i = static_cast<int>(delay);
        const double f = delay - i, a = at(i);
        return a + f * (at(i + 1) - a);
    }
    double cubic(double delay) const noexcept
    {
        delay = std::clamp(delay, 1.0, static_cast<double>(data.size() - 4));
        const int i = static_cast<int>(delay); const double f = delay - i;
        const double a = at(i), b = at(i + 1);
        if (f == 0.0) return a;
        const double p = at(i - 1), n = at(i + 2);
        return ((0.5 * (n - p) + 1.5 * (a - b)) * f + p - 2.5 * a + 2.0 * b - 0.5 * n) * f * f
               + 0.5 * (b - p) * f + a;
    }
    double recentEnergy(int count) const noexcept
    {
        double sum = 0.0;
        for (int i = 0; i < std::min(count, static_cast<int>(data.size()) - 1); ++i)
        { const double x = at(i); sum += x * x; }
        return sum;
    }
};

struct ReverseFragments
{
    struct Grain { int age = 0, length = 1; std::int64_t end = 0; bool active = false; };
    std::array<Delay, 2> history;
    std::array<Grain, 2> grains;
    std::int64_t clock = 0;
    int untilLaunch = 0, next = 0;
    void prepare(double fs)
    {
        for (auto& d : history) d.prepare(static_cast<int>(std::ceil(fs * 4.1)) + 16);
        clear();
    }
    void clear() noexcept
    {
        for (auto& d : history) d.clear();
        grains = {}; clock = 0; untilLaunch = 0; next = 0;
    }
    Pair tick(Pair input, int length) noexcept
    {
        for (int ch = 0; ch < 2; ++ch) history[static_cast<size_t>(ch)].push(input[static_cast<size_t>(ch)]);
        length = std::max(8, length);
        if (untilLaunch <= 0 && clock >= length)
        {
            grains[static_cast<size_t>(next)] = { 0, length, clock - 1, true };
            next = 1 - next; untilLaunch = std::max(1, length / 2);
        }
        if (untilLaunch > 0) --untilLaunch;
        Pair result {};
        for (auto& grain : grains)
        {
            if (!grain.active) continue;
            // Read a captured interval backwards, not a forward signal under a
            // rising envelope. The second captured interval overlaps by 50%.
            const auto offset = clock - grain.end + grain.age;
            const double phase = (grain.age + 0.5) / grain.length;
            const double window = 0.5 - 0.5 * std::cos(2.0 * pi * phase);
            for (int ch = 0; ch < 2; ++ch)
                result[static_cast<size_t>(ch)] += window * history[static_cast<size_t>(ch)].linear(static_cast<double>(offset));
            if (++grain.age >= grain.length) grain.active = false;
        }
        ++clock;
        return result;
    }
};

struct PitchFeedback
{
    std::array<Delay, 2> history;
    double phase = 0.0;
    void prepare(double fs) { for (auto& d : history) d.prepare(static_cast<int>(fs * 0.15) + 16); clear(); }
    void clear() noexcept { for (auto& d : history) d.clear(); phase = 0.0; }
    Pair tick(Pair input, double ratio, double feedback, double fs) noexcept
    {
        const double window = fs * 0.060;
        Pair out {};
        for (int ch = 0; ch < 2; ++ch)
        {
            auto& d = history[static_cast<size_t>(ch)];
            const double p2 = phase < 0.5 ? phase + 0.5 : phase - 0.5;
            const double w1 = 0.5 - 0.5 * std::cos(2.0 * pi * phase), w2 = 1.0 - w1;
            const double a = d.cubic(2.0 + phase * window), b = d.cubic(2.0 + p2 * window);
            out[static_cast<size_t>(ch)] = w1 * a + w2 * b;
            // Bounded contraction; the additional tanh protects hostile live
            // automation without imposing a limiter on the normal wet path.
            d.push(input[static_cast<size_t>(ch)] + std::tanh(out[static_cast<size_t>(ch)]) * feedback);
        }
        phase += (1.0 - ratio) / window;
        phase -= std::floor(phase);
        return out;
    }
};

} // namespace hpeq::dsp::detail
