#include "game/ui_aspect.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

void require(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main() {
    using namespace sims2::ui;
    for(auto shader:{kDirect,kLocal}) {
        const float scale=HorizontalScale(shader,640,480,16.f/9,true);
        require(std::abs(scale-45.f/56)<0.000001f,"640x448 square pixel correction");
        for(float height:{720.f,1080.f,2160.f}) {
            const float width=height*16.f/9;
            const float pixel_width=32.f/640*width*scale;
            const float pixel_height=32.f/448*height;
            require(std::abs(pixel_width-pixel_height)<0.001f,"square remains square at each output");
            const float offset=(1-scale)*width*.5f;
            require(std::abs(offset+width*scale*.5f-width*.5f)<0.001f,"UI center unchanged");
        }
        require(HorizontalScale(shader,640,480,4.f/3,true)==1,"4:3 path unchanged");
        require(HorizontalScale(shader,640,480,16.f/9,false)==1,"runtime option restores baseline");
        require(HorizontalScale(shader,320,240,16.f/9,true)==1,"offscreen target unchanged");
        require(HorizontalScale(shader,640,448,16.f/9,true)==1,"nonbackbuffer unchanged");
        require(HorizontalScale(shader,640,480,NAN,true)==1,"invalid aspect is identity");
        require(HorizontalScale(shader,640,480,16.f/9,true,0x24)==1,"YUY2 movie sharing the UI shader is unchanged");
        require(HorizontalScale(shader,640,480,16.f/9,true,0x25)==1,"UYVY movie sharing the UI shader is unchanged");
    }
    for(uint32_t fvf:{0x4u,0x44u,0x104u,0x144u,0x244u}) {
        require(Classify(fvf,0,false)==DrawPath::Screen2D,"standard pretransformed UI route");
        require(Classify(fvf,kDirect,true)==DrawPath::None,"depth-tested FVF geometry excluded");
    }
    for(uint32_t fvf:{0u,0x2u,0x42u,0x142u,0x242u})
        require(Classify(fvf,0,false)==DrawPath::None,"ordinary XYZ geometry excluded even without depth");
    require(Classify(1,kDirect,false)==DrawPath::Direct,"direct UI shader");
    require(Classify(1,kLocal,false)==DrawPath::Local,"local UI shader");
    require(Classify(0x245,0x9999,false)==DrawPath::None,"unknown programmable route excluded");
    const float ortho[]={0,0,0,1}, scale[]={320,-224,16777215,1}, offset[]={320.53125f,240.53125f,0,0};
    require(OrthographicScreen(ortho,scale,offset),"captured orthographic UI projection");
    const float perspective[]={0,0,1,0}, bad[]={NAN,0,0,1}, other_scale[]={160,-120,1,1};
    require(!OrthographicScreen(perspective,scale,offset),"perspective world projection excluded");
    require(!OrthographicScreen(bad,scale,offset),"nonfinite projection excluded");
    require(!OrthographicScreen(ortho,other_scale,offset),"other viewport projection excluded");
    require(Classify(1,kProjectedTexture,false,true)==DrawPath::Projected2D,"captured projected texture UI");
    require(Classify(1,kProjectedTexture,false,false)==DrawPath::None,"projected program needs verified orthographic matrix");
    require(Classify(1,kProjectedTexture,true,true)==DrawPath::None,"depth-tested projected geometry unchanged");
    require(Classify(1,0x7c0edd0d113b94cbull,false,true)==DrawPath::None,"other projected/world programs stay excluded");
    const float dim_x[]={80.00062561035156f/320,0,0,0}, dim_y[]={0,-44.99909973144531f/224,0,0};
    require(FullCanvasDimmer(dim_x,dim_y),"named root blackPoly is full-bleed");
    const float panel_x[]={.2f,0,0,0}, moved_x[]={80.00062561035156f/320,0,0,.1f};
    require(!FullCanvasDimmer(panel_x,dim_y),"ordinary centered panel is not the dimmer");
    require(!FullCanvasDimmer(moved_x,dim_y),"translated object does not match blackPoly");
    require(!FullCanvasDimmer(bad,dim_y),"invalid dimmer matrix excluded");
    const float direct=ScaleForPath(DrawPath::Direct,640,480,16.f/9,true);
    const float fixed=ScaleForPath(DrawPath::Screen2D,640,480,16.f/9,true);
    require(direct==fixed,"panel and icon routes have identical horizontal map");
    for(float x:{24.f,64.f,320.f,600.f})
        require(std::abs((1-direct)*320+x*direct-((1-fixed)*320+x*fixed))<0.0001f,"HUD layers share anchors");
    require(ScaleForPath(DrawPath::Screen2D,640,480,16.f/9,false)==1,"disabled fixed-route policy");
    require(ScaleForPath(DrawPath::Screen2D,320,240,16.f/9,true)==1,"small offscreen fixed route unchanged");
    require(ScaleForPath(DrawPath::Screen2D,640,480,16.f/9,true,0x24)==1,"fixed-route packed movies excluded");
    require(HorizontalScale(0x7c0edd0d113b94cbull,640,480,16.f/9,true)==1,"movie/world program unchanged");
    require(HorizontalScale(0,640,480,16.f/9,true)==1,"unknown shader unchanged");
    require(MovieTexture(0x12,640,448,1,true),"observed decoded ARGB intro movie bypass");
    require(MovieTexture(0x1e,640,448,1,true),"decoded XRGB movie bypass");
    require(!MovieTexture(0x0e,32,32,3,false),"button texture is UI");
    require(!MovieTexture(0x12,256,256,1,true),"linear UI texture is not a movie");
    require(!MovieTexture(0x12,640,448,3,false),"authored mipmapped UI texture is not a movie");
    std::puts("UI_ASPECT_POLICY_PASS (host numeric policy; Vulkan pixel oracle is separate)");
}
