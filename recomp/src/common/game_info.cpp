// Independent title declaration using the MIT XboxXBERecomp GameInfo interface.
// OpenAI Codex, for szampan. Authored UI patch provenance: PATCH_IMPORTS.json.
#include "app/app.h"
#include "core/log.h"
#include "game/ui_aspect.h"
#include "game/ui_layout.h"
#include "game/render_resolution.h"
#include "game/hd_input_prompts.h"
#include "game/font_upscale.h"
#include "game/menu_diagnostics.h"
#include "xdk/d3d8.h"
#include <cstdlib>
void PollUiDiagnostic(int port,input::Pad&);
#if defined(XBE_VULKAN_HOST)
void InitializeHostVulkan();
#endif

namespace {
bool asyncGpuCallbacks=false; // opt in only after the full GPU integration test is accepted
void Initialize() {
#if defined(XBE_VULKAN_HOST)
    InitializeHostVulkan();
#endif
    if(const char* aspect=std::getenv("SIMS2_TEST_ASPECT")) app::Get().aspect_ratio=aspect;
    sims2::ui::enabled=app::Get().aspect_ratio=="16:9";
    if(std::getenv("SIMS2_GPU_CALLBACKS_SYNC"))asyncGpuCallbacks=false;
    else if(std::getenv("SIMS2_GPU_CALLBACKS_ASYNC"))asyncGpuCallbacks=true;
    d3d::SetAsyncGpuCallbacks(asyncGpuCallbacks);
    sims2::ui::Initialize();
    sims2::render::Initialize();
    sims2::prompts::Initialize();
    LOGR("[SIMS2-INDEPENDENT][BOOT] host services initialized; starting fresh recompilation");
}
}

const app::GameInfo app::kGame = {
    .title = L"The Sims 2",
    .short_title = L"The Sims 2",
    .name = L"Sims2Independent",
    .log = "sims2-independent",
    .settings = "settings.toml",
    .splash = nullptr,
    .missing = L"Supply your USA Xbox disc dump.",
    .images = {{"default.xbe", "Sims2Independent"}},
    .options = {{"ui_aspect_correction", &sims2::ui::enabled, nullptr},{"ui_edge_anchors",&sims2::ui::edge_anchors,nullptr},{"ui_native_portrait",&sims2::ui::native_portrait,nullptr},{"gpu_callbacks_async",&asyncGpuCallbacks,nullptr},{"native_render_resolution",&sims2::render::configured_native_resolution,nullptr},{"hd_input_prompts",&sims2::prompts::enabled,nullptr},{"ui_menu_diagnostics",&sims2::menu::diagnostics,nullptr},{"ui_continue_refresh",&sims2::menu::refresh,nullptr},{"ui_font_upscale",&sims2::fonts::enabled,nullptr}},
    .load = nullptr,
    .save = nullptr,
    .init = Initialize,
    .loaded = nullptr,
    .window = nullptr,
    .message = nullptr,
    .ui = nullptr,
    .pad = PollUiDiagnostic,
    .rumble = nullptr,
    .picture_top = 16,
    .picture_height = 448,
};
