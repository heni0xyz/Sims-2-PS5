#pragma once
#include <cstdint>

// Diagnostic observations only. Scopes are inclusive and may overlap: never
// add them into CPU utilization. CPU time is read separately from the OS.
namespace deep_profile {
enum class Metric : uint32_t {
    Kernel, HandleLock, WaitLock, ReleaseLock, WaitCvRelock, Delay,
    FileRead, FileWrite, Shader, Pipeline, Acquire, Submit, Present, Fence,
    GuestFrame, GuestScene, GuestTick, GuestTask, Audio, Upload, Input, Count
};
constexpr unsigned kMetricCount=unsigned(Metric::Count);
struct Stats {uint64_t calls=0,wall_ns=0,max_ns=0,cpu_ns=0,cpu_samples=0;};
struct FrameStats {
    uint64_t count=0,dropped=0;
    double mean_ms=0,p50_ms=0,p95_ms=0,p99_ms=0,max_ms=0,low1_fps=0;
};
// Handles timestamp counter wrap, including 64 bits; rejects unsupported data.
bool TimestampNs(uint64_t begin,uint64_t end,uint32_t bits,double period_ns,double& elapsed_ns) noexcept;
#if defined(XBE_DEEP_PROFILE) && XBE_DEEP_PROFILE
uint64_t NowNs() noexcept;
void RegisterThread(uint32_t guest_id) noexcept;
void Poll() noexcept;
Stats LocalStats(Metric metric) noexcept;
class Scope {
public:
    explicit Scope(Metric metric,uint32_t target=0,bool cpu=false) noexcept;
    ~Scope() noexcept;
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
private:
    void* local_=nullptr;
    Metric metric_;
    uint64_t start_=0,cpu_=0;
    uint32_t old_state_=0,old_target_=0,target_=0;
    bool cpu_valid_=false;
};
void RecordFrameInterval(uint64_t ns) noexcept;
FrameStats DrainFrames() noexcept;
void FramePresented(uint64_t frame);
void RecordGpu(uint64_t frame,uint64_t segment,double ns);
void GpuUnavailable(int result);
void ReportIfDue();
#else
inline void RegisterThread(uint32_t) noexcept {}
inline void Poll() noexcept {}
class Scope {public: explicit Scope(Metric,uint32_t=0,bool=false) noexcept {}};
inline void FramePresented(uint64_t) {}
inline void RecordGpu(uint64_t,uint64_t,double) {}
inline void GpuUnavailable(int) {}
inline void ReportIfDue() {}
#endif
}
