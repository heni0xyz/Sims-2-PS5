// SPDX-License-Identifier: GPL-3.0-only
// Diagnostics must preserve the original guest ABI, including exceptions.
#include "game/menu_diagnostics.h"
#include "kernel/kernel.h"
#include <array>
#include <cstdio>
#include <stdexcept>
namespace {std::array<uint8_t,65536> arena{};unsigned reports=0,calls=0;bool fail=false;}
namespace kernel {uint8_t* g_base=arena.data();}
namespace mem {bool IsCommitted(uint32_t address){return address<arena.size();}}
namespace logx {void Write(Level,const char*,...){++reports;}}
void Need(bool value){if(!value)throw std::runtime_error("Menu diagnostic ABI regression");}
X86_EXTERN_FUNC(sub_001404C0);
X86_EXTERN_FUNC(sub_00140D00);
X86_FUNC(__imp__sub_001404C0){
    Need(base==arena.data());++calls;
    if(fail)throw std::runtime_error("original failure");
    const auto object=kernel::R32(ctx.esp+4);
    kernel::W32(object+0x16c,kernel::R32(object+0x168));
    ctx.eax=0x12345678;ctx.esp+=8;
}
X86_FUNC(__imp__sub_00140D00){
    Need(base==arena.data());++calls;
    if(fail)throw std::runtime_error("original failure");
    Need(sims2::menu::RefreshReady());ctx.esp+=8;
}
X86_FUNC(__imp__sub_0013E8C0){ctx.eax=0x8000;ctx.esp+=8;}
int main(){try{
    for(bool diagnostic:{false,true})for(uint32_t state:{1u,2u,3u}){
        X86Context ctx{};ctx.esp=0x1000;ctx.ecx=0xabcdef;ctx.st[3]=1.25;
        kernel::W32(0x1000,0x140005);kernel::W32(0x1004,0x4000);
        kernel::W32(0x4168,state);kernel::W32(0x416c,0);
        auto expected=ctx;expected.eax=0x12345678;expected.esp+=8;
        sims2::menu::diagnostics=diagnostic;sub_001404C0(ctx,arena.data());
        Need(std::memcmp(&ctx,&expected,sizeof(ctx))==0 && kernel::R32(0x416c)==state);
    }
    Need(calls==6 && reports>0);fail=true;
    X86Context ctx{};ctx.esp=0x1000;bool propagated=false;
    try{sub_001404C0(ctx,arena.data());}catch(const std::runtime_error&){propagated=true;}
    Need(propagated && ctx.esp==0x1000);fail=false;
    kernel::W32(0x4160,321);kernel::W32(0x4164,57);kernel::W32(0x417c,8);
    Need(!sims2::menu::RefreshReady());sub_00140D00(ctx,arena.data());
    Need(!sims2::menu::RefreshReady() && ctx.esp==0x1008);
    std::puts("MENU_DIAGNOSTIC_PASS: original contexts/state transitions and exceptions retained; not visual acceptance");return 0;
}catch(const std::exception& error){std::fprintf(stderr,"%s\n",error.what());return 1;}}
