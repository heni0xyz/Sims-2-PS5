# Console C++ compiler diagnostic

Authored integration: GPL-3.0-only, OpenAI Codex for szampan.

PPSA99147 is a separate diagnostic title. It contains a native embedded Clang
frontend, embedded LLD, public compiler headers and authored synthetic C++.
It needs no Xbox ISO or game assets. It does not modify the working game title,
execute the resulting ELF or install the game.

## Current evidence, 8 November 2026

Owner-authorized automated run of `b04be1035b09b705`: **PASS on PS5** for actual
C++ compilation, expected syntax error/retry, I/O, link error/retry, identical
outputs and normal process exit. Whole worker: 3.568512 seconds. Returned ELF
and repeated objects match; generated output execution remains NOT TESTED.
The earlier header-order failure was fixed by using the driver's automatic C
include paths after libc++, rather than a user -isystem C directory before it.
See the newest progress entry and `tools/ps5/run_compiler_probe_console.py`.

The first hardware run of `14fb8665bc404469` crashed in libc getcwd during LLVM
physical-VFS construction. All five resource hashes matched. Candidate
`a72d982ec4cb8260` binds getcwd/realpath to the pinned SDK implementations and
logs VFS boundaries. See docs/CONSOLE_COMPILER_CRASH_2026-11-08.md for matched-ELF
symbolization. Successful compilation on PS5 remains unverified.

The complete PS5 compiler libraries and final incremental build pass. The
diagnostic cross-compiles and packages through the existing template/RADV
recipe. The original correction needed the later header-order fix before the
successful run above. The earlier PPSA99146 synthetic
linker test passed on hardware; that result does not establish Clang correctness.

The new test deliberately ships no precompiled fixture objects. On Cross it:

1. Opens a fresh run directory, preferring `/data/sims2-compiler-probe` and
   falling back to `/app0/compiler-runs`. Logs failures and critical resource hashes.
2. Checks write/readback, rename, unlink and file truncation. Reads use LLVM's
   real MemoryBuffer path. Buffers are released before deleting their files.
3. Compiles authored C++ with the embedded driver/frontend: provider, entry,
   intentional syntax error, repeated entry and unresolved-reference fixture.
   Includes exercise the matching LLVM 22 builtin headers and SDK libc++.
4. Requires ordinary syntax-error result 1, then successful subsequent compilation.
   Validates produced ELF64 x86-64 objects and identical repeated-object hashes.
5. Links valid inputs, expects one unresolved-symbol error, then links again.
   Validates both output ELFs and requires byte-identical hashes.
6. Saves the verdict and joins the worker. Circle follows the foundation's
   normal shutdown path. An I/O failure remains an overall FAIL even when the
   later compiler/linker checks succeed.

Klog events include build/run/thread IDs, monotonic time, arguments, diagnostics,
stage-boundary memory availability and before-operation I/O markers. Heartbeats
appear every two seconds. The render thread enforces a 120-second watchdog;
it does not cancel a live compiler thread or independently monitor GPU hangs.
Memory samples are not a measurement of peak use.

## Reproduce

Create the foundation with the pinned template's official factory:

```text
python ps5/tools/new-title.py <workspace>/PS5_Sims2CompilerProbe --title-id PPSA99147 --name "Sims 2 Compiler Diagnostics" --refresh 60
```

From this project, with saved private maintainer settings:

```text
python tools/ps5/build_console_compiler.py --settings out/console-compiler/build-settings.json --phase build-ps5
python tools/ps5/build_console_compiler.py --settings out/console-compiler/build-settings.json --phase finish-current
python tools/ps5/build_linker_probe.py --settings out/console-compiler/build-settings.json --mode compiler --phase prepare --platform-headers <pinned-sdk>/platform/include
python tools/ps5/build_linker_probe.py --settings out/console-compiler/build-settings.json --mode compiler --phase package --platform-headers <pinned-sdk>/platform/include
```

Wait for each prerequisite PASS. Do not start concurrent Ninja writers in a
build directory. After `prepare` passes, `--phase finish` may replace `package`
to await the explicitly started library build and its final fresh pass.
Packaging verifies the current authored inputs, imports, ELF layout and SELF.
It stages the embedded compiler's own resource headers, not the Windows cross
compiler's different-version headers. Retries preserve previous attempts.

Receipts: `out/console-compiler/build-ps5.json`, `build-ps5-final.json` and
`out/console-compiler-probe/package-result.json`. The latter points to the
matching build manifest, title and local debug ELF directory. These private
settings/artifacts are not source-release inputs.

After packaging, create and verify the private tester ZIP in a new directory:

```text
python tools/ps5/package_probe_delivery.py --result out/console-compiler-probe/package-result.json --output <fresh-delivery-directory>
```

This checks every title file against the package manifest, retains matching
local symbols and reads the ZIP back for CRC and per-file SHA-256 verification.

## Tester instructions

Copy the entire PPSA99147 folder, including `compiler-resources`, font, shaders,
`sce_module` and `assets/README.txt`. Do not merge with PPSA99145/PPSA99146.
Close other titles, start klog capture before launching, then press Cross.
Wait for PASS/FAIL and press Circle; preserve capture through process termination.
If both display and heartbeat stop, close through the console shell after two
minutes. Retain the full private klog even if the run folder was not created.

Return the klog and `/data/sims2-compiler-probe/<run>` (or the fallback
`PPSA99147/compiler-runs/<run>`). A screenshot helps identify the visible stage.
Do not publicly post unrelated kernel records or network information.

```text
python tools/ps5/analyze_linker_probe.py <private-klog> --build-id <manifest-build-id> --output out/ps5-debug/<run>/capture-summary.json
python -m unittest discover -s tools/ps5 -p test_linker_probe_log.py
```

The parser requires complete compiler, I/O, linker, worker and shutdown records.
Kernel/process evidence is still required to confirm title termination.
Malformed-input fatal recovery, arbitrary native faults, output execution,
full game generation, private-title conversion/launch and ISO installation
remain separate gates. This package is not a PS5-only game release.
