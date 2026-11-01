#pragma once
#include <cstdint>
namespace sims2::ps5 {
// Initializes the linked ICD and hands a VK_KHR_display surface to the
// existing Xbox Vulkan backend. That backend owns instance/surface teardown.
void InitVideo(uint32_t width,uint32_t height,bool installer=false);
void SetVideoMode(uint32_t width,uint32_t height,uint32_t refresh_millihz);
}
