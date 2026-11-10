// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "fatal.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdio>

namespace {
void compiler_fatal(void *data, const char *message, bool) {
  auto &state = *static_cast<Sims2CompilerFatalState *>(data);
  std::snprintf(state.message, sizeof(state.message), "LLVM fatal: %s",
                message ? message : "unknown error");
  // report_fatal_error releases its handler mutex before invoking this hook.
  // Throwing prevents its default abort/exit path; the caller must not reuse
  // the compiler after this exception. All frames are built with EH enabled.
  throw Sims2CompilerFatal{};
}
} // namespace

Sims2CompilerFatalScope::Sims2CompilerFatalScope(
    Sims2CompilerFatalState &state) {
  llvm::install_fatal_error_handler(compiler_fatal, &state);
}
Sims2CompilerFatalScope::~Sims2CompilerFatalScope() {
  llvm::remove_fatal_error_handler();
}
