# Source snapshot

The published repository includes `recomp/`, the complete local SDK sources,
`PS5_Sims2Installer/`, `PS5_Sims2Independent/` and `vendor/PS5_PayloadSDK/`.
Local commit IDs are provenance; no unpublished submodule SHA must be fetched.
The console compiler's public LLVM source pin is recorded in repositories.lock.json.

The source ZIP can be reproduced with the repository-root tools/package_source.py.
It excludes ISO/XBE, extracted game data, generated guest code, compiled titles,
private logs, credentials and personal save files. Original authored artwork,
legal input prompts/fonts/music and their notices remain in source.

Player installation uses the PS5-only launcher and the owner's supported USA ISO.
Maintainers still need public build tools and the pinned native/RADV dependency
build before compiling the launcher itself. See repository-root BUILD.md for
commands. The old prepare_source_release.py/verify_source_release.py pair is for
historical tar snapshots using the adjacent sims2-independent layout.

## Checks without game data

```text
python -m unittest discover -s tests/ps5
python tools/test_ui_aspect.py --clang <path-to-clang++>
python tools/test_platform.py --clang <path-to-clang++>
python tools/test_install.py --clang <path-to-clang++>
```

The first command runs from `sims2-independent/`. The compiler tests need only
public headers/source and do not load an ISO or contact a console.

## Current feature boundaries

New native builds support first-run extraction from a verified owner ISO,
optional explicit source-ISO deletion after success, 16:9 presentation,
edge-anchored HUD, 4K raster targets and host-only CC0 DualSense icons. Optional
4× font reconstruction uses owner atlas pixels at runtime and preserves glyph
metrics; it does not include/recreate original vector font outlines. The
Continue-menu readiness refresh and font quality need their matching PS5 test.
The 60 FPS cap is not proof of sustained 60 FPS. See `docs/PROGRESS.md` for
measured status and open issues.

English PlayStation/storage-device notices are patched in the newly extracted
private copy. The separate read-only host ISO diagnostic retains original text. Original
UI art/static family portraits are not included in the public texture pack.

Keep public source releases separate from private native title artifacts.
The native game eboot contains translated original game code; this source
license does not grant rights to that original game. No publication is
performed by these scripts.
