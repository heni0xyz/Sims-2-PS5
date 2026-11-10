// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <string>
inline bool Sims2ParseVersion(const std::string &s,std::array<unsigned,3> &parts){
  size_t at=!s.empty() && (s[0]=='v' || s[0]=='V')?1:0;
  for(unsigned i=0;i<3;++i){size_t start=at;unsigned n=0;
    while(at<s.size() && s[at]>='0' && s[at]<='9'){n=n*10+unsigned(s[at++]-'0');if(n>1000000)return false;}
    if(at==start)return false;parts[i]=n;
    if(i<2 && (at>=s.size() || s[at++]!='.'))return false;
  }return at==s.size();
}
inline bool Sims2VersionAfter(const std::string &a,const std::string &b){std::array<unsigned,3> x{},y{};return Sims2ParseVersion(a,x)&&Sims2ParseVersion(b,y)&&x>y;}
