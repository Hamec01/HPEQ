#include "Harness/RtAllocationProbe.h"

#include <cstdlib>
#include <new>

#if defined(_MSC_VER) && defined(_DEBUG)
 #include <crtdbg.h>
 #define HPEQ_ALLOC_PROBE_CRT_HOOK 1
#else
 #define HPEQ_ALLOC_PROBE_CRT_HOOK 0
#endif

namespace hpeq::test
{
namespace
{
// Plain POD thread_locals: no dynamic initialisation, so touching them never allocates.
thread_local int probeDepth = 0;
thread_local std::uint64_t threadAllocations = 0;
thread_local std::uint64_t threadFrees = 0;

void noteAllocation() noexcept
{
    if (probeDepth > 0)
        ++threadAllocations;
}

void noteFree() noexcept
{
    if (probeDepth > 0)
        ++threadFrees;
}

#if HPEQ_ALLOC_PROBE_CRT_HOOK
int __cdecl crtAllocHook(int allocType, void*, size_t, int, long, const unsigned char*, int)
{
    if (allocType == _HOOK_ALLOC || allocType == _HOOK_REALLOC)
        noteAllocation();
    else if (allocType == _HOOK_FREE)
        noteFree();
    return 1; // let the CRT proceed
}

struct HookInstaller
{
    HookInstaller() { _CrtSetAllocHook(crtAllocHook); }
};
const HookInstaller hookInstaller;
#endif
} // namespace

AllocationCoverage allocationCoverage() noexcept
{
    return HPEQ_ALLOC_PROBE_CRT_HOOK ? AllocationCoverage::CrtDebugHook : AllocationCoverage::OperatorNewOnly;
}

const char* allocationCoverageName() noexcept
{
    return HPEQ_ALLOC_PROBE_CRT_HOOK ? "crt-debug-hook (malloc + new)" : "operator-new-only (malloc not seen)";
}

ScopedAllocationProbe::ScopedAllocationProbe() noexcept
    : startCounts { threadAllocations, threadFrees }
{
    ++probeDepth;
}

ScopedAllocationProbe::~ScopedAllocationProbe() noexcept
{
    --probeDepth;
}

AllocationCounts ScopedAllocationProbe::counts() const noexcept
{
    return { threadAllocations - startCounts.allocations, threadFrees - startCounts.frees };
}
} // namespace hpeq::test

#if !HPEQ_ALLOC_PROBE_CRT_HOOK
// Release builds: replace global new/delete so at least C++ allocations are counted.
namespace
{
void* countedAlloc(std::size_t size, bool nothrow)
{
    hpeq::test::noteAllocation();
    if (auto* p = std::malloc(size == 0 ? 1 : size))
        return p;
    if (nothrow)
        return nullptr;
    throw std::bad_alloc();
}

void* countedAlignedAlloc(std::size_t size, std::align_val_t alignment, bool nothrow)
{
    hpeq::test::noteAllocation();
    const auto align = static_cast<std::size_t>(alignment);
 #if defined(_WIN32)
    void* p = _aligned_malloc(size == 0 ? 1 : size, align);
 #else
    void* p = std::aligned_alloc(align, ((size == 0 ? 1 : size) + align - 1) / align * align);
 #endif
    if (p != nullptr || nothrow)
        return p;
    throw std::bad_alloc();
}

void countedFree(void* p) noexcept
{
    if (p == nullptr)
        return;
    hpeq::test::noteFree();
    std::free(p);
}

void countedAlignedFree(void* p) noexcept
{
    if (p == nullptr)
        return;
    hpeq::test::noteFree();
 #if defined(_WIN32)
    _aligned_free(p);
 #else
    std::free(p);
 #endif
}
} // namespace

void* operator new(std::size_t size) { return countedAlloc(size, false); }
void* operator new[](std::size_t size) { return countedAlloc(size, false); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept { return countedAlloc(size, true); }
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept { return countedAlloc(size, true); }
void* operator new(std::size_t size, std::align_val_t a) { return countedAlignedAlloc(size, a, false); }
void* operator new[](std::size_t size, std::align_val_t a) { return countedAlignedAlloc(size, a, false); }
void* operator new(std::size_t size, std::align_val_t a, const std::nothrow_t&) noexcept { return countedAlignedAlloc(size, a, true); }
void* operator new[](std::size_t size, std::align_val_t a, const std::nothrow_t&) noexcept { return countedAlignedAlloc(size, a, true); }

void operator delete(void* p) noexcept { countedFree(p); }
void operator delete[](void* p) noexcept { countedFree(p); }
void operator delete(void* p, std::size_t) noexcept { countedFree(p); }
void operator delete[](void* p, std::size_t) noexcept { countedFree(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { countedFree(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { countedFree(p); }
void operator delete(void* p, std::align_val_t) noexcept { countedAlignedFree(p); }
void operator delete[](void* p, std::align_val_t) noexcept { countedAlignedFree(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { countedAlignedFree(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { countedAlignedFree(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { countedAlignedFree(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { countedAlignedFree(p); }
#endif
