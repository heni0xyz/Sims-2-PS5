#pragma once
#include <cstdint>

// The Xbox maps the same 64 MiB of RAM at 80000000 and F0000000. Native
// shared mappings alone are insufficient: an optimizing C++ compiler can
// prove different constant offsets do not alias and reuse a stale load.
// Normalize only dereference addresses, so both spellings name the same
// C++ object. Guest registers, pointer values and OS mappings remain intact.
// Outside the write-combined RAM window every guest address is unchanged.
inline constexpr uint32_t x86_memory_address(uint32_t address) {
    return address >= 0xF0000000u && address < 0xF4000000u
        ? address - 0x70000000u : address;
}
