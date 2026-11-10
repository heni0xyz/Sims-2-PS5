# Experimental console-side preparation

Copyright (C) 2026 OpenAI Codex, for szampan. Authored integration: GPL-3.0-only.
XboxXBERecomp remains MIT; LLVM remains Apache-2.0 WITH LLVM-exception.

This directory is development work. It is not a working PS5-only release.
The existing Windows builder and public instruction title are unchanged.

Current: PS5 source generation **PASS**, f622a1b9ef81c744, including all 71 file
hashes twice and refusal to overwrite. Exact timings/revisions/evidence are in
docs/CONSOLE_GENERATION_RESULT_2026-11-08.json. Next candidate effac3b70e2d9484
is **BUILD PASS / CONSOLE NOT TESTED** and additionally compiles one real source
unit at O2. The console was unavailable and its startup was deferred. No game
code has yet been compiled/linked/executed through this console-only path.
See [CONSOLE_GENERATION_TEST.md](CONSOLE_GENERATION_TEST.md) to resume.

The entries below are earlier results and their original limits.

Latest hardware result: PPSA99147 `f00dde58ac5ec7c4` **COMPILE / LINK / EXECUTE / I/O
PASS on PS5**. The locally generated modules pass 32 arithmetic oracle calls,
invalid-module rejection, unload/allocation checks and normal exit. Whole worker:
3.699577 s. See [CONSOLE_EXECUTION_TEST.md](CONSOLE_EXECUTION_TEST.md). Full-game
code generation/module integration and ISO installation are still separate gates.

Earlier hardware result, 8 November 2026: PPSA99147 `b04be1035b09b705`
**C++ COMPILATION / I/O / SYNTHETIC LINKING PASS** on the owner's authorized
console. Five compiler jobs, expected syntax error recovery, three link jobs,
repeat hashes and normal title exit pass. Whole worker: 3.568512 s. Retrieved
objects/ELFs were independently inspected. Generated code execution and full
game installation are not yet proven. See the newest `docs/PROGRESS.md` entry.

Earlier hardware failure, 8 November 2026: PPSA99147 `14fb8665bc404469` **FAIL**,
SIGSEGV in libc getcwd while LLVM constructs its physical VFS. Its matched
debug ELF and call-site disassembly identify a missing SDK binding. Candidate
`a72d982ec4cb8260` connects getcwd/realpath to the existing platform implementations,
adds VFS stage logs and rejects remaining libc imports for configured aliases.
Its native rebuild, final fresh pass, native package/SELF and ZIP verification
succeed. Static symbol inspection confirms both aliases match their SDK
implementations and neither is imported. Console retest is required.
See `docs/CONSOLE_COMPILER_CRASH_2026-11-08.md`.

Earlier delivery result, 8 November 2026: the complete native compiler libraries and final
fresh Ninja pass are **BUILD PASS**. Compiler diagnostic `14fb8665bc404469`,
PPSA99147, is linked, packaged and SELF-validated; **CONSOLE NOT TESTED**.
It compiles authored C++ on the console, exercises I/O and handles expected
syntax/link errors. See [CONSOLE_COMPILER_TEST.md](CONSOLE_COMPILER_TEST.md).

Earlier hardware result, 7 November 2026: standalone console linker probe
`ffceb5ef9749efa3` **BUILD PASS / SYNTHETIC CONSOLE LINKING PASS**, packaged as PPSA99146.
The tester's matching klog and returned ELF files verify two valid links and
recovery after one expected unresolved symbol in 0.494429 s. This does not
execute the output or compile C++ on the console.
See [CONSOLE_LINKER_TEST.md](CONSOLE_LINKER_TEST.md) and the newest progress entry.
This does not complete console compiler validation or the PS5-only game installer.

## Components

- `generator.cpp`: invokes the unchanged pinned XboxXBERecomp tool entry in the
  current process. The installer must verify the supported USA XBE first.
  Absolute paths and a new output directory are required. Existing output is
  refused. Its main symbol is renamed at compilation time. The PS5 tool uses an
  SDK stat/mkdir control-file adapter; its decoder, analysis and code-emission
  algorithms are unchanged, with full file parity proven on PS5.
- `embedded.cpp`: calls the Clang frontend as a library. The driver
  may plan one compiler job, but never executes a command or spawns a process.
  The compiler's VFS owns an explicit working directory; the title's process
  working directory is not changed.
