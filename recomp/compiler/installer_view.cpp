// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Selected layout B: artwork on the left, setup on the right. A shared view
// also permits asset-free host layout review without starting the compiler.
#include "installer_view.h"
#include "installer_ui_assets.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr ImU32 white=IM_COL32(245,244,250,255),muted=IM_COL32(192,189,206,255),
  dim=IM_COL32(140,133,155,255),mint=IM_COL32(159,228,207,255),ink=IM_COL32(37,34,52,255);
// Open Sans' line height is 1.362 em. The bundled stb font builder measures
// pixels by that line height, whereas the selected design uses em sizes.
constexpr float font_em=1.362f;
constexpr float tv_text_scale=1.12f;

struct Canvas {
  ImDrawList *draw=ImGui::GetOverlayDrawList();
  ImFont *font=ImGui::GetIO().Fonts->Fonts[0];
  ImVec2 display=ImGui::GetIO().DisplaySize,offset;
  float scale;
  Canvas():scale(std::min(display.x/1280.f,display.y/720.f)) {
    offset=ImVec2((display.x-1280*scale)*.5f,(display.y-720*scale)*.5f);
  }
  ImVec2 p(float x,float y)const{return ImVec2(offset.x+x*scale,offset.y+y*scale);}
  ImVec2 ink_bounds(const std::string &text,float size)const {
    float low=font->FontSize,high=0;
    for(unsigned char c:text){if(c=='\n')break;if(c==' ')continue;
      const auto *g=font->FindGlyph(c);if(g){low=std::min(low,g->Y0);high=std::max(high,g->Y1);}}
    if(high<=low)return ImVec2(0,size);
    return ImVec2(low*size/font->FontSize,high*size/font->FontSize);
  }
  void text(float x,float top,float size,ImU32 color,const std::string &value,float wrap=0) {
    size*=font_em*tv_text_scale;
    const auto bounds=ink_bounds(value,size);
    draw->AddText(font,size*scale,p(x,top-bounds.x),color,value.c_str(),nullptr,wrap*scale);
  }
  void center(float x,float y,float size,ImU32 color,const std::string &value) {
    size*=font_em*tv_text_scale;
    const auto extent=font->CalcTextSizeA(size*scale,100000,0,value.c_str());
    const auto bounds=ink_bounds(value,size);
    // Centre the visible glyphs, not a font line box with unused descent.
    draw->AddText(font,size*scale,p(x-extent.x/scale*.5f,y-(bounds.x+bounds.y)*.5f),color,value.c_str());
  }
  void rect(float x,float y,float w,float h,ImU32 color,float radius) {
    Sims2InstallerRoundedPath(draw,p(x,y),p(x+w,y+h),radius*scale);draw->PathFillConvex(color);
  }
  void outline(float x,float y,float w,float h,ImU32 color,float radius,float thickness=1) {
    Sims2InstallerRoundedPath(draw,p(x,y),p(x+w,y+h),radius*scale);draw->PathStroke(color,true,thickness*scale);
  }
  void image(Sims2InstallerImage id,float x,float y,float w,float h,float radius=0) {
    Sims2DrawInstallerImage(draw,id,p(x,y),p(x+w,y+h),radius*scale);
  }
  void circle(float x,float y,float radius,ImU32 color){draw->AddCircleFilled(p(x,y),radius*scale,color,48);}
  void check(float x,float y) {
    draw->PathLineTo(p(x-5,y));draw->PathLineTo(p(x-1,y+4));draw->PathLineTo(p(x+6,y-5));
    draw->PathStroke(ink,false,2*scale);
  }
  void action(float x,float y,Sims2InstallerImage icon,const std::string &label) {
    image(icon,x,y-15,30,30);text(x+44,y-8,17,white,label);
  }
};
}

