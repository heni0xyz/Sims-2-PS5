// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <filesystem>
#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <string>
#if !defined(_WIN32)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace sims2::files {
struct ExecutableMode {unsigned before=0,after=0;bool repaired=false;
  std::string Describe()const{char value[96];std::snprintf(value,sizeof(value),"mode_before=%04o mode_after=%04o repaired=%d",before,after,int(repaired));return value;}
};
// Content/digest validation belongs to the caller. Change permissions through
// the inspected descriptor, never a symlink. In a title, 0644 is readable but
// syscore refuses LoadExec with EACCES before the application's entry point.
inline ExecutableMode EnsureExecutable(const std::filesystem::path &path){
#if defined(_WIN32)
  if(!std::filesystem::is_regular_file(std::filesystem::symlink_status(path)))
    throw std::runtime_error("Executable is not a regular file: "+path.string());
  return {}; // Windows does not implement PS5/POSIX executable mode bits.
#else
  const int fd=::open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
  if(fd<0)throw std::runtime_error("Cannot inspect executable: "+path.string()+" errno="+std::to_string(errno));
  struct Close {int fd;~Close(){::close(fd);}} close{fd};
  struct stat status{};
  if(::fstat(fd,&status) || !S_ISREG(status.st_mode))
    throw std::runtime_error("Executable descriptor is not a regular file: "+path.string());
  ExecutableMode result{unsigned(status.st_mode&0777),0,false};
  if(result.before!=0777){
    if(::fchmod(fd,0777))throw std::runtime_error("Cannot enable executable permissions: "+path.string()+" errno="+std::to_string(errno));
    result.repaired=true;
  }
  if(::fstat(fd,&status) || (status.st_mode&0777)!=0777)
    throw std::runtime_error("Executable permissions failed verification: "+path.string());
  result.after=unsigned(status.st_mode&0777);return result;
#endif
}
}
