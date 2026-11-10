// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// CPU layout review of the actual ImGui draw lists. This does not validate
// Vulkan, controller hardware, installation or execution on a PS5.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "imgui.h"
#include "installer_view.h"
#include "installer_ui_assets.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static void navigation() {
  Sims2LauncherStartup startup;require(startup.pending && !startup.Resolve(false),"Startup check was bypassed");
  require(startup.Resolve(true) && !startup.pending && !startup.Resolve(true),"Startup check did not resolve exactly once");
  require(startup.Section(Sims2InstallPhase::Ready)==Sims2LauncherSection::Play &&
    startup.Section(Sims2InstallPhase::Missing)==Sims2LauncherSection::Install,"Initial section is not based on checked state");
  Sims2LaunchFade launch;require(launch.Alpha(0)==0 && !launch.Black(0),"Idle launch faded");
  launch.Start(1);launch.Start(1.04);
  require(std::fabs(launch.Alpha(1.09)-.5f)<.001 && !launch.Black(1.09),"Launch fade timing/repeated press failed");
  require(launch.Black(1.181) && launch.Alpha(1.181)==1,"Launch did not become fully black");
  Sims2LauncherTransition fade;
  fade.Select(Sims2LauncherSection::Update,1);
  require(fade.Displayed(1.03)==Sims2LauncherSection::Install && fade.Alpha(1.03)<1,"Outgoing content did not fade");
  require(fade.Displayed(1.071)==Sims2LauncherSection::Update && fade.Alpha(1.071)<.01,"Content did not change at silence midpoint");
  require(std::fabs(fade.Position(1.08)-1.5f)<.001 && fade.Alpha(1.161)==1,"Transition timing/centering failed");
  const float before=fade.Position(1.09),opacity=fade.Alpha(1.09);
  fade.Select(Sims2LauncherSection::Install,1.09);
  require(std::fabs(fade.Position(1.09)-before)<.001 && std::fabs(fade.Alpha(1.09)-opacity)<.001,"Rapid reversal jumped");
  require(fade.Displayed(2)==Sims2LauncherSection::Install && fade.Alpha(2)==1,"Rapid reversal did not settle");
  Sims2SelectionMotion focus;require(!focus.Move(1,false,1) && focus.selected==0,"Disabled option gained focus");
  require(focus.Offset(1.09)>11 && focus.Offset(1.2)==0,"Disabled option did not recoil");
  require(focus.Move(1,true,2) && focus.Position(2.06)>.49f && focus.Position(2.06)<.51f,"Row highlight did not slide");
  Sims2LauncherStick stick;
  require(stick.Move(.2f,.2f,0)==0 && stick.Move(.8f,.2f,1)==1,"Stick dead zone or dominant axis failed");
  require(stick.Move(.8f,.2f,1.2)==0 && stick.Move(.8f,.2f,1.51)==1,"Stick hold repeat failed");
  require(stick.Move(0,0,2)==0 && stick.Move(.2f,-.8f,2.1)==-2,"Stick release/vertical navigation failed");
  Sims2LauncherNavigation n;
  require(!n.Move(1,true,.2,true) && n.section==Sims2LauncherSection::Install,"Update was reachable during preparation");
  require(!n.Move(-1,false,1) && n.section==Sims2LauncherSection::Install,"Uninstalled Play was selectable");
  require(n.Offset(1.09)<-13 && n.Offset(1.2)==0,"Rejected left movement did not return");
  require(n.Move(1,false,2) && n.section==Sims2LauncherSection::Update,"Update must be available before installation");
  require(n.Move(-1,false,2.5) && n.Move(1,true,3) && n.section==Sims2LauncherSection::Update,"Installed Update was not selectable");
  require(!n.Move(1,true,4),"Navigation escaped its right boundary");
  require(n.Move(-1,true,5) && n.Move(-1,true,6) && n.section==Sims2LauncherSection::Play,"Play navigation failed");
  Sims2InstallerCarousel c;
  require(!c.Move(.2f,0) && c.selected==0,"stick drift advanced the image");
  require(c.Move(.8f,1) && c.selected==1,"right tilt failed");
  require(!c.Move(.8f,1.1),"held stick repeated too soon");
  require(c.Move(.8f,1.7) && c.selected==2,"held stick did not repeat");
  c.Move(0,1.8);require(c.Move(.8f,1.9) && c.selected==0,"carousel did not wrap");
  require(c.Move(-.8f,2) && c.selected==2,"opposite tilt did not wrap backwards");
  c.Move(0,2.1);require(!c.Move(-.3f,2.2),"hysteresis did not reject an indecisive tilt");
  Sims2InstallerCarousel automatic;
  require(!automatic.Auto(true,0) && !automatic.Auto(true,6.9) && automatic.Auto(true,7.1) && automatic.selected==1,"Install carousel did not advance on schedule");
  automatic.Move(-.8f,8);require(!automatic.Auto(true,17) && automatic.Auto(true,18.1),"Manual browsing did not delay automatic advance");
  require(!automatic.Auto(false,30) && !automatic.Auto(false,50),"Idle carousel advanced without installation");
}
static double edge(const ImVec2 &a,const ImVec2 &b,double x,double y) {
  return (b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x);
}
static bool top_left(const ImVec2 &a,const ImVec2 &b) {
  return b.y<a.y || (b.y==a.y && b.x>a.x);
}
static void render(const std::filesystem::path &file,const unsigned char *atlas,int tw,int th) {
  const auto *data=ImGui::GetDrawData();const int w=int(data->DisplaySize.x),h=int(data->DisplaySize.y);
  std::vector<unsigned char> pixels(size_t(w)*h*3,0);
  for(int l=0;l<data->CmdListsCount;++l){const auto *list=data->CmdLists[l];
    require(list->VtxBuffer.Size<65536,"UI exceeded the bundled renderer's 16-bit index range");
    size_t offset=0;
    for(const auto &cmd:list->CmdBuffer){
      require(!cmd.UserCallback,"Unexpected UI draw callback");
      for(unsigned k=0;k<cmd.ElemCount;k+=3){
        auto a=list->VtxBuffer[list->IdxBuffer[int(offset+k)]],b=list->VtxBuffer[list->IdxBuffer[int(offset+k+1)]],
             c=list->VtxBuffer[list->IdxBuffer[int(offset+k+2)]];
        double area=edge(a.pos,b.pos,c.pos.x,c.pos.y);if(area==0)continue;
        if(area<0){std::swap(b,c);area=-area;}
        const int x0=std::max({0,int(std::floor(std::min({a.pos.x,b.pos.x,c.pos.x}))),int(std::ceil(cmd.ClipRect.x))});
        const int y0=std::max({0,int(std::floor(std::min({a.pos.y,b.pos.y,c.pos.y}))),int(std::ceil(cmd.ClipRect.y))});
        const int x1=std::min({w,int(std::ceil(std::max({a.pos.x,b.pos.x,c.pos.x}))),int(std::ceil(cmd.ClipRect.z))});
        const int y1=std::min({h,int(std::ceil(std::max({a.pos.y,b.pos.y,c.pos.y}))),int(std::ceil(cmd.ClipRect.w))});
        for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){
          double wa=edge(b.pos,c.pos,x+.5,y+.5),wb=edge(c.pos,a.pos,x+.5,y+.5),wc=edge(a.pos,b.pos,x+.5,y+.5);
          if(wa<0 || (wa==0 && !top_left(b.pos,c.pos)) || wb<0 || (wb==0 && !top_left(c.pos,a.pos)) || wc<0 || (wc==0 && !top_left(a.pos,b.pos)))continue;
          wa/=area;wb/=area;wc/=area;
          const double u=std::clamp((wa*a.uv.x+wb*b.uv.x+wc*c.uv.x)*tw-.5,0.,double(tw-1));
          const double v=std::clamp((wa*a.uv.y+wb*b.uv.y+wc*c.uv.y)*th-.5,0.,double(th-1));
          const int tx=int(u),ty=int(v),nx=std::min(tx+1,tw-1),ny=std::min(ty+1,th-1);const double dx=u-tx,dy=v-ty;
          double rgba[4];
          for(int j=0;j<4;++j){const double sample=(1-dy)*((1-dx)*atlas[(size_t(ty)*tw+tx)*4+j]+dx*atlas[(size_t(ty)*tw+nx)*4+j])+
              dy*((1-dx)*atlas[(size_t(ny)*tw+tx)*4+j]+dx*atlas[(size_t(ny)*tw+nx)*4+j]);
            const double tint=wa*((a.col>>(j*8))&255)+wb*((b.col>>(j*8))&255)+wc*((c.col>>(j*8))&255);rgba[j]=sample*tint/255.;}
          for(int j=0;j<3;++j){auto &dst=pixels[(size_t(y)*w+x)*3+j];dst=uint8_t(std::clamp(std::lround(rgba[j]*rgba[3]/255.+dst*(1-rgba[3]/255.)),0l,255l));}
        }
      }
      offset+=cmd.ElemCount;
    }
  }
  FILE *out=std::fopen(file.string().c_str(),"wb");require(out,"Cannot open preview output");
  std::fprintf(out,"P6\n%d %d\n255\n",w,h);const auto written=std::fwrite(pixels.data(),1,pixels.size(),out);
  std::fclose(out);require(written==pixels.size(),"Incomplete preview output");
}
int main(int argc,char **argv) {
  try {
    require(argc==3,"Usage: preview-ui RESOURCE_DIRECTORY OUTPUT_DIRECTORY");navigation();
    const std::filesystem::path resources=argv[1],output=argv[2];std::filesystem::create_directories(output);
    ImGui::CreateContext();auto &io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=ImVec2(3840,2160);io.DeltaTime=1.f/60;
    require(io.Fonts->AddFontFromFileTTF((resources/"OpenSans-Regular.ttf").string().c_str(),320)!=nullptr,"Missing Open Sans");
    Sims2PrepareInstallerAtlas(io.Fonts,resources.string());unsigned char *atlas;int tw,th;io.Fonts->GetTexDataAsRGBA32(&atlas,&tw,&th);
    io.Fonts->TexID=(ImTextureID)(intptr_t)1;
    struct Scenario {const char *name;Sims2InstallPhase phase;bool install;};
    const Scenario scenarios[]={{"missing",Sims2InstallPhase::Missing,true},{"compiling",Sims2InstallPhase::Compiling,true},
      {"ready",Sims2InstallPhase::Ready,false},{"delete-iso",Sims2InstallPhase::DeleteQuestion,true},{"error",Sims2InstallPhase::Error,true}};
    for(const auto &scenario:scenarios){
      Sims2InstallerView view;view.phase=scenario.phase;view.section=scenario.install?Sims2LauncherSection::Install:Sims2LauncherSection::Play;view.version="1.1.0";view.title_id="PPSA99148";
      view.selection_position=float(int(view.section));
      view.has_game=scenario.phase==Sims2InstallPhase::Ready;
      view.completed=41;view.total=65;view.busy=scenario.phase==Sims2InstallPhase::Compiling;
      view.error="Cannot open the source ISO. Check that the file is fully copied to the title folder, then try again.";
      ImGui::NewFrame();Sims2DrawInstallerView(view);ImGui::Render();render(output/(std::string(scenario.name)+".ppm"),atlas,tw,th);
      std::printf("PASS_HOST_LAYOUT %s 3840x2160\n",scenario.name);std::fflush(stdout);
    }
    {Sims2InstallerView view;view.initial_check=true;view.phase=Sims2InstallPhase::Checking;view.version="1.1.0";
      ImGui::NewFrame();Sims2DrawInstallerView(view);ImGui::Render();render(output/"checking.ppm",atlas,tw,th);std::puts("PASS_HOST_INITIAL_CHECK_VIEW");}
    for(auto status:{Sims2UpdateState::Checking,Sims2UpdateState::Current,Sims2UpdateState::Available,Sims2UpdateState::Unavailable}) {
      Sims2InstallerView view;view.phase=Sims2InstallPhase::Ready;view.section=Sims2LauncherSection::Update;
      view.has_game=true;view.game_update_available=status==Sims2UpdateState::Available;view.update_choice=1;
      view.selection_position=2;view.update_position=view.game_update_available?1.f:0.f;
      view.version="1.1.0";view.update={status,status==Sims2UpdateState::Available?"v1.2.0":"v1.0.0"};
      ImGui::NewFrame();Sims2DrawInstallerView(view);ImGui::Render();
      render(output/("update-"+std::to_string(int(status))+".ppm"),atlas,tw,th);
      std::printf("PASS_HOST_UPDATE_LAYOUT %d 3840x2160\n",int(status));std::fflush(stdout);
    }
    {Sims2InstallerView view;view.phase=Sims2InstallPhase::Missing;view.section=Sims2LauncherSection::Update;
      view.selection_position=2;
      view.version="1.1.0";view.update.state=Sims2UpdateState::Current;view.music_enabled=false;
      ImGui::NewFrame();Sims2DrawInstallerView(view);ImGui::Render();render(output/"update-uninstalled.ppm",atlas,tw,th);}
    io.DisplaySize=ImVec2(1280,720);
    Sims2LauncherTransition animated;animated.Select(Sims2LauncherSection::Update,0);
    for(int frame=0;frame<=10;++frame){const double t=frame*.016;
      Sims2InstallerView view;view.phase=Sims2InstallPhase::Ready;view.has_game=true;view.game_update_available=true;
      view.section=animated.Displayed(t);view.content_alpha=animated.Alpha(t);view.selection_position=animated.Position(t);
      view.version="1.1.0";view.update.state=Sims2UpdateState::Current;
      ImGui::NewFrame();Sims2DrawInstallerView(view);ImGui::Render();render(output/("transition-"+std::to_string(frame)+".ppm"),atlas,tw,th);}
    Sims2LaunchFade launch;launch.Start(0);
    for(int frame=0;frame<=12;++frame){
      Sims2InstallerView view;view.phase=Sims2InstallPhase::Ready;view.has_game=true;
      view.section=Sims2LauncherSection::Play;view.selection_position=0;view.version="1.1.0";
      view.launch_alpha=launch.Alpha(frame*.02);
      ImGui::NewFrame();Sims2DrawInstallerView(view);ImGui::Render();render(output/("play-fade-"+std::to_string(frame)+".ppm"),atlas,tw,th);
    }
    {std::ifstream file(output/"play-fade-12.ppm",std::ios::binary);std::string line;
      for(int n=0;n<3;++n)std::getline(file,line);
      const std::vector<unsigned char> pixels((std::istreambuf_iterator<char>(file)),{});
      require(pixels.size()==1280*720*3 && std::all_of(pixels.begin(),pixels.end(),[](auto v){return v==0;}),"Play fade did not cover the complete frame");}
    std::puts("PASS_HOST_PLAY_FADE: 180ms smooth fade and completely black terminal frame");
    ImGui::DestroyContext();std::puts("PASS_HOST_CAROUSEL");return 0;
  }catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
