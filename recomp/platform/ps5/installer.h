// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "app/app.h"
namespace sims2::ps5 {
// First-run UI. Returns false only for user quit/bounded-test cancellation.
// It leaves a verified extracted game on success and never changes saves.
bool InstallGame(app::GameFiles&,uint32_t width,uint32_t height,uint32_t test_seconds=0);
}
