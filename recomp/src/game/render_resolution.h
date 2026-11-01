// Authored host-only detail policy. OpenAI Codex for szampan, 2026.
#pragma once
#include <atomic>
namespace sims2::render {
extern std::atomic<bool> native_resolution;
extern bool configured_native_resolution;
void Initialize();
}
