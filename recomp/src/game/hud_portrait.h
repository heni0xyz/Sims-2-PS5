// Authored presentation policy; no guest data or instructions.
#pragma once
#include <cmath>
namespace sims2::ui {
inline bool MapPortraitRectangle(float* rect,bool edge_anchor) {
    for(unsigned i=0;i<4;i++)if(!std::isfinite(rect[i]))return false;
    if(rect[0]<0 || rect[2]>.5f || rect[1]<.5f || rect[3]>1 ||
       !(rect[2]>rect[0]) || !(rect[3]>rect[1]) ||
       rect[2]-rect[0]>.25f || rect[3]-rect[1]>.35f)return false;
    constexpr float scale=45.0f/56;
    const float offset=edge_anchor?0:(1-scale)*.5f;
    rect[0]=rect[0]*scale+offset;rect[2]=rect[2]*scale+offset;
    return true;
}
}
