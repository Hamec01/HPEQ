#pragma once
#include "Core/FxProjectState.h"
#include "DSP/LayerSnapshot.h"

namespace hpeq::native
{
// One writer (message thread), one audio reader. No ownership transfers or
// object destruction on the reader. Current snapshots protect pending entries;
// running node slots protect tails through Runtime::retain/release.
class Registry
{
public:
    static constexpr int kActiveCapacity=128,kRetiredCapacity=128;
    bool stage(const FxProjectState&,std::array<dsp::LayerSnapshot,kNumLayers>&,
               double rate,int maximumBlock,juce::String& error,int streamChannels=2);
    void acknowledge(std::uint64_t generation) noexcept { acknowledged.store(generation,std::memory_order_release); }
    std::uint64_t generation() const noexcept { return published; } // writer only
    bool empty() const noexcept { return entries.empty(); } // writer only
    void clear(); // suspended/quiescent, after every node releases its runtime
    double tailSeconds() const noexcept;
    Runtime* find(int layer,int node,int slot) const noexcept;
    Runtime* findRight(int layer,int node,int slot) const noexcept;

private:
    struct Entry
    {
        int key=0;
        FxType type=FxType::Gain;
        bool active=false;
        int channels=2;bool dual=false;
        juce::MemoryBlock structural;
        std::unique_ptr<Runtime> runtime;
        std::unique_ptr<Runtime> right;
        std::uint64_t seen=0,retired=0;
    };
    std::vector<std::unique_ptr<Entry>> entries;
    std::atomic<std::uint64_t> acknowledged {0};
    std::uint64_t published=0;
    void collect();
};
} // namespace hpeq::native
