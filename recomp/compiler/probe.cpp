// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Authored, freestanding fixture. The host test uses a compiler-generated ABI
// bridge: SysV and Windows also differ in their callee-saved registers, even
// for a function with zero arguments. The console calls the SysV entry directly.
// No game data, external imports, TLS or ELF relocations are expected here.
extern "C" __attribute__((noinline)) unsigned sims2_compiler_probe() {
  static_assert(sizeof(void *) == 8);
  static_assert(sizeof(unsigned) == 4);
  volatile unsigned seed = 0x12345678u;
  unsigned value = seed;
  unsigned hash = 0x811c9dc5u;
  for (unsigned index = 0; index != 257; ++index) {
    value = value * 1664525u + 1013904223u;
    unsigned rotated = (value << 7) | (value >> 25);
    unsigned chosen = (value & 0x80000000u) ? rotated : ~rotated;
    hash = (hash ^ (chosen + index)) * 16777619u;
  }
  return hash;
}
