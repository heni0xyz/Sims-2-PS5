// SPDX-License-Identifier: GPL-3.0-only
// Host-only Kenney CC0 replacement. Guest texture sizes and bytes are untouched.
#include "game/hd_input_prompts.h"
#include "game/font_upscale.h"
#include "app/app.h"
#include "core/log.h"
#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>
#if !defined(_WIN32) || defined(XBE_VULKAN_HOST)
#include "gpu/vulkan.h"
#include "gpu/texture.h"
#include "gpu/shaders.h"
namespace {
struct Entry {uint64_t original=0,patched=0;gpu::TextureReplacement texture{};char name[32]{};std::atomic<bool> reported{false};};
std::array<Entry,17> entries;
std::vector<uint8_t> storage;
bool prompt_policy=false;
uint32_t U32(size_t offset) {uint32_t v;std::memcpy(&v,storage.data()+offset,4);return v;}
uint64_t U64(size_t offset) {uint64_t v;std::memcpy(&v,storage.data()+offset,8);return v;}
const gpu::TextureReplacement* Resolve(const gpu::TextureDesc& d,const uint8_t* source,uint32_t bytes,const uint32_t* palette) {
    if(const auto* font=sims2::fonts::Resolve(d,source,bytes,palette))return font;
    if(!prompt_policy)return nullptr;
    if(d.format!=0xe || d.width!=32 || d.height!=32 || d.levels!=3 || d.depth!=1 || d.cube || d.linear || bytes!=1344)return nullptr;
    const auto hash=gpu::Hash(source,bytes);
    for(auto& item:entries)if(hash==item.original || hash==item.patched) {
        if(!item.reported.load(std::memory_order_relaxed) && !item.reported.exchange(true,std::memory_order_relaxed))
            LOGR("[SIMS2][UI][INFO] HD prompt %s guest=32x32 host=%ux%u mips=%u source=%s",item.name,item.texture.width,item.texture.height,item.texture.levels,hash==item.original?"original Xbox":"patched Kenney");
        return &item.texture;
    }
    return nullptr;
}
void Load() {
    auto folder=app::ExeDir()/"public-resources";
#if defined(SIMS2_HEADLESS_RUNTIME)
    if(const auto* path=std::getenv("SIMS2_PUBLIC_RESOURCES"))folder=path;
#endif
    const auto path=folder/"dualsense.s2tex";
    std::ifstream stream(path,std::ios::binary|std::ios::ate);
    if(!stream)throw std::runtime_error("HD prompts enabled but public-resources/dualsense.s2tex is missing");
    const auto size=stream.tellg();
    if(size<1128 || size>8*1024*1024)throw std::runtime_error("Invalid HD prompt pack size");
    storage.resize(size_t(size));stream.seekg(0);
    if(!stream.read(reinterpret_cast<char*>(storage.data()),std::streamsize(storage.size())))throw std::runtime_error("Truncated HD prompt pack read");
    if(std::memcmp(storage.data(),"S2HDPACK",8) || U32(8)!=1 || U32(12)!=entries.size() || U32(16)!=256 || U32(20)!=256 || U32(24)!=9 || U32(28)!=64)
        throw std::runtime_error("Unsupported HD prompt pack header");
    if(gpu::Hash(storage.data(),storage.size()-8)!=U64(storage.size()-8))throw std::runtime_error("HD prompt pack checksum mismatch");
    uint64_t next=32+entries.size()*64;
    for(size_t i=0;i<entries.size();++i) {
        const auto at=32+i*64;auto& e=entries[i];e.original=U64(at);e.patched=U64(at+8);
        const auto offset=U32(at+16),bytes=U32(at+20);
        if(offset!=next || uint64_t(offset)+bytes>storage.size()-8 || bytes!=gpu::ReplacementBytes(256,256,9))throw std::runtime_error("Invalid HD prompt mip range");
        std::memcpy(e.name,storage.data()+at+24,32);
        if(!e.original || !e.patched || !std::memchr(e.name,0,32))throw std::runtime_error("Invalid HD prompt identity");
        e.texture={256,256,9,bytes,storage.data()+offset};next+=bytes;
    }
    if(next!=storage.size()-8)throw std::runtime_error("Unexpected HD prompt pack trailing data");
    LOGR("[SIMS2][UI][INFO] HD prompt pack loaded: %zu textures 256x256, 9 mips, %zu bytes; source=Kenney CC0",entries.size(),storage.size());
}
}
#endif
namespace sims2::prompts {
bool enabled=false;
const gpu::TextureReplacement* UiTexture(const char* resource){
#if !defined(_WIN32) || defined(XBE_VULKAN_HOST)
    if(storage.empty())Load();
    for(const auto& entry:entries)if(!std::strcmp(entry.name,resource))return &entry.texture;
#endif
    return nullptr;
}
void Initialize() {
    const bool active=enabled || std::getenv("SIMS2_HD_PROMPTS");
    if(!active && !sims2::fonts::Active())return;
#if !defined(_WIN32) || defined(XBE_VULKAN_HOST)
    prompt_policy=active;
    if(active && storage.empty())Load();
    if(!gpu::SetVulkanTextureReplacement(Resolve))throw std::runtime_error("Cannot register HD prompt replacement policy");
#else
    throw std::runtime_error("HD prompt replacement requires the Vulkan backend");
#endif
}
}
