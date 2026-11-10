// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <string>
enum class Sims2UpdateState { Checking, Current, Available, Unavailable, Downloading, Preparing,
  ReadyToApply, Applying, RestartReady, Failed };
struct Sims2UpdateStatus {Sims2UpdateState state=Sims2UpdateState::Checking;std::string version;
  bool compatible=false;float progress=0;std::string message;};
void Sims2StartUpdateCheck(const std::string &current_version);
void Sims2StartLauncherUpdate(const std::string &current_version);
void Sims2StartApplyUpdate();
inline bool Sims2UpdateBusy(Sims2UpdateState s){return s==Sims2UpdateState::Downloading || s==Sims2UpdateState::Preparing ||
  s==Sims2UpdateState::Applying || s==Sims2UpdateState::ReadyToApply || s==Sims2UpdateState::RestartReady;}
bool Sims2IsUpdateHelper();
Sims2UpdateStatus Sims2GetUpdateStatus();
bool Sims2NewerVersion(const std::string &candidate,const std::string &current);
