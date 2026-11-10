// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Adapter for the unchanged PS5_RetroArch loader. This diagnostic loads only
// its own validated pure arithmetic fixture, with zero undefined symbols.
// These are intentionally NOT general game-module import/thread services.
#include <cstdio>
#include <cstdlib>
extern "C" void *ps5_core_malloc(size_t bytes) { return std::malloc(bytes); }
extern "C" void *ps5_core_import(const char *name) {
  std::fprintf(stderr, "probe loader: refusing external import %s\n", name ? name : "<null>");
  return nullptr;
}
extern "C" unsigned ps5_core_threads_live() {
  // The checked no-import fixture can neither create nor retain a core thread.
  return 0;
}
