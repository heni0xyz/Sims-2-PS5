# Release direction: ISO-only setup on PS5

## Current decision: one-time PC/Mac preparation

The owner subsequently approved a PC/Mac builder and consolidated public source
in Sims-2-PS5. The earlier console-only requirement below is historical.
`builder/` generates/compiles private game functions from the owner's ISO and
reuses prepared public libraries. A separate public PS5 instruction app uses
Open Sans; it is replaced with the owner-built full title while the app is
closed. No dynamic game-code module loader or on-console compiler is required.
The current working private binary is preserved and excluded from public kits.
See builder/README.md and docs/BUILDER_RELEASE.md for current evidence/status.

## Latest steering: preserve the working build; avoid a major rewrite

Later on 1 November the owner clarified that the immediate priority is a public
release with assessed EA/legal risk, not a large compiler/runtime rebuild. The
on-console compilation direction below is historical context, not approval to
start a lengthy build solely to resolve uncertain legal concerns. No compiler
build has started, no game runtime source has been changed, and the existing
working native candidate is preserved. Public LLVM/loader sources were inspected
and downloaded; the temporary idle QEMU VM is shut down.

The earlier categorical statement that the existing binary cannot legally be
released was not a legal determination and was too strong. What is established
technically is that it contains translated guest code. Requiring the owner ISO
does not establish the original code's absence from the binary. GPL compliance
and original-game redistribution rights are distinct questions. On-console
compilation is not mandated by GPLv3 and does not certify legality by itself.

EA's current content policy was checked:
https://help.ea.com/en/articles/security-and-rules/ea-content-policy/
It restricts noncommercial fan content, expressly withholds permission for game
revivals/fan games, and reserves enforcement rights. It does not expressly grant
permission for this specific port. That policy is not a judicial determination
of statutory rights/exceptions in any jurisdiction. See RELEASE_LEGAL_BRIEF.md
for technical facts to review before a binary publication decision. No contact
with EA, upload, publication or legal clearance has occurred.

## Earlier ISO-only requirement and investigation

Owner requirement, 1 November 2026: the player must not compile anything on a
PC. The target UX is download a public title, put the owner ISO beside it,
and let the PS5 prepare everything. The previous static game eboot is explicitly
excluded from public release. No push, upload or publication is authorized.

This is a requirement, not a completed implementation. The present installer
extracts/patches data; the existing eboot already contains translated guest code.
XboxXBERecomp currently emits C++, so on-console generation additionally needs
a native compiler/backend, linkage and a supported loader/module ABI. These
are not implemented here. A source-only preview and PC generation test do not
satisfy the requested console-only workflow. Preserve the working private build.

Public generic infrastructure can be distributed precompiled subject to its
own licenses and corresponding-source requirements. This includes RADV and
the platform layer; the player need not rebuild RADV. Mesa is predominantly
MIT, with per-file licenses; the PS5 Vulkan/platform integration contains
GPL-3.0-or-later components. Preserve those notices and the exact build sources.
This is distinct from any rights in translated original game code. A combined
GPLv3 distribution does not erase the original permissive grants/notices.

## RecompStore inspection

Inspected public tree 9d9216bea8388569cb2a1c8856acb5c8ad92342a:

- app/payload/installer/installer.cpp downloads/verifies a ready store ZIP
  from GitHub releases (or uses an embedded fallback), then unpacks it.
- app/payload/copier/copier.cpp dispatches unzip/STFS/copy/tree/delete/
  backup/restore jobs, and reads/copies disc files. Its extraction protection
  excludes eboot.bin, sce_sys and sce_module from disc-data overwriting.
- Its Makefile compiles those helpers on the development host; it does not
  embed a C++ CPU compiler. Names such as lzma/Compiler.h are not a compiler
  service. No on-console CPU-code generation was found in the inspected
  installer/copier implementation. This is not an audit of every catalog app.

Primary sources:

- https://github.com/elripalda/RecompStore/blob/9d9216bea8388569cb2a1c8856acb5c8ad92342a/app/payload/copier/copier.cpp
- https://github.com/elripalda/RecompStore/blob/9d9216bea8388569cb2a1c8856acb5c8ad92342a/app/payload/copier/Makefile
- https://github.com/elripalda/RecompStore/blob/9d9216bea8388569cb2a1c8856acb5c8ad92342a/app/payload/installer/installer.cpp
- https://docs.mesa3d.org/license.html
- https://github.com/mihawk-99/PS5_Vulkan/blob/main/NOTICE.md

Next engineering gate: prove the game-code/runtime boundary and an executable
owner-generated module on the PC, then port the generation/compiler/loader path
to PS5. PC testing is a development step; it must not become a player prerequisite.
Do not claim feasibility, installation time, performance or legal clearance
before the relevant evidence exists. Never distribute encoded translated game
code as a substitute for deriving it locally from the owner's copy.
