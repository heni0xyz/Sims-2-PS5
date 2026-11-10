#include "core/deep_profile.h"
#include <cmath>
namespace deep_profile {
bool TimestampNs(uint64_t begin,uint64_t end,uint32_t bits,double period,double& ns) noexcept {
    if(!bits || bits>64 || !std::isfinite(period) || period<=0)return false;
    const uint64_t mask=bits==64?UINT64_MAX:((uint64_t(1)<<bits)-1);
    ns=double((end-begin)&mask)*period;
    return std::isfinite(ns);
}
}
#if defined(XBE_DEEP_PROFILE) && XBE_DEEP_PROFILE
#include "core/log.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cerrno>
#include <mutex>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <time.h>
#include <sys/resource.h>
#endif
#if defined(__PROSPERO__)
#include <ps5platform/kernel.h>
#endif

namespace deep_profile {
namespace {
constexpr size_t kThreads=64,kFrames=4096;
constexpr uint64_t kPublishNs=1000000000ull,kReportNs=5000000000ull;
const char* names[]={"kernel","handle-lock","wait-lock","release-lock","cv-wait-relock","delay",
    "file-read","file-write","shader","pipeline","acquire","submit","present","fence",
    "guest-frame","guest-scene","guest-tick","guest-task","audio","upload","input"};
static_assert(std::size(names)==kMetricCount);
struct AtomicStats {
    std::atomic<uint64_t> calls{0},wall{0},maximum{0},cpu{0},cpu_samples{0};
};
struct Slot {
    std::atomic<bool> used{false};
    std::atomic<uint32_t> id{0},state{0},target{0},sequence{0};
    std::atomic<int> cpu_supported{-1},cpu_error{0};
    std::atomic<uint64_t> sample_time{0},wall{0},cpu{0};
    std::array<AtomicStats,kMetricCount> metrics;
    std::array<std::atomic<uint32_t>,32> target_keys{};
    std::array<AtomicStats,32> targets;
};
std::array<Slot,kThreads> slots;
std::atomic<unsigned> next_slot{0};
std::atomic<uint64_t> dropped_threads{0};
uint64_t tsc_frequency=0,profile_epoch=0;
[[maybe_unused]] uint64_t tsc_base=0;
std::once_flag initialize_once;
bool ReadCpu(uint64_t& ns) noexcept {
#ifdef _WIN32
    FILETIME created,exited,kernel,user;
    if(!GetThreadTimes(GetCurrentThread(),&created,&exited,&kernel,&user))return false;
    ns=((uint64_t(kernel.dwHighDateTime)<<32)|kernel.dwLowDateTime)*100+
       ((uint64_t(user.dwHighDateTime)<<32)|user.dwLowDateTime)*100;
    return true;
#else
    timespec value{};
    if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&value))return false;
    ns=uint64_t(value.tv_sec)*1000000000ull+uint64_t(value.tv_nsec);return true;
#endif
}
uint64_t RawNow() noexcept {
#if defined(__PROSPERO__)
    if(tsc_frequency) {
        const uint64_t ticks=sceKernelReadTsc()-tsc_base;
        return uint64_t((static_cast<unsigned __int128>(ticks)*1000000000ull)/tsc_frequency);
    }
#endif
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
void Initialize() {
#if defined(__PROSPERO__)
    tsc_frequency=sceKernelGetTscFrequency();tsc_base=sceKernelReadTsc();
#endif
    uint64_t scratch=0;const auto start=RawNow();
    for(unsigned i=0;i<2048;++i)scratch^=RawNow();
    const auto timer_ns=(RawNow()-start)/2048;
    const auto cpu_start=RawNow();bool supported=true;
    for(unsigned i=0;i<32;++i) {uint64_t value=0;supported=ReadCpu(value)&&supported;scratch^=value;}
    const auto cpu_ns=(RawNow()-cpu_start)/32;
    LOGI("perf: deep-config schema=1 wall_source=%s tsc_hz=%llu wall_read_ns=%llu cpu_read_ns=%llu cpu_supported=%u slots=64 frame_capacity=4096 publish_s=1 report_s=5 calibration=%llu",
        tsc_frequency?"tsc":"steady",(unsigned long long)tsc_frequency,(unsigned long long)timer_ns,
        (unsigned long long)cpu_ns,unsigned(supported),(unsigned long long)(scratch&1));
    LOGI("perf: deep-limits scopes=inclusive cpu=OS-thread-time cv=wait-plus-relock gpu=segment-elapsed-not-utilization log=periodic profiler=changes-runtime-cost");
    profile_epoch=RawNow();
}
struct Local {
    Slot* slot=nullptr;
    std::array<Stats,kMetricCount> metrics{};
    std::array<uint32_t,32> target_keys{};
    std::array<Stats,32> targets{};
    uint64_t born=0,last_publish=0,cpu_base=0;
    bool cpu_base_valid=false;
};
thread_local Local local;
Local& GetLocal() noexcept {
    std::call_once(initialize_once,Initialize);
    if(!local.born) {
        local.born=RawNow()+1;
        local.cpu_base_valid=ReadCpu(local.cpu_base);
        const unsigned index=next_slot.fetch_add(1,std::memory_order_relaxed);
        if(index<slots.size()) {
            local.slot=&slots[index];local.slot->used.store(true,std::memory_order_release);
        }else dropped_threads.fetch_add(1,std::memory_order_relaxed);
    }
    return local;
}
void Publish(Local& value,uint64_t now) noexcept {
    value.last_publish=now;
    Slot* slot=value.slot;if(!slot)return;
    slot->sequence.fetch_add(1,std::memory_order_acq_rel);
    uint64_t cpu=0;const bool valid=value.cpu_base_valid && ReadCpu(cpu) && cpu>=value.cpu_base;
    slot->cpu_supported.store(valid?1:0,std::memory_order_relaxed);
    slot->cpu_error.store(valid?0:errno,std::memory_order_relaxed);
    slot->sample_time.store(now,std::memory_order_relaxed);
    slot->wall.store(now>=value.born-1?now-(value.born-1):0,std::memory_order_relaxed);
    slot->cpu.store(valid?cpu-value.cpu_base:0,std::memory_order_relaxed);
    for(unsigned i=0;i<kMetricCount;++i) {
        const auto& in=value.metrics[i];auto& out=slot->metrics[i];
        out.calls.store(in.calls,std::memory_order_relaxed);out.wall.store(in.wall_ns,std::memory_order_relaxed);
        out.maximum.store(in.max_ns,std::memory_order_relaxed);out.cpu.store(in.cpu_ns,std::memory_order_relaxed);
        out.cpu_samples.store(in.cpu_samples,std::memory_order_relaxed);
    }
    for(unsigned i=0;i<value.targets.size();++i) {
        const auto& in=value.targets[i];auto& out=slot->targets[i];
        slot->target_keys[i].store(value.target_keys[i],std::memory_order_relaxed);
        out.calls.store(in.calls,std::memory_order_relaxed);out.wall.store(in.wall_ns,std::memory_order_relaxed);
        out.maximum.store(in.max_ns,std::memory_order_relaxed);out.cpu.store(in.cpu_ns,std::memory_order_relaxed);
        out.cpu_samples.store(in.cpu_samples,std::memory_order_relaxed);
    }
    slot->sequence.fetch_add(1,std::memory_order_release);
}
std::mutex frame_mutex;
std::array<uint64_t,kFrames> intervals{};
size_t interval_count=0;uint64_t interval_drops=0;
uint64_t last_frame_time=0;
std::mutex gpu_mutex;
uint64_t gpu_count=0,gpu_errors=0,gpu_lines=0;
double gpu_sum=0,gpu_max=0;
}
uint64_t NowNs() noexcept {std::call_once(initialize_once,Initialize);return RawNow();}
void RegisterThread(uint32_t id) noexcept {
    auto& value=GetLocal();if(value.slot)value.slot->id.store(id,std::memory_order_relaxed);
    Publish(value,RawNow());
}
void Poll() noexcept {
    auto& value=GetLocal();const auto now=RawNow();
    if(now-value.last_publish>=kPublishNs)Publish(value,now);
}
Stats LocalStats(Metric metric) noexcept {return GetLocal().metrics[unsigned(metric)];}
Scope::Scope(Metric metric,uint32_t target,bool cpu) noexcept :metric_(metric),target_(target) {
    auto& value=GetLocal();local_=&value;
    if(value.slot) {
        old_state_=value.slot->state.exchange(unsigned(metric)+1,std::memory_order_relaxed);
        old_target_=value.slot->target.exchange(target,std::memory_order_relaxed);
    }
    if(cpu)cpu_valid_=ReadCpu(cpu_);
    start_=RawNow();
}
Scope::~Scope() noexcept {
    auto& value=*static_cast<Local*>(local_);const auto now=RawNow();
    auto& stats=value.metrics[unsigned(metric_)];
    const auto elapsed=now>=start_?now-start_:0;
    ++stats.calls;stats.wall_ns+=elapsed;stats.max_ns=std::max(stats.max_ns,elapsed);
    if(cpu_valid_) {
        uint64_t end=0;if(ReadCpu(end) && end>=cpu_) {
            ++stats.cpu_samples;stats.cpu_ns+=end-cpu_;
            // Only explicit guest wrappers request CPU samples. Fixed slots
            // preserve separate target costs without logging from hot scopes.
            if(value.slot) {
                const auto target=target_;
                for(unsigned i=0;target && i<value.targets.size();++i) {
                    if(value.target_keys[i] && value.target_keys[i]!=target)continue;
                    value.target_keys[i]=target;auto& entry=value.targets[i];
                    ++entry.calls;entry.wall_ns+=elapsed;entry.max_ns=std::max(entry.max_ns,elapsed);
                    ++entry.cpu_samples;entry.cpu_ns+=end-cpu_;break;
                }
            }
        }
    }
    if(!cpu_valid_ && target_ && unsigned(metric_)>=unsigned(Metric::GuestFrame) && unsigned(metric_)<=unsigned(Metric::GuestTask)) {
        for(unsigned i=0;i<value.targets.size();++i) {
            if(value.target_keys[i] && value.target_keys[i]!=target_)continue;
            value.target_keys[i]=target_;auto& entry=value.targets[i];
            ++entry.calls;entry.wall_ns+=elapsed;entry.max_ns=std::max(entry.max_ns,elapsed);break;
        }
    }
    if(value.slot) {value.slot->target.store(old_target_,std::memory_order_relaxed);value.slot->state.store(old_state_,std::memory_order_relaxed);}
    if(now-value.last_publish>=kPublishNs)Publish(value,now);
}
void RecordFrameInterval(uint64_t ns) noexcept {
    std::lock_guard lock(frame_mutex);
    if(interval_count<intervals.size())intervals[interval_count++]=ns;else ++interval_drops;
}
FrameStats DrainFrames() noexcept {
    std::array<uint64_t,kFrames> values;size_t count;FrameStats result;
    {std::lock_guard lock(frame_mutex);count=interval_count;
        std::copy_n(intervals.begin(),count,values.begin());result.dropped=interval_drops;
        interval_count=0;interval_drops=0;}
    result.count=count;if(!count)return result;
    std::sort(values.begin(),values.begin()+count);
    long double sum=0;for(size_t i=0;i<count;++i)sum+=values[i];
    auto percentile=[&](size_t p) {return values[(count*p+99)/100-1]/1e6;};
    result.mean_ms=double(sum/count)/1e6;result.p50_ms=percentile(50);
    result.p95_ms=percentile(95);result.p99_ms=percentile(99);result.max_ms=values[count-1]/1e6;
    const size_t slow=std::max<size_t>(1,(count+99)/100);long double worst=0;
    for(size_t i=count-slow;i<count;++i)worst+=values[i];
    result.low1_fps=worst>0?double(1e9L*slow/worst):0;return result;
}
void FramePresented(uint64_t) {
    const auto now=NowNs();if(last_frame_time && now>=last_frame_time)RecordFrameInterval(now-last_frame_time);
    last_frame_time=now;Poll();
}
void RecordGpu(uint64_t frame,uint64_t segment,double ns) {
    std::lock_guard lock(gpu_mutex);++gpu_count;gpu_sum+=ns;gpu_max=std::max(gpu_max,ns);
    if(gpu_lines++<256)LOGI("perf: deep-gpu-segment frame=%llu segment=%llu elapsed_ms=%.6f",
        (unsigned long long)frame,(unsigned long long)segment,ns/1e6);
}
void GpuUnavailable(int result) {
    std::lock_guard lock(gpu_mutex);++gpu_errors;
    if(gpu_errors<=8)LOGW("perf: deep-gpu-unavailable result=%d",result);
}
void ReportIfDue() {
    static std::mutex mutex;static std::atomic<uint64_t> last{0};static uint64_t window=0;
    static std::array<std::array<Stats,kMetricCount>,kThreads> previous{};
    static std::array<uint64_t,kThreads> prev_wall{},prev_cpu{};
    static std::array<std::array<Stats,32>,kThreads> old_targets{};
    const auto now=NowNs();const auto previous_report=last.load(std::memory_order_acquire);
    if(now-(previous_report?previous_report:profile_epoch)<kReportNs)return;
    std::unique_lock lock(mutex,std::try_to_lock);if(!lock.owns_lock())return;
    const auto observed=last.load(std::memory_order_relaxed);
    if(now-(observed?observed:profile_epoch)<kReportNs)return;
    const double interval=(now-(observed?observed:profile_epoch))/1e9;
    last.store(now,std::memory_order_release);++window;
    LOGI("perf: deep-window window=%llu interval_s=%.6f dropped_threads=%llu",(unsigned long long)window,interval,
        (unsigned long long)dropped_threads.load(std::memory_order_relaxed));
    for(size_t index=0;index<slots.size();++index) {
        auto& slot=slots[index];if(!slot.used.load(std::memory_order_acquire))continue;
        const auto sequence=slot.sequence.load(std::memory_order_acquire);if(sequence&1)continue;
        std::array<Stats,kMetricCount> stats;
        std::array<Stats,32> targets;
        std::array<uint32_t,32> keys;
        const auto wall=slot.wall.load(std::memory_order_relaxed),cpu=slot.cpu.load(std::memory_order_relaxed);
        const auto sample=slot.sample_time.load(std::memory_order_relaxed);
        const int supported=slot.cpu_supported.load(std::memory_order_relaxed);
        for(unsigned i=0;i<kMetricCount;++i) {const auto& in=slot.metrics[i];stats[i]={in.calls.load(),in.wall.load(),in.maximum.load(),in.cpu.load(),in.cpu_samples.load()};}
        for(unsigned i=0;i<targets.size();++i) {const auto& in=slot.targets[i];keys[i]=slot.target_keys[i].load();
            targets[i]={in.calls.load(),in.wall.load(),in.maximum.load(),in.cpu.load(),in.cpu_samples.load()};}
        if(sequence!=slot.sequence.load(std::memory_order_acquire))continue;
        const auto id=slot.id.load(std::memory_order_relaxed),state=slot.state.load(std::memory_order_relaxed);
        const auto dw=wall-prev_wall[index],dc=cpu>=prev_cpu[index]?cpu-prev_cpu[index]:0;
        LOGI("perf: deep-thread window=%llu slot=%u thread=%08X observed_ms=%.6f cpu_supported=%d cpu_ms=%.6f cpu_pct=%.3f age_ms=%.3f state=%s target=%08X cpu_error=%d",
            (unsigned long long)window,unsigned(index),id,dw/1e6,supported,dc/1e6,
            supported==1 && dw?100.0*double(dc)/double(dw):-1.0,now>=sample?(now-sample)/1e6:0.0,
            state && state<=kMetricCount?names[state-1]:"running",slot.target.load(),slot.cpu_error.load());
        prev_wall[index]=wall;prev_cpu[index]=cpu;
        for(unsigned i=0;i<kMetricCount;++i) {
            const auto& current=stats[i];auto& old=previous[index][i];
            if(current.calls!=old.calls)LOGI("perf: deep-scope window=%llu slot=%u thread=%08X metric=%s calls=%llu wall_ms=%.6f lifetime_max_ms=%.6f cpu_samples=%llu cpu_ms=%.6f",
                (unsigned long long)window,unsigned(index),id,names[i],(unsigned long long)(current.calls-old.calls),
                (current.wall_ns-old.wall_ns)/1e6,current.max_ns/1e6,
                (unsigned long long)(current.cpu_samples-old.cpu_samples),(current.cpu_ns-old.cpu_ns)/1e6);
            old=current;
        }
        std::array<unsigned,32> order{};
        for(unsigned i=0;i<order.size();++i)order[i]=i;
        std::sort(order.begin(),order.end(),[&](unsigned a,unsigned b) {
            return supported==1?targets[a].cpu_ns-old_targets[index][a].cpu_ns>targets[b].cpu_ns-old_targets[index][b].cpu_ns:
                targets[a].wall_ns-old_targets[index][a].wall_ns>targets[b].wall_ns-old_targets[index][b].wall_ns;
        });
        for(unsigned rank=0;rank<12;++rank) {
            const auto i=order[rank];const auto& value=targets[i];const auto& old=old_targets[index][i];
            if(!keys[i] || value.calls==old.calls)continue;
            LOGI("perf: deep-guest window=%llu slot=%u thread=%08X rank=%u target=%08X calls=%llu wall_ms=%.6f cpu_ms=%.6f cpu_samples=%llu lifetime_max_ms=%.6f",
                (unsigned long long)window,unsigned(index),id,rank+1,keys[i],(unsigned long long)(value.calls-old.calls),
                (value.wall_ns-old.wall_ns)/1e6,(value.cpu_ns-old.cpu_ns)/1e6,
                (unsigned long long)(value.cpu_samples-old.cpu_samples),value.max_ns/1e6);
        }
        old_targets[index]=targets;
    }
    const auto frames=DrainFrames();
    LOGI("perf: deep-frames window=%llu count=%llu dropped=%llu mean_ms=%.6f p50_ms=%.6f p95_ms=%.6f p99_ms=%.6f max_ms=%.6f low1_fps=%.6f",
        (unsigned long long)window,(unsigned long long)frames.count,(unsigned long long)frames.dropped,
        frames.mean_ms,frames.p50_ms,frames.p95_ms,frames.p99_ms,frames.max_ms,frames.low1_fps);
    {std::lock_guard gpu_lock(gpu_mutex);
        LOGI("perf: deep-gpu window=%llu segments=%llu sum_ms=%.6f max_ms=%.6f errors=%llu segment_lines=%llu",
            (unsigned long long)window,(unsigned long long)gpu_count,gpu_sum/1e6,gpu_max/1e6,
            (unsigned long long)gpu_errors,(unsigned long long)std::min<uint64_t>(gpu_lines,256));
        gpu_count=0;gpu_sum=gpu_max=0;gpu_lines=0;}
#ifndef _WIN32
    rusage usage{};const int status=getrusage(RUSAGE_SELF,&usage);
    LOGI("perf: deep-memory window=%llu supported=%u rss_peak_kib=%llu error=%d",(unsigned long long)window,
        unsigned(status==0),status==0?(unsigned long long)usage.ru_maxrss:0ull,status==0?0:errno);
#else
    LOGI("perf: deep-memory window=%llu supported=0 rss_peak_kib=0 error=0",(unsigned long long)window);
#endif
}
}
#endif
