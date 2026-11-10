// Host-only minimum detail for an anamorphic, cropped guest picture.
// Keep a uniform scale for the whole render/post/depth chain: existing copies
// and shader texel steps assume it. No guest dimensions or pointers change.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace gpu {
inline float MinimumPictureScale(uint32_t guest_width,uint32_t guest_height,uint32_t picture_height,
        uint32_t output_width,uint32_t output_height,uint32_t percent,float baseline) {
    if(!guest_width || !guest_height || !picture_height || picture_height>guest_height ||
       !output_width || !output_height || !percent || !std::isfinite(baseline) || baseline<=0)return 0;
    const float density=std::max(float(output_width)/guest_width,float(output_height)/picture_height);
    return std::max(baseline,density*(float(percent)/100));
}
}
