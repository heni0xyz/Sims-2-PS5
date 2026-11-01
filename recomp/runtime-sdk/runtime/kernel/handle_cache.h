#pragma once
#include <cstdint>
namespace kernel {
struct HandleCacheStats {
    bool enabled=false;
    uint64_t hits=0,misses=0,generation=0;
};
#if defined(XBE_FAST_HANDLES) && XBE_FAST_HANDLES
HandleCacheStats GetHandleCacheStats() noexcept;
void ReportHandleCachePerf();
#else
inline HandleCacheStats GetHandleCacheStats() noexcept {return {};}
inline void ReportHandleCachePerf() {}
#endif
}
