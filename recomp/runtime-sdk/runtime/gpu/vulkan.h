#pragma once
// Vulkan renderer seam for the non-Windows (native Linux) build. The Direct3D 12 backend in
// d3d12.cpp owns the Windows side; this header is what the SDL3 app layer (runtime/app/main_sdl.cpp)
// uses to hand the Vulkan instance/surface it made to the backend in vulkan.cpp, and to get the
// handles it needs for ImGui_ImplVulkan_Init back out. Nothing here is visible on Windows.
#if !defined(_WIN32) || defined(XBE_VULKAN_HOST)

#include <cstdint>

#include <vulkan/vulkan.h>

#include "gpu/backend.h"
#include "gpu/draw_viewport.h"
#include "gpu/texture_replacement.h"

namespace gpu {

// Optional title-specific affine viewport for selected draws. Identity is the
// default. Register once before device creation. The callback must be read-only,
// bounded and allocation-free; the cached vertex-program hash is already valid.
// Normalized x_offset/x_scale keep the viewport inside the target. Scissors
// receive the same affine map. Guest state, shader code and clear/blit paths stay.
using DrawViewportCallback = DrawViewport (*)(const State&,uint32_t,uint32_t);
bool SetVulkanDrawViewport(DrawViewportCallback callback);
// Optional extended seam; mutually exclusive with the legacy callback above.
using DrawViewportProbeCallback = DrawViewport (*)(const State&,uint32_t,uint32_t,DrawProbe);
bool SetVulkanDrawViewportProbe(DrawViewportProbeCallback callback);

// Optional host-resolution policy. The default remains app::RenderScale.
// Called at initialization/settings refresh, before allocating targets; the
// same returned scalar applies to every colour/depth/post target. Register once
// before device creation; the callback must be bounded and read-only.
using RenderScaleCallback = float (*)(uint32_t guest_width,uint32_t guest_height,float baseline);
bool SetVulkanRenderScale(RenderScaleCallback callback);
// Host startup screens use the whole buffer, without the guest title crop.
void SetVulkanNativePresentation(bool enabled);
struct TextureDesc;
// Optional immutable host texture substitution. Invoked at most once per
// resource per frame, after its guest bytes are available. Never edits them.
using TextureReplacementCallback = const TextureReplacement* (*)(const TextureDesc&,const uint8_t*,uint32_t,const uint32_t* palette);
bool SetVulkanTextureReplacement(TextureReplacementCallback callback);

// A statically linked ICD can provide commands through a dispatch loader
// (volk on PS5). Set before creating the instance; desktop loader builds
// leave this unset. Callbacks load commands only and do not own the handles.
bool SetVulkanLoaderCallbacks(void (*instance_loaded)(VkInstance), void (*device_loaded)(VkDevice),
                              void (*frame_queued)(uint32_t, uint32_t) = nullptr);

// Instance extensions the window system needs (SDL_Vulkan_GetInstanceExtensions), handed over
// before the instance is created. Optional only for a headless run with no surface at all.
void SetVulkanInstanceExtensions(const char* const* extensions, uint32_t count);

// The backend-owned Vulkan instance: created on the first call (with the extensions above plus
// VK_KHR_surface), stable for the rest of the process, destroyed by the backend's teardown.
// The app layer creates its surface with this instance (SDL_Vulkan_CreateSurface).
VkInstance VulkanInstance();

// The surface the app layer created with VulkanInstance(), handed over before Backend::Init
// (the SDL app layer calls this before kernel::Boot). The backend builds its swapchain on it
// and destroys surface and instance at teardown; the app layer never destroys either.
void SetVulkanSurface(VkInstance instance, VkSurfaceKHR surface);
VkSurfaceKHR VulkanSurface();

// Everything ImGui_ImplVulkan_Init needs, valid once Backend::Init has returned true.
// GetVulkanHandles() before Init returns the zeroed struct (device == VK_NULL_HANDLE) and is
// safe to call from any thread: the struct is filled in once, in Init, and never changes after.
struct VulkanHandles {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    // A pool the backend owns, sized for the overlay; ImGui_ImplVulkan_InitInfo::DescriptorPool.
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    // The swapchain's render pass (one colour attachment, subpass 0, final layout PRESENT_SRC).
    // Dynamic rendering is not used, so ImGui takes the RenderPass path.
    VkRenderPass renderPass = VK_NULL_HANDLE;
    bool dynamicRendering = false;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;  // the swapchain format actually chosen
    uint32_t minImageCount = 0;                       // ImGui_ImplVulkan_InitInfo::MinImageCount
    uint32_t imageCount = 0;                          // ... and ImageCount
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
};
const VulkanHandles& GetVulkanHandles();

}  // namespace gpu

#endif  // !defined(_WIN32)
