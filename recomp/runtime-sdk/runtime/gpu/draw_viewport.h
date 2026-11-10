#pragma once
#include <cmath>
#include <cstdint>

namespace gpu {
// Lazy, read-only geometry observation. Valid only during the draw callback;
// callers request it for selected objects, avoiding CPU shader evaluation for
// ordinary draws. Bounds are in guest pixels before presentation correction.
struct DrawBounds {float left=0,top=0,right=0,bottom=0;};
struct DrawProbe {
    const void* context=nullptr;
    bool (*query)(const void*,DrawBounds&)=nullptr;
    bool Bounds(DrawBounds& bounds) const {return query && query(context,bounds);}
};
// Optional draw presentation; identity preserves the original backend behavior.
struct DrawViewport {
    float x_scale=1, x_offset=0;
    bool preserve_fullscreen=false;
    // Apply the affine map before clip rejection. A full-width guest scissor
    // stays full-width so edge-anchored UI may occupy the newly exposed area.
    // Narrower widget scissors still receive the same affine map.
    bool clip_space=false;
};
inline bool ValidDrawViewport(DrawViewport a) {
    return std::isfinite(a.x_scale) && std::isfinite(a.x_offset) &&
        a.x_scale>0 && a.x_offset>=0 && a.x_offset+a.x_scale<=1.00001f;
}
inline void MapDrawScissorX(int32_t& x0,int32_t& x1,float width,DrawViewport a) {
    if(a.clip_space && x0<=0 && x1>=int32_t(width))return;
    x0=int32_t(std::floor(a.x_offset*width+float(x0)*a.x_scale));
    x1=int32_t(std::ceil(a.x_offset*width+float(x1)*a.x_scale));
}
// Evaluated screen-space triangles for an uncoloured cinematic matte. This
// permits separate or batched top/bottom bands, never a centered dialog or
// a narrow widget. Only the opted-in UI presentation path calls this helper.
inline bool FullWidthMatte(const float (*xy)[2],const float (*rgb)[3],uint32_t count,float width,float top,float bottom) {
    if(count<6 || count>12 || count%3 || !(bottom>top) || !(width>0))return false;
    const float band=(bottom-top)*.35f;
    for(uint32_t triangle=0;triangle<count;triangle+=3) {
        unsigned edges=0;bool upper=true,lower=true;
        for(unsigned vertex=0;vertex<3;vertex++) {
            const unsigned i=triangle+vertex;
            const float x=xy[i][0],y=xy[i][1];
            if(!std::isfinite(x) || !std::isfinite(y))return false;
            if(std::fabs(x)<=2)edges|=1;
            else if(std::fabs(x-width)<=2)edges|=2;
            else return false;
            upper=upper && y<=top+band;lower=lower && y>=bottom-band;
            for(unsigned channel=0;channel<3;channel++)if(!std::isfinite(rgb[i][channel]) || std::fabs(rgb[i][channel])>1e-5f)return false;
        }
        if(edges!=3 || (!upper && !lower))return false;
    }
    return true;
}
}
