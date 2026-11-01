# Owner-generated code: required release architecture

The existing f48a3b24 native executable is **excluded from public release** at
szampan's direction. This source preview includes no translated game source or
native game executable. It does not offer a playable public binary.

## What exists and what does not

- PASS on PC: build six host tools from the supplied source, extract the owner's
  supported XBE, and regenerate all 71 files (65 C++ units) byte-identically to
  the current independent generation. The source ISO is unchanged.
- PASS on PC: 20 Python tooling tests, six C++ UI/render/font policy suites,
  27 platform-contract checks and the synthetic installation transaction.
- Existing native private game build: links translated game code statically.
  Its ISO installer copies data; it is not an on-console compiler.
- NOT IMPLEMENTED: a public game-code-free runtime that generates/loads the
  complete private game module on PS5. No playable public replacement exists yet.

## Intended boundary

Public distribution: authored runtime/HLE, rendering, platform code, recompiler,
reviewed configuration, authored hooks, CC0 prompts/art and OFL installer font.
Owner-local output: XBE, extracted data, translated guest C++, machine code,
relocations, address/function tables and compiler cache. Do not publish them.

Target flow: ISO -> verify XBE -> generate owner-local code -> compile/link an
owner-local module -> validate module ABI/identity -> load -> boot the game.
The public executable remains unchanged. Data installation and code compilation
are distinct stages, with distinct progress/error states. Existing asset-ready
checks must not skip module generation or accept an incompatible module cache.
Keep the ISO until both data installation and code preparation have succeeded;
only then offer explicit deletion. A failed build must not remove the source.

## Concrete technical gates

The current generator emits C++, not directly loadable PS5 machine code.
`ps5/guest/CMakeLists.txt` statically archives 64 generated partitions and the
generated function mapping. Runtime dispatch uses `X86FuncMappings`,
`X86Continuations` and generated image constants. Authored hooks also call
generated `__imp__sub_*` functions. All of these bindings must be separated;
simply removing the guest archive leaves unresolved symbols.

The current SDK does expose `ps5_exec_alloc`/`ps5_exec_protect` for executable
memory in its platform layer. That is a potential supported allocation path,
not an ELF/module loader or a C++ compiler. Its use for this title has not been
tested. Never change firmware, enable hidden privileges or bypass console
security to make loading work; the homebrew environment is owner-prepared.

1. Define/version the runtime/module ABI, imports, guest context layout,
   function/continuation tables and authored-hook transitions.
2. Prove real generated code as a separate module on the PC; test callbacks,
   exceptions, thread state, address mapping and current gameplay regressions.
3. Select/port a PS5 code-generation toolchain. Embedding only XboxXBERecomp
   supplies C++ output; an on-console native compiler/linker or another complete
   backend is still required. None is included or ported in this preview.
4. Prove PS5 loading using supported platform services, relocation/range checks,
   cache identity, atomic promotion and bounded failure/retry handling.
5. Repeat console bring-up and performance measurements. Previous static-build
   gameplay results do not validate a new module architecture.

Do not distribute encoded/compressed/encrypted translated game code as a
substitute for deriving it locally. A generic executable loader by itself is
also insufficient. PC-local compilation is the demonstrated intermediate path;
the requested ISO-only PS5 workflow is not currently achieved.

This architecture reduces what original material is distributed; it is not a
legal certification. Original-game rights and dependency licensing require
their own review. No conclusion about another project's legality is asserted.

## Comparison consulted 1 November 2026

GTA IV's PS5 BUILD.md explicitly says recompiled code is already in its repo;
its package script links CMake object/archive inputs into the PS5 PIE/SELF.
Its first-run ISO installation is separate. It therefore is not evidence of
an implemented on-console C++ translation/compilation path we can reuse here.

- https://github.com/elripalda/gta4-ps5/blob/main/ps5/BUILD.md
- https://github.com/elripalda/gta4-ps5/blob/main/ps5/tools/ps5_package.sh
