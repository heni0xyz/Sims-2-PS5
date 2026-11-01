#include "video.h"
#include "services.h"
#include "gpu/vulkan.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <volk.h>

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance,const char*);
namespace {
std::atomic<bool> installer_video{false};
void Check(VkResult result,const char* operation) {
    if (result!=VK_SUCCESS) throw std::runtime_error(std::string("Vulkan ")+operation+" failed: "+std::to_string(result));
}
void InstanceLoaded(VkInstance instance) {volkLoadInstance(instance);}
void DeviceLoaded(VkDevice device) {
    volkLoadDevice(device);
    sims2::ps5::Log("VULKAN","INFO","Logical Vulkan device created","VULKAN");
}
void FrameQueued(uint32_t width,uint32_t height) {
    if(installer_video.load(std::memory_order_relaxed))return;
    static std::atomic<bool> reported{false};
    if (!reported.exchange(true)) {
        char message[160];
        std::snprintf(message,sizeof(message),"Internal render target %ux%u queued for presentation; visibility requires capture",width,height);
        sims2::ps5::Log("GAME","INFO",message,"GAME_FRAME_QUEUED");
    }
}
}
namespace sims2::ps5 {
void InitVideo(uint32_t width,uint32_t height,bool installer) {
    installer_video.store(installer,std::memory_order_relaxed);
    // The template's loader initialization, with the renderer's existing
    // instance/surface seam; no desktop loader or dlopen is involved.
#ifndef NDEBUG
    if (setenv("XBE_VK_STARTUP_TRACE", "1", 1) != 0)
        throw std::runtime_error("Cannot enable bounded Vulkan startup diagnostics");
#endif
#if defined(SIMS2_VULKAN_STARTUP_PROBE)
    if (setenv("XBE_VK_STARTUP_PROBE", "1", 1) != 0)
        throw std::runtime_error("Cannot enable Vulkan completion checkpoint experiment");
#endif
#if defined(SIMS2_PERFORMANCE_LOG)
    if (setenv("XBE_PERF", "1", 1) != 0)
        throw std::runtime_error("Cannot enable renderer performance reports");
#endif
    volkInitializeCustom(vk_icdGetInstanceProcAddr);
    if (!gpu::SetVulkanLoaderCallbacks(InstanceLoaded,DeviceLoaded,FrameQueued))
        throw std::runtime_error("Vulkan dispatch callbacks were configured after initialization");
    const char* extensions[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_DISPLAY_EXTENSION_NAME};
    gpu::SetVulkanInstanceExtensions(extensions,2);
    VkInstance instance=gpu::VulkanInstance();
    if (!instance) throw std::runtime_error("Vulkan instance initialization failed");
    uint32_t count=0;
    Check(vkEnumeratePhysicalDevices(instance,&count,nullptr),"physical-device count");
    std::vector<VkPhysicalDevice> devices(count);
    Check(vkEnumeratePhysicalDevices(instance,&count,devices.data()),"physical-device list");
    for (VkPhysicalDevice device:devices) {
        VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(device,&properties);
        if (properties.apiVersion<VK_API_VERSION_1_2) continue;
        uint32_t extension_count=0;
        Check(vkEnumerateDeviceExtensionProperties(device,nullptr,&extension_count,nullptr),"device-extension count");
        std::vector<VkExtensionProperties> available(extension_count);
        Check(vkEnumerateDeviceExtensionProperties(device,nullptr,&extension_count,available.data()),"device-extension list");
        bool swapchain=false;
        for (const auto& e:available) swapchain|=std::strcmp(e.extensionName,VK_KHR_SWAPCHAIN_EXTENSION_NAME)==0;
        if (!swapchain) continue;
        uint32_t display_count=0,plane_count=0;
        Check(vkGetPhysicalDeviceDisplayPropertiesKHR(device,&display_count,nullptr),"display count");
        std::vector<VkDisplayPropertiesKHR> displays(display_count);
        Check(vkGetPhysicalDeviceDisplayPropertiesKHR(device,&display_count,displays.data()),"display list");
        Check(vkGetPhysicalDeviceDisplayPlanePropertiesKHR(device,&plane_count,nullptr),"display-plane count");
        std::vector<VkDisplayPlanePropertiesKHR> planes(plane_count);
        Check(vkGetPhysicalDeviceDisplayPlanePropertiesKHR(device,&plane_count,planes.data()),"display-plane list");
        for (const auto& display:displays) {
            if (!(display.supportedTransforms&VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)) continue;
            uint32_t mode_count=0;
            Check(vkGetDisplayModePropertiesKHR(device,display.display,&mode_count,nullptr),"display-mode count");
            std::vector<VkDisplayModePropertiesKHR> modes(mode_count);
            Check(vkGetDisplayModePropertiesKHR(device,display.display,&mode_count,modes.data()),"display-mode list");
            const VkDisplayModePropertiesKHR* selected=nullptr;
            uint64_t nearest=std::numeric_limits<uint64_t>::max();
            for (const auto& mode:modes) {
                if (mode.parameters.visibleRegion.width!=width||mode.parameters.visibleRegion.height!=height) continue;
                const uint64_t refresh=mode.parameters.refreshRate;
                const uint64_t distance=refresh>60000?refresh-60000:60000-refresh;
                if (distance<nearest) {nearest=distance;selected=&mode;}
            }
            if (!selected) continue;
            for (uint32_t plane=0;plane<plane_count;plane++) {
                if (planes[plane].currentDisplay && planes[plane].currentDisplay!=display.display) continue;
                uint32_t supported_count=0;
                Check(vkGetDisplayPlaneSupportedDisplaysKHR(device,plane,&supported_count,nullptr),"plane-display count");
                std::vector<VkDisplayKHR> supported(supported_count);
                Check(vkGetDisplayPlaneSupportedDisplaysKHR(device,plane,&supported_count,supported.data()),"plane-display list");
                if (std::find(supported.begin(),supported.end(),display.display)==supported.end()) continue;
                VkDisplayPlaneCapabilitiesKHR capabilities{};
                Check(vkGetDisplayPlaneCapabilitiesKHR(device,selected->displayMode,plane,&capabilities),"plane capabilities");
                const VkDisplayPlaneAlphaFlagBitsKHR choices[]={VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
                    VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR,VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR,
                    VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR};
                VkDisplayPlaneAlphaFlagBitsKHR alpha=static_cast<VkDisplayPlaneAlphaFlagBitsKHR>(0);
                for (auto choice:choices) if (capabilities.supportedAlpha&choice) {alpha=choice;break;}
                if (!alpha) continue;
                VkDisplaySurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR};
                info.displayMode=selected->displayMode;info.planeIndex=plane;
                info.planeStackIndex=planes[plane].currentStackIndex;info.transform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
                info.globalAlpha=1;info.alphaMode=alpha;info.imageExtent={width,height};
                VkSurfaceKHR surface=VK_NULL_HANDLE;
                Check(vkCreateDisplayPlaneSurfaceKHR(instance,&info,nullptr,&surface),"display-plane surface");
                gpu::SetVulkanSurface(instance,surface);
                SetVideoMode(width,height,selected->parameters.refreshRate);
                Log("VULKAN","INFO","VK_KHR_display surface handed to the existing renderer");
                return;
            }
        }
    }
    throw std::runtime_error("Vulkan: requested display mode/swapchain support is unavailable");
}
}
