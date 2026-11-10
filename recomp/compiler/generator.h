// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <string>

// Calls the pinned MIT XboxXBERecomp generator in this process. Paths must be
// absolute; output must not exist. The installer verifies the supported XBE hash
// first. Exceptions become a diagnostic string; stdout/stderr retain the SDK's
// analysis diagnostics and route through the title's existing klog integration.
// No process creation, shell, current-directory change or generated-code edits.
int sims2_generate_source(const char *config, const char *verified_xbe,
                          const char *new_output, const char *xdk,
                          std::string &error);
