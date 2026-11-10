// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "installer_ui_assets.h"
#include "executable_file.h"
#include "imgui_internal.h"
#include "stb_image.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {
struct Image {int rectangle=-1;ImVec2 uv0{},uv1{};};
std::array<Image,size_t(Sims2InstallerImage::Count)> images;
std::string next_executable;
bool next_update_helper=false;
}
bool ps5OverlayFont(std::string &path,float &size) {
  path="/app0/public-resources/OpenSans-Regular.ttf";
  size=128; // Native 320px atlas also keeps the large, centered Play label sharp at 4K.
  return true;
}

// Optional title hook after adding the font, before its texture is uploaded.
// Images share the proven font-atlas descriptor; the renderer needs no new
// image pipeline or changes to its descriptor binding rules.
void ps5InstallerAtlas(ImFontAtlas *atlas) {
  Sims2PrepareInstallerAtlas(atlas,"/app0/public-resources");
}
void Sims2PrepareInstallerAtlas(ImFontAtlas *atlas,const std::string &resource_directory) {
  atlas->TexDesiredWidth=4096; // Retain the supplied 3840x2160 background at native size.
  const char *names[]={"installer-icon.png","installer-cross.png","installer-circle.png","installer-l1.png","installer-r1.png","installer-stick.png","installer-touchpad.png","installer-dpad.png",
                       "preview-1.png","preview-2.png","preview-3.png","installer-background.png"};
  struct Pixels {std::vector<unsigned char> rgba;int width,height;};
  std::vector<Pixels> pixels;
  for(size_t i=0;i<images.size();++i) {
    int width=0,height=0,channels=0;std::string path=resource_directory+"/"+names[i];
    unsigned char *source=stbi_load(path.c_str(),&width,&height,&channels,4);
    if(!source && i>=size_t(Sims2InstallerImage::Preview1) && i<=size_t(Sims2InstallerImage::Preview3))
      source=stbi_load((resource_directory+"/installer-background.png").c_str(),&width,&height,&channels,4);
    if(!source || width<1 || height<1) {if(source)stbi_image_free(source);throw std::runtime_error("Installer image missing: "+path);}
    const bool background=i==size_t(Sims2InstallerImage::Background);
    const bool prompt=i<size_t(Sims2InstallerImage::Preview1);
    const int w=background?3840:(prompt?256:1024),h=background?2160:(prompt?256:576);
    Pixels image{{},w,h};image.rgba.resize(size_t(w)*h*4);
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){
      const float fx=std::max(0.f,(x+.5f)*width/w-.5f),fy=std::max(0.f,(y+.5f)*height/h-.5f);
      const int x0=int(fx),y0=int(fy),x1=std::min(x0+1,width-1),y1=std::min(y0+1,height-1);
      const float dx=fx-x0,dy=fy-y0;
      for(int c=0;c<4;++c)image.rgba[(size_t(y)*w+x)*4+c]=uint8_t(std::clamp(
        (1-dy)*((1-dx)*source[(size_t(y0)*width+x0)*4+c]+dx*source[(size_t(y0)*width+x1)*4+c])+
        dy*((1-dx)*source[(size_t(y1)*width+x0)*4+c]+dx*source[(size_t(y1)*width+x1)*4+c]),0.f,255.f));
    }
    stbi_image_free(source);images[i].rectangle=atlas->AddCustomRectRegular(unsigned(0x10000+i),w,h);pixels.push_back(std::move(image));
  }
  unsigned char *texture=nullptr;int width=0,height=0;atlas->GetTexDataAsRGBA32(&texture,&width,&height);
  for(size_t i=0;i<images.size();++i) {
    const auto *rect=atlas->GetCustomRectByIndex(images[i].rectangle);
    if(!rect || !rect->IsPacked())throw std::runtime_error("Installer image atlas did not pack");
    for(int y=0;y<pixels[i].height;++y)std::copy_n(pixels[i].rgba.data()+size_t(y)*pixels[i].width*4,
      pixels[i].width*4,texture+(size_t(rect->Y+y)*width+rect->X)*4);
    atlas->CalcCustomRectUV(rect,&images[i].uv0,&images[i].uv1);
  }
}
void Sims2InstallerRoundedPath(ImDrawList *draw,ImVec2 from,ImVec2 to,float radius) {
  const float r=std::clamp(radius,0.f,std::min(to.x-from.x,to.y-from.y)*.5f);
  if(r<=0){draw->PathLineTo(from);draw->PathLineTo(ImVec2(to.x,from.y));draw->PathLineTo(to);draw->PathLineTo(ImVec2(from.x,to.y));return;}
  constexpr float pi=3.14159265358979323846f;
  // The bundled ImGui uses only three segments per corner in PathRect.
  // Use smooth native-4K corners locally without modifying that dependency.
  draw->PathArcTo(ImVec2(from.x+r,from.y+r),r,pi,pi*1.5f,16);
  draw->PathArcTo(ImVec2(to.x-r,from.y+r),r,pi*1.5f,pi*2,16);
  draw->PathArcTo(ImVec2(to.x-r,to.y-r),r,0,pi*.5f,16);
  draw->PathArcTo(ImVec2(from.x+r,to.y-r),r,pi*.5f,pi,16);
}
void Sims2DrawInstallerImage(ImDrawList *draw,Sims2InstallerImage image,ImVec2 from,ImVec2 to,float rounding) {
  const auto &item=images[size_t(image)];
  if(rounding<=0){draw->AddImage(ImGui::GetIO().Fonts->TexID,from,to,item.uv0,item.uv1);return;}
  draw->PushTextureID(ImGui::GetIO().Fonts->TexID);
  const int first=draw->VtxBuffer.Size;
  Sims2InstallerRoundedPath(draw,from,to,rounding);draw->PathFillConvex(IM_COL32_WHITE);
  ImGui::ShadeVertsLinearUV(draw,first,draw->VtxBuffer.Size,from,to,item.uv0,item.uv1,true);
  draw->PopTextureID();
}
void Sims2SetNextExecutable(const std::string &path) {
  constexpr const char *prefix="/app0/game-";
  if(path.size()!=std::strlen(prefix)+64+4 || path.compare(0,std::strlen(prefix),prefix)!=0 ||
      path.substr(path.size()-4)!=".bin" || path.substr(std::strlen(prefix),64).find_first_not_of("0123456789abcdef")!=std::string::npos)
    throw std::runtime_error("Refusing an unexpected handoff path");
  const auto mode=sims2::files::EnsureExecutable(path);
  std::fprintf(stderr,"[SIMS2][INSTALLER][INFO] event=handoff_preflight mode_before=%o mode_after=%o repaired=%d\n",mode.before,mode.after,int(mode.repaired));
  std::fflush(stderr);
  next_executable=path;
  next_update_helper=false;
}
void Sims2SetUpdateHandoff(bool helper){
  next_update_helper=helper;next_executable=helper?"/app0/.sims2-update/helper.bin":"/app0/eboot.bin";
}
extern "C" const char *ps5NextExecutable() {return next_executable.empty()?nullptr:next_executable.c_str();}
extern "C" const char *const *ps5NextExecutableArguments(){
  static const char *args[]={"/app0/.sims2-update/helper.bin","--sims2-apply-update",nullptr};
  return next_update_helper?args:nullptr;
}
