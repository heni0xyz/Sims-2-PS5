// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "game/cas_layout.h"
#include "kernel/kernel.h"
#include "core/log.h"
#include <atomic>

namespace {
std::atomic<uint32_t> owner{0};
std::atomic<bool> editor_active{false};
std::atomic<bool> modal_visible{false};
bool Target(uint32_t pointer) {
    if(!pointer || uint64_t(pointer)+0x1268>uint64_t(UINT32_MAX)+1)return false;
    for(uint64_t page=pointer&~4095u;page<uint64_t(pointer)+0x1268;page+=4096)
        if(!mem::IsCommitted(uint32_t(page)))return false;
    return kernel::R32(pointer)==0x00304CC4 && kernel::R32(pointer+0x80)==0x00306070;
}
void Publish(uint32_t pointer) {
    if(pointer!=owner.load(std::memory_order_relaxed) || !Target(pointer))return;
    const auto screen=kernel::R32(pointer+0x90);
    const bool active=(kernel::R32(pointer+0x121C)&0x40) && kernel::R32(pointer+0x1224)!=0 &&
        sims2::ui::CasEditorScreen(screen);
    if(editor_active.exchange(active,std::memory_order_release)!=active)
        LOGR("[SIMS2][UI][INFO] CAS editor active=%u screen=%u",unsigned(active),screen);
}
}
bool sims2::ui::CasEditorActive() {
    return editor_active.load(std::memory_order_acquire) && !modal_visible.load(std::memory_order_acquire);
}
void sims2::ui::NoteCasModal(std::string_view operation,std::string_view payload) {
    if(const int change=CasModalChange(operation,payload)) {
        const bool visible=change>0;
        if(modal_visible.exchange(visible,std::memory_order_release)!=visible)
            LOGR("[SIMS2][UI][INFO] centered dialog visible=%u CAS edges suspended=%u",unsigned(visible),unsigned(visible));
    }
}
X86_EXTERN_FUNC(__imp__sub_00116820);
X86_EXTERN_FUNC(__imp__sub_00116EB0);
X86_EXTERN_FUNC(__imp__sub_001197A0);
X86_EXTERN_FUNC(__imp__sub_00119910);
X86_FUNC(sub_00116820) {
    __imp__sub_00116820(ctx,base);
    // The verified constructor returns the native CASTarget in EAX. Merely
    // record it; all guest registers, allocations and callbacks remain intact.
    if(Target(ctx.eax)){owner.store(ctx.eax,std::memory_order_release);
        LOGR("[SIMS2][UI][INFO] CAS target observed guest=%08x",ctx.eax);}
}
X86_FUNC(sub_00116EB0) {
    uint32_t expected=ctx.ecx;
    if(owner.compare_exchange_strong(expected,0,std::memory_order_acq_rel))editor_active.store(false,std::memory_order_release);
    __imp__sub_00116EB0(ctx,base);
}
// Snapshot only inside the target's original update callbacks, while its
// lifetime is valid. The GPU worker never dereferences a queued guest object.
X86_FUNC(sub_001197A0) {
    const uint32_t pointer=ctx.ecx;__imp__sub_001197A0(ctx,base);Publish(pointer);
}
X86_FUNC(sub_00119910) {
    const uint32_t pointer=ctx.ecx;__imp__sub_00119910(ctx,base);Publish(pointer);
}
