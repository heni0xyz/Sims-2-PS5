// SPDX-License-Identifier: GPL-3.0-only
// Only fingerprints and reconstruction code are public. Glyph pixels come
// from the owner's actual bound texture and palette, once per selected atlas.
#include "game/font_upscale.h"
#include "game/font_resample.h"
#include "gpu/texture.h"
#include "gpu/shaders.h"
#include "core/log.h"
#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
namespace sims2::fonts {
bool enabled=false;
#if defined(SIMS2_HEADLESS_RUNTIME)
namespace {std::atomic<int> test_enabled{-1};}
void TestEnabled(bool value){test_enabled.store(value?1:0,std::memory_order_relaxed);}
#endif
bool Active(){
#if defined(SIMS2_HEADLESS_RUNTIME)
    const auto forced=test_enabled.load(std::memory_order_relaxed);if(forced>=0)return forced!=0;
    static const bool environment=std::getenv("SIMS2_FONT_UPSCALE")!=nullptr;
    return enabled || environment;
#else
    return enabled;
#endif
}
namespace {
struct Atlas {const char* name;uint32_t width,height;uint64_t quick,data,palette;std::vector<uint8_t> pixels;gpu::TextureReplacement replacement;};
std::array<Atlas,3> atlases{{
    {"Frankfurter24",512,512,0x43a979c3e884dc3eull,0x18220bcd0f0020a1ull,0xac876a967eb17309ull},
    {"OneStroke24",512,512,0x7eaf41a6c868b8c4ull,0x1f78f74df6e45f0dull,0xac876a967eb17309ull},
    {"System12",256,128,0xd0ce088dff619951ull,0xdcad8e67e8a07351ull,0xb91834fbfab5fd40ull}
}};
}
const gpu::TextureReplacement* Resolve(const gpu::TextureDesc& d,const uint8_t* source,uint32_t bytes,const uint32_t* palette) {
    if(!Active() || d.format!=0xb || d.depth!=1 || d.cube || d.linear || !palette)return nullptr;
    if(!((d.width==512 && d.height==512) || (d.width==256 && d.height==128)) || bytes<d.width*d.height)return nullptr;
    const auto size=d.width*d.height;std::array<uint8_t,96> probe;
    std::memcpy(probe.data(),source,32);std::memcpy(probe.data()+32,source+size/2,32);std::memcpy(probe.data()+64,source+size-32,32);
    const auto quick=gpu::Hash(probe.data(),probe.size());
    for(auto& atlas:atlases) {
        if(d.width!=atlas.width || d.height!=atlas.height || quick!=atlas.quick)continue;
        if(gpu::Hash(source,d.width*d.height)!=atlas.data || gpu::Hash(palette,1024)!=atlas.palette)continue;
        if(atlas.pixels.empty()) {
            std::vector<uint8_t> original;uint32_t pitch=0;
            gpu::DecodeLevel(d,0,0,source,palette,original,pitch);
            if(pitch!=d.width*4 || original.size()!=size_t(d.width)*d.height*4)throw std::runtime_error("Unsupported font atlas decode");
            atlas.pixels=UpscaleBgra(original,d.width,d.height,4);
            atlas.replacement={d.width*4,d.height*4,1,uint32_t(atlas.pixels.size()),atlas.pixels.data()};
            LOGR("[SIMS2][FONT][INFO] atlas=%s guest=%ux%u host=%ux%u method=monotone-cubic-4x bytes=%u metrics=unchanged",
                atlas.name,d.width,d.height,atlas.replacement.width,atlas.replacement.height,atlas.replacement.bytes);
        }
        return &atlas.replacement;
    }
    return nullptr;
}
}
