# First-run installer and Open Sans â€” 1 November 2026

PC-only implementation. No console contact, deployment or publication. Previous
7b73 native delivery and cab0c92 source snapshot remain preserved separately.
Native9bb7544b is built/linked/locally validated; host passes below are not
PS5 acceptance. See DELIVERY_INSTALLER_2026-11-01.md for the exact artifacts.

## End-user flow

The title ID is **PPSA99145**. With no complete extracted assets, put exactly
one legally supplied USA `.iso` beside `eboot.bin`. Startup shows centered
`No game data detected; extract from .iso in PPSA99145` and a filled-white
DualSense Cross Extract prompt. Cross starts checked extraction; the screen
shows actual copied/verified byte progress and explicit errors/retry.

After data is validated and promoted to `assets/`, ask whether to delete the
source ISO: Cross Delete / Circle Keep. Releasing Extract and making a new
press is required. Closing the app keeps the ISO. Deletion rechecks that the
installed data is ready and that the literal source file has the same path,
size and modification time. No other file or directory is deleted.

Complete legacy extracted assets bypass the installer. Incomplete existing
assets are reported and never overwritten. Existing saves remain intact.

## Transaction and resources

`src/game/install*` streams 1 MiB chunks to owned `.sims2-installing`, validates
safe paths/limits, reads back and SHA-1-checks every file, flushes files with
fsync, patches only exact English visible notices, validates the USA XBE and
required resources, writes an installed-files receipt, then renames the
staging folder to assets. Retry cleans only inspected known staging contents;
links and unknown files block cleanup. Receipt-based installs check all member
sizes on startup and the expected notice archive hash. A changed or failed
installation cannot trigger source deletion. The owner ISO is read-only while
extracting. At least the extracted size plus 64 MiB spare space is preflighted.
The current disc extracts about 1.99 GB, in addition to the source ISO.

Known patched notices require a fresh Xbox Z scratch cache: the existing
`hdd/Z` is renamed to `hdd/Z.before-notice-install` once, without overwriting
an existing backup. A version marker makes repeat startup idempotent. E/UDATA,
TDATA and saves are not modified by this cache preparation. PS5 extraction
speed, writable /app0 behavior and interruption recovery remain hardware gates;
the SDK documents possible write throttling during large writes.

The asset-free UI uses the existing Vulkan backend and template pad/services.
Its short-lived display is destroyed before game graphics initialization.
Runtime SDK c4fbcb6 fixes first presentation of an undrawn CPU-dirty surface;
the original first-black-frame evidence is retained. RADV/link recipe unchanged.

## Font and licenses

Open Sans Regular 3.003 supplied by szampan is unmodified, 130832 bytes,
SHA-256 33e93bec67d91c396876db50694213802a39e43a911bb5c22322f0dbdf4d5e43.
Its embedded license and the official Open Sans OFL agree on SIL OFL 1.1.
`third_party/opensans/OFL.txt` retains copyright and license; upstream license
reference bd7e37632246368c60fdcbd374dbf9bad11969b6. The supplied TTF is not
claimed byte-identical to the current upstream TTF.

Pillow 12.3.0 builds antialiased Basic Latin coverage, metrics and kerning at
12/22/32 px. The versioned/checksummed 104811-byte public pack has SHA-256
d10095369fb555cc5d77e62fc612208c82b8adb7190057402fdc091dfa198800.
The TTF, pack and OFL copyright/license accompany the title. Project code stays
GPL-3.0-only, font resources OFL-1.1, title artwork/Kenney prompts CC0-1.0.
This changes the installation UI font, not the owner's in-game bitmap font.

## Executed host evidence

- `tools/test_install.py --clang <documented-clang++>` PASS, latest fixture
  `out/install-tests/20261101T123619.709558Z`: copy/readback, promotion, cancel/
  retry, unknown preservation, invalid paths/duplicates, patch failure, corrupt
  font, proportional metrics and fresh-button confirmation/deletion guards.
- Actual owner ISO install PASS in `private/installer-owner-final-test`, log
  `out/host/install-owner-final.log`: source unchanged; original file checksums
  verified, notices patched and data promoted; old synthetic Z archived,
  synthetic E save retained, repeated cache setup idempotent. Changed source
  deletion refused; explicit synthetic fixture deletion succeeds. The Desktop
  owner ISO was never deleted.
- `Sims2InstallerProbe` PASS using actual recording/Vulkan paths with the local
  SwiftShader ICD. Five real startup screenshots, first frame visible, resource
  counts return to baseline, actual game hook registration and second backend
  initialization succeed. Evidence: `out/installer-vulkan-opensans/`, reviewed
  first/deletion screens. Output is 1280x720 PC evidence, not PS5 evidence.

## Next acceptance gates

Owner-authorized PS5 testing of
first launch, real extraction, Keep/Delete choices, relaunch, errors/cancellation
and clean title shutdown. Stable60 and intermittent thin Continue are separate
open game issues. Nothing here claims either fixed.
