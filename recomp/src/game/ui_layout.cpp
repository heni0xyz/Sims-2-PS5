// OpenAI Codex for szampan, 30 October 2026.
#include "game/ui_layout.h"
#include "game/ui_aspect.h"
#include "game/menu_diagnostics.h"
#include "app/app.h"
#include "kernel/kernel.h"
#include "core/log.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>

namespace {
constexpr float scale=45.0f/56.0f;
constexpr float shift=320.0f*(1-scale)/scale;
int Anchor(std::string_view id) {
    if(id=="0MOD" || id=="0MOT" || id=="0SKL" || id=="0WAF")return -1;
    if(id=="0ACT" || id=="0CXM")return 1;
    return 0;
}
bool RightChild(std::string_view id) {
    return id=="0MOD_pause_swf" || id=="0MOD_fastforward_swf";
}
bool Number(std::string_view text,std::string_view tag,float& value) {
    const auto open="<"+std::string(tag)+">",close="</"+std::string(tag)+">";
    const auto at=text.find(open);
    if(at==text.npos)return false;
    const auto end=text.find(close,at+open.size());
    if(end==text.npos || end-at-open.size()>32)return false;
    const auto token=std::string(text.substr(at+open.size(),end-at-open.size()));
    char* stop=nullptr;value=std::strtof(token.c_str(),&stop);
    return stop==token.c_str()+token.size() && stop!=token.c_str() && std::isfinite(value) && std::fabs(value)<8192;
}
void Put(std::string& text,std::string_view tag,float value,bool insert=false) {
    const auto open="<"+std::string(tag)+">",close="</"+std::string(tag)+">";
    char token[48];std::snprintf(token,sizeof(token),"%.6f",double(value));
    const auto at=text.find(open);
    if(at==text.npos) {if(insert)text+=open+token+close;return;}
    const auto start=at+open.size(),end=text.find(close,start);
    if(end!=text.npos)text.replace(start,end-start,token);
}
std::string_view Id(std::string_view text,std::string_view tag) {
    const auto open="<"+std::string(tag)+">",close="</"+std::string(tag)+">";
    auto at=text.find(open);if(at==text.npos)return {};
    at+=open.size();const auto end=text.find(close,at);
    return end==text.npos?std::string_view{}:text.substr(at,end-at);
}
}

bool sims2::ui::RefreshContinue(std::string& xml,std::string_view operation,bool geometry_ready) {
    if(!geometry_ready || operation!="Reflow" || !xml.starts_with("<reflow>") || xml.size()>65000 ||
        xml.find("<id>0M2M_swf_container</id>")==xml.npos || xml.find("<id>0M2M_swf</id>")!=xml.npos)return false;
    const auto end=xml.find("</reflow>");if(end==xml.npos)return false;
    xml.insert(end,"<id>0M2M_swf</id><swfAnim>a0</swfAnim>");return true;
}

bool sims2::ui::PatchLayout(std::string& xml,std::string_view operation,bool wide,bool single_player) {
    if(!wide || xml.size()>65536)return false;
    if(operation=="LoadWidget" && xml.starts_with("<scrn>")) {
        const int anchor=Anchor(Id(xml,"scrnid"));
        if(!anchor || Id(xml,"plyr")!="0")return false;
        size_t end=xml.find("</scrn>");
        if(end==xml.npos)return false;
        for(const auto tag:{"<subscrn>","<shdr>","<swf>","<txt>","<btn>","<msk>"}) {
            const auto at=xml.find(tag);if(at!=xml.npos && at<end)end=at;
        }
        auto prefix=xml.substr(0,end);
        float y=0,h=448,x=0;
        // Reject known partial/split screens rather than changing their layout.
        if(Number(prefix,"y",y) && y!=0)return false;
        if(Number(prefix,"h",h) && h!=448)return false;
        if(prefix.find("<x>")!=prefix.npos && !Number(prefix,"x",x))return false;
        Put(prefix,"x",x+anchor*shift,true);
        xml.replace(0,end,prefix);
        // These two right-edge indicators are children of the left HUD root.
        // Compensate its -shift, then add the +shift of a right-anchored group.
        if(Id(xml,"scrnid")=="0MOD") {
            size_t at=0;
            while((at=xml.find("<swf>",at))!=xml.npos) {
                auto stop=xml.find("</swf>",at);if(stop==xml.npos)break;stop+=6;
                auto child=xml.substr(at,stop-at);float child_x=0;
                if(RightChild(Id(child,"swfid")) && Number(child,"x",child_x)) {
                    Put(child,"x",child_x+2*shift);xml.replace(at,stop-at,child);stop=at+child.size();
                }
                at=stop;
            }
        }
        return true;
    }
    if(operation=="Reflow" && xml.starts_with("<reflow>")) {
        bool changed=false;size_t at=0;
        while((at=xml.find("<id>",at))!=xml.npos) {
            auto end=xml.find("<id>",at+4);if(end==xml.npos)end=xml.find("</reflow>",at);
            if(end==xml.npos)return changed;
            auto fields=xml.substr(at,end-at);
            const auto id=std::string(Id(fields,"id"));
            float x=0;bool edit=false;
            if(const int anchor=Anchor(id);anchor && Number(fields,"x",x)) {Put(fields,"x",x+anchor*shift);edit=true;}
            else if(single_player && RightChild(id) && Number(fields,"x",x)) {Put(fields,"x",x+2*shift);edit=true;}
            if(edit) {xml.replace(at,end-at,fields);changed=true;end=at+fields.size();}
            at=end;
        }
        return changed;
    }
    return false;
}

