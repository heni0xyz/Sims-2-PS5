// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "embedded.h"
#include <string>
#include <vector>

namespace sims2::compiler {
struct NativeInput { std::string name, sha256; };
struct NativePackageRequest {
  // Absolute public kit, private object and fresh run directories. Inputs are
  // hash checked before use. Only new files in output_directory are created;
  // a running title, original executable and system software are never edited.
  std::string kit_directory, object_directory, output_directory;
  std::vector<NativeInput> public_inputs, objects;
  // The captured, reviewed RADV recipe with {KIT}/{WORK} placeholders. No shell.
  std::vector<std::string> link_arguments, stub_files, allowed_imports;
};
struct NativePackageResult {
  std::string elf_sha256, self_sha256;
  size_t elf_bytes = 0, self_bytes = 0;
};
// Calls LLVM's archive writer, the existing serialized LLD API and unchanged
// public native writer/FSELF library. Does not launch or install the result.
// Returns 0 success, 1 ordinary failure, 2 invalid request, 3 poisoned worker.
int CreateNativeTitle(const NativePackageRequest &, NativePackageResult &,
                      Sims2CompilerLog, void *user);
}
