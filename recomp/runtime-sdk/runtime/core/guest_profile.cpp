#include "core/deep_profile.h"
#include "core/guest_profile.h"
#if defined(XBE_GUEST_PROFILE) && XBE_GUEST_PROFILE
#include "core/log.h"
#include <atomic>
#include <chrono>
#include <mutex>

namespace guest_profile {
namespace {
constexpr size_t kBins=2048,kProbes=8;
struct Bin {
    std::atomic<uint64_t> key{0};
    std::atomic_flag busy=ATOMIC_FLAG_INIT;
    uint32_t thread=0;
    uint64_t samples=0,wall_ns=0,max_ns=0;
};
std::array<Bin,kBins> bins;
std::atomic<uint64_t> dropped{0},epoch{0};
struct Local {uint32_t random=0x6d2b79f5;bool active=false;};
thread_local Local local;
uint64_t Now() noexcept {
#if defined(XBE_DEEP_PROFILE) && XBE_DEEP_PROFILE
    return deep_profile::NowNs();
#else
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}
uint64_t Key(Kind kind,uint32_t target) noexcept {
    return (uint64_t(uint8_t(kind)+1)<<32)|target;
}
void Insert(Window& window,Entry entry) noexcept {
    uint32_t at=0;
    while(at<window.top_count && window.top[at].wall_ns>=entry.wall_ns)++at;
    if(at==window.top.size())return;
    if(window.top_count<window.top.size())++window.top_count;
    for(uint32_t i=window.top_count-1;i>at;--i)window.top[i]=window.top[i-1];
    window.top[at]=entry;
}
}

Scope::Scope(Kind kind,uint32_t target) noexcept :kind_(kind),target_(target) {
    if(local.active)return; // suppress nested samples, not nested guest execution
    uint32_t value=local.random;
    value^=value<<13;value^=value>>17;value^=value<<5;local.random=value;
    const uint32_t stride=kind==Kind::Kernel?kKernelStride:kIndirectStride;
    if(value&(stride-1))return;
    local.active=true;active_=true;started_=Now();
    uint64_t empty=0;epoch.compare_exchange_strong(empty,started_,std::memory_order_relaxed);
}
Scope::~Scope() noexcept {
    if(!active_)return;
    const uint64_t elapsed=Now()-started_;
    local.active=false;
    Record(kind_,target_,elapsed,thread_);
}
bool Record(Kind kind,uint32_t target,uint64_t elapsed_ns,uint32_t thread) noexcept {
    const uint64_t key=Key(kind,target);
    const size_t first=size_t((key^(key>>32)^(uint64_t(thread)<<17))*0x9e3779b97f4a7c15ull)>>(64-11);
    for(size_t offset=0;offset<kProbes;++offset) {
        Bin& bin=bins[(first+offset)&(kBins-1)];
        uint64_t found=bin.key.load(std::memory_order_relaxed);
        if(!found) {
            uint64_t empty=0;
            if(bin.key.compare_exchange_strong(empty,key,std::memory_order_relaxed))found=key;
            else found=empty;
        }
        if(found!=key)continue;
        if(bin.busy.test_and_set(std::memory_order_acquire))break;
        // Drain may retire this key between our lookup and lock acquisition.
        if(bin.key.load(std::memory_order_relaxed)!=key) {
            bin.busy.clear(std::memory_order_release);continue;
        }
        if(bin.samples && bin.thread!=thread) {
            bin.busy.clear(std::memory_order_release);continue;
        }
        bin.thread=thread;
        ++bin.samples;bin.wall_ns+=elapsed_ns;
        if(elapsed_ns>bin.max_ns)bin.max_ns=elapsed_ns;
        bin.busy.clear(std::memory_order_release);return true;
    }
    dropped.fetch_add(1,std::memory_order_relaxed);return false;
}
Window Drain() noexcept {
    Window window;
    for(Bin& bin:bins) {
        if(bin.busy.test_and_set(std::memory_order_acquire))continue;
        const uint64_t key=bin.key.load(std::memory_order_relaxed);
        if(!key) {bin.busy.clear(std::memory_order_release);continue;}
        Entry entry{Kind(uint8_t(key>>32)-1),uint32_t(key),bin.thread,bin.samples,bin.wall_ns,bin.max_ns};
        bin.samples=bin.wall_ns=bin.max_ns=0;
        // Recycle each report window, so intro targets cannot permanently
        // occupy the bounded table before gameplay starts.
        bin.key.store(0,std::memory_order_relaxed);
        bin.busy.clear(std::memory_order_release);
        if(!entry.samples)continue;
        window.samples+=entry.samples;window.wall_ns+=entry.wall_ns;Insert(window,entry);
    }
    window.dropped=dropped.exchange(0,std::memory_order_relaxed);return window;
}
void ReportIfDue() {
    static std::mutex report_mutex;
    static uint64_t sequence=0;
    std::unique_lock lock(report_mutex,std::try_to_lock);
    if(!lock.owns_lock())return;
    const uint64_t started=epoch.load(std::memory_order_relaxed),now=Now();
    if(!started || now-started<5000000000ull)return;
    epoch.store(now,std::memory_order_relaxed);
    const Window window=Drain();++sequence;
    LOGI("perf: guest-profile window=%llu interval_s=%.3f samples=%llu sampled_wall_ms=%.3f dropped=%llu indirect_stride=%u kernel_stride=%u (sampled call wall time; not CPU/GPU time or FPS)",
        (unsigned long long)sequence,(now-started)/1e9,(unsigned long long)window.samples,
        window.wall_ns/1e6,(unsigned long long)window.dropped,kIndirectStride,kKernelStride);
    for(uint32_t i=0;i<window.top_count;++i) {
        const Entry& entry=window.top[i];
        LOGI("perf: guest-profile-entry window=%llu rank=%u kind=%s target=%08X thread=%08X samples=%llu sampled_wall_ms=%.3f max_wall_ms=%.3f",
            (unsigned long long)sequence,i+1,entry.kind==Kind::Kernel?"kernel":"indirect",entry.target,entry.thread,
            (unsigned long long)entry.samples,entry.wall_ns/1e6,entry.max_ns/1e6);
    }
}
}
#endif
