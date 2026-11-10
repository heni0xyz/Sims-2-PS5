// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// HTTPS handles and redirect support follow Mihawk's PS5_RetroArch
// src/webui_update.cpp, GPL-3.0-or-later, 7268510b12fe7461342e0cc4497c87a0910c1055.
#include "installer_updates.h"
#include "launcher_version.h"
#include "public_update.h"
#include "llvm/Support/JSON.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <pthread.h>
#include <stdexcept>

extern "C" {
int sceNetInit();int sceNetPoolCreate(const char *,int,int);int sceNetPoolDestroy(int);
int sceSslInit(size_t);int sceSslTerm(int);int sceHttp2Init(int,int,size_t,int);int sceHttp2Term(int);
int sceHttp2CreateTemplate(int,const char*,int,int);int sceHttp2DeleteTemplate(int);
int sceHttp2CreateRequestWithURL(int,const char*,const char*,uint64_t);int sceHttp2DeleteRequest(int);
int sceHttp2SendRequest(int,const void*,size_t);int sceHttp2GetStatusCode(int,int*);int sceHttp2ReadData(int,void*,size_t);
int sceHttp2SetConnectTimeOut(int,unsigned);int sceHttp2SetRecvTimeOut(int,unsigned);int sceHttp2SetAutoRedirect(int,int);
}
namespace {
constexpr const char *Root="/app0";
constexpr uint64_t Limit=1024ull*1024*1024;
enum class Operation {Check,Download,Apply};
struct Release {std::string tag,url,sha;uint64_t size=0;};
struct State {std::mutex mutex;bool started=false,helper=false;std::string current;
  Operation operation=Operation::Check;Release release;Sims2UpdateStatus result;};
State &state(){static State *value=new State;return *value;}
void require(bool good,const char *reason){if(!good)throw std::runtime_error(reason);}
void status(Sims2UpdateState value,const std::string &message={},float progress=0){
  auto &s=state();std::lock_guard<std::mutex> guard(s.mutex);
  const bool changed=s.result.state!=value;s.result.state=value;s.result.message=message;s.result.progress=progress;
  if(changed){std::fprintf(stderr,"[SIMS2][UPDATES][INFO] event=state state=%d message=%s\n",int(value),message.c_str());std::fflush(stderr);}
}
struct Http {
  int pool=-1,ssl=-1,http=-1,templ=-1,request=-1;
  ~Http(){if(request>=0)sceHttp2DeleteRequest(request);if(templ>=0)sceHttp2DeleteTemplate(templ);
    if(http>=0)sceHttp2Term(http);if(ssl>=0)sceSslTerm(ssl);if(pool>=0)sceNetPoolDestroy(pool);}
  void Open(const std::string &url,const std::string &current){
    (void)sceNetInit();pool=sceNetPoolCreate("Sims2 public updates",1024*1024,0);require(pool>=0,"Network unavailable");
    ssl=sceSslInit(1024*1024);require(ssl>=0,"Cannot initialize HTTPS");
    http=sceHttp2Init(pool,ssl,1024*1024,1);require(http>=0,"Cannot initialize HTTP");
    const std::string agent="Sims2-PS5-Installer/"+current;
    templ=sceHttp2CreateTemplate(http,agent.c_str(),3,1);require(templ>=0,"Cannot create HTTPS template");
    // Keep certificate and hostname validation enabled. GitHub asset URLs
    // redirect to its HTTPS release CDN using the platform's redirect support.
    require(sceHttp2SetAutoRedirect(templ,1)>=0,"HTTPS redirects unavailable");
    require(sceHttp2SetConnectTimeOut(templ,5000000)>=0 && sceHttp2SetRecvTimeOut(templ,5000000)>=0,"Cannot set network timeout");
    request=sceHttp2CreateRequestWithURL(templ,"GET",url.c_str(),0);
    require(request>=0 && sceHttp2SendRequest(request,nullptr,0)>=0,"HTTPS request failed");
    int code=0;require(sceHttp2GetStatusCode(request,&code)>=0 && code==200,"GitHub response was not successful");
  }
};
void check(const std::string &current){
  Http h;h.Open("https://api.github.com/repos/heni0xyz/Sims-2-PS5/releases/latest",current);
  std::string body;char buffer[8192];int n;
  const auto began=std::chrono::steady_clock::now();
  while((n=sceHttp2ReadData(h.request,buffer,sizeof(buffer)))>0){body.append(buffer,size_t(n));
    require(body.size()<=128*1024 && std::chrono::steady_clock::now()-began<std::chrono::seconds(15),"Release metadata exceeded limits");}
  require(n==0,"Release metadata download failed");
  auto parsed=llvm::json::parse(body);if(!parsed)throw std::runtime_error(llvm::toString(parsed.takeError()));
  auto *object=parsed->getAsObject();auto tag=object?object->getString("tag_name"):std::nullopt;
  std::array<unsigned,3> ignored{};require(tag && Sims2ParseVersion(tag->str(),ignored),"Unsupported release version");
  Release release;release.tag=tag->str();
  const bool newer=Sims2NewerVersion(release.tag,current);
  if(newer){
    require(object->getBoolean("draft")==false && object->getBoolean("prerelease")==false,"Release is not stable");
    const auto *assets=object->getArray("assets");unsigned matches=0;
    if(assets)for(const auto &value:*assets){const auto *asset=value.getAsObject();
      if(!asset || asset->getString("name")!="Sims2-PS5-Launcher.s2u")continue;
      ++matches;auto url=asset->getString("browser_download_url"),digest=asset->getString("digest");auto size=asset->getInteger("size");
      if(!url || !digest || !size || *size<=12 || uint64_t(*size)>Limit)continue;
      const std::string expected="https://github.com/heni0xyz/Sims-2-PS5/releases/download/"+release.tag+"/Sims2-PS5-Launcher.s2u";
      const auto checksum=digest->str();if(url->str()!=expected || checksum.rfind("sha256:",0)!=0 || checksum.size()!=71 ||
          checksum.substr(7).find_first_not_of("0123456789abcdef")!=std::string::npos)continue;
      release.url=url->str();release.sha=checksum.substr(7);release.size=uint64_t(*size);
    }
    if(matches!=1){release.url.clear();release.sha.clear();release.size=0;}
  }
  {auto &s=state();std::lock_guard<std::mutex> guard(s.mutex);s.release=release;
    s.result.version=release.tag;s.result.compatible=!release.url.empty();}
  status(newer?Sims2UpdateState::Available:Sims2UpdateState::Current,
         newer && release.url.empty()?"This release requires a manual package update.":"");
}
void download(const std::string &current,const Release &release){
  require(!release.url.empty(),"No compatible public update is available");
  const auto path=sims2::update::DownloadPath(Root);
  Http h;h.Open(release.url,current);std::ofstream out(path,std::ios::binary|std::ios::trunc);
  require(bool(out),"Cannot create update download");char buffer[65536];uint64_t bytes=0;int n;
  const auto began=std::chrono::steady_clock::now();int last=-1;
  while((n=sceHttp2ReadData(h.request,buffer,sizeof(buffer)))>0){bytes+=uint64_t(n);
    require(bytes<=release.size && std::chrono::steady_clock::now()-began<std::chrono::minutes(15),"Update download exceeded limits");
    out.write(buffer,n);require(bool(out),"Cannot write update download");
    const int percent=int(bytes*100/release.size);if(percent!=last){last=percent;
      status(Sims2UpdateState::Downloading,"Downloading public update",float(bytes)/release.size);
      if(percent%5==0){std::fprintf(stderr,"[SIMS2][UPDATES][INFO] event=download percent=%d bytes=%llu\n",percent,(unsigned long long)bytes);std::fflush(stderr);}}
  }
  require(n==0 && bytes==release.size,"Update download was interrupted");out.close();require(bool(out),"Update download could not close");
  status(Sims2UpdateState::Preparing,"Verifying update");
  sims2::update::Prepare(Root,path,release.sha,release.tag,current,[](unsigned n,unsigned total){
    status(Sims2UpdateState::Preparing,"Verifying public files",float(n)/total);});
  sims2::update::CopyHelper(Root);status(Sims2UpdateState::ReadyToApply,"Restarting to apply update",1);
}
void *worker(void*){
  struct Finished {~Finished(){auto &s=state();std::lock_guard<std::mutex> guard(s.mutex);s.started=false;}} finished;
  Operation operation;std::string current;Release release;
  {auto &s=state();std::lock_guard<std::mutex> guard(s.mutex);operation=s.operation;current=s.current;release=s.release;}
  try{
    if(operation==Operation::Check)check(current);
    else if(operation==Operation::Download)download(current,release);
    else {sims2::update::Apply(Root,[](unsigned n,unsigned total){status(Sims2UpdateState::Applying,"Applying public update",float(n)/total);});
      status(Sims2UpdateState::RestartReady,"Update complete. Restarting launcher.",1);}
  }catch(const std::exception &e){std::fprintf(stderr,"[SIMS2][UPDATES][ERROR] event=failed message=%s\n",e.what());std::fflush(stderr);
    status(operation==Operation::Check?Sims2UpdateState::Unavailable:Sims2UpdateState::Failed,e.what());}
  return nullptr;
}
void start(Operation operation,const std::string &current){
  auto &s=state();std::lock_guard<std::mutex> guard(s.mutex);if(s.started)return;
  s.started=true;s.current=current;s.operation=operation;s.result.message.clear();s.result.progress=0;
  s.result.state=operation==Operation::Check?Sims2UpdateState::Checking:operation==Operation::Download?Sims2UpdateState::Downloading:Sims2UpdateState::Applying;
  pthread_t thread;pthread_attr_t attributes;int result=pthread_attr_init(&attributes);
  if(!result){result=pthread_attr_setstacksize(&attributes,4*1024*1024);if(!result)result=pthread_create(&thread,&attributes,worker,nullptr);pthread_attr_destroy(&attributes);}
  if(result){s.started=false;s.result.state=Sims2UpdateState::Failed;s.result.message="Cannot start update worker";}else pthread_detach(thread);
}
}
bool Sims2NewerVersion(const std::string &candidate,const std::string &current){return Sims2VersionAfter(candidate,current);}
void Sims2StartUpdateCheck(const std::string &current){start(Operation::Check,current);}
void Sims2StartLauncherUpdate(const std::string &current){
  const auto s=Sims2GetUpdateStatus();start(s.state==Sims2UpdateState::Available && s.compatible?Operation::Download:Operation::Check,current);
}
void Sims2StartApplyUpdate(){start(Operation::Apply,{});}
Sims2UpdateStatus Sims2GetUpdateStatus(){auto &s=state();std::lock_guard<std::mutex> guard(s.mutex);return s.result;}
bool Sims2IsUpdateHelper(){return state().helper;}
// Called before font/image loading so an interrupted update is rolled back
// before any partially replaced resources are consumed. Native main catches
// errors and returns through the template's normal title shutdown path.
extern "C" bool ps5InstallerArguments(int argc,char **argv){
  try{sims2::update::Recover(Root);
    state().helper=argc==2 && argv && argv[1] && std::string(argv[1])=="--sims2-apply-update";
    std::fprintf(stderr,"[SIMS2][UPDATES][INFO] event=startup_recovery_ok helper=%d\n",int(state().helper));std::fflush(stderr);return true;
  }catch(const std::exception &e){std::fprintf(stderr,"[SIMS2][UPDATES][ERROR] event=startup_recovery_failed message=%s\n",e.what());std::fflush(stderr);return false;}
}
