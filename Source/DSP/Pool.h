#pragma once

#include <algorithm>
#include <array>

namespace hpeq::dsp
{
// Fixed set of large processor instances (delay lines, history buffers) shared by the slots of one
// layer, allocated in prepare(). acquire()/release() run on the audio thread: no allocation, no lock.
template <typename Processor, int Size>
class SharedPool
{
public:
    static constexpr int kSize = Size;

    void prepare(double sampleRate)
    {
        for (auto& p : items)
            p.prepare(sampleRate);
        used = {};
    }

    int acquire() noexcept // -1 when every instance is in use
    {
        for (int i = 0; i < Size; ++i)
            if (!used[static_cast<size_t>(i)])
            {
                used[static_cast<size_t>(i)] = true;
                items[static_cast<size_t>(i)].reset();
                return i;
            }
        return -1;
    }

    void release(int index) noexcept
    {
        if (index >= 0)
            used[static_cast<size_t>(index)] = false;
    }

    Processor& get(int index) noexcept { return items[static_cast<size_t>(index)]; }
    int inUse() const noexcept { return static_cast<int>(std::count(used.begin(), used.end(), true)); }

private:
    std::array<Processor, Size> items;
    std::array<bool, Size> used {};
};
} // namespace hpeq::dsp
