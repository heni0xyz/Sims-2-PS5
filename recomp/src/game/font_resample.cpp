// SPDX-License-Identifier: GPL-3.0-only
#include "game/font_resample.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace {
float Cubic(float a,float b,float c,float d,float t) {
    const float value=b+0.5f*t*(c-a+t*(2*a-5*b+4*c-d+t*(3*(b-c)+d-a)));
    // A monotonic limiter prevents ringing into empty atlas padding and
    // overshoot beside opaque strokes. No glyph positions/metrics are changed.
    return std::clamp(value,std::min(b,c),std::max(b,c));
}
}
std::vector<uint8_t> sims2::fonts::UpscaleBgra(std::span<const uint8_t> source,uint32_t w,uint32_t h,uint32_t scale) {
    if(!w || !h || w>512 || h>512 || (scale!=2 && scale!=4) || source.size()!=size_t(w)*h*4)
        throw std::invalid_argument("Invalid font reconstruction extent");
    const auto width=w*scale,height=h*scale;
    std::vector<float> horizontal(size_t(width)*h*4);
    for(uint32_t x=0;x<width;++x) {
        const float position=(float(x)+0.5f)/float(scale)-0.5f;const int from=int(std::floor(position));const float t=position-from;
        const auto at=[&](int offset){return uint32_t(std::clamp(from+offset,0,int(w)-1));};
        for(uint32_t y=0;y<h;++y)for(unsigned c=0;c<4;++c)
            horizontal[(size_t(y)*width+x)*4+c]=Cubic(source[(size_t(y)*w+at(-1))*4+c],source[(size_t(y)*w+at(0))*4+c],
                source[(size_t(y)*w+at(1))*4+c],source[(size_t(y)*w+at(2))*4+c],t);
    }
    std::vector<uint8_t> result(size_t(width)*height*4);
    for(uint32_t y=0;y<height;++y) {
        const float position=(float(y)+0.5f)/float(scale)-0.5f;const int from=int(std::floor(position));const float t=position-from;
        const auto at=[&](int offset){return uint32_t(std::clamp(from+offset,0,int(h)-1));};
        for(uint32_t x=0;x<width;++x)for(unsigned c=0;c<4;++c) {
            const auto read=[&](int offset){return horizontal[(size_t(at(offset))*width+x)*4+c];};
            result[(size_t(y)*width+x)*4+c]=uint8_t(std::clamp(std::lround(Cubic(read(-1),read(0),read(1),read(2),t)),0l,255l));
        }
    }
    return result;
}
