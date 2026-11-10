#pragma once
#include <cstdint>
namespace kernel {
struct DispatcherWakeStats {
    bool selective=false;
    uint64_t sleepers=0,object_signals=0,object_notifies=0,avoided=0,broadcast_signals=0,broadcast_notifies=0;
};
#if (defined(XBE_OBJECT_WAIT_CHANNELS) && XBE_OBJECT_WAIT_CHANNELS) || (defined(XBE_WAIT_STATS) && XBE_WAIT_STATS)
DispatcherWakeStats GetDispatcherWakeStats() noexcept;
void ReportDispatcherWakePerf();
#else
inline DispatcherWakeStats GetDispatcherWakeStats() noexcept {return {};}
inline void ReportDispatcherWakePerf() {}
#endif
}
