#include "game/ui_aspect.h"
#include "game/ui_layout.h"
#include "game/menu_logo.h"
#include "game/cas_layout.h"
#include "app/app.h"
#include "core/log.h"
#include <atomic>
#include <cstdlib>
#include <stdexcept>

#if !defined(_WIN32) || defined(XBE_VULKAN_HOST)
#include "app/platform.h"
#include "gpu/vulkan.h"
#include "gpu/texture.h"
#include "gpu/shaders.h"
#include "kernel/kernel.h"
#if defined(SIMS2_HEADLESS_RUNTIME) && defined(XBE_VULKAN_HOST)
void TraceUiTexture(const gpu::State&,const gpu::TextureDesc&,uint64_t);
#endif
namespace {
gpu::DrawViewport UiViewport(const gpu::State& state,uint32_t width,uint32_t height,gpu::DrawProbe probe) {
    if (!sims2::ui::enabled || width!=640 || height!=480) return {};
    const auto hash=(state.vs_handle&1) && state.vs?state.vs->hash:0;
    const bool projected=hash==sims2::ui::kProjectedTexture &&
        state.viewport.x==0 && state.viewport.y==0 && state.viewport.w==640 && state.viewport.h==480 &&
        sims2::ui::OrthographicScreen(state.vsc[132].data(),state.vsc[97].data(),state.vsc[98].data());
    const auto path=sims2::ui::Classify(state.vs_handle,hash,state.rs[gpu::kRsZEnable]!=0,projected);
    if(path==sims2::ui::DrawPath::Projected2D &&
       sims2::ui::FullCanvasDimmer(state.vsc[129].data(),state.vsc[130].data())) {
        static std::atomic<bool> reported{false};
        if(!reported.exchange(true,std::memory_order_relaxed))
            LOGR("[SIMS2][UI][INFO] named blackPoly full-bleed viewport+scissor batch-independent");
        return {};
    }
    if(path==sims2::ui::DrawPath::None) {
        // One bounded record per unselected no-depth route helps identify any
        // further UI programs without changing their behavior or logging per draw.
        if(std::getenv("SIMS2_UI_TRACE") && !state.rs[gpu::kRsZEnable]) {
            struct Key {uint64_t hash;uint32_t handle;};
            static Key routes[24]{};static unsigned count=0;
            bool seen=false;
            for(unsigned i=0;i<count;i++)if(routes[i].hash==hash && (hash || routes[i].handle==state.vs_handle))seen=true;
            if(!seen && count<24) {
                routes[count++]={hash,state.vs_handle};
                LOGR("[SIMS2][UI][INFO] unselected route shader=%016llx handle=%08x depth=0 (diagnostic, not classified as UI)",
                     (unsigned long long)hash,state.vs_handle);
                const auto& w=state.vsc[132];const auto& other=state.vsc[168];
                LOGR("[SIMS2][UI][INFO] route-projection shader=%016llx viewport=%u,%u,%u,%u c132=%.6g,%.6g,%.6g,%.6g c168=%.6g,%.6g,%.6g,%.6g words=%zu",
                     (unsigned long long)hash,state.viewport.x,state.viewport.y,state.viewport.w,state.viewport.h,
                     w[0],w[1],w[2],w[3],other[0],other[1],other[2],other[3],state.vs?state.vs->program.size():0);
                // Bounded, once per program, to analyze another omitted route
                // from the owner's saved klog without requiring a new dump build.
                if(std::getenv("SIMS2_UI_PROGRAM_TRACE") && state.vs && hash && state.vs->program.size()<=136*4) {
                    const auto& words=state.vs->program;
                    for(size_t i=0;i+4<=words.size();i+=4)
                        LOGR("[SIMS2][UI][INFO] route-program shader=%016llx instruction=%zu words=%08x,%08x,%08x,%08x",
                             (unsigned long long)hash,i/4,words[i],words[i+1],words[i+2],words[i+3]);
                }
            }
        }
        return {};
    }
    gpu::TextureDesc texture{};
    const bool has_texture=state.textures[0] && gpu::Describe(state.textures[0],texture);
    const uint32_t format=has_texture?texture.format:0xffffffffu;
    static std::atomic<bool> movie_reported{false};
    if(has_texture && sims2::ui::MovieTexture(format,texture.width,texture.height,texture.levels,texture.linear)) {
        if(!movie_reported.exchange(true,std::memory_order_relaxed))
            LOGR("[SIMS2][UI][INFO] movie bypass shader=%016llx texture_format=%02x size=%ux%u",
                 (unsigned long long)hash,format,texture.width,texture.height);
        return {};
    }
#if defined(SIMS2_HEADLESS_RUNTIME) && defined(XBE_VULKAN_HOST)
    if(has_texture)TraceUiTexture(state,texture,hash);
#endif
    // Vulkan has already computed the cached program hash before calling here.
    // The explicit presentation choice wins, as it does in BlitFrame.
    float aspect=app::Widescreen()?16.0f/9.0f:4.0f/3.0f;
    const auto& selected=app::Get().aspect_ratio;
    if(selected=="4:3")aspect=4.0f/3.0f;
    else if(selected=="16:10")aspect=16.0f/10.0f;
    else if(selected=="21:9")aspect=21.0f/9.0f;
    else if(selected=="32:9")aspect=32.0f/9.0f;
    else if(selected=="16:9")aspect=16.0f/9.0f;
    else if(app::HostAspect()>0)aspect=app::HostAspect();
    const float scale=sims2::ui::ScaleForPath(path,width,height,aspect,sims2::ui::enabled,format);
    if(scale==1)return {};
    if(sims2::ui::cas_edge_anchors.load(std::memory_order_relaxed) && sims2::ui::edge_anchors && selected=="16:9" &&
       !state.rs[gpu::kRsZEnable] && sims2::ui::CasEditorActive()) {
        gpu::DrawBounds bounds{};
        if(probe.Bounds(bounds))if(const int anchor=sims2::ui::CasDrawAnchor(bounds)) {
            static std::atomic<unsigned> reported{0};const unsigned bit=anchor<0?1:2;
            if(!(reported.fetch_or(bit,std::memory_order_relaxed)&bit))
                LOGR("[SIMS2][UI][INFO] CAS editor anchor=%s bounds=%.3f,%.3f..%.3f,%.3f x_scale=%.9f",
                    anchor<0?"left":"right",bounds.left,bounds.top,bounds.right,bounds.bottom,scale);
            return {scale,anchor<0?0.f:1-scale,true,true};
        }
    }
    // The same artwork is also shown large and centered during startup. Match
    // both its exact mip identity and the evaluated upper-left draw bounds.
    if(sims2::ui::edge_anchors && selected=="16:9" && hash==sims2::ui::kDirect &&
       has_texture && format==0xb && texture.width==512 && texture.height==512 && texture.levels==7) {
        gpu::DrawBounds bounds{};
        if(probe.Bounds(bounds) && bounds.left>=0 && bounds.top>=0 && bounds.right<=320 && bounds.bottom<=240) {
            const auto bytes=gpu::LevelSize(texture,0);
            bool committed=bytes==262144 && uint64_t(texture.data)+bytes<=uint64_t(UINT32_MAX)+1;
            for(uint64_t p=texture.data&~4095u;committed && p<uint64_t(texture.data)+bytes;p+=4096)
                committed=mem::IsCommitted(uint32_t(p));
            const auto mip=committed?gpu::Hash(kernel::HostPtr(texture.data),bytes):0;
            if(sims2::ui::CornerLogo(hash,format,texture.width,texture.height,texture.levels,mip,bounds)) {
                static std::atomic<bool> reported{false};
                if(!reported.exchange(true))LOGR("[SIMS2][UI][INFO] mode logo anchor=left bounds=%.3f,%.3f..%.3f,%.3f x_scale=%.9f x_offset=0",
                    bounds.left,bounds.top,bounds.right,bounds.bottom,scale);
                return {scale,0,true,true};
            }
        }
    }
    static std::atomic<unsigned> seen{0};
    const unsigned bit=path==sims2::ui::DrawPath::Direct?1:path==sims2::ui::DrawPath::Local?2:path==sims2::ui::DrawPath::Screen2D?4:8;
    if(!(seen.load(std::memory_order_relaxed)&bit) && !(seen.fetch_or(bit,std::memory_order_relaxed)&bit))
        LOGR("[SIMS2][UI][INFO] aspect correction shader=%016llx path=%s handle=%08x source=640x448 target_aspect=%.6f x_scale=%.9f anchor=center scissor=matched texture_format=%02x texture=%ux%u linear=%u",
             (unsigned long long)hash,sims2::ui::PathName(path),state.vs_handle,aspect,scale,format,texture.width,texture.height,unsigned(texture.linear));
    return {scale,(1-scale)*0.5f,true,true};
}
}
#endif

void sims2::ui::Initialize() {
#if !defined(_WIN32) || defined(XBE_VULKAN_HOST)
    if(!gpu::SetVulkanDrawViewportProbe(UiViewport))
        throw std::runtime_error("UI draw viewport was registered after Vulkan device creation or twice");
    LOGR("[SIMS2][UI][INFO] ui_aspect_correction=%u policy=USA-UI+screen-2d+guarded-projected-2d centered-safe-area world=unchanged",
         unsigned(enabled));
#endif
}
