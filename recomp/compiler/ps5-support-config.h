// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "llvm/Config/config.h"
#if defined(__PROSPERO__)
// The platform heap uses direct memory, not jemalloc or a process brk arena.
// FreeBSD header declarations alone do not establish either runtime facility.
// LLVM's existing unsupported malloc-counter fallback returns zero; the probe
// measures actual memory availability separately through ps5_memory_query.
#undef HAVE_MALLCTL
#undef HAVE_SBRK
#endif