#if !defined(SIMS2_UI_LAYOUT_UNIT_TEST)
namespace {
std::string GuestString(uint32_t address,size_t limit) {
    std::string text;uint32_t page=UINT32_MAX;
    for(size_t i=0;i<limit;i++) {
        const uint64_t current=uint64_t(address)+i;if(current>UINT32_MAX)return {};
        const uint32_t va=uint32_t(current);
        if((va>>12)!=page) {if(!mem::IsCommitted(va))return {};page=va>>12;}
        const char c=*static_cast<const char*>(kernel::HostPtr(va));
        if(!c)return text;text+=c;
    }
    return {};
}
}
X86_EXTERN_FUNC(__imp__sub_0001E080);
#if defined(SIMS2_HEADLESS_RUNTIME)
void TraceUiLayout(const X86Context&,const std::string&,const std::string&);
#endif
X86_FUNC(sub_0001E080) {
    const uint32_t stack=ctx.esp;
    const bool active=sims2::ui::enabled && sims2::ui::edge_anchors && app::Get().aspect_ratio=="16:9";
#if defined(SIMS2_HEADLESS_RUNTIME)
    const bool trace=std::getenv("SIMS2_UI_LAYOUT_TRACE")!=nullptr;
#else
    constexpr bool trace=false;
#endif
#if defined(SIMS2_HEADLESS_RUNTIME)
    if(trace) {
        const auto function=GuestString(kernel::R32(stack+4),80);
        const uint32_t count=kernel::R32(stack+16);
        if(function!="cmdProc" && count<=8) {
            std::string arguments;
            for(uint32_t i=0;i<count;i++) {
                const auto value=GuestString(kernel::R32(stack+20+4*i),512);
                if(!value.empty())arguments+="arg"+std::to_string(i)+"="+value+";";
            }
            if(arguments.find("0M2M")!=arguments.npos || arguments.find("_box")!=arguments.npos)
                TraceUiLayout(ctx,"Call-"+function,arguments);
        }
    }
#endif
    if((sims2::ui::enabled || trace || sims2::menu::refresh) && GuestString(kernel::R32(stack+4),80)=="cmdProc") {
        const uint32_t count=kernel::R32(stack+16);
        if(count>=2 && count<=5) {
            const auto operation=GuestString(kernel::R32(stack+20),80);
#if defined(SIMS2_HEADLESS_RUNTIME)
            if(trace && operation!="LoadWidget" && operation!="Reflow") {
                const auto argument=GuestString(kernel::R32(stack+24),65536);
                if(argument.find("0M2M")!=argument.npos || argument.find("dialogbox")!=argument.npos)
                    TraceUiLayout(ctx,operation,argument);
            }
#endif
            if(operation=="LoadWidget" || operation=="Reflow") {
                const uint32_t original=kernel::R32(stack+24);
                auto xml=GuestString(original,65536);
                if(operation=="LoadWidget") {
                    const auto id=Id(xml,"scrnid");
                    if(id=="0MOD") {float h=0;sims2::ui::single_player_hud=Number(xml,"h",h) && h==448;}
                    else if(id=="1MOD")sims2::ui::single_player_hud=false;
                }
#if defined(SIMS2_HEADLESS_RUNTIME)
                if(trace)TraceUiLayout(ctx,operation,xml);
#endif
                bool changed=active && sims2::ui::PatchLayout(xml,operation,true,sims2::ui::single_player_hud.load(std::memory_order_relaxed));
                if(sims2::ui::RefreshContinue(xml,operation,sims2::menu::RefreshReady())) {
                    changed=true;
                    static std::atomic<unsigned> refresh_reports{0};
                    if(refresh_reports.fetch_add(1,std::memory_order_relaxed)<16)
                        LOGR("[SIMS2][MENU][REFRESH] ready geometry published; restarting 0M2M_swf a0 initializer");
                }
                if(changed) {
#if defined(SIMS2_HEADLESS_RUNTIME)
                    if(trace)TraceUiLayout(ctx,operation+"Patched",xml);
#endif
                    const auto temporary=mem::AllocSystem(uint32_t(xml.size()+1));
                    if(!temporary)throw std::bad_alloc();
                    std::memcpy(kernel::HostPtr(temporary),xml.c_str(),xml.size()+1);
                    kernel::W32(stack+24,temporary);
                    try {__imp__sub_0001E080(ctx,base);}
                    catch(...) {kernel::W32(stack+24,original);mem::FreeSystem(temporary);throw;}
                    kernel::W32(stack+24,original);mem::FreeSystem(temporary);return;
                }
            }
        }
    }
    __imp__sub_0001E080(ctx,base);
}
#endif
