// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "public_update.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
int main(int argc,char **argv){
  try{
    if(argc<3)throw std::runtime_error("operation root [bundle hash version current] [fail/crash at entry]");
    const std::string operation=argv[1];
    auto root=std::filesystem::absolute(argv[2]);
    if(operation=="prepare"){
      if(argc!=7)throw std::runtime_error("prepare arguments");
      sims2::update::Prepare(root,argv[3],argv[4],argv[5],argv[6]);
      sims2::update::CopyHelper(root);
    }else if(operation=="recover")sims2::update::Recover(root);
    else if(operation=="apply")sims2::update::Apply(root,[&](unsigned completed,unsigned){
      if(argc>4 && completed==unsigned(std::stoi(argv[4]))){
        if(std::string(argv[3])=="crash")std::_Exit(23);
        throw std::runtime_error("injected interruption");
      }
    });else throw std::runtime_error("unknown operation");
    std::puts("PASS");return 0;
  }catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
