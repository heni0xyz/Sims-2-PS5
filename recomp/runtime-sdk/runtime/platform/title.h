#pragma once
#include <cstdint>
#include <functional>

// A native title's process lifecycle. The PS5 implementation uses the shell's
// documented LoadExec path; it never calls libc exit/_Exit or returns from _start.
namespace title_platform {
[[noreturn]] void Shutdown(int code);
[[noreturn]] void Relaunch(const char* executable);
[[noreturn]] void Fatal(const char* message, uint32_t guest = 0, uintptr_t native = 0);
void ThreadName(const char* name);
void GuestStarting(uint32_t address);
void StartThread(std::function<void()> routine);
[[noreturn]] void ExitThread();
}