- `linker.cpp`, `worker.*`: standalone embedded ELF linker and shared serialized
  LLVM state. This target builds without the Clang frontend. The complete
  compiler uses the same worker state, rather than a second set of LLVM globals.
- `console_probe*`, `link_fixture.cpp`, `compile_fixture.cpp`: isolated native
  console diagnostics. The shared runner supports precompiled linker fixtures
  and real C++ compilation on PS5. See the linked diagnostic guides for evidence,
  input/output validation, klog collection and required hardware checks.
- `probe.cpp`: authored, freestanding synthetic code; contains no game content.
- `probe_windows_bridge.cpp`: host-only Windows DLL bridge preserving Windows
  callee-saved registers around the native SysV probe call. It is not a console
  artifact. The host probe disables the SysV red zone, which Windows does not
  guarantee. The bridge is checked in disassembly; no machine bytes are written
  by hand. The console probe must still be tested with the console ABI.
- `ps5-toolchain.cmake`: cross-builds ELF x86-64 libraries against the existing
  public kit's SDK headers, preserving the PS5 ABI/TLS/FP requirements. It does
  not substitute a new RADV link recipe.

The frontend/driver APIs are from PS5_LLVM commit
`be5b58f51c93c67947b7e18cd45c2f9913f2c311` (22.1.8). Windows cross-compilation uses
the already verified Clang 23.1.4. These are different roles: a Windows compiler
targeting PS5 is not itself a compiler executable that runs on PS5.

When staging the embedded 22.1.8 compiler, stage its own builtin headers from
`clang/lib/Headers`. Do not silently reuse the Windows kit's Clang 23 resource
headers. Keep the SDK's C/C++ headers and target flags matched to the runtime.

## Measured evidence, 3 November 2026

Artifacts below are relative to the ignored `out/console-compiler` directory.

| Check | Evidence | Result |
| --- | --- | --- |
| Target synthetic code against independent integer oracle on Windows CPU | `host-probe/result.json`; result 1380434275 | PASS, host only |
| In-process generator versus the released builder's hashes | `generator-parity/result.json`; all 71 identical, overwrite refused | PASS, host only |
| Native generator static libraries | `build-generator.json`, `generator-ps5/libsims2-console-generator.a`; ELF64 x86-64 objects | BUILD PASS; console NOT TESTED |
| Matching host TableGen tools | `build-host.json`, 333 steps, 1264.598 seconds | PASS |
| Initial PS5 LLVM Support archive | `build-support.json`; assembly target corrected | BUILD PASS before explicit EH/RTTI flag migration |
| Embedded frontend interface object | `build-wrapper.json`; repeat after ABI flag migration | Initial BUILD PASS; final build status is in the live JSON |
| Reported LLVM fatal capture and C++ cleanup, twice | `fatal-host-run.log` / `.json` | PASS, host only |
| Updated interface and fatal handler target objects | `check-interface-embedded.json`, `check-interface-fatal.json` | PS5 object compilation PASS; console NOT TESTED |
| All compiler libraries and final fresh dependency pass, updated 8 November | `build-ps5.json`, `build-ps5-final.json` | BUILD PASS |
| Compiler diagnostic native link/imports/conversion/SELF | `out/console-compiler-probe/package-result.json` | BUILD PASS; console NOT TESTED |
| On-console C++ compilation and generated output execution | PPSA99147 tester gate | NOT TESTED |

Preserved first failures:

- `build-generator-first-failure.log`: PS5 defaults disable C++ exceptions, which
  also selects tomlplusplus's nonexception parse-result API. Explicitly enabled
  exceptions/RTTI; unchanged SDK generator then built successfully.
- `build-support-first-failure.log`: C/C++ targeted PS5, but assembly inherited
  Windows's COFF target. Set the ASM compiler target explicitly; all four
  optimized BLAKE3 assembly objects and Support archive then built successfully.
- LLVM's positive `LLVM_ENABLE_EH/RTTI` settings assume enabled compiler defaults.
  The PS5 driver defaults differ. The toolchain and cached C++ flags now enable
  both explicitly; the interface refuses to compile without those macros. The
  full build must regenerate any earlier objects with the corrected flags.

