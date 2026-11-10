// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "embedded.h"
#include "llvm/Support/raw_ostream.h"
#include <mutex>

namespace sims2::compiler {
// One process-wide lock protects LLVM globals, fatal handlers and worker state.
// Both the standalone linker and complete compiler use this same state.
extern std::mutex compiler_mutex;
extern bool compiler_usable;
extern bool linker_usable;
int poison_compiler(const char *, Sims2CompilerLog, void *);
class LogStream final : public llvm::raw_ostream {
  Sims2CompilerLog callback;
  void *user;
  uint64_t position = 0;
  void write_impl(const char *data, std::size_t size) override {
    position += size;
    if (callback) callback(data, size, user);
  }
  uint64_t current_pos() const override { return position; }
public:
  LogStream(Sims2CompilerLog callback, void *user) : callback(callback), user(user) {
    SetUnbuffered();
  }
};
} // namespace sims2::compiler
