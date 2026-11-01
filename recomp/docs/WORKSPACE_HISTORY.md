# Historical development README

This records the private development workspace before public source preparation. Use the repository README and PUBLIC_BUILD.md for this source layout.

# Sims 2 independent recompilation

Display title: **The Sims 2**. Project-authored code is GPL-3.0-only;
szampan's title artwork and Kenney prompts are CC0-1.0. Dependency notices
remain separate. See LICENSING.md for the exact scope and release boundaries.

This is a separate alternative workspace requested by szampan. The existing
working PS5 port remains its own project while upstream permission is pending.
It freshly translates the owner's USA executable with TGP482's public tools.
A native PS5 candidate has been built. Bounded native Windows tests reach the
island menu, character creation, save/reload and actual lots. Vulkan readbacks
verify corrected colours/alpha; CPU reference tests observe basic control and
simulation progress. Owner logs/screenshots verify limited PS5 gameplay;
full-game coverage, stability and sustained 60 FPS remain open. Read docs/PROGRESS.md to
resume and docs/PLAYABILITY.md for the gates.

The game-specific configuration, hooks and generated C++ from
abilliontokens/sims2-recomp are not inputs to this pipeline. The SDK is pinned
separately in repositories.lock.json. SDK code remains credited to TGP482 under
its MIT license. The MIT Linux/Vulkan runtime fork is an explicit dependency;
prior Codex PS5 adapters are listed in docs/PLATFORM_IMPORTS.json. This is not
a strict clean-room claim. New build glue is credited in CREDITS.txt.

Configure this directory with CMake/Ninja and Clang, build its four host tools,
then run `python tools/analyze.py --iso <your-ISO>`. Exact local commands and
results are recorded in docs/PROGRESS.md. The executable is extracted by the
upstream SDK's read-only Disc parser and must match SHA-1
16f9e41ce26c315a758f25d43ba1fa9d3809d1ad.

Game files, generated C++, dumps and binaries are ignored by Git. No ISO, XBE,
assets or saves may be published with this repository. This workspace has no
console deployment action. No existing runtime is described as newly authored.

tools/extract_assets.py extracts a complete private owner-disc directory with
a SHA-256 inventory, without overwriting existing data. tools/check_assets.py
checks required members and optionally the whole inventory. Root files
default.NGH and sims2.bin must accompany DATA, movies and default.xbe.

Use tools/recompile.py with the reviewed config/game.toml when resuming.
Build/test commands are in docs/BUILD.md and versions in docs/ENVIRONMENT.md.
The PS5 link uses the established template/RADV recipe. Unpublished local SDK
pins need their source bundles when restoring elsewhere. No automatic push,
pull request, publication or console test is part of this workflow.

For the source-only release layout, local SDK sources, excluded owner data and
remaining new-machine prerequisites, start with [SOURCE-RELEASE.md](SOURCE-RELEASE.md).

## Play a private PS5 test build

Use title folder **PPSA99145**. Put exactly one legally supplied USA Xbox ISO
beside `eboot.bin`; the supported executable SHA-1 is listed above. New
installer builds show **Cross Extract** when assets are missing, then checked
progress. Keep at least about 2 GB additional free space for extracted data.
After success choose **Cross Delete source ISO** or **Circle Keep**. Deletion
is optional and requires a fresh confirmation. Complete existing `assets/`
folders are used directly; incomplete folders are never overwritten. Saves
stay under `hdd/`; preserve that folder when updating.

The installer uses Open Sans under SIL OFL 1.1, with its original copyright/
license included separately. Authored program code remains GPL-3.0-only.
See [docs/INSTALLER_WORK_2026-11-01.md](docs/INSTALLER_WORK_2026-11-01.md) for
implementation, host evidence and outstanding PS5 tests. Earlier 7b73 and
older binaries stream ISO directly and do not contain this installer.
