// SPDX-License-Identifier: GPL-3.0-only
#include "game/font_resample.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
void Check(bool value,const char* why){if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main(){
    using sims2::fonts::UpscaleBgra;
    std::vector<uint8_t> constant(4*3*4);
    for(size_t i=0;i<constant.size();i+=4){constant[i]=16;constant[i+1]=32;constant[i+2]=64;constant[i+3]=128;}
    for(unsigned scale:{2u,4u}){
        auto out=UpscaleBgra(constant,4,3,scale);
        Check(out.size()==constant.size()*scale*scale,"scaled extent");
        for(size_t i=0;i<out.size();i++)Check(out[i]==constant[i%4],"constant colour and coverage retained including borders");
    }
    std::vector<uint8_t> stroke(8*8*4);
    for(unsigned y=3;y<5;y++)for(unsigned x=3;x<5;x++)for(unsigned c=0;c<4;c++)stroke[(y*8+x)*4+c]=255;
    auto out=UpscaleBgra(stroke,8,8,4);unsigned smooth=0;uint64_t alpha=0;
    for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
        const auto at=(y*32+x)*4;alpha+=out[at+3];smooth+=out[at+3]>0 && out[at+3]<255;
        Check(out[at]==out[at+3] && out[at+1]==out[at+3] && out[at+2]==out[at+3],"premultiplied colour follows coverage");
        if(x<8 || x>=24 || y<8 || y>=24)Check(out[at+3]==0,"empty glyph padding has no ringing/halo");
    }
    Check(smooth>0,"reconstructed edge has intermediate coverage");
    Check(alpha>15500 && alpha<17150,"stroke coverage preserved within rounding/filter tolerance");
    bool rejected=false;try{UpscaleBgra(stroke,8,8,3);}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected,"unsupported size factor refused");rejected=false;
    try{UpscaleBgra(std::span(stroke).subspan(1),8,8,4);}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected,"truncated input refused");
    std::puts("FONT_RESAMPLE_PASS: sizes, constant coverage, smooth edges, empty padding, premultiplied colours, invalid inputs");
}
