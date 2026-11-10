// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

// Used only inside the serialized compiler worker, with EH enabled throughout
// LLVM. A reported LLVM fatal error unwinds to our API, which poisons the
// worker and requests a title restart. This does not claim recovery from
// arbitrary native memory corruption/signals or fatal errors on unrelated
// threads.
struct Sims2CompilerFatal {};
struct Sims2CompilerFatalState {
  char message[1024]{};
};
struct Sims2CompilerFatalScope {
  explicit Sims2CompilerFatalScope(Sims2CompilerFatalState &);
  ~Sims2CompilerFatalScope();
  Sims2CompilerFatalScope(const Sims2CompilerFatalScope &) = delete;
  Sims2CompilerFatalScope &operator=(const Sims2CompilerFatalScope &) = delete;
};
