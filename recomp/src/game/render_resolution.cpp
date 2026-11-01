// Preserve uniform colour/depth/post scaling and the game's guest coordinates.
// 640x480 with a 640x448 picture needs scale 6 for a 3840-wide image:
// backing 3840x2880, picture 3840x2688, presented at 3840x2160.
#include "game/render_resolution.h"
#include "app/app.h"
#include "core/log.h"
#include <cstdlib>
#include <stdexcept>
#if !defined(_WIN32) || defined(XBE_VULKAN_HOST)
#include "gpu/vulkan.h"
#include "gpu/render_resolution.h"
#endif
namespace sims2::render {
std::atomic<bool> native_resolution{false};
// The settings API uses bool pointers; rendering reads the atomic switch.
bool configured_native_resolution=false;
void Initialize() {
    native_resolution=configured_native_resolution || std::getenv("SIMS2_NATIVE_RESOLUTION");
#if !defined(_WIN32) || defined(XBE_VULKAN_HOST)
    if(!gpu::SetVulkanRenderScale([](uint32_t width,uint32_t height,float baseline) {
        if(!native_resolution.load(std::memory_order_relaxed))return baseline;
        const auto output=app::Output();
        return gpu::MinimumPictureScale(width,height,app::kGame.picture_height?app::kGame.picture_height:height,
            output.width,output.height,app::Get().render_percent,baseline);
    }))throw std::runtime_error("Cannot register title render-resolution policy");
#endif
    LOGR("[SIMS2][RENDER][INFO] minimum output-detail policy=%d (source textures and movies unchanged)",int(native_resolution.load()));
}
}
