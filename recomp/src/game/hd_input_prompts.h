// SPDX-License-Identifier: GPL-3.0-only
#pragma once
namespace gpu {struct TextureReplacement;}
namespace sims2::prompts {
extern bool enabled;
void Initialize();
// Read the same validated public CC0 pack for the asset-free first-run UI.
const gpu::TextureReplacement* UiTexture(const char* resource);
}
