#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cerrno>
#include <filesystem>
#include <string>
#include <vector>

namespace kernel::hostfs {
inline std::string Fold(std::string value,bool upper) {
    for(char& c:value)c=char(upper?std::toupper(uint8_t(c)):std::tolower(uint8_t(c)));
    return value;
}
// Xbox names are case-insensitive. Prefer exact names; probe the conventional
// lower/upper aliases before enumeration, which some virtual/execute-only
// directory mounts cannot provide although known files are stat/read accessible.
// Mixed-case directories still use the original case-insensitive enumeration.
inline std::filesystem::path MatchCase(const std::filesystem::path& dir,const std::vector<std::string>& parts) {
    namespace fs=std::filesystem;
    fs::path cur=dir;
    for(const auto& part:parts) {
        if(part.empty() || part==".")continue;
        if(part=="..") {cur=cur.parent_path();continue;}
        const auto exact=cur/part;std::error_code ec;
        if(fs::exists(exact,ec)) {cur=exact;continue;}
        const auto lower=cur/Fold(part,false);
        if(lower!=exact && fs::exists(lower,ec)) {cur=lower;continue;}
        const auto upper=cur/Fold(part,true);
        if(upper!=exact && upper!=lower && fs::exists(upper,ec)) {cur=upper;continue;}
        bool found=false;
        for(const auto& entry:fs::directory_iterator(cur,ec)) {
            if(Fold(entry.path().filename().string(),true)==Fold(part,true)) {cur=entry.path();found=true;break;}
        }
        if(!found)cur=exact;
    }
    return cur;
}
// Existing-file modes never create a second differently-cased file. Retain the
// original r+b -> rb fallback; retry aliases only if the original open failed.
// Some virtual filesystems expose inconsistent stat/open case handling.
template<class Open> FILE* OpenExisting(std::filesystem::path& path,Open open) {
    auto attempt=[&](const std::filesystem::path& candidate) {
        const auto name=candidate.string();FILE* fp=open(name.c_str(),"r+b");
        return fp?fp:open(name.c_str(),"rb");
    };
    if(FILE* fp=attempt(path))return fp;
    const int original_error=errno;
    const auto original=path;
    for(bool upper:{false,true}) {
        const auto alias=original.parent_path()/Fold(original.filename().string(),upper);
        if(alias==original)continue;
        if(FILE* fp=attempt(alias)) {path=alias;return fp;}
    }
    errno=original_error;return nullptr;
}
inline FILE* OpenExisting(std::filesystem::path& path) {
    return OpenExisting(path,[](const char* name,const char* mode){return std::fopen(name,mode);});
}
}
