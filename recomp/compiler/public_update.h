// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <filesystem>
#include <functional>
#include <string>
namespace sims2::update {
using Progress=std::function<void(unsigned,unsigned)>;
std::string HashFile(const std::filesystem::path &);
bool ManagedPath(const std::string &);
std::filesystem::path DownloadPath(const std::filesystem::path &root);
// These operations never execute the downloaded code. Apply is called only
// by a copy of the current trusted launcher, after a clean LoadExec handoff.
void Prepare(const std::filesystem::path &root,const std::filesystem::path &bundle,
             const std::string &expected_sha,const std::string &expected_version,
             const std::string &current_version,const Progress &progress=Progress{});
void CopyHelper(const std::filesystem::path &root);
void Apply(const std::filesystem::path &root,const Progress &progress=Progress{});
void Recover(const std::filesystem::path &root);
}
