// Bounded PC-only observations of actual UI texture/palette bindings.
// Original owner pixels stay in the ignored run directory. Codex for szampan.
#if defined(SIMS2_HEADLESS_RUNTIME) && defined(XBE_VULKAN_HOST)
#include "app/app.h"
#include "gpu/texture.h"
#include "gpu/shaders.h"
#include "kernel/kernel.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <vector>
namespace {
bool Committed(uint32_t address,uint32_t bytes) {
    if(!address || !bytes || uint64_t(address)+bytes>uint64_t(UINT32_MAX)+1)return false;
    for(uint64_t p=address&~4095u;p<uint64_t(address)+bytes;p+=4096)
        if(!mem::IsCommitted(uint32_t(p)))return false;
    return true;
}
}
void TraceUiTexture(const gpu::State& state,const gpu::TextureDesc& d,uint64_t program) {
    static const bool enabled=std::getenv("SIMS2_UI_TEXTURE_TRACE")!=nullptr;
    if(!enabled || d.width*d.height<8192 || d.width>512 || d.height>512 || d.depth!=1 || d.cube)return;
    static std::array<uint64_t,48> seen{};static unsigned count=0;
    if(count==seen.size())return;
    const auto bytes=gpu::LevelSize(d,0);
    if(!Committed(d.data,bytes))return;
    std::array<uint32_t,256> palette{};const uint32_t* colours=nullptr;
    if(d.format==0xb) {
        const auto header=state.palettes[0];if(!Committed(header,8))return;
        const auto va=mem::VirtualFromPhysical(kernel::R32(header+4));
        const unsigned size=1024u>>((kernel::R32(header)>>30)&3);
        if(!Committed(va,size))return;
        std::memcpy(palette.data(),kernel::HostPtr(va),size);colours=palette.data();
    }
    const auto dataHash=gpu::Hash(kernel::HostPtr(d.data),bytes);
    const auto paletteHash=colours?gpu::Hash(palette.data(),sizeof(palette)):0;
    const auto key=dataHash^paletteHash;
    for(unsigned i=0;i<count;i++)if(seen[i]==key)return;
    std::vector<uint8_t> pixels;uint32_t pitch=0;
    gpu::DecodeLevel(d,0,0,static_cast<const uint8_t*>(kernel::HostPtr(d.data)),colours,pixels,pitch);
    if(pitch!=d.width*4 || pixels.size()!=size_t(d.width)*d.height*4)return;
    seen[count++]=key;
    const auto folder=app::ExeDir()/"ui-textures";std::filesystem::create_directories(folder);
    char name[80];std::snprintf(name,sizeof(name),"%02u-%016llx",count,(unsigned long long)key);
    const auto path=folder/(std::string(name)+".bmp");
    if(FILE* f=std::fopen(path.string().c_str(),"wb")) {
        uint8_t h[54]={'B','M'};auto put=[&](int at,uint32_t v){std::memcpy(h+at,&v,4);};
        put(2,54+uint32_t(pixels.size()));put(10,54);put(14,40);put(18,d.width);put(22,uint32_t(-int32_t(d.height)));h[26]=1;h[28]=32;put(34,uint32_t(pixels.size()));
        std::fwrite(h,1,54,f);std::fwrite(pixels.data(),1,pixels.size(),f);std::fclose(f);
    }
    const auto info=folder/(std::string(name)+".json");
    if(FILE* f=std::fopen(info.string().c_str(),"w")) {
        std::fprintf(f,"{\"format\":%u,\"width\":%u,\"height\":%u,\"levels\":%u,\"bytes\":%u,\"program\":\"%016llx\",\"data_hash\":\"%016llx\",\"palette_hash\":\"%016llx\",\"source\":\"%08x\"}\n",d.format,d.width,d.height,d.levels,bytes,(unsigned long long)program,(unsigned long long)dataHash,(unsigned long long)paletteHash,d.data);std::fclose(f);
    }
}
#endif
