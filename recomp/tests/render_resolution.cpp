// Asset-free regression of actual render density, not a display-size assertion.
#include "gpu/render_resolution.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main() {
    using gpu::MinimumPictureScale;
    const float k=MinimumPictureScale(640,480,448,3840,2160,100,4.5f);
    assert(k==6);assert(640*k==3840);assert(480*k==2880);assert(448*k==2688);
    // The former height-only policy really did upscale horizontally.
    assert(640*4.5f<3840);assert(448*4.5f<2160);
    // Every post/depth target shares the same integer scale, keeping copy
    // rectangles and shader sample steps consistent with the full frame.
    for(unsigned w: {640u,320u,160u,80u,40u,128u})assert(unsigned(w*k)==w*6);
    assert(MinimumPictureScale(640,480,448,1920,1080,100,2.25f)==3);
    assert(MinimumPictureScale(640,480,448,1280,720,100,1)==2);
    assert(MinimumPictureScale(640,480,448,3840,2160,50,2.25f)==3);
    assert(MinimumPictureScale(640,480,448,3840,2160,100,8)==8);
    assert(MinimumPictureScale(640,480,448,3840,2160,0,1)==0);
    assert(MinimumPictureScale(640,480,481,3840,2160,100,1)==0);
    assert(MinimumPictureScale(0,480,448,3840,2160,100,1)==0);
    assert(MinimumPictureScale(640,480,448,3840,2160,100,std::numeric_limits<float>::quiet_NaN())==0);
    std::puts("PASS: 4K picture density, uniform post/depth scaling, percent, invalid-input guards");
}
