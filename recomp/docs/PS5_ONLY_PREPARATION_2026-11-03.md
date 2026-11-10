# Console-only owner-copy preparation: engineering decision

Author: OpenAI Codex, for szampan. SPDX-License-Identifier: GPL-3.0-only.

Status, 3 November 2026: implementation started. In-process generator, frontend
interface and compiler cross-build prototypes exist; host generator parity and
native static generator build pass. Full native compiler libraries are building.
No complete console-only installation or generated-result launch is validated.
The owner reports that the public repository contains source and release files.
The current Windows builder remains the working fallback. This request does not
authorize publishing a new release or replacing it with an experimental build.

## Decision

Prefer an on-console port of the existing owner-copy generation/compiler pipeline
before attempting a new translator. Keep RADV, the renderer, HLE, platform code,
authored hooks, UI policy and public resources precompiled. Only the guest code
and tables derived from the player's XBE need generation and compilation during
installation. No RADV rebuild is needed on the player's console.

This is **partial compilation of the project**, but still compilation of all
required guest functions. It cannot currently be reduced to ISO extraction or
filling in a few constants in the existing full-game eboot. The static eboot
already contains guest function bodies and their control flow. Moving, encoding,
encrypting or renaming those bodies does not establish their absence from a
distributed release.

Recommended player flow:

1. Copy the public title and the supported USA ISO to the console.
2. Start the title; select Prepare. Verify the XBE SHA-1 before code generation.
3. Extract and apply authored data patches locally, with resumable progress.
4. Generate the existing reviewed C++ and compile the guest units on PS5. Persist
   checked intermediate results for retry after failure.
5. Initially link/package a complete private title using the existing evaluated
   recipe and precompiled infrastructure. Close the installer and start the
   prepared title through the owner's ordinary launcher, **if** its supported
   writable title-directory workflow is verified. A separately loaded guest
   module is an alternative requiring more runtime refactoring.
6. Play. Subsequent starts reuse the private result if ISO, compiler and ABI fingerprints
   match. Ask about deleting the source ISO only after successful installation;
   never remove it automatically.

The running public eboot must remain unchanged. Write the private title to a new
verified writable directory; do not assume `/app0` is writable, replace a mounted
eboot, or assume a generated title will automatically appear in the launcher.
This first route preserves the current game ABI and RADV link recipe, avoiding
a game-module loader initially. Its gate is that the configured launcher can
normally start the generated title without a PC. If that supported workflow is
unavailable, use a private module in writable data storage and a narrow versioned
runtime interface; that requires replacing the present link-time guest globals.

## Evidence from this workspace

- `builder/core.py`: GENERATE, COMPILE, ARCHIVE, LINK, import checking, native
  ELF validation and installation are separate stages.
- `tools/builder/export_native_recipe.py`: excludes `libsims2-guest.a` while
  exporting the evaluated template/RADV link recipe and public native inputs.
- `tools/builder/assemble_windows_kit.py`: the current public kit has prebuilt
  infrastructure, SDK headers and host tools. It rejects generated function
  definitions in exported native inputs. Windows `.exe` tools cannot be used
  directly as PS5 compiler binaries.
- Current code generation has 71 files and 65 compiled units, about 125 MiB of
  generated source. The Windows owner-copy test took 811.629 seconds end to end
  on this PC; that is not a PS5 installation-time estimate.
- `runtime-sdk/runtime/cpu/x86_context.h`: guest calls use `X86Context` plus a
  guest-memory base pointer; function mapping tables are presently link-time
  globals. These are useful seams but are not yet a module ABI.
- The vendored SDK has `platform/include/ps5platform/exec.h`, including executable
  allocations and a toggled RW/RX mode. Its documentation reports upstream
  console probes. This is not a successful loader test in this game/title.
- The SDK's general `crt/rtld_so.c` also contains kernel-assisted protection
  handling. Do not assume generic `dlopen` is a drop-in safe title solution or
  introduce privilege changes. Use only the already available supported platform
  service and fail clearly when unavailable.

## Alternatives

| Route | Reuse of working guest semantics | Main cost / uncertainty |
| --- | --- | --- |
| Embedded Clang/LLVM + LLD, complete private title | Highest; retain generator, ABI, flags and evaluated link recipe | Port compiler/packager, bound memory, prove writable output plus normal launcher workflow |
| Same compiler, separate guest module | High; functions retain semantics | Also refactor link-time dependencies and prove module ABI/loading |
| Direct x86-to-native backend / copy-and-patch | Can reuse HLE and patches, but needs a new code generator | Flags, x87/SSE, exceptions, indirect calls, callbacks and optimization all need differential tests |
| Decoded bytecode interpreter | Reuses platform/HLE but changes CPU execution engine | Extensive instruction semantics work; 4K60 performance is unproven and may regress |

