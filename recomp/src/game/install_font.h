// SPDX-License-Identifier: GPL-3.0-only
// Reader/layout code only. Open Sans glyph resource remains OFL-1.1.
#pragma once
#include <cstdint>
#include <span>
#include <string_view>
namespace sims2::install {
void LoadInstallFont(std::span<const uint8_t>);
bool InstallFontLoaded();
int TextWidth(std::string_view,unsigned style);
void RasterText(std::span<uint32_t> canvas,std::string_view,int y,unsigned style,uint32_t argb);
}
