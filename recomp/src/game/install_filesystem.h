// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <filesystem>
#if defined(__PROSPERO__) || defined(SIMS2_INSTALL_POSIX_METADATA_TEST)
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace sims2::install::files {
// lstat is refused in a PS5 title. O_NOFOLLOW verifies the final component
// through a real descriptor; failures never become fabricated permissions.
inline std::filesystem::file_status NoFollow(const std::filesystem::path& path){
#if defined(__PROSPERO__) || defined(SIMS2_INSTALL_POSIX_METADATA_TEST)
    const int fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
    if(fd<0){if(errno==ENOENT || errno==ENOTDIR)return std::filesystem::file_status(std::filesystem::file_type::not_found);
        throw std::filesystem::filesystem_error("Cannot open metadata without following links",path,std::error_code(errno,std::generic_category()));}
    struct stat value{};const int result=::fstat(fd,&value),saved=errno;::close(fd);
    if(result)throw std::filesystem::filesystem_error("Cannot inspect file descriptor",path,std::error_code(saved,std::generic_category()));
    const auto type=S_ISREG(value.st_mode)?std::filesystem::file_type::regular:S_ISDIR(value.st_mode)?std::filesystem::file_type::directory:std::filesystem::file_type::unknown;
    return std::filesystem::file_status(type,std::filesystem::perms(value.st_mode&0777));
#else
    return std::filesystem::symlink_status(path);
#endif
}
inline bool Remove(const std::filesystem::path& path,bool directory=false){
#if defined(__PROSPERO__) || defined(SIMS2_INSTALL_POSIX_METADATA_TEST)
    if((directory? ::rmdir(path.c_str()) : ::unlink(path.c_str()))==0)return true;
    if(errno==ENOENT)return false;
    throw std::filesystem::filesystem_error("Cannot remove inspected install member",path,std::error_code(errno,std::generic_category()));
#else
    (void)directory;return std::filesystem::remove(path);
#endif
}
}
