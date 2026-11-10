# First console compiler test: getcwd import fault

Build 14fb8665bc404469, PPSA99147: **console test FAIL**. This supersedes the
NOT TESTED status recorded when that package was delivered. Original private
klog and intake SHA-256 are retained under ignored
out/ps5-debug/compiler-crash-20261108-14fb8665. No console was contacted during
analysis, and no game or saved data was modified.

## Evidence and first failure

The capture matches the delivered build ID. Vulkan initialized at 3840x2160,
the worker started with its 16 MiB stack, and a run folder opened under
/data/sims2-compiler-probe. All five critical resource hashes match.

The kernel records SIGSEGV, instruction-fetch page fault, RIP equal to the
unmapped fault address 0x100026d486. The title's text base is 0x400000. Subtracting
that base from the kernel's return addresses and using the matching debug ELF:

| Return address | ELF-relative address | Symbol |
| --- | --- | --- |
| 0xaeaed6 | 0x6eaed6 | llvm::sys::fs::current_path |
| 0xabbd84 | 0x6bbd84 | RealFileSystem::RealFileSystem |
| 0xaaa9b2 | 0x6aa9b2 | llvm::vfs::createPhysicalFileSystem |
| 0xb5cbc4 | 0x75cbc4 | sims2_compiler_compile_driver |
| 0x51946f | 0x11946f | Run::compile |
| 0x51500d | 0x11500d | compile_inputs / execute / worker |

Disassembly places the first return address immediately after current_path's
indirect getcwd call. The package's undefined-import inventory contains getcwd.
The pinned SDK header and directory.c explicitly document that a title's libc
getcwd calls an unavailable __getcwd import; ps5_getcwd implements the operation
through directory traversal instead. Its real definition is present in the
exact hash-verified platform archive already linked into this title.

The physical VFS snapshots the process directory in its constructor even though
the wrapper immediately supplies a separate absolute working directory. The
earlier API comment overlooked this constructor call. Resource and kernel log
lines interleave, and the capture does not contain the preceding I/O results or
compiler-start records. No I/O PASS, successful compilation or link is inferred.
The per-run probe.log would supply the flushed application records if returned.

## Correction

Extend the existing captured-recipe binding inventory with getcwd=ps5_getcwd
and realpath=ps5_realpath. Both real platform definitions are verified before
linking. The latter is also called during VFS initialization and was another
unbound platform-covered import. No libc replacement was invented and LLVM's
filesystem behavior is preserved.

The wrapper now reports driver/frontend physical-VFS construction and successful
working-directory setup. The header documents the actual constructor dependency.
Packaging additionally rejects any platform alias that remains an undefined libc
import, even if the old public-kit allowlist admitted that name.

An audit found sysconf and pthread_exit also have platform implementations while
remaining imported. They are not blindly aliased: ps5_sysconf delegates other
queries to sysconf, so an unconditional defsym would recurse. Neither is the
reported call site. Preserve the established thread recipe and investigate only
with relevant source/console evidence.

Rebuild the affected interface and probe objects, link through the same RADV
recipe, verify local getcwd/realpath definitions and absence of their imports,
then repeat the same bounded console test. A successful new package establishes
BUILD PASS only; on-console correction remains NOT TESTED until a new klog.

## Corrected delivery

Candidate a72d982ec4cb8260, package-attempt-0003: native build/link/import/SELF
checks PASS. Local symbol inspection verifies getcwd and ps5_getcwd both resolve
to ELF 0x517d080, and realpath and ps5_realpath both resolve to 0x517bd10. Neither
name remains an undefined runtime import. Matching debug ELF is retained.
All 3076 title files match the verified ZIP. Corrected console runtime is
NOT TESTED. The test still requires no game data and executes no generated ELF.
