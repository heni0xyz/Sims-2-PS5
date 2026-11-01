// SPDX-License-Identifier: GPL-3.0-only
// Observe original Continue geometry and its numeric variable provider.
// Publish a title-only ready scope so the original Reflow can reinitialize
// the dialog background after the original calculation finishes writing it.
#include "game/menu_diagnostics.h"
#include "kernel/kernel.h"
#include "core/log.h"
#include <atomic>
#include <climits>
#include <cstdlib>
#include <string>
namespace sims2::menu {bool diagnostics=false,refresh=true;}
namespace {
std::atomic<unsigned> sequence{0},records{0},controller_records{0};
thread_local uint32_t geometry_object=0;
bool Enabled(){
#if defined(SIMS2_HEADLESS_RUNTIME)
    static const bool environment=std::getenv("SIMS2_UI_MENU_DIAGNOSTICS")!=nullptr;
    return sims2::menu::diagnostics || environment;
#else
    return sims2::menu::diagnostics;
#endif
}
bool Readable(uint32_t address,uint32_t size) {
    if(!address || uint64_t(address)+size>UINT32_MAX)return false;
    for(uint64_t page=address&~4095u;page<uint64_t(address)+size;page+=4096)
        if(!mem::IsCommitted(uint32_t(page)))return false;
    return true;
}
std::string Text(uint32_t address,unsigned limit) {
    std::string out;
    for(unsigned i=0;i<limit;i++) {
        if(!Readable(address+i,1))return {};
        const char c=*static_cast<const char*>(kernel::HostPtr(address+i));
        if(!c)return out;out+=c;
    }
    return {};
}
void Geometry(const char* phase,unsigned id,uint32_t object,uint32_t caller) {
    if(!Readable(object,0x180))return;
    LOGR("[SIMS2][MENU][GEOMETRY] event=%u phase=%s object=%08x caller=%08x box=%u box2=%u item_count=%u state=%u previous_state=%u",
        id,phase,object,caller,kernel::R32(object+0x160),kernel::R32(object+0x164),kernel::R32(object+0x17c),
        kernel::R32(object+0x168),kernel::R32(object+0x16c));
}
}
bool sims2::menu::RefreshReady() {
    if(!refresh || !Readable(geometry_object,0x180))return false;
    const auto count=kernel::R32(geometry_object+0x17c);
    const auto box=kernel::R32(geometry_object+0x160),box2=kernel::R32(geometry_object+0x164);
    return count>=1 && count<=8 && box>0 && box<=448 && box2>0 && box2<=box;
}
X86_EXTERN_FUNC(__imp__sub_00140D00);
// The controller's state-2 branch does not call 140D00. Observe both branches
// independently: a successful 140D00 trace alone cannot cover every entry.
// The original guest body and context are passed through without mutation.
X86_EXTERN_FUNC(__imp__sub_001404C0);
X86_FUNC(sub_001404C0) {
    const auto object=kernel::R32(ctx.esp+4),caller=kernel::R32(ctx.esp);
    const bool trace=Enabled() && Readable(object,0x180) &&
        controller_records.load(std::memory_order_relaxed)<96;
    const auto before=trace?kernel::R32(object+0x168):0;
    const bool transition=trace && before!=kernel::R32(object+0x16c);
    if(transition) {
        controller_records.fetch_add(1,std::memory_order_relaxed);
        Geometry("controller-before",sequence.load(std::memory_order_relaxed),object,caller);
    }
    __imp__sub_001404C0(ctx,base);
    if(trace && (transition || before!=kernel::R32(object+0x168))) {
        controller_records.fetch_add(1,std::memory_order_relaxed);
        Geometry("controller-after",sequence.load(std::memory_order_relaxed),object,caller);
    }
}
X86_FUNC(sub_00140D00) {
    const auto object=kernel::R32(ctx.esp+4),caller=kernel::R32(ctx.esp);
    const bool trace=Enabled() && records.load(std::memory_order_relaxed)<96;
    const auto id=trace?++sequence:sequence.load(std::memory_order_relaxed);
    if(trace)Geometry("before",id,object,caller);
    const auto previous=geometry_object;geometry_object=object;
    try {__imp__sub_00140D00(ctx,base);}
    catch(...) {geometry_object=previous;throw;}
    geometry_object=previous;
    if(trace){Geometry("after",id,object,caller);records+=2;}
}
X86_EXTERN_FUNC(__imp__sub_0013E8C0);
X86_FUNC(sub_0013E8C0) {
    if(!Enabled() || records.load(std::memory_order_relaxed)>=96){__imp__sub_0013E8C0(ctx,base);return;}
    const auto object=ctx.ecx;const auto name=Text(kernel::R32(ctx.esp+4),64);
    const bool selected=name=="0M2M_swf_boxHeight" || name=="0M2M_swf_boxHeight2";
    __imp__sub_0013E8C0(ctx,base);
    if(!selected)return;
    const auto value=Text(ctx.eax,24);char* stop=nullptr;
    const auto number=std::strtol(value.c_str(),&stop,10);
    const bool numeric=!value.empty() && stop==value.c_str()+value.size();
    struct Previous {uint32_t object=0;unsigned event=0;long value[2]={LONG_MIN,LONG_MIN};};
    static thread_local Previous previous;
    const auto event=sequence.load(std::memory_order_relaxed);
    const auto index=name.ends_with("2")?1:0;
    if(previous.object!=object || previous.event!=event)previous={object,event,{LONG_MIN,LONG_MIN}};
    const long observed=numeric?number:LONG_MIN+1;
    if(previous.value[index]==observed)return;previous.value[index]=observed;
    ++records;
    const bool readable=Readable(object,0x180);
    LOGR("[SIMS2][MENU][VARIABLE] event=%u object=%08x variable=%s numeric=%d value=%ld readable=%u box=%u box2=%u item_count=%u state=%u previous_state=%u",
        event,object,name.c_str(),numeric,observed,unsigned(readable),
        readable?kernel::R32(object+0x160):0,readable?kernel::R32(object+0x164):0,
        readable?kernel::R32(object+0x17c):0,readable?kernel::R32(object+0x168):0,readable?kernel::R32(object+0x16c):0);
}