void Sims2DrawInstallerView(const Sims2InstallerView &s) {
  Canvas c;const auto phase=s.phase;
  const bool ready=phase==Sims2InstallPhase::Ready,play=ready && s.section==Sims2LauncherSection::Play;
  const bool update=s.section==Sims2LauncherSection::Update;
  const bool question=phase==Sims2InstallPhase::DeleteQuestion;
  const bool error=phase==Sims2InstallPhase::Error,stopped=phase==Sims2InstallPhase::Cancelled;
  c.image(Sims2InstallerImage::Background,-c.offset.x/c.scale,-c.offset.y/c.scale,c.display.x/c.scale,c.display.y/c.scale);
  c.draw->AddRectFilled(ImVec2(0,0),c.display,IM_COL32(0,0,0,191)); // 75% black.
  c.image(Sims2InstallerImage::Logo,40,28,70,70,16);
  c.text(130,35,34,white,"The Sims 2");c.text(132,80,15,muted,"v"+s.version);
  c.center(1140,60,13,muted,"LAUNCHER MENU");
  if(s.initial_check){
    c.center(640,346,30,white,"Checking your game...");
    c.center(640,394,16,muted,"Please wait");
    for(int i=0;i<3;++i)c.circle(625+i*15,432,3.5f,IM_COL32(245,244,250,
      int(110+130*(.5+.5*std::sin(s.time*3-i*.8)))));
    return;
  }
  const int content_begin=c.draw->VtxBuffer.Size;
  if(play){
    const float text_size=64*font_em*tv_text_scale;
    const float text_width=c.font->CalcTextSizeA(text_size*c.scale,100000,0,"Play").x/c.scale;
    const float left=(1280-80-24-text_width)*.5f;
    c.image(Sims2InstallerImage::Cross,left,320,80,80);
    c.center(left+104+text_width*.5f,360,64,white,"Play");
  }else{
  c.rect(40,128,1200,474,IM_COL32(41,37,51,255),22);
  const int preview=std::clamp(s.preview,0,2);
  const int image_begin=c.draw->VtxBuffer.Size;
  c.image(Sims2InstallerImage(int(Sims2InstallerImage::Preview1)+preview),64,152,664,373.5f,16);
  for(int i=image_begin;i<c.draw->VtxBuffer.Size;++i){auto &color=c.draw->VtxBuffer[i].col;
    const auto alpha=unsigned(std::lround(((color>>IM_COL32_A_SHIFT)&255)*std::clamp(s.preview_alpha,0.f,1.f)));
    color=(color&~(255u<<IM_COL32_A_SHIFT))|(alpha<<IM_COL32_A_SHIFT);}
  c.rect(82,170,128,32,IM_COL32(32,27,44,245),16);
  c.center(146,186,12,white,"GAMEPLAY");
  for(int i=0;i<3;++i)c.circle(374+i*22,549,4.5f,IM_COL32(114,103,129,255));
  c.circle(374+22*s.preview_position,549,4.5f,white);
  c.image(Sims2InstallerImage::RightStick,325,564,30,30);
  c.text(369,572,14,muted,"Browse images");

  c.text(770,164,29,white,update?"Updates":question?"Your game is ready.":error?"Let's try again.":"Enjoy the game!");
  c.text(770,205,29,mint,update?"Keep your game fresh.":question?"Keep your ISO?":error?"We couldn't finish.":"The Sims 2 PS5");
  if(update){c.text(770,259,15,muted,"Update the launcher and your personal game.");c.text(770,284,15,muted,"Your settings and saved games stay safe.");}
  else if(question){c.text(770,259,15,muted,"Installation completed successfully.");c.text(770,284,15,muted,"You can now delete the source ISO to free space.");}
  else if(error || stopped){c.text(770,259,15,muted,stopped?"Installation was stopped.":"Setup needs your attention.");
    c.text(770,284,15,muted,"Your saves are preserved.");}
  else if(s.busy){c.text(770,259,15,muted,phase==Sims2InstallPhase::Checking?"Checking for an existing installation...":"Your game is being prepared on this PS5.");
    c.text(770,284,15,muted,"Please keep this application open.");}
  else{c.text(770,259,15,muted,"Add your Xbox USA ISO to "+s.title_id+".");
    c.text(770,284,15,muted,"Select Install to prepare it on this console.");}

  int stage=-1;float progress=0;
  if(phase==Sims2InstallPhase::Extracting){stage=0;progress=.25f*std::min(s.extraction,100u)/100.f;}
  if(phase==Sims2InstallPhase::Verifying){stage=1;progress=.25f;}
  if(phase==Sims2InstallPhase::Generating){stage=1;progress=.28f;}
  if(phase==Sims2InstallPhase::Compiling){stage=1;progress=.3f+.65f*(s.total?std::min(float(s.completed)/s.total,1.f):0);}
  if(phase==Sims2InstallPhase::Linking){stage=2;progress=.95f;}
  if(ready || question){stage=3;progress=1;}
  if(update){
    const auto status=s.update.state;
    const bool available=status==Sims2UpdateState::Available;
    const bool updating=Sims2UpdateBusy(status);
    c.rect(754,328,466,86,IM_COL32(50,43,62,255),16);
    c.rect(754,428,466,86,IM_COL32(50,43,62,255),16);
    c.rect(754,328+100*s.update_position+s.update_offset,466,86,IM_COL32(87,69,113,255),16);
    c.text(774,345,20,white,"Update launcher");
    const auto description=updating?s.update.message+(" ("+std::to_string(int(s.update.progress*100))+"%)"):
      status==Sims2UpdateState::Failed?(s.restart_required?"Close and reopen the launcher to retry.":"Update failed. See klog; retry or close."):
      available && !s.update.compatible?"New release needs a manual package update.":
      status==Sims2UpdateState::Checking?"Checking GitHub...":
      status==Sims2UpdateState::Unavailable?"Check your connection and try again.":
      available?("New release: "+s.update.version):("Launcher v"+s.version+" is current.");
    c.draw->PushClipRect(c.p(774,378),c.p(1204,410),true);c.text(774,382,13,muted,description);c.draw->PopClipRect();
    const bool gameUpdate=s.has_game && s.game_update_available;
    c.text(774,445,20,gameUpdate || s.busy?white:dim,"Update game");
    c.text(774,482,13,muted,!s.has_game?"Install your game first.":s.busy?"Updating with the launcher's game data...":
      gameUpdate?"New game data is included in this launcher.":"Your game matches this launcher's data.");
    if(s.busy || updating){c.rect(770,525,434,8,IM_COL32(99,90,115,255),4);
      const float fraction=updating?s.update.progress:progress;
      if(fraction>0)c.rect(770,525,434*std::clamp(fraction,0.f,1.f),8,mint,4);}
  }else if(!error && !stopped){
    const char *titles[]={"Extract game data","Prepare for PS5","Finish installation"};
    std::string descriptions[]={"Read and verify your ISO.","Prepare your game for this console.","Verify and get ready to play."};
    if(phase==Sims2InstallPhase::Extracting)descriptions[0]="Extracting game data: "+std::to_string(s.extraction)+"%";
    if(phase==Sims2InstallPhase::Verifying)descriptions[1]="Verifying installation resources...";
    if(phase==Sims2InstallPhase::Generating)descriptions[1]="Analyzing the Xbox executable...";
    if(phase==Sims2InstallPhase::Compiling)descriptions[1]=std::to_string(s.completed)+" of "+std::to_string(s.total)+" parts compiled";
    if(phase==Sims2InstallPhase::Linking)descriptions[2]="Creating your playable game...";
    for(int i=0;i<3;++i){const float y=342+i*59;
      const bool complete=i<stage;
      c.circle(784,y+8,11,complete?mint:i==stage?IM_COL32(184,158,220,255):IM_COL32(80,71,89,255));
      if(complete)c.check(784,y+8);else c.center(784,y+8,12,i==stage?ink:muted,std::to_string(i+1));
      c.text(807,y-2,18,complete || i==stage?white:muted,titles[i]);
      c.text(807,y+24,13,muted,complete?"Complete":descriptions[i]);
    }
    if(!play){c.rect(770,525,434,8,IM_COL32(99,90,115,255),4);
      if(progress>0)c.rect(770,525,434*progress,8,mint,4);}
  }else{
    c.rect(754,328,466,194,error?IM_COL32(68,41,54,255):IM_COL32(50,43,62,255),16);
    c.text(774,348,18,white,stopped?"You can continue the setup.":"Installation couldn't be completed.");
    c.draw->PushClipRect(c.p(774,381),c.p(1200,477),true);
    c.text(774,382,14,muted,s.error.empty()?"Try again, or close the application and return later.":s.error,420);
    c.draw->PopClipRect();c.text(774,494,12,muted,"Technical details are saved to klog.");
  }

  if(update){
    if(!s.busy && !s.restart_required && !Sims2UpdateBusy(s.update.state) && (s.update_choice==0 || (s.has_game && s.game_update_available)))
      c.action(770,561,Sims2InstallerImage::Cross,s.update_choice==0?
        (s.update.state==Sims2UpdateState::Available && s.update.compatible?"Download and update":"Check launcher update"):"Update game");
  }else if(question){c.action(770,561,Sims2InstallerImage::Cross,"Delete ISO");c.action(1000,561,Sims2InstallerImage::Circle,"Keep ISO");}
  else if((error || stopped) && !s.restart_required)c.action(770,561,Sims2InstallerImage::Cross,"Retry installation");
  else if(s.restart_required)c.text(770,553,14,muted,"Close and reopen the application to retry.");
  else if(ready)c.action(770,561,Sims2InstallerImage::Cross,"Go to Play");
  else if(phase==Sims2InstallPhase::Missing)c.action(770,561,Sims2InstallerImage::Cross,"Install game");
  else c.text(770,556,14,muted,s.cancelling?"Stopping after the current operation...":"Your saves are preserved.");
  c.text(770,582,11,muted,"Updates: github.com/heni0xyz/Sims-2-PS5");
  }

  const float opacity=std::clamp(s.content_alpha,0.f,1.f);
  for(int i=content_begin;i<c.draw->VtxBuffer.Size;++i){auto &color=c.draw->VtxBuffer[i].col;
    const auto alpha=unsigned(std::lround(((color>>IM_COL32_A_SHIFT)&255)*opacity));
    color=(color&~(255u<<IM_COL32_A_SHIFT))|(alpha<<IM_COL32_A_SHIFT);}

  // Thin glass-like rim with a restrained halo, following the supplied reference.
  // Explicit rounded paths keep it smooth at native 4K with this ImGui revision.
  for(int spread=3;spread>=1;--spread)
    c.outline(388-spread,624-spread,504+spread*2,60+spread*2,IM_COL32(201,187,224,5+2*(3-spread)),30+spread,1.4f);
  c.rect(388,624,504,60,IM_COL32(40,36,51,255),30);
  c.outline(388.5f,624.5f,503,59,IM_COL32(213,204,230,52),29.5f,1);
  const float selected=472+168*s.selection_position+s.selection_offset;
  c.rect(selected-78,630,156,48,IM_COL32(102,84,131,255),24);
  c.outline(selected-77.5f,630.5f,155,47,IM_COL32(235,224,253,35),23.5f,.8f);
  const int pulse=239+int(10*(.5+.5*std::sin(s.time*1.85)));
  const ImU32 active=IM_COL32(pulse,pulse,pulse,255);
  const auto label=[&](Sims2LauncherSection section,bool enabled){return !enabled?dim:s.section==section?active:muted;};
  c.center(472,654,23,label(Sims2LauncherSection::Play,ready && !s.busy),"Play");
  c.center(640,654,23,label(Sims2LauncherSection::Install,true),"Install");
  c.center(808,654,23,label(Sims2LauncherSection::Update,!s.busy),"Update");
  c.image(Sims2InstallerImage::L1,40,645,34,34);c.center(87,662,16,muted,"/");
  c.image(Sims2InstallerImage::R1,100,645,34,34);c.text(150,655,17,white,"Move section");
  c.image(Sims2InstallerImage::Circle,1040,645,34,34);
  c.text(1088,655,17,white,question?"Keep ISO":s.busy?"Cancel":"Close");
  c.image(Sims2InstallerImage::Touchpad,40,690,28,22);
  c.text(80,695,12,muted,s.music_enabled?"Music on - press to mute":"Music off - press to enable");
  if(update){c.image(Sims2InstallerImage::DPad,790,690,26,22);c.text(830,695,12,muted,"Choose update");}
  if(s.launch_alpha>0)c.draw->AddRectFilled(ImVec2(0,0),c.display,IM_COL32(0,0,0,
    unsigned(std::lround(255*std::clamp(s.launch_alpha,0.f,1.f)))));
}
