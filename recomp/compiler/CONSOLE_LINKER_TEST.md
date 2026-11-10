# Console linker diagnostic title

This is an isolated development test, PPSA99146. It is not a PS5-only installer
or a new playable Sims build. The working PPSA99145 title is unaffected.
Authored integration: GPL-3.0-only, OpenAI Codex for szampan.

## Build status

On 7 November 2026 I finished the native LLD libraries and packaged build
`ffceb5ef9749efa3`. Linkage, runtime-import checks, native conversion, signing and
SELF integrity inspection pass. The four compiler/linker interfaces and complete
probe foundation cross-compile. A tester has now verified **synthetic linking on PS5: PASS**.
The three-case worker scenario took 0.494429 seconds; both valid output files
are binary identical. C++ compilation and linked output execution remain
NOT TESTED. See `docs/CONSOLE_LINKER_RESULT_2026-11-07.json`.
The libraries and probe foundation are tracked independently:

```text
out/console-compiler/build-linker.json
out/console-compiler/build-linker.log
out/console-linker-probe/build-probe-objects.json
out/console-linker-probe/build-probe-objects.log
```

A packaged title exists at `package-attempt-0005/PPSA99146`, with its matching
`build-manifest.json` and debug `llvm-pie.elf` in that attempt directory.
The validated tester ZIP is in the workspace's
`outputs/PS5-Ready/LinkerProbe-ffceb5ef9749efa3` folder. The returned klog and run folder are independently reviewed; see the newest
entry in `docs/PROGRESS.md` for timing, shutdown evidence and the temporary-file
cleanup follow-up.
Retry directories preserve failed attempts; `package-result.json` points to
the latest successfully packaged title and its manifest.

## Reproduce

The official PS5_VulkanTemplate `ps5/tools/new-title.py` creates the foundation:

```text
python ps5/tools/new-title.py <workspace>/PS5_Sims2LinkerProbe --title-id PPSA99146 --name "Sims 2 Linker Diagnostics" --refresh 60
```

From the independent project, using the saved maintainer tool settings:

```text
python tools/ps5/build_console_compiler.py --settings out/console-compiler/build-settings.json --phase build-linker
python tools/ps5/build_linker_probe.py --settings out/console-compiler/build-settings.json --phase prepare --platform-headers <pinned-sdk>/platform/include
python tools/ps5/build_linker_probe.py --settings out/console-compiler/build-settings.json --phase package --platform-headers <pinned-sdk>/platform/include
```

The native library build and probe object build use different Ninja directories.
Never start two writers in either directory. `prepare` can run while native LLD
builds; `package` requires its PASS receipt. No tool deploys or executes on a
console. Development-PC execution tests of the compiler/game are not needed.
After `prepare` passes, `--phase finish` can replace the final `package` command
to wait for the current native library job. It does not start another Ninja,
deploy, retry a failed link blindly or override the runtime import check.
Its PID/status are saved in `state-finish-probe.json`; stop that watcher too
when pausing before shutdown.

Packaging preserves the captured template/RADV startup, heap/thread wraps,
driver linkage, platform bindings, conversion and signing commands. Only the
application objects and native LLD dependency archives are substituted. Additional
verified SDK bindings follow the captured recipe's `--defsym`/local visibility
pattern. The PS5 title has no jemalloc/brk arena: a Process.cpp-only capability
header selects LLVM's existing no-counter fallback. LLD's documented
`-z nodynamic-undefined-weak` mode resolves LLVM's absent optional TLS initializer
statically; required imports still fail checks. I verified the final TLS initial
value and wrapper disassembly. Full failure/fix evidence is in `docs/PROGRESS.md`.
Packaging
checks native ELF layout, rejects game symbol tables and unverified imports,
and retains `llvm-pie.elf` locally for symbolization. It never changes the
working public kit or running game eboot.

## What the console does

After the tester starts klog capture and presses Cross:

1. Initialize a worker with explicit 16 MiB stack and IEEE floating-point state.
2. Try a fresh per-run folder under `/app0/linker-runs`, then
   `/data/sims2-linker-probe` if needed. Log every failed operation. Do not infer
   write permission from a mount point or hide errors with an installed marker.
3. Verify the three authored input objects by SHA-256 and ELF64LE x86-64 ET_REL.
4. Link two objects with embedded LLD to a shared synthetic ELF, map and build ID.
5. Check ELF64LE x86-64 ET_DYN, required definitions and absence of named imports.
6. Link an intentionally unresolved input. Expect an ordinary failure (1).
7. Repeat the valid link, validate it and compare its SHA-256 with the first.
8. Emit an application verdict, save JSON and join the worker. Circle requests
   the foundation's normal shell shutdown. The automatic frame-budget harness
   reports failure when the application or independent wall-clock watchdog fails.

The output is a test ELF, not a signed runnable title. The probe does not load
or execute its output. LTO, malformed-input fatal recovery, arbitrary native
crashes, Clang compilation on PS5, game linking, private-title packaging and
launching remain separate required tests.

The render thread continues presenting and emits a heartbeat every two seconds.
A 120-second wall-clock timeout checked by that thread records failure and closes through the foundation; it does
not cancel a thread inside LLVM or destroy live worker state. LLD diagnostics
are preserved in bounded continuation records, including split/long lines.
Events carry build ID, run ID, monotonic time and worker thread ID. Memory
availability is sampled at stage boundaries; this is not peak-memory profiling.
A GPU/driver hang can also stall the render thread: if frames and heartbeats
stop, close the title through the console shell after two minutes and retain
the kernel log. The UI watchdog is not an independent process monitor.

## Tester handoff, after packaging

Copy the complete PPSA99146 folder, including `assets/README.txt`, shader files,
public font and `linker-fixtures`. Do not merge it into the game title. No ISO,
Xbox files, saves or copyrighted game assets are needed for this test.

Start a full klog capture before launching, close other titles, then launch
Sims 2 Linker Diagnostics and press Cross. Wait for PASS or FAIL, press Circle
and retain the capture through title termination. Capture the screen if it
fails. Return the full private klog and the per-run folder; do not publicly post
unrelated system records or network details.

Analyze the returned capture without contacting the console:

```text
python tools/ps5/analyze_linker_probe.py <private-klog> --build-id <manifest-build-id> --output out/ps5-debug/<run>/capture-summary.json
```

An incomplete/truncated capture cannot pass. The application PASS covers the
synthetic links; successful process termination still requires the framework's
kernel/process evidence. No PS5-only release or performance claim follows from
this test alone.
