#include "HeapAudit.h"
#include <cstdlib>
#include <new>

namespace heapAudit {
thread_local bool audit = false;
thread_local unsigned allocations = 0, deallocations = 0;
}
namespace {
void* allocate(std::size_t size) {
    if (heapAudit::audit) ++heapAudit::allocations;
    if (auto* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void release(void* p) noexcept {
    if (heapAudit::audit && p) ++heapAudit::deallocations;
    std::free(p);
}
}
void* operator new(std::size_t n) { return allocate(n); }
void* operator new[](std::size_t n) { return allocate(n); }
void operator delete(void* p) noexcept { release(p); }
void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p, std::size_t) noexcept { release(p); }
void operator delete[](void* p, std::size_t) noexcept { release(p); }
void* operator new(std::size_t n, std::align_val_t alignment) {
    if (heapAudit::audit) ++heapAudit::allocations;
    void* p = nullptr;
    if (posix_memalign(&p, static_cast<std::size_t>(alignment), n == 0 ? 1 : n) != 0) throw std::bad_alloc();
    return p;
}
void* operator new[](std::size_t n, std::align_val_t a) { return ::operator new(n, a); }
void operator delete(void* p, std::align_val_t) noexcept { release(p); }
void operator delete[](void* p, std::align_val_t) noexcept { release(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { release(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { release(p); }
