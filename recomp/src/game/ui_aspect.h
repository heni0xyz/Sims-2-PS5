#pragma once
#include <cmath>
#include <cstdint>

namespace sims2::ui {
// Fingerprints of the two USA UI programs translated by the pinned SDK. Both
// project through c121..124; the second first applies c149..151 to local vertices.
// Metadata only: no extracted instructions or game artwork are stored here.
inline constexpr uint64_t kDirect = 0xa5b24efa069dd7e3ull;
inline constexpr uint64_t kLocal = 0x72140698898e582bull;
// Runtime-generated program observed in the owner klog and captured separately
// on the PC. It uses c129..132 plus c97/c98, rather than the static UI matrix.
inline constexpr uint64_t kProjectedTexture = 0x832fedc25ae8faffull;
enum class DrawPath { None, Direct, Local, Screen2D, Projected2D };
inline bool OrthographicScreen(const float* w,const float* scale,const float* offset) {
    for(unsigned i=0;i<4;i++)
        if(!std::isfinite(w[i]) || !std::isfinite(scale[i]) || !std::isfinite(offset[i]))return false;
    return std::fabs(w[0])<1e-6f && std::fabs(w[1])<1e-6f && std::fabs(w[2])<1e-6f &&
        std::fabs(w[3]-1)<1e-6f && std::fabs(scale[0]-320)<1e-4f &&
        std::fabs(scale[1]+224)<1e-4f && std::fabs(offset[0]-320)<1 && std::fabs(offset[1]-240)<1;
}
// The named blackPoly movie in the verified USA root UI has this authored
// transform: (80.0006256, 44.9990997) at (320,224) on its 640x448 stage.
// The runtime capture independently confirms its projection rows. Treat this
// complete layer as full-bleed, including its scissor, regardless of batching.
inline bool FullCanvasDimmer(const float* x,const float* y) {
    constexpr float sx=80.00062561035156f/320, sy=-44.99909973144531f/224;
    for(unsigned i=0;i<4;i++)if(!std::isfinite(x[i]) || !std::isfinite(y[i]))return false;
    return std::fabs(x[0]-sx)<1e-6f && std::fabs(y[1]-sy)<1e-6f &&
        std::fabs(x[1])<1e-6f && std::fabs(x[2])<1e-6f && std::fabs(x[3])<1e-6f &&
        std::fabs(y[0])<1e-6f && std::fabs(y[2])<1e-6f && std::fabs(y[3])<1e-6f;
}
// Same XYZRHW/FVF test as the SDK's vertex assembler. Depth-tested screen
// geometry and ordinary XYZ/world shaders keep their existing projection.
inline DrawPath Classify(uint32_t handle,uint64_t hash,bool depth_enabled,bool orthographic_screen=false) {
    if(handle&1) {
        if(hash==kDirect)return DrawPath::Direct;
        if(hash==kLocal)return DrawPath::Local;
        if(hash==kProjectedTexture && !depth_enabled && orthographic_screen)return DrawPath::Projected2D;
        return DrawPath::None;
    }
    return !depth_enabled && (handle&0xE)==0x4?DrawPath::Screen2D:DrawPath::None;
}
inline const char* PathName(DrawPath path) {
    switch(path) {
        case DrawPath::Direct:return "direct";
        case DrawPath::Local:return "local";
        case DrawPath::Screen2D:return "screen-2d";
        case DrawPath::Projected2D:return "projected-2d";
        default:return "none";
    }
}
inline bool enabled = false;  // desktop defaults stay unchanged; PS5 sets its default before Load
inline bool native_portrait=true;

inline bool MovieTexture(uint32_t format,uint32_t width,uint32_t height,uint32_t levels,bool linear) {
    return format==0x24 || format==0x25 ||
        (linear && width==640 && height==448 && levels==1 && (format==0x12 || format==0x1e));
}

inline float ScaleForPath(DrawPath path,uint32_t width,uint32_t height,
                          float presented_aspect,bool active,uint32_t texture_format=0xffffffffu) {
    if (!active || path==DrawPath::None || width != 640 || height != 480 ||
        texture_format==0x24 || texture_format==0x25 ||
        !std::isfinite(presented_aspect) || presented_aspect <= 640.0f / 448.0f) return 1;
    // A square in the authored 640x448 picture must remain square after the
    // picture is presented at 16:9. At 1080p: 32*3*(45/56) == 32*1080/448.
    return (640.0f / 448.0f) / presented_aspect;
}
inline float HorizontalScale(uint64_t program,uint32_t width,uint32_t height,
                             float aspect,bool active,uint32_t format=0xffffffffu) {
    return ScaleForPath(Classify(1,program,false),width,height,aspect,active,format);
}
void Initialize();
}
