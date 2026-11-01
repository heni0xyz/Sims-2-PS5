// SPDX-License-Identifier: GPL-3.0-only
#include "game/install_display.h"
#include "gpu/backend.h"
#include "gpu/vulkan.h"
#include "gpu/texture.h"
#include "kernel/kernel.h"
#include "game/hd_input_prompts.h"
#include "game/install_font.h"
#include "app/app.h"
#include "gpu/texture_replacement.h"
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <cstdlib>
namespace sims2::install {
Display::Display(){try{
    if(!InstallFontLoaded()){
        auto folder=app::ExeDir()/"public-resources";
#if defined(SIMS2_HEADLESS_RUNTIME)
        if(const auto* path=std::getenv("SIMS2_PUBLIC_RESOURCES"))folder=path;
#endif
        std::ifstream stream(folder/"installer-font.s2font",std::ios::binary|std::ios::ate);
        if(!stream || stream.tellg()<44 || stream.tellg()>4*1024*1024)throw std::runtime_error("Installer Open Sans resource is missing or invalid");
        std::vector<uint8_t> bytes(size_t(stream.tellg()));stream.seekg(0);
        if(!stream.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))throw std::runtime_error("Installer Open Sans read failed");
        LoadInstallFont(bytes);
    }
    // The normal Boot sets the same pointer later. No guest code runs yet.
    kernel::g_base=mem::Base();
    if(!kernel::g_base)throw std::runtime_error("Installer guest arena is unavailable");
    data_=mem::AllocContiguous(kInstallWidth*kInstallHeight*4,0,0xffffffff,4096);
    header_=mem::AllocSystem(20);
    if(!data_ || !header_)throw std::runtime_error("Installer framebuffer allocation failed");
    kernel::W32(header_,0x50000);kernel::W32(header_+4,mem::PhysicalFromVirtual(data_));kernel::W32(header_+8,0);
    kernel::W32(header_+12,(0x12u<<8)|(2u<<4));
    kernel::W32(header_+16,(kInstallWidth-1)|((kInstallHeight-1)<<12)|((kInstallWidth*4/64-1)<<24));
    gpu::SetVulkanNativePresentation(true);
    backend_=gpu::CreateBackend();
    if(!backend_ || !backend_->Init(kInstallWidth,kInstallHeight))throw std::runtime_error("Installer Vulkan initialization failed");
}catch(...){backend_.reset();gpu::SetVulkanNativePresentation(false);if(header_)mem::FreeSystem(header_);if(data_)mem::FreeContiguous(data_);throw;}
}
Display::~Display(){backend_.reset();gpu::SetVulkanNativePresentation(false);if(header_)mem::FreeSystem(header_);if(data_)mem::FreeContiguous(data_);}
void Display::Draw(const View& view){
    const auto icon=[](const char* name)->std::span<const uint8_t>{
        const auto* texture=sims2::prompts::UiTexture(name);
        if(!texture || texture->width!=256 || texture->height!=256 || texture->levels!=9)throw std::runtime_error("Installer button icon is unavailable");
        return {texture->bgra,256*256*4};
    };
    auto pixels=DrawInstallScreen(view,{icon("button_xbx_a"),icon("button_xbx_b")});std::memcpy(kernel::HostPtr(data_),pixels.data(),pixels.size()*4);
    gpu::MarkDirty(header_);
    const gpu::Surface front{header_,data_,kernel::R32(header_+12),kernel::R32(header_+16)};
    backend_->Present(front);backend_->Flush();
}
}
