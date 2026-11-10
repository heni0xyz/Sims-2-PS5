// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "public_update.h"
#include "executable_file.h"
#include "launcher_version.h"
#include "game/install_filesystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <set>
#include <stdexcept>
#include <system_error>
#if defined(_WIN32)
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
namespace fs=std::filesystem;
namespace sims2::update {
namespace {
using Object=llvm::json::Object;using Value=llvm::json::Value;
constexpr uint64_t Limit=1024ull*1024*1024;
constexpr const char *Marker="SIMS2-PUBLIC-UPDATE-1\n";
void require(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
bool regular(const fs::path &p){return fs::is_regular_file(install::files::NoFollow(p));}
bool present(const fs::path &p){return fs::exists(install::files::NoFollow(p));}
void sync_directory(const fs::path &p);
void directory(const fs::path &p){if(!present(p)){require(fs::create_directory(p),"Cannot create update folder");sync_directory(p.parent_path());}require(fs::is_directory(install::files::NoFollow(p)),"Update folder is not a normal directory");}
std::string read(const fs::path &p,size_t cap=4*1024*1024){require(regular(p),"Update input is not a regular file");
  std::ifstream in(p,std::ios::binary);require(bool(in),"Cannot read update input");std::string out;char b[4096];
  while(in){in.read(b,sizeof(b));out.append(b,size_t(in.gcount()));require(out.size()<=cap,"Update metadata is too large");}
  require(in.eof(),"Update metadata read failed");return out;}
void durable(const fs::path &p){FILE *f=std::fopen(p.string().c_str(),"r+b");require(f,"Cannot flush update file");
#if defined(_WIN32)
  int code=_commit(_fileno(f));
#else
  int code=::fsync(fileno(f));
#endif
  const int closed=std::fclose(f);require(code==0 && closed==0,"Cannot make update file durable");}
void sync_directory(const fs::path &p){
#if !defined(_WIN32)
  int fd=::open(p.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW);require(fd>=0,"Cannot open update directory for sync");
  const int result=::fsync(fd),closed=::close(fd);require(!result && !closed,"Cannot sync update directory");
#else
  (void)p; // Windows host tests cover process interruption, not power loss.
#endif
}
void atomic(const fs::path &p,const std::string &bytes){const auto temp=p.string()+".new";
  if(present(temp))require(regular(temp),"Unsafe temporary update file");
  {std::ofstream out(temp,std::ios::binary|std::ios::trunc);out.write(bytes.data(),bytes.size());require(bool(out),"Cannot write update metadata");}
  durable(temp);
  // Metadata names are owned by the validated marker. Windows rename cannot
  // replace a destination; retain the old record until the new file is ready.
#if defined(_WIN32)
  const auto previous=p.string()+".previous";
  if(present(previous)){require(regular(previous),"Unsafe prior update metadata");install::files::Remove(previous);}
  if(present(p))fs::rename(p,previous);
  try{fs::rename(temp,p);}catch(...){if(present(previous))fs::rename(previous,p);throw;}
  if(present(previous))install::files::Remove(previous);
#else
  fs::rename(temp,p);
#endif
  sync_directory(p.parent_path());
}
Value parse(const std::string &bytes){auto v=llvm::json::parse(bytes);if(!v)throw std::runtime_error(llvm::toString(v.takeError()));return std::move(*v);}
std::string encode(const Value &value){std::string out;llvm::raw_string_ostream stream(out);stream<<value;return out;}
std::string text(const Object &o,const char *key){auto v=o.getString(key);require(bool(v),std::string("Missing update field: ")+key);return v->str();}
bool hex(const std::string &s){return s.size()==64 && s.find_first_not_of("0123456789abcdef")==s.npos;}
fs::path workspace(const fs::path &root){require(fs::is_directory(install::files::NoFollow(root)),"Invalid title folder");const auto w=root/".sims2-update";
  if(present(w)){require(fs::is_directory(install::files::NoFollow(w)),"Unsafe update workspace");require(read(w/"owner",128)==Marker,"Unknown update folder; nothing was overwritten");}
  else {directory(w);atomic(w/"owner",Marker);}return w;}
void parents(const fs::path &root,const std::string &name){require(ManagedPath(name),"Unmanaged update path");fs::path p=root;
  for(const auto &part:fs::path(name).parent_path()){p/=part;directory(p);}}
struct Entry {std::string name,sha,old;uint64_t size=0,offset=0;bool existed=false;};
std::vector<Entry> entries(const Object &manifest){const auto *files=manifest.getArray("files");require(files && !files->empty() && files->size()<=10000,"Invalid update inventory");
  std::vector<Entry> result;std::set<std::string> seen;uint64_t offset=0;
  for(const auto &v:*files){auto *o=v.getAsObject();require(o,"Invalid update entry");Entry e;e.name=text(*o,"name");e.sha=text(*o,"sha256");
    auto key=e.name;for(char &c:key)if(c>='A' && c<='Z')c=char(c-'A'+'a');
    auto size=o->getInteger("size"),at=o->getInteger("offset");require(ManagedPath(e.name)&&hex(e.sha)&&seen.insert(key).second,"Unsafe or duplicate update member");
    require(size && at && *size>=0 && *at>=0 && uint64_t(*at)==offset && uint64_t(*size)<=512*1024*1024,"Invalid update bounds");
    e.size=uint64_t(*size);e.offset=offset;offset+=e.size;require(offset<=Limit,"Update payload too large");
    if(auto old=o->getString("old_sha256")){e.old=old->str();require(e.old.empty()||hex(e.old),"Invalid old file digest");e.existed=!e.old.empty();}
    result.push_back(e);
  }
  require(seen.count("eboot.bin") && seen.count("installer-manifest.json") && seen.count("sce_sys/param.json"),"Incomplete public update");
  return result;
}
std::string pending(const fs::path &w){auto id=read(w/"pending",128);require(hex(id),"Invalid pending update id");return id;}
void copy(const fs::path &from,const fs::path &to,const std::string &sha,bool executable=false){require(regular(from),"Update copy source is not regular");
  const auto temp=to.string()+".new";if(present(temp))require(regular(temp),"Unsafe update temporary");
  std::ifstream in(from,std::ios::binary);std::ofstream out(temp,std::ios::binary|std::ios::trunc);char b[65536];
  while(in){in.read(b,sizeof(b));out.write(b,in.gcount());require(bool(out),"Update copy failed");}
  require(in.eof(),"Update copy read failed");out.close();require(bool(out),"Update copy close failed");durable(temp);
  require(HashFile(temp)==sha,"Update copy checksum failed");
  if(executable)(void)sims2::files::EnsureExecutable(temp);
  if(present(to))require(regular(to),"Unsafe copy destination");
  // Filesystem rename replaces a regular destination atomically. In particular,
  // never unlink the shell's eboot before its replacement exists.
  fs::rename(temp,to);sync_directory(to.parent_path());
}
}
bool ManagedPath(const std::string &name){
  if(name.empty() || name.size()>512 || name.front()=='/' || name.find('\\')!=name.npos || name.find(':')!=name.npos)return false;
  for(unsigned char c:name)if(c<32 || c>126)return false;
  if(name.back()=='/' || name.find("//")!=name.npos)return false;
  for(const auto &part:fs::path(name))if(part=="." || part==".." || part.empty())return false;
  for(const auto &part:fs::path(name)){const auto s=part.string();if(s.back()=='.' || s.back()==' ')return false;}
  if(name=="eboot.bin" || name=="installer-manifest.json" || name=="CREDITS.txt" || name=="LICENSE" || name=="LEGAL.txt")return true;
  for(const char *prefix:{"sce_sys/","sce_module/","public-resources/","compiler-resources/","native-title-kit/","shaders/","licenses/"})if(name.rfind(prefix,0)==0)return true;
  return false;
}
std::string HashFile(const fs::path &path){require(regular(path),"Hash input is not a normal file");std::ifstream in(path,std::ios::binary);require(bool(in),"Cannot hash update file");
  llvm::SHA256 hash;char buffer[65536];while(in){in.read(buffer,sizeof(buffer));hash.update(llvm::StringRef(buffer,size_t(in.gcount())));}require(in.eof(),"Update checksum read failed");
  auto bytes=hash.final();std::string out;for(auto b:bytes){out+="0123456789abcdef"[b>>4];out+="0123456789abcdef"[b&15];}return out;}
fs::path DownloadPath(const fs::path &root){const auto w=workspace(root);const auto p=w/"download.s2u.part";
  require(!present(w/"journal.json"),"Update recovery must finish first");require(!present(p)||regular(p),"Unsafe download destination");return p;}
void Prepare(const fs::path &root,const fs::path &bundle,const std::string &sha,const std::string &version,const std::string &current,const Progress &progress){
  require(hex(sha) && HashFile(bundle)==sha,"Downloaded update checksum mismatch");require(Sims2VersionAfter(version,current),"Update is not newer than this launcher");
  std::ifstream in(bundle,std::ios::binary);char header[12];in.read(header,12);require(in.gcount()==12 && std::string(header,8)=="S2UPD001","Unknown public update format");
  uint32_t length=0;for(int i=0;i<4;++i)length|=uint32_t(uint8_t(header[8+i]))<<(i*8);require(length && length<=4*1024*1024,"Invalid update manifest length");
  std::string bytes(length,'\0');in.read(bytes.data(),length);require(in.gcount()==length,"Truncated update manifest");auto value=parse(bytes);auto *manifest=value.getAsObject();require(manifest,"Invalid update manifest");
  require(manifest->getInteger("schema")==1 && manifest->getInteger("launcher_abi")==1,"Unsupported launcher update ABI");
  auto current_param=parse(read(root/"sce_sys/param.json"));auto *parameter=current_param.getAsObject();require(parameter && text(*parameter,"titleId")==text(*manifest,"title_id"),"Update title does not match");
  std::array<unsigned,3> a{},b{};require(Sims2ParseVersion(version,a)&&Sims2ParseVersion(text(*manifest,"version"),b)&&a==b,"Update release version differs");
  auto files=entries(*manifest);uint64_t size=0;for(const auto &e:files)size+=e.size;
  // Title sandboxes refuse path-based posix_stat. Inspect the already opened
  // regular file through its stream instead, as the proven ISO reader does.
  in.seekg(0,std::ios::end);const auto bytes_on_disk=in.tellg();
  require(bytes_on_disk>=0 && uint64_t(bytes_on_disk)==12+length+size,"Update length differs from inventory");
  in.seekg(12+length,std::ios::beg);require(bool(in),"Cannot seek update payload");
  Recover(root);const auto w=workspace(root),job=w/("job-"+sha);directory(job);directory(job/"stage");directory(job/"backup");
  unsigned count=0;char buffer[65536];
  for(const auto &e:files){parents(job/"stage",e.name);const auto target=job/"stage"/e.name,temp=fs::path(target.string()+".new");
    if(present(temp))require(regular(temp),"Unsafe staged temporary");
    std::ofstream out(temp,std::ios::binary|std::ios::trunc);uint64_t remaining=e.size;
    while(remaining){const auto n=std::min<uint64_t>(remaining,sizeof(buffer));in.read(buffer,n);require(uint64_t(in.gcount())==n,"Truncated update payload");out.write(buffer,n);require(bool(out),"Update staging write failed");remaining-=n;}
    out.close();require(bool(out),"Update staging close failed");durable(temp);require(HashFile(temp)==e.sha,"Staged update checksum failed");
    // Keep the currently loaded libc ABI stable across power interruption and
    // helper handoff. Module/SDK changes require a complete manual package.
    if(e.name.rfind("sce_module/",0)==0)require(regular(root/e.name)&&HashFile(root/e.name)==e.sha,"This release changes runtime modules; install the complete package manually");
    if(present(target)){require(regular(target),"Unsafe stage destination");install::files::Remove(target);}fs::rename(temp,target);sync_directory(target.parent_path());
    if(progress)progress(++count,unsigned(files.size()));
  }
  auto next_param=parse(read(job/"stage/sce_sys/param.json"));const auto *np=next_param.getAsObject();
  require(np && text(*np,"titleId")==text(*manifest,"title_id"),"Staged title identity differs");
  auto next_manifest=parse(read(job/"stage/installer-manifest.json"));const auto *nm=next_manifest.getAsObject();
  std::array<unsigned,3> nv{};require(nm && Sims2ParseVersion(text(*nm,"version"),nv) && nv==a,"Staged launcher version differs");
  atomic(job/"plan.json",encode(value));atomic(w/"pending",sha);
}
void CopyHelper(const fs::path &root){const auto w=workspace(root);(void)pending(w);const auto file=root/"eboot.bin";
  require(regular(file),"Missing trusted current launcher");copy(file,w/"helper.bin",HashFile(file),true);}
void Recover(const fs::path &root){const auto w=root/".sims2-update";if(!present(w))return;(void)workspace(root);
  if(!present(w/"journal.json"))return;auto value=parse(read(w/"journal.json"));auto *journal=value.getAsObject();require(journal,"Invalid recovery journal");
  const auto id=text(*journal,"id");require(hex(id),"Invalid recovery job");const auto job=w/("job-"+id);require(fs::is_directory(install::files::NoFollow(job)),"Missing recovery directory");auto files=entries(*journal);
  if(present(w/"completed") && read(w/"completed",128)==id){
    for(const auto &e:files)require(regular(root/e.name)&&HashFile(root/e.name)==e.sha,"Completed update no longer matches");
  }else for(auto it=files.rbegin();it!=files.rend();++it){const auto &e=*it;parents(root,e.name);parents(job/"backup",e.name);
    const auto dest=root/e.name,backup=job/"backup"/e.name;
    if(e.existed && present(backup)){require(regular(backup)&&HashFile(backup)==e.old,"Recovery backup differs");
      if(present(dest)){const auto digest=HashFile(dest);require(regular(dest)&&(digest==e.sha || digest==e.old),"Changed update destination; recovery refused");}
      fs::rename(backup,dest);sync_directory(dest.parent_path());sync_directory(backup.parent_path());
    }else if(e.existed)require(regular(dest)&&HashFile(dest)==e.old,"Original file missing during recovery");
    else if(present(dest)){require(regular(dest)&&HashFile(dest)==e.sha,"Unrecognized new file; recovery refused");install::files::Remove(dest);}
  }
  install::files::Remove(w/"journal.json");
}
void Apply(const fs::path &root,const Progress &progress){Recover(root);const auto w=workspace(root);const auto id=pending(w);const auto job=w/("job-"+id);
  auto value=parse(read(job/"plan.json"));auto *plan=value.getAsObject();require(plan,"Missing update plan");auto files=entries(*plan);
  // Publish eboot last, after all assets. Stable libc plus startup recovery
  // allows the old or new launcher to restore an interrupted transaction.
  std::stable_sort(files.begin(),files.end(),[](const Entry &a,const Entry &b){return a.name!="eboot.bin" && b.name=="eboot.bin";});
  llvm::json::Array records;
  for(auto &e:files){parents(root,e.name);parents(job/"stage",e.name);parents(job/"backup",e.name);
    require(regular(job/"stage"/e.name)&&HashFile(job/"stage"/e.name)==e.sha,"Update stage changed before commit");
    if(present(root/e.name)){require(regular(root/e.name),"Refusing non-regular managed destination");e.existed=true;e.old=HashFile(root/e.name);}
    require(!present(job/"backup"/e.name),"Existing update backup must not be overwritten");
    records.push_back(Object{{"name",e.name},{"size",int64_t(e.size)},{"offset",int64_t(0)},{"sha256",e.sha},{"old_sha256",e.old}});
  }
  // Journal entries preserve payload offsets solely for one shared strict parser.
  uint64_t offset=0;for(auto &v:records){auto *o=v.getAsObject();(*o)["offset"]=int64_t(offset);offset+=uint64_t(*o->getInteger("size"));}
  atomic(w/"journal.json",encode(Object{{"id",id},{"files",std::move(records)}}));
  try{unsigned count=0;for(const auto &e:files){
      // Do not rewrite unchanged resources, especially the currently mapped
      // libc module. They remain covered by journal/commit verification.
      if(e.existed && e.old==e.sha){require(HashFile(root/e.name)==e.old,"Unchanged public file differs");
        if(e.name=="eboot.bin")(void)sims2::files::EnsureExecutable(root/e.name);
        if(progress)progress(++count,unsigned(files.size()));continue;}
      if(e.existed){require(HashFile(root/e.name)==e.old,"Public file changed during update");
        copy(root/e.name,job/"backup"/e.name,e.old,e.name=="eboot.bin");}
      else require(!present(root/e.name),"New update destination appeared concurrently");
      copy(job/"stage"/e.name,root/e.name,e.sha,e.name=="eboot.bin");if(progress)progress(++count,unsigned(files.size()));
    }
    atomic(w/"completed",id);Recover(root);install::files::Remove(w/"pending");
  }catch(...){Recover(root);throw;}
}
}
