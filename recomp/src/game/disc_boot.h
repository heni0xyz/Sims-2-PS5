// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "app/app.h"
#include <string>
namespace kernel {class Disc;}
namespace sims2::disc {
bool Validate(const kernel::Disc&,std::vector<uint8_t>& xbe,std::string& error);
// Read-only SDK disc mount. A mismatch never replaces the current mount.
bool Open(app::GameFiles&,const std::filesystem::path&,std::string& error);
// Root ISO is explicit ownership input; reject ambiguity rather than guessing.
bool RootImage(const std::filesystem::path& root,std::filesystem::path& image,std::string& error);
}
