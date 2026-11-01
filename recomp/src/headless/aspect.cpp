// Optional 16:9 PC correctness probe. No world Hor+ widening is applied.
// Used only with the optional Vulkan runtime SDK; no display is created here.
#if defined(SIMS2_HEADLESS_RUNTIME)
#include "app/app.h"
namespace app {
float HostAspect() { return 0; }
float AspectWiden() { return 1; }
float TargetAspect() { return Get().aspect_ratio=="16:9"?16.0f/9.0f:4.0f/3.0f; }
}
#endif
