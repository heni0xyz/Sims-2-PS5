// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Host-only ABI bridge. Compile as a Windows DLL, never as a PS5 title object.
// Clang saves Windows nonvolatile registers around the SysV function call.
#if !defined(_WIN32)
#error "This bridge is only for the Windows host oracle test"
#endif
using SysVFunction = unsigned (__attribute__((sysv_abi)) *)();
extern "C" __declspec(dllexport) unsigned sims2_probe_call(void *address) {
  return reinterpret_cast<SysVFunction>(address)();
}
