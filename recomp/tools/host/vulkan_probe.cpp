// Read-only local Vulkan capabilities, OpenAI Codex for szampan.
#define VK_NO_PROTOTYPES
#include <windows.h>
#include <vulkan/vulkan.h>
#include <cstdio>
#include <string>
#include <vector>
int main() {
    wchar_t directory[MAX_PATH];
    if (!GetSystemDirectoryW(directory,MAX_PATH)) return 1;
    auto library=LoadLibraryW((std::wstring(directory)+L"\\vulkan-1.dll").c_str());
    if (!library) {std::puts("VULKAN_LOADER_MISSING");return 2;}
    auto get=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(library,"vkGetInstanceProcAddr"));
    if (!get) return 1;
    auto create=reinterpret_cast<PFN_vkCreateInstance>(get(nullptr,"vkCreateInstance"));
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName="Sims2Independent host capability probe";app.apiVersion=VK_API_VERSION_1_2;
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};info.pApplicationInfo=&app;
    VkInstance instance{};auto result=create(&info,nullptr,&instance);
    std::printf("VULKAN_12_INSTANCE result=%d\n",int(result));
    if (result!=VK_SUCCESS) {
        app.apiVersion=VK_API_VERSION_1_0;
        result=create(&info,nullptr,&instance);
        std::printf("VULKAN_10_INSTANCE result=%d\n",int(result));
        if(result!=VK_SUCCESS)return 2;
    }
    auto enumerate=reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(get(instance,"vkEnumeratePhysicalDevices"));
    auto properties=reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(get(instance,"vkGetPhysicalDeviceProperties"));
    auto destroy=reinterpret_cast<PFN_vkDestroyInstance>(get(instance,"vkDestroyInstance"));
    uint32_t count=0;result=enumerate(instance,&count,nullptr);
    if(result!=VK_SUCCESS){destroy(instance,nullptr);return 1;}
    std::vector<VkPhysicalDevice> devices(count);
    result=enumerate(instance,&count,devices.data());
    if(result!=VK_SUCCESS){destroy(instance,nullptr);return 1;}
    for(auto device:devices){VkPhysicalDeviceProperties p{};properties(device,&p);
        std::printf("DEVICE name=%s api=%u.%u.%u type=%u vendor=%04X\n",p.deviceName,
            VK_API_VERSION_MAJOR(p.apiVersion),VK_API_VERSION_MINOR(p.apiVersion),VK_API_VERSION_PATCH(p.apiVersion),unsigned(p.deviceType),p.vendorID);}
    destroy(instance,nullptr);return devices.empty()?2:0;
}
