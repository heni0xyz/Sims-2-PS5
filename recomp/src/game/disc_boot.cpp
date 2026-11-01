// SPDX-License-Identifier: GPL-3.0-only
// Title policy only. XDVDFS parsing/streaming remains the MIT SDK's kernel::Disc.
#include "game/disc_boot.h"
#include "kernel/disc.h"
#include "kernel/crypto.h"
#include "core/log.h"
#include <algorithm>
#include <cctype>
namespace sims2::disc {
bool Validate(const kernel::Disc& image,std::vector<uint8_t>& xbe,std::string& error) {
    const auto* disc=&image;
    const auto* entry=disc->Find("default.xbe");
    if(!entry || entry->dir || !entry->size || entry->size>256*1024*1024) {error="Missing or invalid default.xbe in ISO";return false;}
    if(!disc->Load("default.xbe",xbe)) {error="Cannot read default.xbe from ISO";return false;}
    alignas(8) uint8_t context[128]{},digest[20]{};
    crypto::ShaInit(context);crypto::ShaUpdate(context,xbe.data(),xbe.size());crypto::ShaFinal(context,digest);
    std::string hash;for(auto byte:digest){hash+="0123456789abcdef"[byte>>4];hash+="0123456789abcdef"[byte&15];}
    if(hash!="16f9e41ce26c315a758f25d43ba1fa9d3809d1ad") {error="Unsupported ISO executable SHA-1: "+hash;return false;}
    // Essential member presence catches a truncated directory-only dump early.
    for(const auto* member:{"default.NGH","sims2.bin","DATA\\index.ind","DATA\\datasets.arc","DATA\\textures.arc","DATA\\shaders.arc","movies\\ea_games.xmv"}) {
        const auto* file=disc->Find(member);
        if(!file || file->dir || !file->size) {error=std::string("Missing ISO member: ")+member;return false;}
    }
    error.clear();
    return true;
}
bool Open(app::GameFiles& game,const std::filesystem::path& path,std::string& error) {
    auto image=std::make_unique<kernel::Disc>();
    std::vector<uint8_t> xbe;
    if(!image->Open(path,error) || !Validate(*image,xbe,error))return false;
    game={path.parent_path(),std::move(xbe)};kernel::MountDisc(std::move(image));error.clear();
    LOGR("[SIMS2][DISC][INFO] Mounted owner ISO read-only; xbe_sha1=16f9e41ce26c315a758f25d43ba1fa9d3809d1ad; streamed XDVDFS, no extraction");
    return true;
}
bool RootImage(const std::filesystem::path& root,std::filesystem::path& image,std::string& error) {
    image.clear();error.clear();std::error_code ec;
    std::filesystem::directory_iterator it(root,ec),end;
    if(ec){error="Cannot inspect title folder: "+ec.message();return false;}
    for(;it!=end;it.increment(ec)) {
        if(ec){error="Cannot enumerate title folder: "+ec.message();return false;}
        auto ext=it->path().extension().string();
        std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(ext!=".iso")continue;
        if(!it->is_regular_file(ec) || ec){error="ISO entry is not a readable regular file";return false;}
        if(!image.empty()){error="Multiple root ISO files; keep exactly one owner game image";return false;}
        image=it->path();
    }
    if(ec){error="Cannot enumerate title folder: "+ec.message();return false;}
    return true;
}
}
