// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "installer_pipeline.h"
#include "embedded.h"
#include "generator.h"
#include "native_packager.h"
#include "executable_file.h"
#include "game/disc_boot.h"
#include "game/install_filesystem.h"
#include "kernel/disc.h"
#include "core/log.h"
#include "platform.h"
#include "ps5platform/fp.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
namespace {
using Object = llvm::json::Object;
using Value = llvm::json::Value;
constexpr const char *cache_marker = "SIMS2-OWNER-BUILD-CACHE-1\n";
std::string read(const fs::path &path) {
  auto bytes = llvm::MemoryBuffer::getFile(path.string());
  if (!bytes) throw std::runtime_error("Cannot read "+path.string()+": "+bytes.getError().message());
  return (*bytes)->getBuffer().str();
}
std::string hash(llvm::StringRef bytes) {
  const auto value = llvm::SHA256::hash(llvm::ArrayRef(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size()));
  std::string result; for (auto c:value) {result += "0123456789abcdef"[c>>4];result += "0123456789abcdef"[c&15];} return result;
}
std::string file_hash(const fs::path &path) { return hash(read(path)); }
std::string text(const Object &object, llvm::StringRef key) {
  auto value = object.getString(key);
  if (!value) throw std::runtime_error("Missing manifest field: "+key.str());
  return value->str();
}
const Object &object(const Object &value, llvm::StringRef key) {
  auto *child = value.getObject(key);
  if (!child) throw std::runtime_error("Missing manifest object: "+key.str()); return *child;
}
bool hex64(const std::string &value) { return value.size()==64 && value.find_first_not_of("0123456789abcdef")==std::string::npos; }
std::string identity(const Object &value, llvm::StringRef key) {
  auto id=text(value,key);if(!hex64(id))throw std::runtime_error("Invalid manifest identity: "+key.str());return id;
}
bool safe(const std::string &name) {
  if(name.empty() || name.front()=='/' || name.find('\\')!=name.npos || name.find(':')!=name.npos)return false;
  for(const auto &part:fs::path(name))if(part=="." || part==".." || part.empty())return false;return true;
}
Value json(const fs::path &path) {
  auto parsed=llvm::json::parse(read(path));
  if(!parsed)throw std::runtime_error(llvm::toString(parsed.takeError()));return std::move(*parsed);
}
std::string serialized(const Value &value) {std::string out;llvm::raw_string_ostream stream(out);stream<<value;return out;}
void write_atomic(const fs::path &path, const std::string &bytes) {
  const auto temp=path.string()+".new-"+std::to_string(uint64_t(now_seconds()*1000000));
  FILE *file=std::fopen(temp.c_str(),"wb");if(!file)throw std::runtime_error("Cannot create "+temp);
  const bool wrote=std::fwrite(bytes.data(),1,bytes.size(),file)==bytes.size();const int closed=std::fclose(file);
  if(!wrote || closed || file_hash(temp)!=hash(bytes))throw std::runtime_error("Write verification failed: "+temp);
  if(::rename(temp.c_str(),path.c_str()))throw std::runtime_error("Cannot promote "+path.string());
}
void directory(const fs::path &path) {
  if(::mkdir(path.c_str(),0755) && errno!=EEXIST)throw std::runtime_error("Cannot create cache directory: "+path.string());
  const auto kind=sims2::install::files::NoFollow(path);
  if(!fs::is_directory(kind))throw std::runtime_error("Cache path is not a directory");
}
void log(const char *event, const std::string &detail) {
  std::fprintf(stderr,"[SIMS2][INSTALLER][INFO] t=%.6f thread=%llx event=%s %s\n",now_seconds(),
    (unsigned long long)(uintptr_t)pthread_self(),event,detail.c_str());std::fflush(stderr);
}
void diagnostic(const char *bytes,size_t count,void*) {
  // Keep complete diagnostics, including callbacks split in the middle of a line.
  std::fwrite(bytes,1,count,stderr);std::fflush(stderr);
}
bool valid_object(const fs::path &path) {
  auto bytes=llvm::MemoryBuffer::getFile(path.string());if(!bytes)return false;
  auto parsed=llvm::object::ObjectFile::createObjectFile((*bytes)->getMemBufferRef());
  if(!parsed){llvm::consumeError(parsed.takeError());return false;}
  auto *elf=llvm::dyn_cast<llvm::object::ELF64LEObjectFile>(parsed->get());
  return elf && elf->getELFFile().getHeader().e_type==llvm::ELF::ET_REL && elf->getELFFile().getHeader().e_machine==llvm::ELF::EM_X86_64;
}
void cancelled(Sims2Installer &state) {if(state.cancel.load())throw std::runtime_error("Preparation cancelled; completed files are retained for retry");}

void prepare(Sims2Installer &state,bool inspect) {
  const fs::path root="/app0",cache=root/".sims2-build";
  auto manifest_value=json(root/"installer-manifest.json");const auto *manifest=manifest_value.getAsObject();
  if(!manifest || manifest->getInteger("schema")!=1 || text(*manifest,"kind")!="ps5_owner_iso_installer")
    throw std::runtime_error("Invalid installer manifest");
  const auto gen_id=identity(*manifest,"generator_fingerprint"),compile_id=identity(*manifest,"compile_fingerprint"),native_id=identity(*manifest,"native_fingerprint");
  sims2::install::Source remembered;app::GameFiles game;std::string reason;bool extracted=false;
  const bool ready=sims2::install::Ready(root,game,reason);
  const bool cache_exists=::access((cache/"owner.txt").c_str(),F_OK)==0;
  if(inspect && (!ready || !cache_exists)){state.phase.store(Sims2InstallPhase::Missing);return;}
  directory(cache);
  if(cache_exists){if(read(cache/"owner.txt")!=cache_marker)throw std::runtime_error("Unknown build cache; move it aside before preparing");}
  else {
    // Refuse an existing directory containing unrelated files.
    if(fs::directory_iterator(cache)!=fs::directory_iterator{})throw std::runtime_error("Unrecognized build-cache contents");
    write_atomic(cache/"owner.txt",cache_marker);
  }
  Value receipt_value=Object{{"schema",1},{"objects",Object{}}};
  if(::access((cache/"cache.json").c_str(),F_OK)==0)receipt_value=json(cache/"cache.json");
  auto *receipt=receipt_value.getAsObject();if(!receipt || receipt->getInteger("schema")!=1)throw std::runtime_error("Invalid private cache receipt");
  const bool matching=receipt->getString("generator_id")==gen_id &&
      receipt->getString("compile_id")==compile_id && receipt->getString("native_id")==native_id &&
      receipt->getString("data_patch_fingerprint")==text(*manifest,"data_patch_fingerprint");
  if(inspect && ready) {
    const auto link_id=identity(*receipt,"link_id"),expected_sha=identity(*receipt,"game_sha256");
    const fs::path executable=root/("game-"+link_id+".bin");
    if(fs::is_regular_file(sims2::install::files::NoFollow(executable)) && file_hash(executable)==expected_sha) {
      const auto mode=sims2::files::EnsureExecutable(executable);
      log("executable_mode",mode.Describe());
      if(!sims2::install::PrepareCache(root,reason))throw std::runtime_error(reason);
      auto version=receipt->getString("launcher_version");
      // Legacy receipts can be associated with this launcher only if all game
      // payload identities match. A UI-only launcher update needs no recompile.
      if(!version && matching){(*receipt)["launcher_version"]=text(*manifest,"version");
        write_atomic(cache/"cache.json",serialized(receipt_value));version=receipt->getString("launcher_version");}
      {std::lock_guard<std::mutex> guard(state.mutex);state.executable=executable.string();
        state.game_launcher_version=version?version->str():"Earlier launcher";}
      state.has_game=true;state.game_update_available=!matching;
      log("cache_reused",std::string("status=PASS game_code_executed=0 update_available=")+(matching?"0":"1"));
      state.phase.store(Sims2InstallPhase::Ready);return;
    }
    throw std::runtime_error("Installed game executable failed verification; retry preparation to repair it. Saves were preserved.");
  }
  if(!ready) {
    fs::path iso;if(!sims2::disc::RootImage(root,iso,reason) || iso.empty())throw std::runtime_error(reason.empty()?"No game data detected. Put your Xbox USA ISO in this title folder.":reason);
    if(!sims2::install::RememberSource(root,iso,remembered,reason))throw std::runtime_error(reason);
    {std::lock_guard<std::mutex> guard(state.mutex);state.source_name=iso.filename().string();}
    state.phase.store(Sims2InstallPhase::Extracting);log("extract_begin",iso.filename().string());
    if(!sims2::install::FromIso(iso,root,[&](auto progress){state.percent.store(progress.total?unsigned(progress.done*100/progress.total):0);return !state.cancel.load();},reason))throw std::runtime_error(reason);
    if(!sims2::install::Ready(root,game,reason))throw std::runtime_error(reason);extracted=true;
    (*receipt)["data_patch_fingerprint"]=text(*manifest,"data_patch_fingerprint");
    write_atomic(cache/"cache.json",serialized(receipt_value));log("extract_end","status=PASS");
  }
  if(auto old=receipt->getString("data_patch_fingerprint");old && *old!=text(*manifest,"data_patch_fingerprint"))
    throw std::runtime_error("This update changes installed game data. Keep your data and ISO; a verified data migration is required.");
  cancelled(state);state.phase.store(Sims2InstallPhase::Verifying);
  const auto &files=object(*manifest,"files");unsigned verified=0;state.total.store(files.size());
  for(const auto &item:files) {
    const std::string name=item.first.str();if(!safe(name) || !item.second.getAsObject())throw std::runtime_error("Unsafe resource manifest entry");
    // The packaged default is inventoried for distribution, but the player may
    // edit live settings. Updating code must retain those personal choices.
    if(name=="settings.toml"){state.completed.store(++verified);continue;}
    if(file_hash(root/name)!=identity(*item.second.getAsObject(),"sha256"))throw std::runtime_error("Public resource differs: "+name);
    state.completed.store(++verified);cancelled(state);
  }
  const fs::path generated=cache/("generated-"+gen_id);const auto &expected=object(*manifest,"generated_files");
  bool generated_ok=::access(generated.c_str(),F_OK)==0;
  if(generated_ok)for(const auto &item:expected){auto value=item.second.getAsString();if(!value || !safe(item.first.str()) || file_hash(generated/item.first.str())!=*value)throw std::runtime_error("Generated source cache differs; it was not overwritten");}
  else {
    state.phase.store(Sims2InstallPhase::Generating);state.operation_began.store(now_seconds());
    const int code=sims2_generate_source("/app0/compiler-resources/generator/config/game.toml",(game.dir/"default.xbe").c_str(),generated.c_str(),"/app0/compiler-resources/generator/xdk",reason);
    if(code)throw std::runtime_error(reason);
    for(const auto &item:expected){auto value=item.second.getAsString();if(!value || !safe(item.first.str()) || file_hash(generated/item.first.str())!=*value)throw std::runtime_error("Generated source differs from the reviewed revision");}
  }
  log("generation_verified","status=PASS files="+std::to_string(expected.size()));cancelled(state);
  (*receipt)["generator_id"]=gen_id;
  const fs::path objects=cache/("objects-"+compile_id);directory(objects);
  auto *records=receipt->getObject("objects");if(!records){(*receipt)["objects"]=Object{};records=receipt->getObject("objects");}
  if(receipt->getString("compile_id")!=compile_id){*records=Object{};(*receipt)["compile_id"]=compile_id;}
  auto *sources=manifest->getArray("cpp_files");auto *planned=manifest->getArray("compile_argv");
  if(!sources || sources->size()!=65 || !planned)throw std::runtime_error("Invalid compilation plan");
  state.phase.store(Sims2InstallPhase::Compiling);state.completed.store(0);state.total.store(sources->size());
  sims2::compiler::NativePackageRequest native;
  native.kit_directory="/app0/native-title-kit";native.object_directory=objects.string();
  std::string object_identity;
  for(const auto &unit:*sources) {
    auto value=unit.getAsString();if(!value || !safe(value->str()))throw std::runtime_error("Invalid compilation unit");
    const std::string name=value->str();const std::string object_name=name.substr(0,name.size()-4)+".o";const fs::path target=objects/object_name;
    const std::string unit_id=hash(text(expected,name)+text(expected,"x86_recomp_shared.h")+compile_id);
    auto *old=records->getObject(name);std::string digest;
    if(old && old->getString("unit_id")==unit_id && ::access(target.c_str(),F_OK)==0 && valid_object(target) && file_hash(target)==text(*old,"sha256"))digest=text(*old,"sha256");
    else {
      cancelled(state);const std::string temp=target.string()+".building-"+std::to_string(uint64_t(now_seconds()*1000000));
      std::vector<std::string> arguments;
      for(const auto &argument:*planned){auto item=argument.getAsString();if(!item)throw std::runtime_error("Invalid compiler argument");std::string arg=item->str();
        for(const auto &replacement:{std::pair<std::string,std::string>{"{GENERATED}",generated.string()},{"{SOURCE}",(generated/name).string()},{"{OBJECT}",temp}}){size_t at=0;while((at=arg.find(replacement.first,at))!=arg.npos){arg.replace(at,replacement.first.size(),replacement.second);at+=replacement.second.size();}}arguments.push_back(std::move(arg));}
      std::vector<const char*> argv;for(const auto &arg:arguments)argv.push_back(arg.c_str());
      state.operation_began.store(now_seconds());log("compile_begin","file="+name);
      const int code=sims2_compiler_compile_driver(generated.c_str(),argv.data(),argv.size(),diagnostic,nullptr);
      log("compile_end","file="+name+" result="+std::to_string(code));if(code){state.restart_required=code==3;throw std::runtime_error("Compilation failed for "+name+(code==3?"; close and restart the application":"; see klog"));}
      if(!valid_object(temp))throw std::runtime_error("Compiler produced an invalid object");digest=file_hash(temp);
      if(::access(target.c_str(),F_OK)==0 && ::rename(target.c_str(),(target.string()+".previous-"+std::to_string(uint64_t(now_seconds()*1000000))).c_str()))throw std::runtime_error("Cannot preserve old cache object");
      if(::rename(temp.c_str(),target.c_str()))throw std::runtime_error("Cannot promote compiled object");
      (*records)[name]=Object{{"unit_id",unit_id},{"sha256",digest}};write_atomic(cache/"cache.json",serialized(receipt_value));
    }
    native.objects.push_back({object_name,digest});object_identity+=object_name+digest;state.completed.fetch_add(1);
  }
  const std::string link_id=hash(native_id+object_identity),filename="game-"+link_id+".bin";const fs::path game_path=root/filename;
  bool linked=receipt->getString("link_id")==link_id && ::access(game_path.c_str(),F_OK)==0 && file_hash(game_path)==text(*receipt,"game_sha256");
  if(!linked) {
    state.phase.store(Sims2InstallPhase::Linking);state.operation_began.store(now_seconds());cancelled(state);
    const auto output=cache/("link-"+link_id+"-"+std::to_string(uint64_t(now_seconds()*1000000)));directory(output);native.output_directory=output.string();
    auto *inputs=manifest->getArray("native_inputs");if(!inputs)throw std::runtime_error("Missing native inputs");
    for(const auto &item:*inputs){const auto *entry=item.getAsObject();if(!entry)throw std::runtime_error("Invalid native input");native.public_inputs.push_back({text(*entry,"name"),identity(*entry,"sha256")});}
    for(auto pair:{std::pair<const char*,std::vector<std::string>*>{"link_argv",&native.link_arguments},{"stub_files",&native.stub_files},{"allowed_imports",&native.allowed_imports}}){auto *list=manifest->getArray(pair.first);if(!list)throw std::runtime_error("Missing native recipe");for(const auto &item:*list){auto s=item.getAsString();if(!s)throw std::runtime_error("Invalid native recipe item");pair.second->push_back(s->str());}}
    sims2::compiler::NativePackageResult result;const int code=sims2::compiler::CreateNativeTitle(native,result,diagnostic,nullptr);
    if(code){state.restart_required=code==3;throw std::runtime_error("Native packaging failed; see klog");}
    if(::access(game_path.c_str(),F_OK)==0){if(file_hash(game_path)!=result.self_sha256)throw std::runtime_error("Existing private game differs; it was not overwritten");}
    else if(::rename((output/"private-eboot.bin").c_str(),game_path.c_str()))throw std::runtime_error("Cannot promote private game executable");
    (*receipt)["link_id"]=link_id;(*receipt)["game_sha256"]=result.self_sha256;
  }
  (*receipt)["native_id"]=native_id;
  const auto mode=sims2::files::EnsureExecutable(game_path);
  log("executable_mode",mode.Describe());
  (*receipt)["launcher_version"]=text(*manifest,"version");
  if(!receipt->getString("data_patch_fingerprint"))(*receipt)["data_patch_fingerprint"]=text(*manifest,"data_patch_fingerprint");
  if(!sims2::install::PrepareCache(root,reason))throw std::runtime_error(reason);
  write_atomic(cache/"cache.json",serialized(receipt_value));
  {std::lock_guard<std::mutex> guard(state.mutex);state.executable=game_path.string();state.source=remembered;
    state.game_launcher_version=text(*manifest,"version");}
  state.has_game=true;state.game_update_available=false;
  log("game_prepared","status=PASS filename="+filename+" game_code_executed=0");
  state.phase.store(extracted?Sims2InstallPhase::DeleteQuestion:Sims2InstallPhase::Ready);
}
struct StartRequest {Sims2Installer *state;bool inspect;};
void *worker(void *opaque) {
  const auto request=*static_cast<StartRequest*>(opaque);delete static_cast<StartRequest*>(opaque);auto &state=*request.state;
  ps5_fp_ieee();try{prepare(state,request.inspect);}catch(const std::exception &e){std::lock_guard<std::mutex> guard(state.mutex);state.error=e.what();state.phase.store(state.cancel.load()?Sims2InstallPhase::Cancelled:Sims2InstallPhase::Error);log("preparation_error",state.error);}
  state.done.store(true,std::memory_order_release);return nullptr;
}
}

