#pragma once
#include <array>
#include <cstdint>

// Diagnostic sampled call-wall scopes, not a CPU-cycle or GPU profiler.
namespace guest_profile {
enum class Kind : uint8_t { Indirect, Kernel };
constexpr uint32_t kIndirectStride=128, kKernelStride=32;
struct Entry {
    Kind kind{};
    uint32_t target=0, thread=0;
    uint64_t samples=0, wall_ns=0, max_ns=0;
};
struct Window {
    std::array<Entry,8> top{};
    uint32_t top_count=0;
    uint64_t samples=0, wall_ns=0, dropped=0;
};
#if defined(XBE_GUEST_PROFILE) && XBE_GUEST_PROFILE
class Scope {
public:
    Scope(Kind kind,uint32_t target) noexcept;
    ~Scope() noexcept;
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
    bool Sampled() const noexcept {return active_;}
    void SetThread(uint32_t value) noexcept {thread_=value;}
private:
    Kind kind_;
    uint32_t target_, thread_=0;
    uint64_t started_=0;
    bool active_=false;
};
bool Record(Kind kind,uint32_t target,uint64_t elapsed_ns,uint32_t thread=0) noexcept;
Window Drain() noexcept;
void ReportIfDue();
#else
class Scope {
public:
    Scope(Kind,uint32_t) noexcept {}
    bool Sampled() const noexcept {return false;}
    void SetThread(uint32_t) noexcept {}
};
inline bool Record(Kind,uint32_t,uint64_t,uint32_t=0) noexcept {return false;}
inline Window Drain() noexcept {return {};}
inline void ReportIfDue() {}
#endif
}