The initial interface-object target pulled library dependencies before checking
the interface. It was separated into an object library, and the old owned build
worker was intentionally stopped. Completed reusable objects remain in Ninja's
cache. The same controlled interruption was used before the EH/RTTI correction.
An interrupted pipeline's FAIL is not an additional source/compiler diagnostic.

## Reproduce and resume

The maintainer runs `tools/ps5/build_console_compiler.py`. First invocation supplies
`--llvm`, `--cmake`, `--ninja`, `--clang-bin`, `--python`, `--kit` and a `--phase`.
The LLVM revision is checked. The tool writes `build-settings.json` with local
tool paths; keep that file private. Later invocations can use:

```text
python tools/ps5/build_console_compiler.py --settings out/console-compiler/build-settings.json --phase configure-host
python tools/ps5/build_console_compiler.py --settings out/console-compiler/build-settings.json --phase build-host
python tools/ps5/build_console_compiler.py --settings out/console-compiler/build-settings.json --phase configure-ps5
python tools/ps5/build_console_compiler.py --settings out/console-compiler/build-settings.json --phase build-support
python tools/ps5/build_console_compiler.py --settings out/console-compiler/build-settings.json --phase continue
```

`continue` waits for the explicitly started host/Support stages to pass, checks
the interface object and builds the complete static dependency chain with two
jobs. It stops on failure. Do not start another Ninja in the same target build
directory while one is running. Read each stage's JSON and the end of its log;
appended logs retain earlier failed attempts. Existing objects are reused when
their commands and inputs match. Never interpret an old PASS from another flag
profile as validation of a new binary.

The standalone generator supports `configure-generator` and `build-generator`.
Its host parity check is `tools/ps5/test_console_generator.py`; it uses the
private verified XBE and leaves all generated content under ignored output.

The earlier standalone linker build uses `--phase build-linker`. Its receipt is
`build-linker.json`; it does not finish the complete Clang compiler. The probe
packager's optional `--phase finish` waits for that explicitly started build and
packages once, stopping on failure and never deploying. Its owned watcher PID
and verdict are in `out/console-linker-probe/state-finish-probe.json`; include it
when pausing work. Package retries preserve earlier artifacts in separate
attempt directories. The latest completed package is in `package-result.json`.

`test-fatal` reproduces the host fatal-handler test against the matching host
Support archive. `check-interface` reads Ninja's real compiler command and emits
separate probe objects outside the active build tree. `finish-current` is the
current waiting watcher: after the long build passes, it runs one fresh Ninja
pass (`build-ps5-final.json`) to include source/CMake updates made during that
build. It stops if the first build fails; it never deploys or ignores errors.

## Required gates before any PS5-only release

1. Native compiler libraries, final title linkage and import validation pass
   for PPSA99147 through the existing template/RADV recipe. Actual compiler
   runtime compatibility remains subject to the hardware gate.
2. Test the bounded synthetic compiler probe in the existing Vulkan foundation
   with its normal logging, heap, pad and shell shutdown. Verify compilation on
   PS5 and execution of the result through the configured supported launcher or
   separately proven module-loading path. Give the worker an explicit stack and
   the platform's IEEE floating-point state, and measure peak direct/flexible
   memory and disk-write behavior.
3. Test malformed input and compiler/linker fatal errors on hardware. Normal
   syntax errors return diagnostics. The reported-worker-fatal handler now
   unwinds and permanently disables the worker until restart; its scope/cleanup
   passes the host test. The title must turn result 3 into a diagnostic and
   normal shell shutdown/restart. Arbitrary native crashes, linker recovery,
   cross-thread fatals and the console shutdown path remain unverified. This
   is a release blocker, not a suppressed warning.
4. Verify writable output paths on the actual launcher setup. Keep the running
   public eboot unchanged. Generate the private result in a new directory; do
   not assume `/app0` is writable or that a new title appears automatically.
5. Integrate supported ISO hash checking, resumable generation/compilation,
   existing data extraction/patches, native conversion and packaging, then
   successful-install-only ISO deletion confirmation. Check failure/restart paths
   before treating an installed marker as ready.
6. Verify game initialization, saves, Continue, gameplay and performance. There
   is no measured PS5 preparation-time or new framerate claim yet.

Upstream source was not modified. No upstream PR or new public release was
submitted. Synthetic LLD has console evidence; the Clang frontend awaits it.
The Windows fallback remains.
