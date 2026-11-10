// SPDX-License-Identifier: GPL-3.0-only
// OpenAI Codex for szampan. First-run installation, independent of the renderer.
#pragma once
#include "app/app.h"
#include <functional>
#include <span>
namespace sims2::install {
struct File {std::string name;uint64_t bytes;};
enum class Stage {Copy,Verify,Patch,Complete};
struct Progress {Stage stage;uint64_t done,total;};
using Notify=std::function<bool(Progress)>; // false cancels at a chunk boundary
using Read=std::function<bool(size_t,uint64_t,std::span<uint8_t>)>;
using Finish=std::function<bool(const std::filesystem::path&,std::string&)>;
// Creates assets only by a final same-parent rename. Existing assets/saves/ISO
// are never overwritten. Recovery removes only our marked, known staging tree;
// unknown members or symlinks cause refusal. Writes and readbacks are bounded.
bool Extract(const std::filesystem::path& root,const std::vector<File>&,
             const Read&,const Finish&,const Notify&,std::string& error);
bool FromIso(const std::filesystem::path& iso,const std::filesystem::path& root,
             const Notify&,std::string& error);
// Only the exact, sole public README marker may be removed. Accept the legacy
// Windows CRLF spelling as well; real or unknown owner data is preserved.
bool RemovePlaceholder(const std::filesystem::path& root,std::string& error);
// Strict supported XBE plus required disc members. Does not mount an ISO.
bool Ready(const std::filesystem::path& root,app::GameFiles&,std::string& error);
// One-time invalidation of the scratch Z cache for the known notice patch.
// Archives that cache without deleting it; all save drives stay untouched.
bool PrepareCache(const std::filesystem::path& root,std::string& error);
bool PatchNotices(std::vector<uint8_t>& archive,std::string& error);
struct Source {
    std::filesystem::path root,path;
    uintmax_t bytes=0;
    std::filesystem::file_time_type modified{};
};
bool RememberSource(const std::filesystem::path& root,const std::filesystem::path& iso,Source&,std::string& error);
// Call only after a fresh Delete confirmation. Rechecks installed assets and
// the original regular-file identity; never deletes a directory or another ISO.
bool DeleteSource(const Source&,std::string& error);
}
