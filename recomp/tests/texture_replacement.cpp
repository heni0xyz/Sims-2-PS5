// SPDX-License-Identifier: GPL-3.0-only
#include "gpu/texture_replacement.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main(){
    uint8_t byte=0;
    Check(gpu::ReplacementBytes(256,256,9)==349524,"complete HD mip chain size");
    Check(gpu::ReplacementBytes(32,16,6)==2732,"non-square mip tail");
    for(const auto value:{gpu::TextureReplacement{0,256,9,349524,&byte},
                         {256,0,9,349524,&byte},{256,256,10,349524,&byte},
                         {8192,256,9,349524,&byte},{256,256,0,349524,&byte},
                         {256,256,9,349523,&byte},{256,256,9,349524,nullptr}})
        Check(!gpu::ValidReplacement(value),"invalid replacement must be rejected");
    Check(gpu::ValidReplacement({256,256,9,349524,&byte}),"valid shape");
    std::puts("TEXTURE_REPLACEMENT_PASS: dimensions, mip tails, extent/count limits, byte bounds, null data");
}
