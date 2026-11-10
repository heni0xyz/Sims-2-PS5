// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Synthetic C++ input for the compiler running on PS5. No game content.
#include <array>
#include <cstdint>
#include <stddef.h>
static_assert(sizeof(void *) == 8);
static_assert(sizeof(std::uint32_t) == 4);
#if defined(SIMS2_FIXTURE_SYNTAX_ERROR)
#error SIMS2_expected_compiler_diagnostic
#elif defined(SIMS2_FIXTURE_PROVIDER)
extern "C" std::uint32_t sims2_probe_mix(std::uint32_t value) {
  constexpr std::array<std::uint32_t, 3> constants{
    0x53494d53u, 1664525u, 1013904223u};
  return (value ^ constants[0]) * constants[1] + constants[2];
}
#elif defined(SIMS2_FIXTURE_MISSING)
extern "C" std::uint32_t sims2_deliberately_missing(std::uint32_t);
extern "C" std::uint32_t sims2_probe_entry(std::uint32_t value) {
  return sims2_deliberately_missing(value);
}
#else
extern "C" std::uint32_t sims2_probe_mix(std::uint32_t);
extern "C" std::uint32_t sims2_probe_entry(std::uint32_t value) {
  return sims2_probe_mix(value) ^ 0x505335u;
}
#endif
