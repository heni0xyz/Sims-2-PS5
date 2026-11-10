// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "launcher_preferences.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv){try{
  check(argc==2,"Supply a fresh test directory");std::filesystem::path root=argv[1];
  check(!std::filesystem::exists(root),"Use a fresh directory");std::filesystem::create_directories(root);
  auto file=(root/"prefs").string();bool enabled=false;
  check(Sims2LoadMusicPreference(file.c_str(),enabled) && enabled,"Missing preference must default on");
  check(Sims2SaveMusicPreference(file.c_str(),false),"Cannot save muted preference");
  check(Sims2LoadMusicPreference(file.c_str(),enabled) && !enabled,"Muted preference not retained");
  check(Sims2SaveMusicPreference(file.c_str(),true),"Cannot atomically replace preference");
  check(Sims2LoadMusicPreference(file.c_str(),enabled) && enabled,"Enabled preference not retained");
  {std::ofstream out(file);out<<"unrecognized settings\n";}
  check(!Sims2LoadMusicPreference(file.c_str(),enabled) && enabled,"Malformed preference accepted");
  check(!Sims2SaveMusicPreference((root/"missing/prefs").string().c_str(),false),"Missing parent save succeeded");
  check(std::distance(std::filesystem::directory_iterator(root),std::filesystem::directory_iterator{})==1,"Abandoned temporary settings");
  std::puts("LAUNCHER_PREFERENCES_PASS default read write replace malformed failure_cleanup");return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
