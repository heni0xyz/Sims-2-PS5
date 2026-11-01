// Observational game diagnostics, independently decoded from the owner XBE.
// OpenAI Codex for szampan. Original generated bodies always execute unchanged.
#include "kernel/kernel.h"
#include "core/log.h"

X86_EXTERN_FUNC(__imp__sub_001F7EE0);
X86_FUNC(sub_001F7EE0) {
    LOGE("[SIMS2-INDEPENDENT][GAME_FATAL] guest=001F7EE0 disc-error screen; caller=%08X", kernel::R32(ctx.esp));
    kernel::LogGuestStack(ctx, 32);
    __imp__sub_001F7EE0(ctx, base);
}
