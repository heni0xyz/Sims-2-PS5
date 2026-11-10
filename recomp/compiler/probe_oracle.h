// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Integer oracle computed independently with Python arbitrary-precision integers.
#pragma once
#include <cstdint>
struct Sims2ProbeCase { uint32_t input, expected; };
static constexpr Sims2ProbeCase sims2ProbeCases[] = {
  {0x00000000u, 0x3941a1a3u},
  {0x00000001u, 0x38a8dfbcu},
  {0x00000002u, 0x388f7549u},
  {0x00000003u, 0x3895935au},
  {0x7fffffffu, 0xbfe2dd2eu},
  {0x80000000u, 0xb941a1a3u},
  {0xffffffffu, 0x3fe2dd2eu},
  {0x12345678u, 0x0f9f25bbu},
  {0xdeadbeefu, 0x77d868deu},
  {0x53494d53u, 0x3c3ea06au},
  {0x00505335u, 0x6e9871b8u},
  {0x01020304u, 0x11a7c4ffu},
  {0xaaaaaaaau, 0x62efa231u},
  {0x55555555u, 0x1654dc98u},
  {0xabcdef01u, 0x8d898ebcu},
  {0x87654321u, 0xf9df481cu},
};
