// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "gpu/texture_replacement.h"
namespace gpu {struct TextureDesc;}
namespace sims2::fonts {
extern bool enabled;
bool Active();
#if defined(SIMS2_HEADLESS_RUNTIME)
void TestEnabled(bool);
#endif
const gpu::TextureReplacement* Resolve(const gpu::TextureDesc&,const uint8_t*,uint32_t,const uint32_t* palette);
}
