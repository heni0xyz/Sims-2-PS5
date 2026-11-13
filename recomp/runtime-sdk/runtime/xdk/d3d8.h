#pragma once
#include <mutex>

#include "gpu/state.h"

namespace d3d {
// Set before the first callback. False retains the synchronous correctness
// baseline. Async dispatch still requires successful native GPU completion.
void SetAsyncGpuCallbacks(bool enabled);
#if defined(SIMS2_HEADLESS_RUNTIME) || defined(SIMS2_NATIVE_THUMBNAILS)
// The game submits the render from a different thread while the producer waits.
enum class PortraitRaster { None, Thumbnail, CasPortrait, Artwork };
void BeginPortraitRaster(PortraitRaster pass);
void EndPortraitRaster(PortraitRaster pass);
#endif

// read-only: the XDK entry points write it and mark what they wrote for the render thread (gpu::State::Mark)
const gpu::State& State();
std::recursive_mutex& Lock();

}  // namespace d3d
