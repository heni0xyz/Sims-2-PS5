// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "imgui.h"
#include <string>
enum class Sims2InstallerImage { Logo, Cross, Circle, L1, R1, RightStick, Touchpad, DPad, Preview1, Preview2, Preview3, Background, Count };
void Sims2InstallerRoundedPath(ImDrawList *, ImVec2, ImVec2, float radius);
void Sims2PrepareInstallerAtlas(ImFontAtlas *, const std::string &resource_directory);
void Sims2DrawInstallerImage(ImDrawList *, Sims2InstallerImage, ImVec2 top_left, ImVec2 bottom_right, float rounding=0);
void Sims2SetNextExecutable(const std::string &);
void Sims2SetUpdateHandoff(bool helper);
