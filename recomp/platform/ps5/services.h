#pragma once
#include <cstdint>
#include <functional>

namespace sims2::ps5 {
// The caller owns these strings for the whole session. InitLogging runs after
// the template's platform_init has established its existing klog transport.
struct LogSession { const char* build; const char* run; bool diagnostic; };
void InitLogging(LogSession session);
void Log(const char* subsystem, const char* severity, const char* message,
         const char* milestone = "", uint32_t guest = 0, uintptr_t native = 0);
// Call once per frame on the template's pad thread, after pad_open.
// Guest threads only read snapshots through the normal input::Poll API.
void PublishPads();
void StopAudio(); // join/stop the template callback before guest state is freed
int16_t AudioSample(float value);
}