Copy-and-patch uses generic precompiled instruction fragments, with addresses,
constants and ordering derived locally from the player's code. Fragments must
not be precompiled Sims functions in disguise. It still generates native code
and needs supported executable memory/loading. It avoids shipping a full C++
frontend but is a larger game-correctness project than moving the existing
compiler stage. It should be a later size/startup optimization, not a claimed
quick fix.

## First implementation gates

For the initial complete-title route:

1. Confirm the existing launcher supports starting a generated title in a new
   writable directory after the installer has exited. This is an owner-authorized
   title workflow, not firmware modification. No automatic self-replacement.
2. Embed a PS5-built Clang/LLVM and LLD and compile a synthetic C++ example on
   PS5. Package it with the existing native conversion/signing implementation and
   title resources. Start the result through that normal launcher and check its
   expected result/log. The compiler must run on PS5, not merely target PS5.
3. Port the XBE parser/generator, then process one real function and all units,
   retaining compiler flags and bounded resumable work. Link the existing full
   native title from its prebuilt public inputs and locally generated guest
   archive. Verify install, launch, saves, Continue and representative gameplay.

If a guest module is needed instead, its additional gates are:

1. On PC, make a synthetic generated module with a versioned C entry interface;
   pass runtime services explicitly rather than exporting all runtime symbols.
   Test guest memory, integer flags, FP state, indirect calls, runtime-to-guest
   callbacks, guest-to-runtime calls and thread-local context.
2. On PS5, load a small synthetic module from writable storage, execute it, check
   results and exit cleanly through the existing title lifecycle. Validate ELF
   ranges, relocations, imports and ABI before execution. Do not unload code while
   threads or callbacks can refer to it. Use the SDK's supported RW/RX service;
   no firmware or security changes.
3. Port an embedded compiler invocation to produce that same synthetic module
   **on PS5**. Include only the required frontend, x86 backend, integrated
   assembler and ELF linker, not unrelated targets/tools. Match the pinned
   compiler's target support and flags; do not silently substitute an unmodified
   host compiler that lacks the PS5 target. LLD can be embedded without spawning
   an external process; Clang driver subprocess assumptions must be removed.
4. Generate and compile one real function, then all 65 units, with bounded
   concurrency, memory measurements and restartable work. Do not change O2 or
   floating-point semantics merely to shorten installation.
5. Convert the guest mapping globals/continuations/hooks to the module ABI;
   test actual game startup, saves, repeated Continue, gameplay and frame times.
   Preserve the proven RADV link recipe for the public runtime. A game-only
   module needs its own reviewed recipe; do not relink RADV as an improvised
   dynamic dependency.

Stop at the first failed gate and record diagnostics. No installation-time,
60-FPS or compatibility promise is supported until measurements exist. Upstream
executable-memory support alone does not prove this architecture works.

## Distribution boundary

There is no general percentage of precompilation that makes a release legally
safe. A practical lower-risk design distributes generic tools/runtime and derives
game-specific instructions from the owner's copy locally. Public infrastructure
may be distributed precompiled when its licenses and corresponding-source
obligations are met. Compiler output does not automatically become GPL because
the tool is GPL; original-game rights and GPL compliance are separate questions.

Neither this design nor ISO possession guarantees legality in every jurisdiction.
EU Directive 2009/24/EC separately protects reproduction, translation/adaptation
and distribution, with conditional lawful-use/interoperability exceptions.
This document is an engineering boundary, not a legal clearance for EA code.

Primary sources checked:

- https://www.gnu.org/licenses/gpl-faq.en.html#WhatCaseIsOutputGPL
- https://eur-lex.europa.eu/legal-content/EN/TXT/PDF/?uri=CELEX:32009L0024
- https://clang.llvm.org/docs/Toolchain.html
- https://lld.llvm.org/ (embedding and cross-linking)
- https://compilers.stanford.edu/publications/copy-and-patch/
- https://github.com/heni0xyz/Sims-2-PS5

## Cleanup and retained work

Cleanup manifest: workspace `work/maintenance/20261103/cleanup-completed.json`.
Removed old private data test copies, run-cache Z partitions, repeated executables,
uncompressed BMP/TGA captures and superseded builder packages. Retained runtime
logs/summaries and non-Z save partitions, current source/history, owner ISO and
one extracted asset set, current full owner-built title/debug artifacts, portable
kit/public native archives, LLVM sources/toolchains and the RADV build VM.
Do not delete that VM solely because public release files exist: it retains the
expensive build environment and is useful for the proposed compiler port.
