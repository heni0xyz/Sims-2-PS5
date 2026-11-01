#pragma once
#include <cstdint>
#include <string>
#include "core/input.h"
namespace sims2::ps5 {
// The template exposes these exact pad fields, after it has handled users,
// deadzones and intercepted samples. No SCE types leak into Xbox HLE.
struct PadSample {
    bool connected = false;
    uint32_t held = 0;
    float lx = 0, ly = 0, rx = 0, ry = 0, l2 = 0, r2 = 0;
};
input::Pad MapPad(const PadSample& sample);
// Lexical mount translation. Case-insensitive asset lookup remains the SDK's
// job. Reject traversal above a mount, embedded NULs and unknown drive letters.
bool TranslatePath(const std::string& guest, const std::string& assets,
                   const std::string& saves, std::string& result);
// Native addresses are offsets into a reserved 4 GiB window, never truncated.
bool ToGuest(uintptr_t base, uintptr_t native, uint32_t& guest);
bool ToNative(uintptr_t base, uint32_t guest, uint64_t bytes, uintptr_t& native);
}