bool Sims2Installer::Start(bool inspect) {
  if(started || restart_required)return false;cancel=false;done=false;completed=0;percent=0;began=now_seconds();operation_began=began.load();phase=Sims2InstallPhase::Checking;
  {std::lock_guard<std::mutex> guard(mutex);error.clear();}
  pthread_attr_t attributes;int code=pthread_attr_init(&attributes);if(code)return false;
  code=pthread_attr_setstacksize(&attributes,16*1024*1024);auto *request=new StartRequest{this,inspect};
  if(!code)code=pthread_create(&worker,&attributes,::worker,request);pthread_attr_destroy(&attributes);
  if(code){delete request;return false;}started=true;return true;
}
void Sims2Installer::Join(){if(started && done.load(std::memory_order_acquire)){pthread_join(worker,nullptr);started=false;}}
bool Sims2Installer::ResolveSource(bool remove) {
  if(phase.load()!=Sims2InstallPhase::DeleteQuestion || started)return false;
  if(remove){std::string reason;if(!sims2::install::DeleteSource(source,reason)){std::lock_guard<std::mutex> guard(mutex);error=reason;return false;}}
  phase.store(Sims2InstallPhase::Ready);return true;
}
std::string Sims2Installer::Error(){std::lock_guard<std::mutex> guard(mutex);return error;}
std::string Sims2Installer::Executable(){std::lock_guard<std::mutex> guard(mutex);return phase.load()==Sims2InstallPhase::Ready?executable:std::string{};}
std::string Sims2Installer::GameLauncherVersion(){std::lock_guard<std::mutex> guard(mutex);return game_launcher_version;}

// The extracted-data library logs through the foundation's existing stderr to
// klog route. No guest kernel, renderer or second network transport is linked.
namespace logx {
void Init(const std::string&,const std::string&,bool){}
void Flush(){std::fflush(stderr);}
const std::string& Path(){static const std::string empty;return empty;}
void Write(Level level,const char *format,...){std::fprintf(stderr,"[SIMS2][INSTALLER][%s] ",level==Level::Error?"ERROR":"INFO");va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fprintf(stderr,"\n");std::fflush(stderr);}
}
