// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "game/install_screen.h"
#include <memory>
namespace gpu {class Backend;}
namespace sims2::install {
// Owns one short-lived backend and framebuffer. Destroy it before the game's
// renderer is initialized; the established Vulkan backend owns its teardown.
class Display {
public:
    Display();~Display();
    Display(const Display&)=delete;
    void Draw(const View&);
private:
    uint32_t header_=0,data_=0;
    std::unique_ptr<gpu::Backend> backend_;
};
}
