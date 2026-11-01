// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include <string>
#ifndef PS5_APP_ROOT
#define PS5_APP_ROOT "/app0"
#endif
// Strong override of the foundation's weak font customization hook.
bool ps5OverlayFont(std::string &filename, float &size) {
    filename = PS5_APP_ROOT "/public-resources/OpenSans-Regular.ttf";
    size = 22.0f; // At the native 4K scale of 2.5: 55-pixel font.
    return true;
}
