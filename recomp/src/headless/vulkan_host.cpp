// Native Windows offscreen validation; no game or generated code is replaced.
// OpenAI Codex for szampan; loader is the template's MIT volk dependency.
#if defined(XBE_VULKAN_HOST)
#include <volk.h>
#include "app/platform.h"
#include "gpu/vulkan.h"
#include "core/log.h"
#include <stdexcept>
namespace app::platform {
Size ClientSize() { return {}; }  // offscreen: no window exists
}
void InitializeHostVulkan() {
    if (volkInitialize() != VK_SUCCESS)
        throw std::runtime_error("Native Windows Vulkan loader unavailable");
    if (!gpu::SetVulkanLoaderCallbacks(volkLoadInstance,volkLoadDevice))
        throw std::runtime_error("Native Windows Vulkan loader registration failed");
    LOGR("harness: backend=VULKAN_OFFSCREEN");
}
#endif
