// SPDX-License-Identifier: GPL-3.0-only
#include "game/install_font.h"
#include "kernel/crypto.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>
namespace {
struct Glyph {int advance,x,y;unsigned width,height,offset;};
struct Face {unsigned size,ascent;std::array<Glyph,95> glyphs;std::array<int16_t,95*95> kern;};
std::array<Face,3> faces{};
std::vector<uint8_t> pixels;
template<class T>T At(std::span<const uint8_t> data,size_t at){
    if(at>data.size() || sizeof(T)>data.size()-at)throw std::runtime_error("Truncated installer font");T value;std::memcpy(&value,data.data()+at,sizeof(T));return value;
}
unsigned Code(unsigned char c){return c>=32 && c<127?c-32:'?'-32;}
const Face& Font(unsigned style){if(pixels.empty() || style<1 || style>3)throw std::runtime_error("Installer Open Sans font is unavailable");return faces[style-1];}
}
void sims2::install::LoadInstallFont(std::span<const uint8_t> data){
    if(data.size()<24+20 || data.size()>4*1024*1024 || std::memcmp(data.data(),"S2FONT1\0",8) || At<uint32_t>(data,8)!=1 || At<uint32_t>(data,12)!=3 || At<uint32_t>(data,16)!=95 || At<uint32_t>(data,20)!=16)throw std::runtime_error("Invalid installer font resource");
    alignas(8) uint8_t state[128]{},digest[20]{};crypto::ShaInit(state);crypto::ShaUpdate(state,data.data(),data.size()-20);crypto::ShaFinal(state,digest);
    if(std::memcmp(digest,data.data()+data.size()-20,20))throw std::runtime_error("Installer font checksum mismatch");
    std::array<Face,3> decoded;size_t cursor=24;constexpr size_t payload=24+3*(16+95*16+95*95*2);
    for(unsigned index=0;index<3;++index){
        auto& face=decoded[index];face.size=At<uint16_t>(data,cursor);face.ascent=At<uint16_t>(data,cursor+2);
        if(face.size!=std::array<unsigned,3>{56,96,144}[index] || face.ascent>256 || At<uint32_t>(data,cursor+8)!=95 || At<uint32_t>(data,cursor+12)!=95*95)throw std::runtime_error("Unsupported installer font metrics");
        cursor+=16;
        for(auto& glyph:face.glyphs){
            glyph={At<int16_t>(data,cursor),At<int16_t>(data,cursor+2),At<int16_t>(data,cursor+4),At<uint16_t>(data,cursor+8),At<uint16_t>(data,cursor+10),At<uint32_t>(data,cursor+12)};cursor+=16;
            if(glyph.advance<0 || glyph.advance>32767 || glyph.width>512 || glyph.height>512 || glyph.x < -512 || glyph.x>512 || glyph.y < -512 || glyph.y>512 || glyph.offset<payload || uint64_t(glyph.offset)+glyph.width*glyph.height>data.size()-20)throw std::runtime_error("Invalid installer glyph range");
        }
        for(auto& pair:face.kern){pair=At<int16_t>(data,cursor);cursor+=2;if(pair < -4096 || pair>4096)throw std::runtime_error("Invalid installer kerning");}
    }
    if(cursor!=payload)throw std::runtime_error("Installer font table length mismatch");
    pixels.assign(data.begin(),data.end()-20);faces=std::move(decoded);
}
bool sims2::install::InstallFontLoaded(){return !pixels.empty();}
int sims2::install::TextWidth(std::string_view text,unsigned style){
    const auto& face=Font(style);int width=0;unsigned previous=0;bool first=true;
    for(unsigned char c:text){const auto code=Code(c);if(!first)width+=face.kern[previous*95+code];width+=face.glyphs[code].advance;previous=code;first=false;}
    return (width+32)/64;
}
void sims2::install::RasterText(std::span<uint32_t> canvas,std::string_view text,int y,unsigned style,uint32_t colour){
    if(canvas.size()!=3840*2160 || text.size()>1024)throw std::runtime_error("Invalid installer text canvas");
    const auto& face=Font(style);int pen=((3840-TextWidth(text,style))/2)*64;unsigned previous=0;bool first=true;
    for(unsigned char c:text){
        const auto code=Code(c);if(!first)pen+=face.kern[previous*95+code];const auto& glyph=face.glyphs[code];
        const int left=(pen+32)/64+glyph.x,top=y+int(face.ascent)+glyph.y;
        for(unsigned row=0;row<glyph.height;++row)for(unsigned column=0;column<glyph.width;++column){
            const int x=left+column,dy=top+row;if(x<0 || x>=3840 || dy<0 || dy>=2160)continue;
            const auto alpha=pixels[glyph.offset+row*glyph.width+column];auto& target=canvas[dy*3840+x];uint32_t blended=0xff000000;
            for(unsigned channel=0;channel<3;++channel)blended|=uint32_t((((colour>>(channel*8))&255)*alpha+((target>>(channel*8))&255)*(255-alpha)+127)/255)<<(channel*8);
            target=blended;
        }
        pen+=glyph.advance;previous=code;first=false;
    }
}
