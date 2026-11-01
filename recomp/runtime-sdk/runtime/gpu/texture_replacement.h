// SPDX-License-Identifier: GPL-3.0-only
// Optional project extension; original SDK license remains in LICENSE.
#pragma once
#include <cstdint>
namespace gpu {
// Immutable host BGRA8 mip chain. The guest descriptor/UVs/readbacks retain
// their original dimensions. Storage must outlive the renderer.
struct TextureReplacement {
    uint32_t width=0,height=0,levels=0,bytes=0;
    const uint8_t* bgra=nullptr;
};
inline uint64_t ReplacementBytes(uint32_t width,uint32_t height,uint32_t levels) {
    if(!width || !height || width>4096 || height>4096 || !levels)return 0;
    uint32_t maximum=1;for(uint32_t n=width>height?width:height;n>1;n>>=1)++maximum;
    if(levels>maximum)return 0;
    uint64_t bytes=0;
    for(uint32_t i=0;i<levels;++i) {
        bytes+=uint64_t(width)*height*4;
        width=width>1?width>>1:1;height=height>1?height>>1:1;
    }
    return bytes;
}
inline bool ValidReplacement(const TextureReplacement& value) {
    return value.bgra && value.bytes && ReplacementBytes(value.width,value.height,value.levels)==value.bytes;
}
}
