// Synthetic schema fixtures; no captured game XML is stored here.
#include "game/ui_layout.h"
#include "game/hud_portrait.h"
#include "game/menu_logo.h"
#include "gpu/draw_viewport.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
void Require(bool ok,const char* reason) {if(!ok){std::fprintf(stderr,"FAIL %s\n",reason);std::exit(1);}}
float Field(const std::string& text,const char* name) {
    const auto tag="<"+std::string(name)+">";
    auto pos=text.find(tag);Require(pos!=text.npos,"field exists");return std::strtof(text.c_str()+pos+tag.size(),nullptr);
}
int main() {
    using sims2::ui::PatchLayout;
    using sims2::ui::RefreshContinue;
    std::string menu="<reflow><id>0M2M_swf_container</id><x>10</x><y>20</y><id>0M2M_body</id><h>33</h></reflow>";
    const auto menu_before=menu;
    Require(!RefreshContinue(menu,"Reflow",false) && menu==menu_before,"unready Continue does not initialize background");
    Require(!RefreshContinue(menu,"LoadWidget",true) && menu==menu_before,"initial widget creation is not interrupted");
    Require(RefreshContinue(menu,"Reflow",true),"ready Continue geometry refreshes its exact movie");
    Require(menu==menu_before.substr(0,menu_before.size()-9)+"<id>0M2M_swf</id><swfAnim>a0</swfAnim></reflow>","original dimensions retained with a0 refresh");
    const auto menu_after=menu;
    Require(!RefreshContinue(menu,"Reflow",true) && menu==menu_after,"refresh is idempotent");
    menu="<reflow><id>0G2D_swf_container</id><x>10</x></reflow>";const auto unrelated_menu=menu;
    Require(!RefreshContinue(menu,"Reflow",true) && menu==unrelated_menu,"other dialogs unchanged");
    const std::string original="<scrn><scrnid>0MOD</scrnid><plyr>0</plyr><x>0</x><y>0</y><w>300</w><h>448</h><shdr><shdrid>0MOD_head</shdrid><type>CB</type><x>145</x><y>373</y><w>74</w><h>74</h></shdr></scrn>";
    std::string xml=original;
    Require(!PatchLayout(xml,"LoadWidget",false) && xml==original,"4:3 exact pass-through");
    Require(PatchLayout(xml,"LoadWidget",true),"left group selected");
    const float root=Field(xml,"x");
    const auto head=xml.substr(xml.find("<shdr>"));
    Require(std::fabs(root+78.222222f)<.00002f,"left edge displacement");
    Require(Field(head,"x")==145 && Field(head,"w")==74,"native CB fields preserved; runtime rejected XML-only portrait fix");
    Require(Field(head,"y")==373 && Field(head,"h")==74,"portrait vertical geometry unchanged");
    xml="<scrn><scrnid>0MOD</scrnid><plyr>0</plyr><x>0</x><y>0</y><h>448</h><swf><swfid>0MOD_pause_swf</swfid><x>578</x><y>30</y></swf><swf><swfid>0MOD_fastforward_swf</swfid><x>578</x><y>30</y></swf><swf><swfid>other</swfid><x>578</x></swf></scrn>";
    Require(PatchLayout(xml,"LoadWidget",true),"right indicators inside left HUD selected");
    const float parent=Field(xml,"x"),pause=Field(xml.substr(xml.find("<swfid>0MOD_pause_swf")),"x");
    Require(std::fabs(parent+pause-(578+78.222222f))<.0001f,"pause compensates left parent and reaches right anchor");
    Require(Field(xml.substr(xml.find("<swfid>0MOD_fastforward_swf")),"x")==pause,"fast-forward uses the same right anchor");
    Require(Field(xml.substr(xml.find("<swfid>other")),"x")==578,"other children unchanged");
    xml="<reflow><id>0MOD_pause_swf</id><x>578</x><vis>1</vis></reflow>";
    auto child_before=xml;Require(!PatchLayout(xml,"Reflow",true,false) && xml==child_before,"split/unknown HUD indicator reflow unchanged");
    Require(PatchLayout(xml,"Reflow",true,true) && std::fabs(Field(xml,"x")-734.444444f)<.0001f,"animated right indicator retains parent compensation");
    xml="<scrn><scrnid>0CXM</scrnid><plyr>0</plyr><x>0</x><y>0</y><w>640</w><h>448</h></scrn>";
    Require(PatchLayout(xml,"LoadWidget",true) && std::fabs(Field(xml,"x")-78.222222f)<.00002f,"right group displacement");
    xml="<scrn><scrnid>0MOT</scrnid><plyr>0</plyr><subscrn><subscrnid>test</subscrnid><x>33</x></subscrn></scrn>";
    Require(PatchLayout(xml,"LoadWidget",true),"missing root x inserted");
    Require(xml.find("<x>33</x>")!=xml.npos && Field(xml,"x")<0,"child position retained");
    for(const auto* fixture:{"<scrn><scrnid>1MOD</scrnid><plyr>1</plyr></scrn>",
        "<scrn><scrnid>0MOD</scrnid><plyr>0</plyr><h>224</h></scrn>",
        "<scrn><scrnid>0BUILD</scrnid><plyr>0</plyr></scrn>",
        "<scrn><scrnid>0MOD</scrnid><plyr>0</plyr><x>nan</x></scrn>"}) {
        xml=fixture;Require(!PatchLayout(xml,"LoadWidget",true) && xml==fixture,"unknown/split/invalid screen unchanged");
    }
    xml="<reflow><id>0MOT</id><x>20</x><vis>1</vis><id>0CXM</id><x>5</x><id>0MOD_head</id><x>145</x><w>74</w><id>other</id><x>8</x></reflow>";
    Require(PatchLayout(xml,"Reflow",true),"coordinate reflow stays anchored");
    Require(std::fabs(Field(xml,"x")+58.222222f)<.00002f,"animated root coordinate transformed");
    Require(xml.find("<id>other</id><x>8</x>")!=xml.npos,"unrelated reflow preserved");
    xml="<reflow><id>0MOD</id><vis>1</vis></reflow>";const auto before=xml;
    Require(!PatchLayout(xml,"Reflow",true) && xml==before,"visibility-only reflow exact pass-through");
    gpu::DrawViewport draw{45.0f/56,(1-45.0f/56)*.5f,true,true};
    Require(gpu::ValidDrawViewport(draw),"selected draw transform valid");
    // A left-anchored source point outside the original clip volume must land
    // inside the image BEFORE rejection, not merely inside a smaller viewport.
    const float clip=(-64.0f/320-1)*draw.x_scale+draw.x_scale+2*draw.x_offset-1;
    Require(clip>-1 && clip<1,"left gutter geometry survives clip rejection");
    int32_t x0=0,x1=640;gpu::MapDrawScissorX(x0,x1,640,draw);
    Require(x0==0 && x1==640,"full-width clip-space scissor permits edge groups");
    x0=100;x1=200;gpu::MapDrawScissorX(x0,x1,640,draw);
    Require(x0==143 && x1==224,"narrow widget mask follows affine mapping");
    draw.clip_space=false;x0=0;x1=640;gpu::MapDrawScissorX(x0,x1,640,draw);
    Require(x0==62 && x1==578,"legacy viewport scissor behavior retained");
    Require(!gpu::ValidDrawViewport({1,.1f}),"out-of-target transform rejected");
    Require(!gpu::ValidDrawViewport({NAN,0}),"nonfinite transform rejected");
    float matte[12][2]={{0,16},{640,16},{0,104},{0,104},{640,16},{640,104},
                       {0,376},{640,376},{0,464},{0,464},{640,376},{640,464}};
    float black[12][3]{};
    Require(gpu::FullWidthMatte(matte,black,12,640,16,464),"batched cinematic bands retain full screen width");
    Require(gpu::FullWidthMatte(matte,black,6,640,16,464),"single top band retains full width");
    black[0][0]=1;Require(!gpu::FullWidthMatte(matte,black,6,640,16,464),"coloured UI is not a black matte");black[0][0]=0;
    matte[0][0]=80;Require(!gpu::FullWidthMatte(matte,black,6,640,16,464),"narrow element excluded");matte[0][0]=0;
    matte[0][1]=224;Require(!gpu::FullWidthMatte(matte,black,6,640,16,464),"centered panel excluded");matte[0][1]=NAN;
    Require(!gpu::FullWidthMatte(matte,black,6,640,16,464),"invalid matte excluded");
    float portrait[4]={.18f,.75f,.28f,.9f};
    Require(sims2::ui::MapPortraitRectangle(portrait,true),"portrait rectangle selected");
    Require(std::fabs((portrait[0]+portrait[2])*.5f-.23f*45/56)<1e-6f,"native camera center follows left HUD scale");
    Require(portrait[1]==.75f && portrait[3]==.9f,"native camera vertical bounds retained");
    float centered[4]={.18f,.75f,.28f,.9f};
    Require(sims2::ui::MapPortraitRectangle(centered,false) && std::fabs(centered[0]-(.18f*45/56+(1-45.0f/56)*.5f))<1e-6f,"centered layout follows its surrounding widgets");
    float other[4]={0,0,1,1};Require(!sims2::ui::MapPortraitRectangle(other,true) && other[2]==1,"world camera unchanged");
    const gpu::DrawBounds corner{32,32,128,136};
    Require(sims2::ui::CornerLogo(sims2::ui::kDirect,0xb,512,512,7,sims2::ui::kSimsLogoMip,corner),"identified logo at upper-left selected");
    Require(!sims2::ui::CornerLogo(sims2::ui::kDirect,0xb,512,512,7,sims2::ui::kSimsLogoMip,{128,32,512,416}),"same logo in centered startup stays centered");
    Require(!sims2::ui::CornerLogo(sims2::ui::kDirect,0xb,512,512,7,0,corner),"unrelated same-size texture unchanged");
    Require(!sims2::ui::CornerLogo(sims2::ui::kLocal,0xb,512,512,7,sims2::ui::kSimsLogoMip,corner),"other shader route unchanged");
    Require(!sims2::ui::CornerLogo(sims2::ui::kDirect,0xb,512,512,7,sims2::ui::kSimsLogoMip,{NAN,0,128,128}),"invalid logo bounds unchanged");
    Require(std::fabs(32*45.0f/56*2-51.428571f)<.0001f,"left-anchored 720p margin retains authored position without center gutter");
    std::puts("PASS_UI_LAYOUT: synthetic group anchoring, preserved CB fields, reflow, mattes and pass-through");
}
