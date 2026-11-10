// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "worker.h"
#include <cstring>
namespace sims2::compiler {
std::mutex compiler_mutex;
bool compiler_usable = true;
bool linker_usable = true;
int poison_compiler(const char *message, Sims2CompilerLog callback, void *user) {
  compiler_usable = linker_usable = false;
  if (callback) {
    callback(message, std::strlen(message), user);
    callback("\n", 1, user);
  }
  return 3;
}
} // namespace sims2::compiler
