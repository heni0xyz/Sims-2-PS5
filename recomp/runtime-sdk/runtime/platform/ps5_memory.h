#pragma once
#include <cstdint>

// Native backing only. Guest allocation policy remains in kernel/memory.cpp.
// PS5 has 16 KiB protection pages; guest commit state remains 4 KiB. Sibling
// guest pages sharing a native page must survive another page's decommit.
// Calls are serialized by mem::g_lock; Init/Shutdown occur outside execution.
namespace mem::ps5host {
uint8_t* Init();
bool Commit(uint32_t va, uint32_t bytes);
bool Decommit(uint32_t va, uint32_t bytes);
bool Shutdown();
bool IsCommitted(uint32_t va);
}
