#pragma once

#include <array>
#include <atomic>
#include <vector>

#include <juce_core/juce_core.h>

namespace hpeq
{
// Real PCM from the audio thread to the UI (03 Part B "Калибровка анализа"): a bounded lock-free
// single-producer / single-consumer queue of frames {in L, in R, out L, out R}, written at the end of
// processBlock with the plugin input (before the layer) and the output (after output gain), sample-
// aligned (the plugin has no latency). If the UI does not read in time, new frames are dropped and
// counted: the UI may lose a frame, the audio never waits.
class AudioCapture
{
public:
    static constexpr int kCapacity = 1 << 16; // frames (~1.4 s at 48 kHz)

    struct Frame
    {
        float inL = 0, inR = 0, outL = 0, outR = 0;
    };

    AudioCapture() : frames(static_cast<size_t>(kCapacity)) {}

    // Audio thread. Mono: R = L.
    void push(const float* const* in, const float* const* out, int numChannels, int numSamples) noexcept
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite(numSamples, start1, size1, start2, size2);
        const int written = size1 + size2;
        const auto copy = [&](int dest, int count, int offset)
        {
            for (int i = 0; i < count; ++i)
            {
                auto& f = frames[static_cast<size_t>(dest + i)];
                const int s = offset + i;
                f.inL = in[0][s];
                f.inR = numChannels > 1 ? in[1][s] : in[0][s];
                f.outL = out[0][s];
                f.outR = numChannels > 1 ? out[1][s] : out[0][s];
            }
        };
        copy(start1, size1, 0);
        copy(start2, size2, size1);
        fifo.finishedWrite(written);
        if (written < numSamples)
            dropped.fetch_add(numSamples - written, std::memory_order_relaxed);
    }

    // UI thread: moves up to `maxFrames` frames into `dest`, returns how many.
    int pull(Frame* dest, int maxFrames) noexcept
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead(maxFrames, start1, size1, start2, size2);
        std::copy(frames.begin() + start1, frames.begin() + start1 + size1, dest);
        std::copy(frames.begin() + start2, frames.begin() + start2 + size2, dest + size1);
        fifo.finishedRead(size1 + size2);
        return size1 + size2;
    }

    void reset() noexcept { fifo.reset(); }
    long long droppedFrames() const noexcept { return dropped.load(std::memory_order_relaxed); }

private:
    juce::AbstractFifo fifo { kCapacity };
    std::vector<Frame> frames;
    std::atomic<long long> dropped { 0 };
};
} // namespace hpeq
