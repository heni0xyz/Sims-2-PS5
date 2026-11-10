// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
extern "C" void *ps5_core_dlopen(const char *, int);
extern "C" void *ps5_core_dlsym(void *, const char *);
extern "C" int ps5_core_dlclose(void *);
extern "C" char *ps5_core_dlerror();
