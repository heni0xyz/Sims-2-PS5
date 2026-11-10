// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "fatal.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdio>
#include <cstring>

struct Cleanup {
  bool &done;
  ~Cleanup() { done = true; }
};
int main() {
  // Report a real LLVM fatal twice under separate scopes. Check exception
  // cleanup, diagnostic capture and handler removal, not just a return value.
  for (unsigned pass = 0; pass < 2; ++pass) {
    Sims2CompilerFatalState state;
    bool cleaned = false, caught = false;
    try {
      Sims2CompilerFatalScope scope(state);
      Cleanup cleanup{cleaned};
      llvm::report_fatal_error("authored compiler failure probe", false);
    } catch (const Sims2CompilerFatal &) {
      caught = true;
    }
    if (!caught || !cleaned ||
        !std::strstr(state.message, "authored compiler failure probe")) {
      std::puts("{\"status\":\"FAIL\",\"case\":\"reported-fatal-unwind\"}");
      return 1;
    }
  }
  std::puts("{\"status\":\"PASS\",\"host_only\":true,\"reported_fatals\":2,"
            "\"cleanup_verified\":true}");
  return 0;
}
