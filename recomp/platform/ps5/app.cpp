#include "app/app.h"
#include "app/platform.h"
#include "services.h"
#include "video.h"
#include "game/ui_aspect.h"
#include "game/ui_layout.h"
#include "game/render_resolution.h"
#include "game/hd_input_prompts.h"
#include "game/font_upscale.h"
#include "game/menu_diagnostics.h"
#include "xdk/d3d8.h"
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace {
bool asyncGpuCallbacks=false; // experimental queue is not yet accepted by a full GPU run
void InitializeGame() {
    d3d::SetAsyncGpuCallbacks(asyncGpuCallbacks);
    sims2::ui::Initialize();
    sims2::render::Initialize();
    sims2::prompts::Initialize();
}
std::atomic<uint32_t> display_width{1920},display_height{1080};
std::atomic<bool> visible{true},cursor{false};
}
namespace sims2::ps5 {
void SetVideoMode(uint32_t width,uint32_t height,uint32_t refresh_millihz) {
    display_width=width;display_height=height;
    char message[128];
    std::snprintf(message,sizeof(message),"Display surface %ux%u at %u mHz (internal render size is separate)",width,height,refresh_millihz);
    Log("VULKAN","INFO",message);
}
}
namespace app {
const GameInfo kGame={
    .title=L"The Sims 2",.short_title=L"The Sims 2",.name=L"Sims2Independent",.log="sims2-independent",
    .settings="settings.toml",.splash=nullptr,
    .missing=L"Supply the verified USA game files in /app0/assets.",
    .images={{"default.xbe","Sims2Independent"}},.options={{"ui_aspect_correction",&sims2::ui::enabled,nullptr},{"ui_edge_anchors",&sims2::ui::edge_anchors,nullptr},{"ui_native_portrait",&sims2::ui::native_portrait,nullptr},{"gpu_callbacks_async",&asyncGpuCallbacks,nullptr},{"native_render_resolution",&sims2::render::configured_native_resolution,nullptr},{"hd_input_prompts",&sims2::prompts::enabled,nullptr},{"ui_menu_diagnostics",&sims2::menu::diagnostics,nullptr},{"ui_continue_refresh",&sims2::menu::refresh,nullptr},{"ui_font_upscale",&sims2::fonts::enabled,nullptr}},.load=nullptr,.save=nullptr,
    .init=InitializeGame,.loaded=nullptr,.window=nullptr,.message=nullptr,.ui=nullptr,
    .pad=nullptr,.rumble=nullptr,.picture_top=16,.picture_height=448
};
std::filesystem::path ExeDir() {return "/app0";}
int Instance() {return 0;}
void* Window() {return nullptr;}
std::recursive_mutex& UiLock() {static std::recursive_mutex mutex;return mutex;}
void ApplyDisplay() {
    const Mode requested=Output();
    if (requested.width!=display_width||requested.height!=display_height)
        throw std::runtime_error("PS5 display mode changes require restarting the title");
}
bool OpenSplash() {return false;}
void SplashFirstFrame() {} // This pre-present hook is deliberately not a frame milestone.
void CloseSplash() {}
void OpenGameWindow(bool apply) {if (apply) ApplyDisplay();visible=true;}
bool StartHandover() {return false;} // The shell owns native title replacement.
bool HandingOver() {return false;}
void FinishHandover() {}
void LogSystem() {
    sims2::ps5::Log("BOOT","INFO","PS5 static recompilation; linked RADV and glslang; no desktop window system");
    LogSettings();
}
void Pace() {
    using clock=std::chrono::steady_clock;
    static auto next=clock::now();
    const uint32_t cap=Get().fps_cap;
    const auto now=clock::now();
    if (!cap) {next=now;return;}
    const auto period=std::chrono::nanoseconds(1000000000ull/cap);
    next+=period;
    if (next<now-period) next=now;
    while (clock::now()+std::chrono::milliseconds(2)<next) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    while (clock::now()<next) std::this_thread::yield();
}
namespace platform {
NativeWindow NativeHandle() {return nullptr;}
Size ClientSize() {return {display_width.load(),display_height.load()};}
Size DesktopSize() {return ClientSize();}
bool HasFocus() {return visible;}
bool IsVisible() {return visible;}
bool IsMinimized() {return false;}
void SetVisible(bool value) {visible=value;}
void Raise() {visible=true;}
void SetCursorVisible(bool value) {cursor=value;}
bool CursorVisible() {return cursor;}
HdrInfo QueryHdr() {return {false,false,0,"PS5 HDR output is not implemented"};}
}
}
