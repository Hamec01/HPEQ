#pragma once

#include <cstdint>

namespace hpeq::test
{
// Counts heap allocations made by the current thread while a probe scope is open.
//
// Coverage depends on the build:
//  - MSVC Debug: _CrtSetAllocHook sees every CRT heap call (malloc/realloc/new, incl. JUCE HeapBlock
//    used by AudioBuffer::setSize). This is the configuration RT-allocation verdicts come from.
//  - Other builds: global operator new/delete replacement only; plain malloc is NOT seen.
// `coverage()` says which one is active so reports never overstate what was checked.
enum class AllocationCoverage { CrtDebugHook, OperatorNewOnly };

struct AllocationCounts
{
    std::uint64_t allocations = 0;
    std::uint64_t frees = 0;
};

AllocationCoverage allocationCoverage() noexcept;
const char* allocationCoverageName() noexcept;

class ScopedAllocationProbe
{
public:
    ScopedAllocationProbe() noexcept;
    ~ScopedAllocationProbe() noexcept;
    AllocationCounts counts() const noexcept;

    ScopedAllocationProbe(const ScopedAllocationProbe&) = delete;
    ScopedAllocationProbe& operator=(const ScopedAllocationProbe&) = delete;

private:
    AllocationCounts startCounts;
};
} // namespace hpeq::test
