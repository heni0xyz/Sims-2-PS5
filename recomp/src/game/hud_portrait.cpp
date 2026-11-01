// OpenAI Codex for szampan, 31 October 2026.
// USA 0MOD_head callback 0x147D60 dispatches actor->5EC virtual method +8
// (0x67D60). That method sets a separate normalized camera rectangle through
// 0x1CBC20, called at 0x67E67. It queues rendering on another game thread;
// a thread-local GPU annotation cannot follow that queue. Transform the
// rectangle before the original setter computes/copies the camera state.
// No extracted instructions, generated source or artwork are embedded.
#include "kernel/kernel.h"
#include "game/ui_aspect.h"
#include "game/ui_layout.h"
#include "game/hud_portrait.h"
#include "app/app.h"
#include "core/log.h"
#include <atomic>
#include <cstring>
X86_EXTERN_FUNC(__imp__sub_001CBC20);
X86_FUNC(sub_001CBC20) {
    if(!sims2::ui::enabled || !sims2::ui::native_portrait || !sims2::ui::single_player_hud ||
       app::Get().aspect_ratio!="16:9" || kernel::R32(ctx.esp)!=0x00067E6C) {
        __imp__sub_001CBC20(ctx,base);return;
    }
    const uint32_t pointer=ctx.ecx;
    float original[4],mapped[4];
    std::memcpy(original,kernel::HostPtr(pointer),sizeof(original));
    std::memcpy(mapped,original,sizeof(mapped));
    if(!sims2::ui::MapPortraitRectangle(mapped,sims2::ui::edge_anchors)) {
        __imp__sub_001CBC20(ctx,base);return;
    }
    static std::atomic<bool> reported{false};
    if(!reported.exchange(true))LOGR("[SIMS2][UI][INFO] native portrait camera rect x=%.8f..%.8f -> %.8f..%.8f y=%.8f..%.8f anchor=%s",
        original[0],original[2],mapped[0],mapped[2],original[1],original[3],sims2::ui::edge_anchors?"left":"center");
    std::memcpy(kernel::HostPtr(pointer),mapped,sizeof(mapped));
    try {__imp__sub_001CBC20(ctx,base);}
    catch(...) {std::memcpy(kernel::HostPtr(pointer),original,sizeof(original));throw;}
    std::memcpy(kernel::HostPtr(pointer),original,sizeof(original));
}
