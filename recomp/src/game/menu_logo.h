// Observed USA s2c_logo identity; metadata only, no owner artwork.
// Authored left anchoring, OpenAI Codex for szampan.
#pragma once
#include "gpu/draw_viewport.h"
#include "game/ui_aspect.h"
namespace sims2::ui {
inline constexpr uint64_t kSimsLogoMip=0x01e493ab7db0d458ull;
inline bool CornerLogo(uint64_t program,uint32_t format,uint32_t w,uint32_t h,uint32_t levels,
                       uint64_t mip,const gpu::DrawBounds& b) {
    return program==kDirect && format==0xb && w==512 && h==512 && levels==7 && mip==kSimsLogoMip &&
        std::isfinite(b.left) && std::isfinite(b.top) && std::isfinite(b.right) && std::isfinite(b.bottom) &&
        b.left>=0 && b.top>=0 && b.right<=320 && b.bottom<=240 && b.right>b.left && b.bottom>b.top;
}
}
