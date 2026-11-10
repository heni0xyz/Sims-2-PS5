// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "embedded.h"
#include "fatal.h"
#include "worker.h"
#include "lld/Common/Driver.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include <exception>

#if defined(__PROSPERO__) && (!defined(__EXCEPTIONS) || !defined(__GXX_RTTI))
#error "The embedded PS5 linker requires explicit -fexceptions -frtti"
#endif
LLD_HAS_DRIVER(elf)
using namespace sims2::compiler;
namespace {
// LLD's standalone entry also enables this. Without a recovery context a
// fatal link error calls Process::Exit instead of returning to the title.
// Restoring handlers is scoped to the serialized worker; it does not promise
// recovery from arbitrary corruption or faults on unrelated threads.
struct LinkRecovery {
  LinkRecovery() { llvm::CrashRecoveryContext::Enable(); }
  ~LinkRecovery() { llvm::CrashRecoveryContext::Disable(); }
};
}
int sims2_compiler_link(const char *const *arguments, std::size_t count,
                       Sims2CompilerLog callback, void *user) {
  if (!arguments || !count) return 2;
  for (std::size_t i = 0; i < count; ++i) if (!arguments[i]) return 2;
  std::lock_guard<std::mutex> lock(compiler_mutex);
  if (!linker_usable || !compiler_usable) return 3;
  LogStream output(callback, user);
  Sims2CompilerFatalState failure;
  try {
    Sims2CompilerFatalScope fatal_scope(failure);
    LinkRecovery recovery;
    const lld::DriverDef drivers[] = {{lld::Gnu, &lld::elf::link}};
    auto result = lld::lldMain(llvm::ArrayRef(arguments, count), output, output, drivers);
    linker_usable = result.canRunAgain;
    if (!result.canRunAgain)
      return poison_compiler("LLD recovery requires a title restart", callback, user);
    return result.retCode;
  } catch (const Sims2CompilerFatal &) {
    return poison_compiler(failure.message, callback, user);
  } catch (const std::exception &error) {
    return poison_compiler(error.what(), callback, user);
  } catch (...) {
    return poison_compiler("Unknown linker exception", callback, user);
  }
}
