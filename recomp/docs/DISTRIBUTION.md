# Local preparation for a future release

No repository or release has been pushed/published by this work.
szampan selected GPL-3.0-only for project code and CC0-1.0 for the four title
artwork files. Dependencies retain their licenses; see LICENSING.md.

## ISO workflow

New installer builds prefer complete extracted `assets/`. When absent,
startup displays Cross Extract for exactly one root `.iso` beside `eboot.bin`
in PPSA99145. Checked streaming extraction and readback verify the USA XBE and
all copied files, apply the same PlayStation/storage-device notice patch, then
promote a private assets folder. About 2 GB extra space is required. A subsequent
explicit choice optionally deletes only that source ISO; it is never deleted
automatically. Saves and other files are retained. Incomplete existing assets
are never overwritten. See INSTALLER_WORK_2026-11-01.md for transaction details
and the outstanding hardware acceptance gates.

The separate host diagnostic still accepts a read-only streamed ISO through
`iso_verify` and `run_headless.py --assets <owner.iso>`. That path preserves
original notice text. Earlier native binaries up to 7b73 also stream ISO and
do not contain the new first-run installer. The public Kenney HD prompts,
16:9, render-scale and runtime font reconstruction do not require disc rewriting.
No original general UI/static family-portrait pixels ship in public resources.

## Source preparation

After committing the project, both SDK integrations and foundation:

```text
python tools/prepare_source_release.py --git <git> --output out/release-source/<new-id>
```

This exports HEAD trees without Git history, includes the local SDK integration
sources (so unpublished submodule commits are recoverable), retains licenses,
and inventories every file/hash. Archive paths restore sims2-independent/
with sdk/ and runtime-sdk/ inside it, plus the adjacent PS5_Sims2Independent/
foundation expected by the build configuration. It refuses game/private/generated paths,
game/native-binary extensions, executable magic, symlinks and unexpected large
files. Owner data, generated guest C++, linked game objects and personal saves
must stay excluded. PNG/PXZ artwork is source material; PXZ never enters the
PS5 title. The source archive is prepared locally; publication requires a
separate explicit instruction.

The gzip/tar metadata is deterministic. Every export is re-opened and checked
against `source-manifest.json` by `tools/verify_source_release.py`, including
exact member inventory, sizes/hashes and required licenses. Credential filenames
are rejected. `SOURCE-RELEASE.md` describes restore layout and remaining external
compiler/RADV prerequisites; a clean new-machine bootstrap is still outstanding.

## Binary distribution gate

The requested download-title + add-own-ISO UX is technically being implemented,
but a static-recompilation eboot already contains translated original game code.
An absent ISO/assets folder does not make that eboot purely project-authored.
The SDK's MIT license and this project's GPL do not grant rights to the original
game. Review original-game rights and the combined dependencies/corresponding
source before deciding what binaries can be published.

A source/builder/patch-tool distribution can instead generate the private game
code/artifacts from the owner's own verified dump on their PC. Fully compiling
that code on the PS5 itself would require a substantial additional toolchain;
the installer is not an on-console recompiler. No ready public native
game release or redistribution clearance is claimed here.

## Rebuild public shell assets

Use `tools/ps5/requirements-artwork.txt` in a workspace-local Python environment.
`prepare_branding.py` converts szampan's art into the template's icon/DDS inputs.
`build_hd_prompts.py` renders the retained CC0 SVGs at 256px with nine mip levels.
Its default build requires no game data; the checked-in selector fingerprints
identify the supported original/patched textures without containing their pixels.
Export includes these public resources, GPL text and artwork CC0 dedication.

The public installer font compiler uses the same Pillow dependency to build
`installer-font.s2font` from unmodified `third_party/opensans/OpenSans-Regular.ttf`.
Font derivatives retain OFL-1.1 and ship with the original TTF/copyright/license;
they are not relicensed under GPL. The installer source remains GPL-3.0-only.
