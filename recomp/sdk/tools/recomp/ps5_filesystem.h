// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: MIT
// Control-file operations for the PS5-hosted generator. Keep its decoder,
// analysis and generated guest code unchanged. libc++'s monolithic filesystem
// operations object imports pathconf from a module unavailable to titles.
#pragma once
#include <cerrno>
#include <filesystem>
#include <sys/stat.h>
#include <system_error>

namespace ps5_recomp_fs {
using path = std::filesystem::path;
inline bool exists(const path &name, std::error_code &error) {
  struct stat status{};
  if (::stat(name.c_str(), &status) == 0) { error.clear(); return true; }
  const int code = errno;
  if (code == ENOENT || code == ENOTDIR) { error.clear(); return false; }
  error = std::error_code(code, std::generic_category());
  return false;
}
inline bool exists(const path &name) {
  std::error_code error;
  bool found = ps5_recomp_fs::exists(name, error);
  if (error) throw std::system_error(error, "generator stat: " + name.string());
  return found;
}
inline bool create_directories(const path &name) {
  if (name.empty()) throw std::system_error(EINVAL, std::generic_category(), "empty generator output path");
  path current;
  bool created = false;
  for (const auto &part : name) {
    current /= part;
    struct stat status{};
    if (::stat(current.c_str(), &status) == 0) {
      if (!S_ISDIR(status.st_mode))
        throw std::system_error(ENOTDIR, std::generic_category(), current.string());
      continue;
    }
    const int error = errno;
    if (error != ENOENT) throw std::system_error(error, std::generic_category(), current.string());
    if (::mkdir(current.c_str(), 0777) == 0) { created = true; continue; }
    const int make_error = errno;
    if (make_error == EEXIST && ::stat(current.c_str(), &status) == 0 && S_ISDIR(status.st_mode)) continue;
    throw std::system_error(make_error, std::generic_category(), current.string());
  }
  return created;
}
} // namespace ps5_recomp_fs
