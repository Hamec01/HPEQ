#pragma once
#include "DSP/FaderKernel.h"
#include <array>
#include <atomic>
#include <cstring>
#include <limits>
#include <type_traits>

namespace hpeq::standalone
{
struct FaderStatus
{
    hamlo::fader::Reading reading;
    hamlo::fader::MatchProposal proposal;
    std::uint64_t epoch=0,frame=0,commandSequence=0;
    bool legacy=true,bypassed=false;
    double legacyInput=-120,legacyOutput=-120,legacyLong=-120;
};
// Each word is atomic: unlike a plain-data seqlock, concurrent copies have no C++ data race.
// Single audio writer; bounded control reader. No UI/control access to kernel internals.
template<class T> class FaderSnapshot
{
    static_assert(std::is_trivially_copyable_v<T>);
    static constexpr size_t words=(sizeof(T)+7)/8;
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    std::array<std::atomic<std::uint64_t>,words> data{};
    std::atomic<std::uint64_t> sequence{0};
public:
    void publish(const T& value) noexcept
    {
        std::array<std::uint64_t,words> copy{};std::memcpy(copy.data(),&value,sizeof(T));
        sequence.fetch_add(1);for(size_t n=0;n<words;++n)data[n].store(copy[n]);sequence.fetch_add(1);
    }
    bool read(T& result) const noexcept
    {
        for(int attempt=0;attempt<4;++attempt)
        {
            const auto first=sequence.load();if(first==0||(first&1))continue;
            std::array<std::uint64_t,words> copy{};for(size_t n=0;n<words;++n)copy[n]=data[n].load();
            if(first==sequence.load()){std::memcpy(&result,copy.data(),sizeof(T));return true;}
        }
        result=T{};return false;
    }
};
enum class FaderAction{StartLearn,StopLearn,Reset};
struct FaderCommand{FaderAction action=FaderAction::Reset;std::uint64_t frame=0,epoch=0;};
// Multiple control producers try a bounded flag, never wait. One audio consumer.
class FaderCommands
{
    std::array<FaderCommand,64> entries{};
    std::atomic<std::uint64_t> head{0},tail{0};
    std::atomic_flag producer=ATOMIC_FLAG_INIT;
public:
    bool push(FaderCommand c) noexcept
    {
        if(producer.test_and_set(std::memory_order_acquire))return false;
        const auto h=head.load(std::memory_order_relaxed),t=tail.load(std::memory_order_acquire);
        bool ok=h-t<entries.size();
        if(ok&&h!=t){const auto& previous=entries[(h-1)%entries.size()];ok=previous.epoch!=c.epoch||previous.frame<=c.frame;}
        if(ok){entries[h%entries.size()]=c;head.store(h+1,std::memory_order_release);}
        producer.clear(std::memory_order_release);return ok;
    }
    bool peek(FaderCommand& c) const noexcept
    {
        const auto t=tail.load(std::memory_order_relaxed);if(t==head.load(std::memory_order_acquire))return false;
        c=entries[t%entries.size()];return true;
    }
    std::uint64_t produced() const noexcept{return head.load(std::memory_order_acquire);}
    std::uint64_t consumed() const noexcept{return tail.load(std::memory_order_acquire);}
    void pop() noexcept{tail.fetch_add(1,std::memory_order_release);}
};
}
