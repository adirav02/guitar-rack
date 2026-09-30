#pragma once
namespace heapAudit {
extern thread_local bool audit;
extern thread_local unsigned allocations, deallocations;
inline void begin() noexcept { allocations = deallocations = 0; audit = true; }
inline bool finish() noexcept { audit = false; return allocations == 0 && deallocations == 0; }
}
