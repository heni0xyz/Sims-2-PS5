// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <atomic>
#include <pthread.h>

// Worker never touches Vulkan or ImGui. The render thread continues presenting
// and emitting heartbeats while LLD owns its serialized process-wide state.
struct Sims2LinkerProbe {
  std::atomic<int> phase{0}; // 0 idle, 1 preparing, 2 linking, 3 done, 4 compiling, 5 I/O, 6 executing, 7 generating, 8 game unit
  std::atomic<int> verdict{-1}; // 0 pass, 1 fail, 3 worker restart required
  std::atomic<double> began{0};
  std::atomic<double> operation_began{0};
  std::atomic<unsigned> units_done{0};
  pthread_t thread{};
  bool started = false;
  bool start();
  void finish(); // Join only after phase 3. No cancellation inside LLVM.
};
