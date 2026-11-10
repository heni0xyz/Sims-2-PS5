// SPDX-License-Identifier: GPL-3.0-only
// OpenAI Codex for szampan. Source ISO is read-only; promotion is transactional.
#include "game/install.h"
#include "game/install_filesystem.h"
#include "game/disc_boot.h"
#include "kernel/disc.h"
#include "kernel/crypto.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <fstream>
#include <set>
#include <stdexcept>
#include <cstdio>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
namespace sims2::install {
namespace fs=std::filesystem;
namespace {
constexpr const char* token="SIMS2-INSTALL-STAGING-1\n";
constexpr const char* marker=".sims2-install-owner";
constexpr const char* receipt=".sims2-installed-files";
constexpr const char* receipt_magic="SIMS2-INSTALL-RECEIPT-1";
std::string Fold(std::string value){for(auto& c:value)c=char(std::tolower(static_cast<unsigned char>(c)));return value;}
struct Sha {
    alignas(8) uint8_t state[128]{};
    Sha(){crypto::ShaInit(state);}
    void Add(std::span<const uint8_t> bytes){crypto::ShaUpdate(state,bytes.data(),bytes.size());}
    std::string End(){uint8_t digest[20];crypto::ShaFinal(state,digest);std::string out;for(auto b:digest){out+="0123456789abcdef"[b>>4];out+="0123456789abcdef"[b&15];}return out;}
};
std::string Hash(std::span<const uint8_t> bytes){Sha sha;sha.Add(bytes);return sha.End();}
bool Safe(std::string_view name){
    if(name.empty() || name.size()>240 || name.front()=='/' || name.back()=='/')return false;
    size_t from=0;
    while(from<name.size()){
        auto end=name.find('/',from);if(end==name.npos)end=name.size();
        auto part=name.substr(from,end-from);
        if(part.empty() || part=="." || part==".." || part.back()=='.' || part.back()==' ')return false;
        for(auto c:part)if(c<32 || c>=127 || std::string_view("\\:*?\"<>|").find(c)!=std::string_view::npos)return false;
        const auto stem=Fold(std::string(part.substr(0,part.find('.'))));
        if(stem=="con" || stem=="prn" || stem=="aux" || stem=="nul" ||
           (stem.size()==4 && (stem.starts_with("com") || stem.starts_with("lpt")) && std::isdigit(stem[3])))return false;
        from=end+1;
    }
    return true;
}
void Mode(const fs::path& path,bool directory){
    fs::permissions(path,directory?fs::perms::all:fs::perms(0666),fs::perm_options::replace);
}
void Durable(const fs::path& path){
    FILE* file=std::fopen(path.string().c_str(),"rb+");if(!file)throw std::runtime_error("Cannot flush installed game data");
#if defined(_WIN32)
    const int status=_commit(_fileno(file));
#else
    const int status=fsync(fileno(file));
#endif
    const int closed=std::fclose(file);
    if(status || closed)throw std::runtime_error("Flushing installed game data failed");
}
void Parents(const fs::path& stage,const fs::path& relative){
    fs::path path=stage;
    for(const auto& part:relative.parent_path()){
        path/=part;
        if(!fs::exists(path)) {fs::create_directory(path);Mode(path,true);}
        if(fs::is_symlink(files::NoFollow(path)) || !fs::is_directory(path))throw std::runtime_error("Unsafe install folder");
    }
}
bool Recover(const fs::path& stage,const std::set<std::string>& allowed,std::string& error){
    if(!fs::exists(stage))return true;
    if(fs::is_symlink(files::NoFollow(stage)) || !fs::is_directory(stage)){error="Install staging path is not a folder";return false;}
    const auto ownership=stage/marker;
    if(!fs::is_regular_file(files::NoFollow(ownership)) || fs::file_size(ownership)!=std::strlen(token)){error="Invalid install staging ownership marker";return false;}
    std::ifstream owner(ownership,std::ios::binary);std::string contents((std::istreambuf_iterator<char>(owner)),{});owner.close();
    if(contents!=token){error="Existing install staging folder is not owned by this installer";return false;}
    // Inspect the entire tree before removing anything. Never follow a link or
    // remove an unrecognized user's file, even inside an abandoned staging dir.
    std::vector<std::pair<fs::path,bool>> removal;
    for(const auto& item:fs::recursive_directory_iterator(stage)){
        const auto kind=files::NoFollow(item.path());
        const auto name=item.path().lexically_relative(stage).generic_string();
        if(fs::is_symlink(kind) || (!fs::is_directory(kind) && !fs::is_regular_file(kind)) || !allowed.contains(Fold(name))){error="Unknown files in install staging folder; move it aside before retrying";return false;}
        removal.emplace_back(item.path(),fs::is_directory(kind));
    }
    if(stage.filename()!=".sims2-installing" || stage.parent_path().empty())throw std::runtime_error("Invalid staging cleanup target");
    std::sort(removal.begin(),removal.end(),[](const auto& a,const auto& b){return a.first.native().size()>b.first.native().size();});
    for(const auto& [path,directory]:removal)files::Remove(path,directory);
    files::Remove(stage,true);return true;
}
bool NotifyAt(const Notify& notify,Stage stage,uint64_t done,uint64_t total,std::string& error){
    if(notify && !notify({stage,done,total})){error="Extraction cancelled";return false;}return true;
}
fs::path Child(const fs::path& parent,const std::string& name){
    fs::path found;
    for(const auto& entry:fs::directory_iterator(parent))if(Fold(entry.path().filename().string())==Fold(name)){
        if(!found.empty())throw std::runtime_error("Ambiguous case-insensitive game path");found=entry.path();
    }
    return found;
}
bool ReadFile(const fs::path& path,std::vector<uint8_t>& bytes,size_t limit){
    if(path.empty() || fs::is_symlink(files::NoFollow(path)) || !fs::is_regular_file(path))return false;
    const auto size=fs::file_size(path);if(!size || size>limit)return false;
    bytes.resize(size);std::ifstream stream(path,std::ios::binary);return bool(stream.read(reinterpret_cast<char*>(bytes.data()),bytes.size()));
}
}
bool RemovePlaceholder(const fs::path& root,std::string& error){
    try{
        const auto assets=Child(root,"assets");if(assets.empty())return true;
        if(!fs::is_directory(files::NoFollow(assets)))throw std::runtime_error("Existing assets path is not a normal folder");
        const auto marker_path=Child(assets,"README.txt");std::vector<uint8_t> bytes;
        if(!ReadFile(marker_path,bytes,64))throw std::runtime_error("Existing assets are preserved; move incomplete data aside before retrying");
        const std::string value(bytes.begin(),bytes.end());
        if(value!="SIMS2-INSTALLER-PLACEHOLDER-1\n" && value!="SIMS2-INSTALLER-PLACEHOLDER-1\r\n")
            throw std::runtime_error("Unrecognized assets README; it was not removed");
        size_t count=0;for(const auto& entry:fs::directory_iterator(assets)){(void)entry;++count;}
        if(count!=1)throw std::runtime_error("Existing assets contain other files; they will not be replaced");
        files::Remove(marker_path);files::Remove(assets,true);return true;
    }catch(const std::exception& exception){error=exception.what();return false;}
}
bool Extract(const fs::path& root,const std::vector<File>& files,const Read& read,const Finish& finish,const Notify& notify,std::string& error){
    error.clear();
    try{
        if(!root.is_absolute() || !fs::is_directory(root) || files.empty() || files.size()>8192 || !read)throw std::runtime_error("Invalid install source or destination");
        const auto destination=root/"assets",stage=root/".sims2-installing";
        if(!Child(root,"assets").empty() || fs::is_symlink(files::NoFollow(destination)))throw std::runtime_error("Assets folder already exists; it will not be overwritten");
        uint64_t total=0;std::set<std::string> allowed{marker,receipt};
        for(const auto& file:files){
            if(!Safe(file.name) || file.bytes>(8ull<<30) || !allowed.insert(Fold(file.name)).second)throw std::runtime_error("Unsafe, duplicate or oversized ISO member");
            for(auto path=fs::path(file.name).parent_path();!path.empty();path=path.parent_path())allowed.insert(Fold(path.generic_string()));
            total+=file.bytes;if(total>(32ull<<30))throw std::runtime_error("ISO game data exceeds the install limit");
        }
        if(!total)throw std::runtime_error("ISO contains no game data");
        if(!Recover(stage,allowed,error))return false;
        std::error_code space_error;auto space=fs::space(root,space_error);
        if(!space_error && space.available<total+(64ull<<20))throw std::runtime_error("Not enough free storage for extracted game data");
        fs::create_directory(stage);Mode(stage,true);
        {std::ofstream owner(stage/marker,std::ios::binary);owner<<token;owner.close();if(!owner)throw std::runtime_error("Cannot create install staging marker");Mode(stage/marker,false);}
        std::vector<uint8_t> buffer(1u<<20);uint64_t done=0;
        for(size_t index=0;index<files.size();++index){
            const auto& file=files[index];const fs::path relative=file.name;Parents(stage,relative);
            const auto target=stage/relative;std::ofstream output(target,std::ios::binary);
            if(!output)throw std::runtime_error("Cannot create extracted game file");
            Sha source;
            for(uint64_t position=0;position<file.bytes;){
                if(!NotifyAt(notify,Stage::Copy,done,total,error))return false;
                const auto count=size_t(std::min<uint64_t>(buffer.size(),file.bytes-position));
                std::span<uint8_t> chunk(buffer.data(),count);
                if(!read(index,position,chunk))throw std::runtime_error("ISO read failed or image is truncated");
                source.Add(chunk);output.write(reinterpret_cast<const char*>(chunk.data()),count);
                if(!output)throw std::runtime_error("Game data write failed; check free storage");
                done+=count;position+=count;
            }
            output.close();if(!output)throw std::runtime_error("Closing extracted game file failed");Mode(target,false);
            Durable(target);
            if(fs::file_size(target)!=file.bytes)throw std::runtime_error("Extracted game file has the wrong size");
            Sha copied;std::ifstream input(target,std::ios::binary);
            for(uint64_t position=0;position<file.bytes;){
                if(!NotifyAt(notify,Stage::Verify,done,total,error))return false;
                const auto count=size_t(std::min<uint64_t>(buffer.size(),file.bytes-position));
                if(!input.read(reinterpret_cast<char*>(buffer.data()),count))throw std::runtime_error("Extracted game file readback failed");
                copied.Add(std::span(buffer.data(),count));position+=count;
            }
            if(source.End()!=copied.End())throw std::runtime_error("Extracted game file checksum mismatch");
        }
        if(!NotifyAt(notify,Stage::Patch,done,total,error))return false;
        if(finish && !finish(stage,error))return false;
        {
            std::ofstream inventory(stage/receipt,std::ios::binary);
            inventory<<receipt_magic<<'\n'<<files.size()<<'\n';
            for(const auto& file:files)inventory<<file.bytes<<'\t'<<file.name<<'\n';
            inventory.close();if(!inventory)throw std::runtime_error("Cannot write installed data receipt");
            Mode(stage/receipt,false);Durable(stage/receipt);
        }
        if(!Child(root,"assets").empty())throw std::runtime_error("Assets appeared during installation; refusing to overwrite");
        fs::rename(stage,destination);
        // Completion reports only after all readbacks/patches and promotion.
        if(notify)notify({Stage::Complete,total,total});return true;
    }catch(const std::exception& exception){error=exception.what();return false;}
}
static bool ReadyDirectory(const fs::path& assets,app::GameFiles& game,std::string& error,bool check_receipt=true){
    error.clear();
    try{
        if(assets.empty() || !fs::is_directory(assets))return false;
        std::vector<uint8_t> xbe;
        if(!ReadFile(Child(assets,"default.xbe"),xbe,256u<<20) || Hash(xbe)!="16f9e41ce26c315a758f25d43ba1fa9d3809d1ad"){error="Assets contain a missing or unsupported USA executable";return false;}
        for(const auto* name:{"default.NGH","sims2.bin"}){
            auto file=Child(assets,name);if(file.empty() || !fs::is_regular_file(file) || !fs::file_size(file))throw std::runtime_error("Incomplete assets folder");
        }
        const auto data=Child(assets,"DATA"),movies=Child(assets,"movies");
        if(data.empty() || movies.empty())throw std::runtime_error("Incomplete assets folder");
        for(const auto* name:{"index.ind","datasets.arc","textures.arc","shaders.arc","quickdat.arc"}){
            auto file=Child(data,name);if(file.empty() || !fs::is_regular_file(file) || !fs::file_size(file))throw std::runtime_error("Incomplete DATA folder");
        }
        auto movie=Child(movies,"ea_games.xmv");if(movie.empty() || !fs::file_size(movie))throw std::runtime_error("Incomplete movies folder");
        if(check_receipt && fs::exists(assets/marker) && !fs::exists(assets/receipt))throw std::runtime_error("Installed data receipt is missing");
        if(check_receipt && fs::exists(assets/receipt)){
            if(!fs::is_regular_file(files::NoFollow(assets/receipt)) || fs::file_size(assets/receipt)>(1u<<20))throw std::runtime_error("Invalid installed data receipt");
            std::ifstream inventory(assets/receipt,std::ios::binary);std::string magic,line;
            if(!std::getline(inventory,magic) || magic!=receipt_magic || !std::getline(inventory,line) || line.empty() || line.size()>5)throw std::runtime_error("Invalid installed data receipt header");
            const auto count=std::stoul(line);if(!count || count>8192)throw std::runtime_error("Invalid installed file count");
            std::set<std::string> seen;
            for(unsigned i=0;i<count;++i){
                if(!std::getline(inventory,line) || line.size()>512)throw std::runtime_error("Truncated installed data receipt");
                const auto tab=line.find('\t');if(tab==line.npos || tab>16 || !tab)throw std::runtime_error("Invalid installed file record");
                const auto name=line.substr(tab+1);size_t used=0;const auto size=std::stoull(line.substr(0,tab),&used);
                if(used!=tab || !Safe(name) || !seen.insert(Fold(name)).second)throw std::runtime_error("Invalid installed file path");
                const auto path=assets/fs::path(name);
                if(!fs::is_regular_file(files::NoFollow(path)) || fs::file_size(path)!=size)throw std::runtime_error("Installed game file is missing or truncated");
            }
            if(std::getline(inventory,line))throw std::runtime_error("Unexpected installed receipt content");
            std::vector<uint8_t> notices;
            if(!ReadFile(Child(data,"quickdat.arc"),notices,32u<<20) || Hash(notices)!="35a76c78a1b20ccffa3c75431550ebd5e6a5dc67")throw std::runtime_error("Installed UI notice patch is missing or changed");
        }
        game={assets,std::move(xbe)};return true;
    }catch(const std::exception& exception){error=exception.what();return false;}
}
bool Ready(const fs::path& root,app::GameFiles& game,std::string& error){
    try{return ReadyDirectory(Child(root,"assets"),game,error);}
    catch(const std::exception& exception){error=exception.what();return false;}
}
static bool PrepareCacheFor(const fs::path& root,const fs::path& assets,std::string& error){
    try{
        const auto data=Child(assets,"DATA");std::vector<uint8_t> notices;
        if(!ReadFile(Child(data,"quickdat.arc"),notices,32u<<20) || Hash(notices)!="35a76c78a1b20ccffa3c75431550ebd5e6a5dc67")return true;
        const auto hdd=root/"hdd";
        if(!fs::exists(hdd)){fs::create_directory(hdd);Mode(hdd,true);}
        if(!fs::is_directory(files::NoFollow(hdd)))throw std::runtime_error("HDD path is not a normal folder");
        const auto tag=hdd/".sims2-notice-cache-v1";
        if(fs::exists(tag)){
            if(!fs::is_regular_file(files::NoFollow(tag)) || fs::file_size(tag)!=std::strlen("SIMS2-NOTICE-CACHE-1\n"))throw std::runtime_error("Invalid notice-cache version marker");
            std::ifstream input(tag);std::string value((std::istreambuf_iterator<char>(input)),{});
            if(value!="SIMS2-NOTICE-CACHE-1\n")throw std::runtime_error("Unknown notice-cache version marker");return true;
        }
        const auto cache=Child(hdd,"Z"),backup=hdd/"Z.before-notice-install";
        if(!cache.empty()){
            if(!fs::is_directory(files::NoFollow(cache)))throw std::runtime_error("Xbox Z cache is not a normal folder");
            if(fs::exists(backup)){
                if(!fs::is_empty(cache))throw std::runtime_error("Previous cache backup exists; it will not be overwritten");
            }else fs::rename(cache,backup);
        }
        if(Child(hdd,"Z").empty()){fs::create_directory(hdd/"Z");Mode(hdd/"Z",true);}
        std::ofstream output(tag,std::ios::binary);output<<"SIMS2-NOTICE-CACHE-1\n";output.close();
        if(!output)throw std::runtime_error("Cannot record notice-cache version");Mode(tag,false);Durable(tag);return true;
    }catch(const std::exception& exception){error=exception.what();return false;}
}
bool PrepareCache(const fs::path& root,std::string& error){
    try{return PrepareCacheFor(root,Child(root,"assets"),error);}
    catch(const std::exception& exception){error=exception.what();return false;}
}
bool RememberSource(const fs::path& root,const fs::path& iso,Source& result,std::string& error){
    try{
        if(!root.is_absolute() || !iso.is_absolute() || iso.parent_path().lexically_normal()!=root.lexically_normal() ||
           Fold(iso.extension().string())!=".iso" || !fs::is_regular_file(files::NoFollow(iso)))throw std::runtime_error("Source ISO must be a regular file in the title folder");
        result={root,iso,fs::file_size(iso),fs::last_write_time(iso)};return true;
    }catch(const std::exception& exception){error=exception.what();return false;}
}
bool DeleteSource(const Source& original,std::string& error){
    Source current;app::GameFiles game;
    if(!Ready(original.root,game,error)){error="Installed game data is not verified; source ISO kept";return false;}
    if(!RememberSource(original.root,original.path,current,error))return false;
    if(current.bytes!=original.bytes || current.modified!=original.modified){error="Source ISO changed since installation; it was kept";return false;}
    try{if(!files::Remove(original.path)){error="Source ISO could not be removed";return false;}return true;}
    catch(const std::exception& exception){error=exception.what();return false;}
}
bool FromIso(const fs::path& iso,const fs::path& root,const Notify& notify,std::string& error){
    kernel::Disc image;std::vector<uint8_t> xbe;
    if(!image.Open(iso,error) || !sims2::disc::Validate(image,xbe,error))return false;
    std::vector<File> files;std::vector<const kernel::Disc::Entry*> entries;
    const auto walk=[&](auto&& self,const kernel::Disc::Entry& entry,const std::string& parent)->void{
        const auto name=parent+entry.name;
        if(!Safe(name))throw std::runtime_error("Unsafe ISO member name");
        if(entry.dir){for(const auto& child:entry.children)self(self,child,name+'/');}
        else{files.push_back({name,entry.size});entries.push_back(&entry);}
    };
    try{for(const auto& entry:image.Find(std::vector<std::string>{})->children)walk(walk,entry,"");}
    catch(const std::exception& exception){error=exception.what();return false;}
    if(!RemovePlaceholder(root,error))return false;
    return Extract(root,files,[&](size_t index,uint64_t offset,std::span<uint8_t> out){return image.Read(*entries[index],offset,out.data(),uint32_t(out.size()))==out.size();},
        [&](const fs::path& stage,std::string& reason){
            const auto data=Child(stage,"DATA"),path=Child(data,"quickdat.arc");std::vector<uint8_t> archive;
            if(!ReadFile(path,archive,32u<<20) || !PatchNotices(archive,reason)){if(reason.empty())reason="Cannot read extracted UI notices";return false;}
            std::ofstream stream(path,std::ios::binary|std::ios::trunc);stream.write(reinterpret_cast<char*>(archive.data()),archive.size());stream.close();
            if(!stream){reason="Cannot write UI notice patch";return false;}
            Durable(path);
            std::vector<uint8_t> verify;if(!ReadFile(path,verify,32u<<20) || verify!=archive){reason="UI notice patch readback failed";return false;}
            app::GameFiles checked;return ReadyDirectory(stage,checked,reason,false) && PrepareCacheFor(root,stage,reason);
        },notify,error);
}
}
