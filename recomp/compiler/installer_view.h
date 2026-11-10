// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <string>
#include <cmath>
#include "installer_updates.h"

enum class Sims2InstallPhase { Checking, Missing, Verifying, Extracting, Generating,
  Compiling, Linking, DeleteQuestion, Ready, Error, Cancelled };

enum class Sims2LauncherSection { Play, Install, Update };
struct Sims2LauncherStartup {
  bool pending=true;
  bool Resolve(bool done){if(!pending || !done)return false;pending=false;return true;}
  static Sims2LauncherSection Section(Sims2InstallPhase phase){return phase==Sims2InstallPhase::Ready?
    Sims2LauncherSection::Play:Sims2LauncherSection::Install;}
};
struct Sims2LaunchFade {
  bool active=false;double began=0;
  void Start(double now){if(!active){active=true;began=now;}}
  float Alpha(double now)const {
    if(!active)return 0;
    const float t=float(std::fmax(0.,std::fmin(1.,(now-began)/.18)));
    return t*t*(3-2*t);
  }
  bool Black(double now)const{return active && now-began>=.18;}
};
// 70 ms fade-out followed by 90 ms fade-in. Re-targeting starts at the
// current opacity/position, including rapid direction changes mid-transition.
struct Sims2LauncherTransition {
  Sims2LauncherSection visible=Sims2LauncherSection::Install,target=visible;
  double began=-1;
  float bar_from=1,alpha_from=1;
  explicit Sims2LauncherTransition(Sims2LauncherSection initial=Sims2LauncherSection::Install):visible(initial),target(initial),bar_from(float(int(initial))){}
  static float Smooth(double value){const float t=float(std::fmax(0.,std::fmin(1.,value)));return t*t*(3-2*t);}
  float Position(double now)const{return began<0?float(int(target)):bar_from+(int(target)-bar_from)*Smooth((now-began)/.16);}
  float Alpha(double now)const {
    if(began<0)return 1;
    const double elapsed=std::fmax(0.,now-began);
    if(visible==target)return alpha_from+(1-alpha_from)*Smooth(elapsed/.09);
    return elapsed<.07?alpha_from*(1-Smooth(elapsed/.07)):Smooth((elapsed-.07)/.09);
  }
  Sims2LauncherSection Displayed(double now)const{return began<0 || now-began>=.07?target:visible;}
  bool Settled(double now)const{return began<0 || now-began>=.16;}
  void Select(Sims2LauncherSection next,double now){
    if(next==target)return;
    const auto previous=Displayed(now);const float position=Position(now),alpha=Alpha(now);
    visible=previous;target=next;bar_from=position;alpha_from=alpha;began=now;
  }
};
struct Sims2SelectionMotion {
  int selected=0,reject_direction=0;
  float from=0;
  double began=-1,rejected_at=-1;
  float Position(double now)const{return began<0?float(selected):from+(selected-from)*Sims2LauncherTransition::Smooth((now-began)/.12);}
  float Offset(double now)const {const double t=now-rejected_at;return rejected_at>=0 && t>=0 && t<.18?float(reject_direction*12*std::sin(t/.18*3.141592653589793)):0;}
  bool Settled(double now)const{return began<0 || now-began>=.12;}
  void Select(int value,double now){if(value==selected)return;from=Position(now);selected=value;began=now;rejected_at=-1;}
  bool Move(int direction,bool second_enabled,double now){
    const int next=selected+direction;
    if(next<0 || next>1 || (next==1 && !second_enabled)){reject_direction=direction;rejected_at=now;return false;}
    Select(next,now);return true;
  }
};
// Dominant axis, dead-zone hysteresis, one initial move and bounded hold repeat.
// Used for the left stick; the right stick stays dedicated to the carousel.
struct Sims2LauncherStick {
  int held=0;
  double repeat_at=0;
  int Move(float x,float y,double now){
    if(std::fmax(std::fabs(x),std::fabs(y))<.25f){held=0;return 0;}
    int next=0;
    if(std::fabs(x)>std::fabs(y)){if(std::fabs(x)>=.55f)next=x>0?1:-1;}
    else if(std::fabs(y)>=.55f)next=y>0?2:-2;
    if(!next || (next==held && now<repeat_at))return 0;
    repeat_at=now+(next==held?.30:.50);held=next;return next;
  }
};
struct Sims2LauncherNavigation {
  Sims2LauncherSection section=Sims2LauncherSection::Install;
  double rejected_at=-1;
  int rejected_direction=0;
  bool Move(int direction,bool ready,double now,bool locked=false) {
    const int next=int(section)+direction;
    if(locked || next<0 || next>2 || (!ready && next==int(Sims2LauncherSection::Play))) {
      rejected_at=now;rejected_direction=direction;return false;
    }
    section=Sims2LauncherSection(next);rejected_at=-1;return true;
  }
  float Offset(double now) const {
    const double t=now-rejected_at;
    return rejected_at>=0 && t>=0 && t<.18 ? float(rejected_direction*14*std::sin(t/.18*3.141592653589793)) : 0;
  }
};

struct Sims2InstallerView {
  Sims2InstallPhase phase=Sims2InstallPhase::Checking;
  Sims2LauncherSection section=Sims2LauncherSection::Install;
  Sims2UpdateStatus update;
  float selection_offset=0;
  float selection_position=1,content_alpha=1,launch_alpha=0;
  float update_position=0,update_offset=0,preview_position=0,preview_alpha=1;
  int update_choice=0;
  bool music_enabled=true,has_game=false,game_update_available=false,initial_check=false;
  double time=0;
  bool busy=false, cancelling=false, restart_required=false;
  unsigned completed=0,total=0,extraction=0;
  int preview=0;
  std::string error,version,title_id,game_launcher_version;
};

// One deliberate tilt changes one image. Holding repeats slowly; drift and
// the return through the dead zone never advance the carousel.
struct Sims2InstallerCarousel {
  int selected=0,direction=0;
  double repeat_at=0,next_auto=0;
  bool auto_running=false;
  bool Auto(bool running,double now){
    if(!running){auto_running=false;return false;}
    if(!auto_running){auto_running=true;next_auto=std::fmax(next_auto,now+7);return false;}
    if(now<next_auto)return false;
    selected=(selected+1)%3;next_auto=now+7;return true;
  }
  bool Move(float right_x,double now) {
    if(right_x>-.25f && right_x<.25f){direction=0;return false;}
    const int next=right_x>=.55f?1:right_x<=-.55f?-1:0;
    if(!next)return false;
    if(next==direction && now<repeat_at)return false;
    repeat_at=now+(next==direction?.35:.65);direction=next;next_auto=now+10;
    selected=(selected+next+3)%3;
    return true;
  }
};

void Sims2DrawInstallerView(const Sims2InstallerView &);
