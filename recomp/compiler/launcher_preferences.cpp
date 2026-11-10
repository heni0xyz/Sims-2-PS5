// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "launcher_preferences.h"
#include <atomic>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <filesystem>
#endif
namespace {
constexpr const char *on="SIMS2-LAUNCHER-PREFS-1\nmusic=1\n";
constexpr const char *off="SIMS2-LAUNCHER-PREFS-1\nmusic=0\n";
int open_file(const char *path,bool create) {
#if defined(_WIN32)
    auto wide=std::filesystem::u8path(path);
    HANDLE file=CreateFileW(wide.c_str(),create?GENERIC_WRITE:GENERIC_READ,FILE_SHARE_READ,nullptr,
        create?CREATE_NEW:OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(file==INVALID_HANDLE_VALUE){errno=GetLastError()==ERROR_FILE_NOT_FOUND?ENOENT:EACCES;return -1;}
    BY_HANDLE_FILE_INFORMATION info{};
    if(!GetFileInformationByHandle(file,&info) || (info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))){CloseHandle(file);errno=EACCES;return -1;}
    int fd=_open_osfhandle((intptr_t)file,create?_O_WRONLY|_O_BINARY:_O_RDONLY|_O_BINARY);
    if(fd<0)CloseHandle(file);return fd;
#else
    return ::open(path,create?O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW:O_RDONLY|O_NOFOLLOW,0600);
#endif
}
bool sync_file(int file) {
#if defined(_WIN32)
    return _commit(file)==0;
#else
    return ::fsync(file)==0;
#endif
}
bool promote(const std::string &from,const char *to) {
#if defined(_WIN32)
    return MoveFileExW(std::filesystem::u8path(from).c_str(),std::filesystem::u8path(to).c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return ::rename(from.c_str(),to)==0;
#endif
}
}
bool Sims2LoadMusicPreference(const char *path,bool &enabled) {
    enabled=true;int file=open_file(path,false);
    if(file<0)return errno==ENOENT;
    char bytes[64];size_t used=0;bool ok=true;
    while(used<sizeof(bytes)) {
        auto got=::read(file,bytes+used,sizeof(bytes)-used);
        if(got<0){if(errno==EINTR)continue;ok=false;break;}
        if(!got)break;used+=size_t(got);
    }
    if(::close(file))ok=false;
    if(!ok || used!=std::strlen(on))return false;
    if(!std::memcmp(bytes,off,used)){enabled=false;return true;}
    return !std::memcmp(bytes,on,used);
}
bool Sims2SaveMusicPreference(const char *path,bool enabled) {
    static std::atomic<unsigned> sequence{0};
    const std::string temporary=std::string(path)+".new-"+std::to_string(::getpid())+"-"+std::to_string(sequence++);
    int file=open_file(temporary.c_str(),true);if(file<0)return false;
    const char *bytes=enabled?on:off;size_t at=0,size=std::strlen(bytes);bool ok=true;
    while(at<size){auto wrote=::write(file,bytes+at,size-at);if(wrote<0 && errno==EINTR)continue;
        if(wrote<=0){ok=false;break;}at+=size_t(wrote);}
    if(ok && !sync_file(file))ok=false;
    if(::close(file))ok=false;
    if(ok && promote(temporary,path))return true;
    ::unlink(temporary.c_str());return false;
}
