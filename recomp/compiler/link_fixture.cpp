// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Authored synthetic inputs. Never derived from the game executable.
#if defined(SIMS2_FIXTURE_PROVIDER)
extern "C" unsigned sims2_probe_mix(unsigned value) {
  return (value ^ 0x53494d53u) * 1664525u + 1013904223u;
}
#elif defined(SIMS2_FIXTURE_MISSING)
extern "C" unsigned sims2_deliberately_missing(unsigned);
extern "C" unsigned sims2_probe_entry(unsigned value) {
  return sims2_deliberately_missing(value);
}
#else
extern "C" unsigned sims2_probe_mix(unsigned);
extern "C" unsigned sims2_probe_entry(unsigned value) {
  return sims2_probe_mix(value) ^ 0x505335u;
}
#endif
