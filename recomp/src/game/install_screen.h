// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <span>
namespace sims2::install {
enum class Screen {Missing,Copying,Error,DeleteQuestion,DeleteError};
enum class Action {None,Extract,DeleteIso,KeepIso};
struct Choice {
    Screen screen=Screen::Missing;bool armed=false;
    void Set(Screen value){screen=value;armed=false;}
    Action Poll(bool cross,bool circle,bool held,bool can_extract);
};
struct View {Screen screen=Screen::Missing;unsigned percent=0;bool can_extract=false;std::string error;};
struct Icons {std::span<const uint8_t> cross,circle;}; // 256x256 BGRA, bottom-up
// Native 4K canvas; rendered through the existing GPU upload/present path.
constexpr unsigned kInstallWidth=3840,kInstallHeight=2160;
std::vector<uint32_t> DrawInstallScreen(const View&,Icons={});
}
