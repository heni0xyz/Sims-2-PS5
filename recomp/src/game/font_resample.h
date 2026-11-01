// SPDX-License-Identifier: GPL-3.0-only
// Mathematical reconstruction only; contains no game glyphs or font data.
#pragma once
#include <cstdint>
#include <span>
#include <vector>
namespace sims2::fonts {
std::vector<uint8_t> UpscaleBgra(std::span<const uint8_t>,uint32_t width,uint32_t height,uint32_t scale);
}
