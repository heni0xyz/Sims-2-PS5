# Console compile/link/load/execute diagnostic

Authored adapter: OpenAI Codex for szampan, GPL-3.0-only. The unchanged loader and
linker layout are Mihawk's PS5_RetroArch work, GPL-3.0-or-later, at
[7268510b12fe7461342e0cc4497c87a0910c1055](https://github.com/mihawk-99/PS5_RetroArch/blob/7268510b12fe7461342e0cc4497c87a0910c1055/src/core_loader_ps5.cpp).
Original files, hashes, origin and license are under third_party/ps5_retroarch.

PPSA99147 f00dde58ac5ec7c4 **PASS on PS5**, 8 November 2026. Whole worker:
3.699577 seconds. It compiles its own C++, checks I/O, recovers from an expected
syntax error, links twice around an expected unresolved symbol, then:

1. Requires the checked no-import ELF fixture and its expected definitions.
2. Rejects an object file as a module with the unchanged bounded loader.
3. Loads first.elf, looks up sims2_probe_entry, calls it for 16 fixed input/output
   pairs independently computed with Python arbitrary-precision arithmetic.
4. Unloads, repeats with repeat.elf, and requires all 32 answers to match.
5. Requires executable-region/byte counts to return to their original values.
6. Reports PASS, joins and exits normally through the foundation's frame budget.

The fixture modules use default-visible exports, LLD's elf_x86_64_fbsd emulation
and the upstream 16 KiB RX/R/RW linker layout. No import table or separate C/C++
runtime is embedded in the fixture. The adapter rejects every external import;
the zero core-thread count applies only to this validated no-import pure function.
It must not be reused as a general game-module import/thread implementation.
The upstream loader performs its normal bounds, feature and protection checks,
using the pinned SDK's executable direct-memory API and actual title unwinder.

## Reproduce

Use CONSOLE_COMPILER_TEST.md's prepare/package commands with **--execute** added
to both. This option requires --mode compiler and is recorded in the manifest.
The original compilation-only profile remains available without it.

```text
python tools/ps5/run_compiler_probe_console.py --console-tools <PS5_Vulkan>/tools --result out/console-compiler-probe/package-result.json --output <fresh-private-evidence-directory>
```

The existing tools use PS5_HOST, FTP_PORT, KLOG_PORT and PS5VKCTL_PORT. Load the
standard control payload into the owner's already prepared environment once per
boot. No firmware or security changes are part of this workflow. The wrapper
refuses to deploy or launch over an active application, verifies remote eboot,
param and critical resource hashes, stages the foundation's native bounded test
hook, captures before launch, fetches the run folder and checks the final idle
state. ftpsrv's per-connection SELF mode provides exact raw executable readback.
Failed or disconnected captures never become a passing run. Full private logs
stay outside source control.

The parser automatically recognizes execution-required worker records, or can
be forced to require them with --require-execution. It checks all 32 calls,
the invalid-module rejection, both unloads, memory counts and normal lifecycle.
The normal console run must be owner-authorized; no deployment is done by builds.

## Limits

This proves native execution of a tiny authored fixture compiled on PS5. It
does not compile or run The Sims 2, prove guest semantics, support arbitrary
game imports/TLS/threads, validate full compiler memory use, implement a complete
private game module, or complete ISO installation. The returned synthetic ELFs
are 50808 bytes and byte-identical, SHA-256
f33d9e446fd4c2059e2ace2d8ea5edd0d4276fe063c78405707ef5b02f00a743.
Evidence is recorded in the newest progress entry, with matching symbols kept
locally. Installer cleanup of compiler/linker temporary placeholders remains open.
