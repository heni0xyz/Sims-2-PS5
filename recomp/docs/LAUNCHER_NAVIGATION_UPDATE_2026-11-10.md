# Launcher navigation and update candidate, 10 November 2026

## Official title correction: PPSA99145 (10 November 2026)

The requested ready candidate is now **03006e893030c4da / PPSA99145**, not the separate PPSA99148 development title. Native compilation/link/import/SELF checks and all 3,256 ready-file hashes PASS. The UI title macro, installer manifest and sce_sys title identity are PPSA99145. The ready copy preserves the existing PPSA99145 contentId/conceptId while retaining The Sims 2 branding. Pointer: out/ps5-launcher-99145/ready-result.json; sanitized result: LAUNCHER_PPSA99145_RESULT_2026-11-10.json. Build command adds --title-id PPSA99145 --output out/ps5-launcher-99145 to both prepare/package phases.

This is the PS5-only launcher/installer. Play requires a verified private prepared game/cache; a legacy standalone eboot alone is not that receipt. Existing complete assets can be used for preparation without re-extraction. Preserve assets, hdd and personal settings when merging the ready files. No console deployment or runtime acceptance was performed. The PPSA99148 transaction/UI test results below remain scoped development evidence.

This is a local candidate, not a published release or a console acceptance pass.
The latest title/debug ELF/manifest paths and build ID are recorded in
`out/ps5-installer/package-result.json`. The deployed console title remains the
earlier 1bd9b9f22e9e99bb build; this work performs no deployment or launch.

## Player-visible changes

- Play contains only a large centered filled-white DualSense X and Play label.
  The shared header/navigation remain, with no purple content card, carousel or
  feature list in this section. Install and Update keep the detailed layout.
- Activating Play fades the complete launcher to black over 180 ms, mutes the
  launcher track without changing the saved music preference, submits a plain
  black clear/present, and requests the private game handoff on the next frame.
  Input is frozen during the fade. Actual PS5 handoff/display timing still needs
  hardware validation. The font atlas is 320px for the large 4K Play label.
- Three explicit szampan-supplied 16:9 captures replace the carousel placeholders.
  Original JPEGs and PNG hashes/provenance are retained under artwork/installer.
  They are separate from the CC0 shell artwork dedication.
- L1/R1, D-pad and the left stick navigate sections. Up/Down or the left stick
  selects update actions. The right stick stays dedicated to the carousel.
- Navigation uses dominant-axis selection, dead-zone hysteresis and delayed
  repeat. A held stick cannot rapidly skip across the menu.
- The bottom selection slides in 160 ms. Content fades out for 70 ms and in for
  90 ms. Reversals start at the current position and opacity. Update row focus
  slides in 120 ms; carousel image/dot changes are animated as well.
- Unavailable/boundary choices recoil over 180 ms without changing selection.
  Both section and action rejection use the same 55 ms strong vibration; normal
  selection uses a light 40 ms pulse. These timings are software deadlines, not
  measured motor behavior on hardware.
- The bottom panel has a thin rounded rim and a restrained layered halo following
  the supplied reference. Visible glyphs remain vertically centered.
- Keep ISO consumes Circle without also triggering Close. Cache preparation must
  succeed before a completed private-build receipt can report Ready.
- Public placeholder files are written with exact LF bytes. The extraction
  library also accepts the exact legacy Windows CRLF marker, but refuses to
  remove an unknown README or a folder containing any additional file.

## Updater implementation

`compiler/public_update.cpp` is the actual transaction core, shared by host tests
and the console target. `installer_updates.cpp` uses the pinned PS5_RetroArch
HTTP2 handle/redirect recipe, retaining certificate and hostname validation.
Reads have per-operation timeouts plus overall metadata/download limits.

Only a newer stable release with the exact `Sims2-PS5-Launcher.s2u` asset and
GitHub SHA-256 digest enables Download and update. A release without this format
gets a manual-package message; it is never interpreted as a compatible update.

The deterministic S2UPD001 archive is an uncompressed, bounded JSON inventory
plus sequential payload. Paths are checked against a positive allowlist, with
traversal, duplicate/case aliases, special paths, wrong title/version and changed
runtime modules refused. Files stream through 64 KiB buffers with SHA-256 checks.
Path metadata uses the PS5 descriptor helper, avoiding title-blocked posix_stat.

The launcher stages public files, copies its current trusted eboot to the owned
update workspace, drains Vulkan and hands off to that helper with controlled
arguments. The helper journals old/new hashes, backs up managed files, atomically
replaces them and publishes root eboot last. It then restarts the root launcher.
Startup recovery precedes font/image loading. Unknown concurrent modifications
cause refusal; they are not silently deleted during recovery.

`hdd/`, assets, movies, settings.toml, source ISO, private game executables,
`.sims2-build` and `.sims2-launcher-prefs` never enter the update bundle. Native
game updates remain an explicit separate action using verified cached objects.
An owner-data patch version change still requires a reviewed migration and is
refused rather than silently replacing game data.

The helper requires unchanged sce_module files. It does not implement title
re-registration for the shell's Home artwork/music cache. Both that behavior and
real helper LoadExec remain hardware acceptance items before public release.

## Reproducible local checks

With the existing settings file and template-created PPSA99148 foundation:

```powershell
python tools/ps5/test_launcher_host.py --settings out/console-compiler/build-settings.json
python tools/ps5/build_installer.py --settings out/console-compiler/build-settings.json --platform-headers <pinned-SDK-platform-include> --phase prepare
python tools/ps5/build_installer.py --settings out/console-compiler/build-settings.json --platform-headers <pinned-SDK-platform-include> --phase package
python tools/ps5/package_launcher_update.py --title <verified-PPSA99148-folder> --manifest <build-manifest.json> --output <output-folder>/Sims2-PS5-Launcher.s2u
```

Settings contain local dependency paths and are not release inputs. The native
packager retains the captured RADV recipe, validates imports/ELF/SELF and rejects
game-code symbols in the public installer. Do not re-create the link command.

The host runner covers the real ImGui draw lists at 3840x2160, transition frames,
navigation, the production C++ transaction, mocked PS5 HTTP contract, and the
extraction library. Transaction tests include successful apply, caught failures
and forced process termination after every applied entry, recovery, preservation
of synthetic saves/preferences/ISO/private code, corrupt archives/stages,
path/identity/version rejection, module changes and unknown external writes.
These are process-interruption tests; no power loss is simulated.

A fresh owner ISO was also extracted, checked and patched through the production
FromIso library, starting with the legacy public placeholder and synthetic HDD
fixtures. The supported XBE SHA-1 was verified. The production generator emitted
71 files/65 C++ units matching the installer's reviewed hashes. No cached guest
objects were used for that test. Source ISO bytes were not modified or deleted.

Full game compilation/linking/Play is not covered by this host extraction test.
Earlier separate console generation, all-unit compilation and native-packaging
passes remain valid stage evidence, not a complete fresh ISO-to-Play pass.

## Release gates

Required next console scenario: fresh public title plus owner ISO -> Install ->
keep/delete choice -> the same launcher's Play tab -> actual gameplay -> close
and relaunch without rebuild. Then update from a controlled newer compatible
package, verify helper startup, game cache reuse, unchanged save/preferences and
Home metadata behavior. No compatible public update asset was published here.
Stable 60 FPS and the latest performance-runtime kit refresh are separate work;
the current installer kit still uses the existing f419-derived runtime.
