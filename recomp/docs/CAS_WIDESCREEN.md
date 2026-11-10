# Create-a-Sim widescreen correction — 9 November 2026

Authored by OpenAI Codex for szampan; GPL-3.0-only.

The existing uniform, centered UI correction pulled the CAS side panel and
right-hand Back/Select prompts toward the middle. CAS editor pages now anchor
their complete left and right draw groups to the corresponding screen edge.
The 3D Sim, mirror and room keep their original camera/projection. Genetics,
keyboard, family selection, unknown states and centered dialog roots retain
their existing layout.

`src/game/cas_layout.cpp` observes the original USA CASTarget constructor,
destructor and update callbacks, calling each original exactly once. It validates
the target and publishes an atomic editor-state snapshot. Renderer threads never
dereference that guest object. `ui_aspect.cpp` applies the existing aspect scale
only to classified, depth-disabled UI draws within the verified editor state.
Left/right bounds are evaluated in the original 640x480 coordinate system.
Generic dialog 0G2D suspends edge mapping until its actual RemoveScreen, including
fade-out. No generated recompilation output or game assets were edited.

## Evidence

- Host policy suites: PASS (`out/cas-regression/policy-atomic-tests.log`).
- Actual game, offscreen Vulkan, 1280x720, aspect 16:9: PASS for reviewed main
  CAS, Body/Skin Tone, Fashion and Head Accessories screens. This uses owner game
  data and software Vulkan, not mocked UI or a performance benchmark.
- A/B in the same running game, `20261109T160916Z-19078b`: with CAS anchors OFF,
  panel left x=210 and right prompt edge x=1094; ON, x=84 and x=1220. Both shifts
  are 126 pixels. Captures `presented_04238.bmp` (OFF),
  `presented_03779.bmp` (ON), `presented_04524.bmp` (Body).
- Modal-guard run `20261109T162527Z-21a70b`: reviewed captures
  `presented_02436.bmp` (CAS), `presented_03431.bmp` (Fashion),
  `presented_04341.bmp` (Head Accessories). Recorded modal load/removal precedes
  entering CAS. The run was deliberately stopped after 853 seconds; no fatal
  records or first failure. This is not a clean-title-exit test.
- The final atomic calibration-flag change was rebuilt on host and PS5 and all
  policy suites rerun. Captures precede that thread-safety-only change; no
  calibration flag was changed in the modal-guard run.
- PS5 Profile/O2 runtime build `f419954392867d74f193d70e`: 52 authored/runtime
  objects, fast handles and object wait channels enabled. Native link using the
  captured RADV recipe, ELF layout/import checks and SELF integrity: PASS.
- Private owner test binary:
  `out/cas-regression/native-f419/PPSA99145/eboot.bin` (SHA-256
  `0f1de557370f73d9dbab9b91acd2d47827bf1e9e6d28fa82481c5944eeca3102`).
  Matching debug ELF: `native-f419/llvm-pie.elf`. Replace only eboot.bin in the
  existing private PPSA99145 installation, preserving assets and hdd.

PS5 visual/runtime validation: **NOT TESTED** for this correction. Not every
clothing, naming, family and confirmation page has been visually reviewed.
4:3 pass-through and centered-dialog policy have synthetic coverage; there was
no separate full 4:3 visual run. No new 60 FPS or 4K performance claim is made.

## Reproduce and continue

Run `python tools/test_ui_aspect.py` for host policies. Build the existing
`out/windows-headless` target `Sims2Independent` using its configured Ninja.
Use the existing bounded host runner and `tools/headless_command.py --cas-edges
on|off` for same-session comparisons (runner needs `--ui-layout-trace`). Test
evidence and input events remain under each `out/runs/<run-id>` directory.

For PS5, the exact object build command is:

```powershell
./tools/ps5/build_objects.ps1 -Mode Profile -ObjectWaitChannels -Jobs 2 `
  -Dependencies "$old/deps" -FoundationDirectory ../PS5_Sims2Independent `
  -VulkanDirectory "$old/PS5_Vulkan" -GlslangSource "$old/glslang" `
  -GlslangBuild "$old/sims2-recomp/out/build/ps5-glslang"
```

`$old` denotes the already discovered dependency workspace recorded in local
build settings. Full logs: `out/cas-regression/build-ps5-atomic.log` and
`out/build/ps5-game-profile/objects-build-status.json`.

`tools/builder/refresh_runtime_export.py` refreshes authored runtime objects in
the evaluated public RADV recipe while retaining all non-runtime arguments and
library ordering. It rejects generated guest definitions. The verified local
kit `out/builder-dev/kit-cas-f428` now contains runtime **f419**, despite its
historical directory suffix. Console installer packaging uses that kit through
`out/console-compiler/build-settings.json`; current result is always
`out/ps5-installer/package-result.json`.

`tools/ps5/link_cached_private_game.py` relinks the private owner-generated
65-member guest archive with that runtime using the same recipe. It does not
make that compiled game suitable for public distribution. Public installer
resources contain no guest archive, generated game source, ISO, game assets or
saves. A native-runtime-only update changes the native fingerprint; existing
compatible generated/compiled guest caches can be reused and relinked. This
update-cache path still needs a complete PS5 installer test.
