// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "game/install.h"
#include "installer_view.h"
#include <atomic>
#include <mutex>
#include <pthread.h>
#include <string>

// One worker owns files and tool invocations. The renderer only reads snapshots
// or requests cancellation between operations; it never cancels inside LLVM.
struct Sims2Installer {
  std::atomic<Sims2InstallPhase> phase{Sims2InstallPhase::Checking};
  std::atomic<unsigned> completed{0}, total{0}, percent{0};
  std::atomic<bool> cancel{false}, done{false};
  std::atomic<bool> has_game{false},game_update_available{false};
  std::atomic<double> began{0}, operation_began{0};
  std::mutex mutex;
  std::string error, executable, source_name, game_launcher_version;
  sims2::install::Source source;
  pthread_t worker{};
  bool started = false, restart_required = false;
  // inspect=true checks an existing installation without starting a fresh one.
  bool Start(bool inspect);
  void Join(); // only when done
  bool ResolveSource(bool remove); // only after successful preparation
  std::string Error();
  std::string Executable(); // only after complete digest validation
  std::string GameLauncherVersion();
};
