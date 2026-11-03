#include "AllocationAudit.h"
#include <algorithm>
#include <cstdlib>
#include <malloc.h>
#include <new>
namespace hamlo::test::allocation
{
thread_local bool enabled=false;
thread_local size_t count=0;
void begin() noexcept{count=0;enabled=true;}
size_t finish() noexcept{enabled=false;return count;}
}
void* operator new(size_t n)
{
    if(hamlo::test::allocation::enabled)++hamlo::test::allocation::count;
    if(auto* p=std::malloc(std::max(size_t(1),n)))return p;throw std::bad_alloc();
}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,size_t) noexcept{std::free(p);}
void operator delete[](void* p,size_t) noexcept{std::free(p);}
void* operator new(size_t n,std::align_val_t alignment)
{
    if(hamlo::test::allocation::enabled)++hamlo::test::allocation::count;
    if(auto* p=_aligned_malloc(std::max(size_t(1),n),static_cast<size_t>(alignment)))return p;throw std::bad_alloc();
}
void* operator new[](size_t n,std::align_val_t a){return ::operator new(n,a);}
void operator delete(void* p,std::align_val_t) noexcept{_aligned_free(p);}
void operator delete[](void* p,std::align_val_t) noexcept{_aligned_free(p);}
void operator delete(void* p,size_t,std::align_val_t) noexcept{_aligned_free(p);}
void operator delete[](void* p,size_t,std::align_val_t) noexcept{_aligned_free(p);}
