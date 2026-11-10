// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Mock only the PS5 HTTP transport. Exercise the actual release selection,
// worker, protocol limits and status code. No title writes or network requests.
#include "installer_updates.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
namespace {
std::string body;size_t at=0;int code=200;bool redirected=false,requested=false;
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
Sims2UpdateStatus check(const std::string &json){
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  body=json;at=0;redirected=requested=false;Sims2StartUpdateCheck("1.1.0");
  for(unsigned n=0;n<1000;++n){auto s=Sims2GetUpdateStatus();if(s.state!=Sims2UpdateState::Checking)return s;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));}
  throw std::runtime_error("Metadata test timed out");
}
const std::string asset="{\"name\":\"Sims2-PS5-Launcher.s2u\",\"browser_download_url\":\"https://github.com/heni0xyz/Sims-2-PS5/releases/download/v1.2.0/Sims2-PS5-Launcher.s2u\",\"digest\":\"sha256:"+std::string(64,'a')+"\",\"size\":1234}";
std::string release(const std::string &assets){return "{\"tag_name\":\"v1.2.0\",\"draft\":false,\"prerelease\":false,\"assets\":["+assets+"]}";}
}
extern "C" {
int sceNetInit(){return 0;}int sceNetPoolCreate(const char*,int,int){return 1;}int sceNetPoolDestroy(int){return 0;}
int sceSslInit(size_t){return 2;}int sceSslTerm(int){return 0;}int sceHttp2Init(int,int,size_t,int){return 3;}int sceHttp2Term(int){return 0;}
int sceHttp2CreateTemplate(int,const char*,int,int){return 4;}int sceHttp2DeleteTemplate(int){return 0;}
int sceHttp2SetAutoRedirect(int,int enable){redirected=enable==1;return 0;}
int sceHttp2SetConnectTimeOut(int,unsigned n){need(n==5000000,"Connect timeout missing");return 0;}
int sceHttp2SetRecvTimeOut(int,unsigned n){need(n==5000000,"Read timeout missing");return 0;}
int sceHttp2CreateRequestWithURL(int,const char *method,const char *url,uint64_t){
  need(std::string(method)=="GET" && std::string(url)=="https://api.github.com/repos/heni0xyz/Sims-2-PS5/releases/latest","Unexpected network destination");requested=true;return 5;}
int sceHttp2DeleteRequest(int){return 0;}int sceHttp2SendRequest(int,const void*,size_t){return 0;}
int sceHttp2GetStatusCode(int,int *result){*result=code;return 0;}
int sceHttp2ReadData(int,void *out,size_t capacity){const auto count=std::min(capacity,body.size()-at);std::memcpy(out,body.data()+at,count);at+=count;return int(count);}
}
int main(){try{
  auto result=check(release(asset));need(result.state==Sims2UpdateState::Available && result.compatible && redirected && requested,"Compatible public update was not recognized");
  result=check(release(""));need(result.state==Sims2UpdateState::Available && !result.compatible,"Older/manual release was accepted for automatic install");
  result=check(release(asset+","+asset));need(!result.compatible,"Duplicate asset accepted");
  auto malicious=asset;malicious.replace(malicious.find("https://github.com"),18,"http://evil.test/x");
  result=check(release(malicious));need(!result.compatible,"Wrong asset origin accepted");
  auto digest=asset;digest.replace(digest.find("sha256:"),7,"sha512:");
  need(!check(release(digest)).compatible,"Wrong digest algorithm accepted");
  result=check("{\"tag_name\":\"v1.0.0\"}");need(result.state==Sims2UpdateState::Current,"Version comparison failed");
  result=check("{\"tag_name\":\"v1.2.0-rc1\"}");need(result.state==Sims2UpdateState::Unavailable,"Unsupported version accepted");
  result=check(std::string(140*1024,'x'));need(result.state==Sims2UpdateState::Unavailable,"Oversized metadata accepted");
  code=500;result=check(release(asset));need(result.state==Sims2UpdateState::Unavailable,"HTTP error was treated as success");
  need(!Sims2NewerVersion("1.1.0","1.1.0") && Sims2NewerVersion("v1.2.0","1.1.9"),"Version ordering failed");
  std::this_thread::sleep_for(std::chrono::milliseconds(20));std::puts("PASS_HOST_HTTP_CONTRACT: 10 cases; PS5 transport and helper LoadExec NOT TESTED");return 0;
}catch(const std::exception &e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
