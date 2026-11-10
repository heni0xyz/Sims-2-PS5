// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstddef>

// Serialized worker API. Arguments are Clang frontend (-cc1) arguments, without
// argv[0] or -cc1; no driver/subprocess is invoked. Diagnostic callback data is
// borrowed only during the call; callbacks must not throw or reenter this API.
// Invalid C++ returns failure. A reported LLVM worker fatal returns 3 and poisons
// the compiler/linker until process restart. This path has host coverage, but
// console fatal/shutdown behavior and arbitrary native crashes remain unverified.
using Sims2CompilerLog = void (*)(const char *, std::size_t, void *);
// working_directory must be absolute and exist. It is local to Clang's VFS;
// this API never changes the process working directory. LLVM snapshots the
// process cwd while constructing its physical VFS, so title linkage must bind
// getcwd/realpath to the SDK platform implementations. All output paths in the
// frontend arguments must be absolute.
int sims2_compiler_compile(const char *working_directory,
                          const char *const *arguments, std::size_t count,
                          Sims2CompilerLog log, void *user);
// Driver argv includes argv[0], -c, explicit PS5 target, resource directory,
// sysroot/includes and absolute source/output paths. BuildCompilation is used
// only to obtain one -cc1 job; Command::Execute is never called. This keeps the
// frontend arguments native to the embedded compiler's own version.
int sims2_compiler_compile_driver(const char *working_directory,
                                 const char *const *arguments, std::size_t count,
                                 Sims2CompilerLog log, void *user);
int sims2_compiler_link(const char *const *arguments, std::size_t count,
                       Sims2CompilerLog log, void *user);
