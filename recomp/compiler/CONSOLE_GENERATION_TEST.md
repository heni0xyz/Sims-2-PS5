# Console source generation and first game-unit gate

Authored integration: OpenAI Codex for szampan, GPL-3.0-only. XboxXBERecomp's
PS5 control-file adapter remains MIT. LLVM and the unchanged loader retain their
existing notices. No game executable, generated source or game object ships here.

## Verified generation

I ran f622a1b9ef81c744 on my authorized PS5. The supported XBE was already verified
and staged separately under /data/sims2-compiler-inputs/default.xbe. Its SHA-1 is
checked again on the console before generation. No ISO/XBE is in the title.

The native generator passed all 71 reference file hashes, refused an existing
output directory and passed all hashes again. I independently compared the
returned records against the reviewed Windows builder's generated_files map.
The preceding synthetic compile, I/O, linker, load/call/unload checks also passed.
The title exited normally and the final process query confirmed idle. Exact
timings, revisions, run identity and evidence location are recorded in
[the result](../docs/CONSOLE_GENERATION_RESULT_2026-11-08.json).

The first generator package failed the existing import check on pathconf.
libc++ filesystem operations pulled a monolithic object referencing that symbol.
I replaced only the tool's control-file operations with a PS5 stat/mkdir adapter
in the SDK; analysis, decoder and code-emission algorithms remain unchanged.
Host builds keep std::filesystem. No unsupported import was allowed through and
no fake libc implementation was added. The SDK pin and local patch are recorded
in repositories.lock.json. Public submission is deferred by the instruction to
keep this work local; no repository or PR was published.

## Next candidate: one real unit

effac3b70e2d9484, PPSA99147: **BUILD PASS, CONSOLE NOT TESTED**.
The console was unreachable, then I was told it would be started later.
No deployment or launch of this candidate took place.

The optional private tester ZIP is under
out/console-compiler-probe/delivery-effac3b7. All 3086 title files and the archive's
CRC/member hashes pass the saved manifest. Debug symbols remain in
out/console-compiler-probe/package-attempt-0009/llvm-pie.elf. This does not upgrade
the candidate's console status. The normal direct-test runner uses the title
folder from package-result.json and does not need this ZIP.

With --game-unit the existing diagnostic additionally compiles generated
x86_recomp.0.cpp at O2. It preserves the reviewed builder's target, guest CPU
headers, PIC, exception/TLS and floating-point flags, replacing only resource
paths and the compiler's matching LLVM 22 builtin headers. Sources are generated
privately on PS5; the title includes only public headers and tools. The result
must be an ELF64 x86-64 relocatable object with a logged SHA-256.

This is intentionally one unit: its memory and time are not yet known on PS5.
The whole worker remains bounded by 120 seconds, with two-second heartbeats,
before/after memory samples and all compiler arguments/diagnostics in klog and
the per-run probe.log. Memory snapshots do not measure peak consumption.
The object stays private. No generated game code is linked or executed.

## Reproduce

Use the existing private settings and pinned public platform headers:

```text
python tools/ps5/build_linker_probe.py --settings out/console-compiler/build-settings.json --mode compiler --execute --generate --game-unit --phase prepare --platform-headers <pinned-sdk>/platform/include
python tools/ps5/build_linker_probe.py --settings out/console-compiler/build-settings.json --mode compiler --execute --generate --game-unit --phase package --platform-headers <pinned-sdk>/platform/include
```

After each preceding command passes, and only with the authorized console's
services available and no other title running:

```text
python tools/ps5/run_compiler_probe_console.py --console-tools <PS5_Vulkan>/tools --result out/console-compiler-probe/package-result.json --output <fresh-private-run-directory>
```

The runner checks idle, deploys with the existing tools, verifies remote hashes,
captures klog before launch and requires both the capture and saved run result
to pass. It fetches flat artifacts (including the private object), leaves nested
generated source on the console, removes its own test hook and checks final idle.
It never closes a different active title. After a reboot the standard control
payload may need loading again through the already configured homebrew loader.

The parser also accepts --require-game-unit. Its verdict requires verified game
CPU headers, preceding execution/generation checks, a successful unit compiler
return, valid object hash and the explicit unit verdict. A source-generation
capture alone cannot pass. Current parser regression suite: 32 tests PASS.

## Remaining gates

First inspect the actual unit's result and native object before extending to the
full set. A full-game module needs the existing strong HLE/hook overrides, real
import binding, emulated TLS, exceptions/unwinding, thread ownership and correct
renderer lifetime. The current pure-fixture loader adapter rejects imports and
cannot act as a game adapter. Complete compilation, module linking/loading,
game startup, gameplay and resumable ISO installation are still unverified.
The established playable static title, its assets and saves are unchanged.
No total installation-time estimate follows from generation or the tiny fixture.
