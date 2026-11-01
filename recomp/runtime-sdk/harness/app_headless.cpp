// the host side without a window: settings at their defaults, the run folder as the exe folder
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "app/app.h"
#include "core/log.h"
#include "harness.h"

namespace app {

Settings& Get() {
    static Settings s;
    return s;
}
void Load() {}
void Save() {}
void ApplyDisplay() {}
Mode Desktop() {
    static const Mode mode=[] {
        const long w=harness::Env("HARNESS_OUTPUT_WIDTH"),h=harness::Env("HARNESS_OUTPUT_HEIGHT");
        if(!w && !h)return Mode{1280,720};
        if(w<1 || h<1 || w>4096 || h>4096)throw std::runtime_error("Invalid offscreen extent");
        return Mode{uint32_t(w),uint32_t(h)};
    }();
    return mode;
}
Mode Output() { return Desktop(); }
float RenderScale(uint32_t) { return 1.0f; }
bool Widescreen() { return true; }

std::filesystem::path ExeDir() {
    static const std::filesystem::path dir = std::filesystem::current_path();
    return dir;
}
int Instance() {
    static const int n = [] {
        const char* e = std::getenv("INSTANCE");
        return e ? std::atoi(e) : 0;
    }();
    return n;
}
void* Window() { return nullptr; }

bool OpenSplash() { return false; }
void SplashFirstFrame() {}
void CloseSplash() {}
void OpenGameWindow(bool) {}
bool StartHandover() { return false; }
bool HandingOver() { return false; }
void FinishHandover() {}

std::recursive_mutex& UiLock() {
    static std::recursive_mutex m;
    return m;
}

void Pace() {
#if defined(XBE_VULKAN_HOST)
    // Logical offscreen frame count, not a claim of display presentation.
    const auto frame = harness::g_frames.fetch_add(1) + 1;
    if (frame % 60 == 0) LOGI("renderer: frame %llu (Vulkan offscreen)", (unsigned long long)frame);
#endif
    // Optional correctness runs use the title's target cadence. Unset/zero
    // retains the SDK harness's original unpaced throughput-probe behaviour.
    static const long fps = [] {
        const long value = harness::Env("HARNESS_FPS");
        LOGR("harness: pacing fps=%ld", value);
        return value;
    }();
    if (fps < 1 || fps > 240) return;
    using clock = std::chrono::steady_clock;
    static thread_local auto due = clock::now();
    const auto period = std::chrono::nanoseconds(1000000000ll / fps);
    due = std::max(due + period, clock::now());
    std::this_thread::sleep_until(due);
}

}  // namespace app
