// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
// Preferences live outside the managed public-update inventory and game saves.
bool Sims2LoadMusicPreference(const char *path,bool &enabled);
bool Sims2SaveMusicPreference(const char *path,bool enabled);
