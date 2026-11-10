# Current failures and external requirements

## 10 November: Play spawn failure resolved on hardware

Candidate 8db5af5a5dd85551 repairs the previously verified private game from 0644
to 0777. Real cached-game handoff/boot/Vulkan presentation/60-second own shutdown
PASS, followed by a launcher screenshot/reopen PASS. No game recompilation was
needed. See LAUNCHER_PLAY_FIX_2026-11-10.md. The original errno13/EACCES is closed;
the template's separate errno22 diagnostic on its final LoadExec("exit") remains
recorded and is not treated as another failed game spawn.

Remaining gates: complete public download/helper/update/recovery on hardware;
owner acceptance of Home visual/audio and motor feedback; fresh full ISO-to-Play
repeat after the fix; gameplay/performance measurement with the latest runtime
kit; reproducible publication of local runtime-sdk changes before source release.
Real console GitHub HTTPS metadata checking now PASS. No public release/push/PR
was performed during this authorized console session.

## Current launcher release gates (10 November 2026)

Native candidate 7e81417b20d88218 and full host update/extraction/generation tests PASS. A complete fresh ISO-to-Play console run, actual HTTPS/helper LoadExec/recovery, haptic timing and Home metadata refresh are still required. This turn is explicitly local-only; no console or GitHub mutation was performed. The current public runtime kit remains f419-derived. Source PR preparation must also resolve local runtime-sdk commit publication/pinning without shipping owner game code. See LAUNCHER_NAVIGATION_UPDATE_2026-11-10.md and its result JSON. Earlier build-only/checker-only limitations below are historical.

## 8 November 2026: PS5-only preparation

Latest gate: console access restored; real first-unit and complete 65-unit O2
compilation PASS on PS5, including independent object checks and clean exits.
See CONSOLE_COMPILATION_RESULT_2026-11-08.json. Earlier console-unavailable and
compilation-unverified entries below are superseded.

Native-title linking/conversion is the next hardware gate. The new private
packager preserves the captured RADV recipe and original public native writer;
it does not require a new game-module ABI/import adapter. Private output loading,
gameplay and a complete resumable ISO installer still need validation through
this path. The existing playable static title is preserved.

Current gate: effac3b70e2d9484 BUILD PASS, CONSOLE NOT TESTED. It will compile one
real generated source unit at O2, check its ELF object and record all diagnostics.
Console services did not answer; testing is deferred until the console is
started later. No new candidate was deployed or launched. The established
working title and saves are untouched.

The previous native generator gate is closed by f622a1b9ef81c744 on PS5: all 71
files match the reviewed host hashes twice, existing output is refused and the
title exits normally. Canonical evidence: CONSOLE_GENERATION_RESULT_2026-11-08.json.
Next steps and exact commands: compiler/CONSOLE_GENERATION_TEST.md. Full-game
compilation, strong HLE/hook binding, import/TLS/thread/unwind integration,
module loading, gameplay and resumable ISO installation remain unverified.
The no-import fixture adapter cannot load the full game unchanged. No total
install-time or performance estimate is supported by these tests.

### Historical gates below (superseded where newer evidence is recorded)

Latest: f00dde58ac5ec7c4 additionally executes its PS5-compiled authored fixture
through the unchanged pinned PS5_RetroArch loader: 32 oracle calls, rejection,
unload/allocation checks and normal exit PASS. The adapter rejects external
imports and is not the full game's loader/runtime. Native generator parity and
game-module integration are next. See compiler/CONSOLE_EXECUTION_TEST.md.

Earlier: b04be1035b09b705 passes actual C++ compilation, I/O and synthetic linking
on the owner's console, with normal exit. Earlier getcwd/header-order failures
below are resolved for this scenario. Generated code execution, full game
generation/compilation, private-title conversion or module integration, resumed
installation and playable game execution through that path remain open.

The first PPSA99147 compiler diagnostic 14fb8665bc404469 now has hardware FAIL:
SIGSEGV in libc getcwd during LLVM physical-VFS construction. Source and matching
debug-ELF analysis identify the missing SDK binding. Candidate a72d982ec4cb8260
binds getcwd and realpath to existing verified platform implementations and adds
VFS diagnostics. Native rebuild/final fresh check PASS; corrected runtime awaits
a new full klog and run folder. See CONSOLE_COMPILER_CRASH_2026-11-08.md.
The separate prior
PPSA99146 synthetic linker scenario has hardware PASS. Next requirement is a
tester run of PPSA99147 with full klog and its generated run folder. It checks
actual console C++ compilation and filesystem operations, not just linking
precompiled objects. No ISO or game files are required by this diagnostic.

Successful on-console C++ compilation, malformed-input/fatal recovery and
generated-output execution remain unverified. Full ISO generation/compilation, resumable install,
native private-title conversion/packaging and launching still need integration
and hardware validation. In particular, the current private-title converter in
the public kit is a Windows-host tool; this diagnostic does not port it to PS5
or prove an alternative loader. Do not publish this as a PS5-only installer.

No new gameplay or performance changes are included. Existing release and saves
are preserved. Older entries below are historical requirements/results.

Owner-copy release: Windows native build and private data install PASS. Public
PS5 instruction title build PASS; new title/Windows-linked game execution on PS5
NOT TESTED. Windows8.1 compatibility is a target without a real test host. No Mac
is available to build/test a Catalina-compatible native .app/tool bundle; common
Python source does not establish that compatibility. Single-repository Windows
exe/source/public-title packaging is ongoing. No publication or remote writes.

Earlier-port optimization audit complete: all audited generic speedups are
already inherited/enabled. Exact old SDK8f5b279 is an ancestor of current163c80f;
critical memory/dispatcher/filesystem files unchanged. GPU completion policy,
render resolution and game-timing hooks are the material differences. See
OLD_PORT_PERFORMANCE_AUDIT_2026-11-01.md. No runtime changes in this audit.

Current performance work: owner f48 tail remains around43FPS with7.58ms
render-queue flush wait. Separate settings-only ordered-WRITE comparison is
prepared; default f48/synchronous settings preserved. Actual GPU queue contract
and20 Python tests PASS; 480-second/3000-frame PC Vulkan lot test completed without fatal records
(watchdog end, not clean shutdown). PS5 experiment
and stable60 remain NOT TESTED. See PERFORMANCE_ORDERED_GPU_2026-11-01.md.

Current installer correction: see INSTALLER_FIX_2026-11-01.md. Native4K PC
frames PASS; descriptor metadata replaces refused lstat. Native f48a3b24 compile/link/package
PASS; PS5 retest required. Gameplay/performance unchanged.

Latest delivered candidate9bb7544b: native O2 compilation/link/package and
50-file local validation PASS; matching symbols retained. Guest/deep profiling
OFF, per-second performance reporting ON, native4K/cap60 unchanged. New actual
PS5 runtime/FPS NOT TESTED. Installer/Open Sans host contracts/Vulkan probe
PASS. Thin Continue remains OPEN after7b73 owner failure. The7b73 heavy log
mean is42.69FPS, not sustained60. See DELIVERY_INSTALLER_2026-11-01.md and
PERFORMANCE_7B73_2026-11-01.md. No console contact or publication.

1 November current gates: new7b73 native compile/link/package/local validation
PASS. Quiet PC actual4K font A/B readbacks now PASS dimension/binding checks;
visual gain is subtle smoothing, not recovered outlines. Repeated Continue
entry on the matching PS5 build is required
before closing the thin-background defect. Font reconstruction quality and
stability on PS5 remain NOT TESTED. Performance remains below stable60 in
latest owner7bd windows (~44.83FPS aggregate, last39.23). No new optimization
benefit is claimed. Source export is local only; clean new-machine toolchain/
RADV bootstrap and direct-ISO storage-text overlay remain unfinished.

31 October evening update: the newer owner a80d231d logs now establish
PS5 startup, real gameplay and runtime-reported 3840x2880 targets (scale 6,
3840x2688 picture, 3840x2160 output). The final full gameplay window is
22.103136 ms / 45.24 FPS; measured GPU work is about 6.174 ms/frame.
This fails a stable-60 target. The earlier NOT TESTED statements below refer
to the state before these owner logs arrived. No console was contacted.
See HD_ISO_2026-10-31.md for current changes, evidence and outstanding work.


Mode-select logo margin is corrected and verified on PC: the identified
corner logo uses left anchoring; centered startup draws/other textures are
excluded by bounds/identity guards. Remaining requirement is owner PS5 test
of native candidate a80d231d1fa2e8cc35a1523c with the existing 4K settings.
No console was contacted. See MENU_LOGO.md for the actual image and limits.

4K candidate e9dd23d580f6407a3f3bc9b2 is compiled/linked/packaged, and its
render policy has actual PC Vulkan buffer evidence. Remaining requirement:
owner test of this specific binary/settings on PS5, including a gameplay lot,
HUD, shader effects, controller, audio, shutdown and frame timing. No console
was contacted. Software Vulkan only reached the 3D menu at native 4K during
the bounded run; it supplies no PS5 FPS estimate. Native output support,
resource use and stable 60 FPS at 4K remain unverified. Earlier gray-icon and
intermittent save-panel issues remain open; this resolution change does not
claim to fix them. See RENDER_4K.md and OPEN_UI_ISSUES.md.

31 October 2026. Real e16 gameplay has owner evidence; full-game, stability
and stable-60 acceptance are still open. The latest active issues and A/B
diagnostics are recorded in OPEN_UI_ISSUES.md.

- Resolved on PC null and Vulkan backends: the dropped GPU completion interrupt left the
  main frame semaphore waiting after two primed frames. The independently
  decoded InsertCallback binding now advances thousands of frames. Real
  Vulkan callbacks and images work; the matching owner e16 PS5 log now also
  verifies native execution and continuing frames/gameplay.
- Resolved test-host overhead: direct Windows null-backend execution replaces
  slow software-emulated Linux runs for most CPU/HLE checks. Island menu and
  save-slot selection now have image evidence; gameplay remains open.
- Resolved visual defect: independently binding SetPalette at 0x272020 fixes
  gray UI/logo art and black rectangles in actual Vulkan readbacks. CPU
  reference palette support passes its asset-free regression.
  No working default hardware ICD exists on this PC. The already installed
  SwiftShader ICD initializes with an isolated per-process configuration;
  software rendering can verify images, not PS5 performance.
- Resolved windowless Vulkan stall: Init now calls VulkanInstance outside its
  own handover lock. Repeat tests reach the island menu and generated Sim.
- Resolved capture-path error: canonicalization was denied by the host sandbox
  and its unchecked empty result wrote images to the run root. The runtime now
  uses the configured lexical path and verifies output before acknowledging.
  Preserve the old INCOMPLETE summaries alongside the inspected real images.
- Resolved data-preparation failure: sims2.bin was omitted from the initial
  PC test assets. Extracting the root files from the owner's ISO advances past
  the deliberate disc-error screen and into the EA movie path.
- Validation still required: object interactions, goal completion, clean title
  exit and stability. Character creation, first lots, save/reload and basic
  direct/classic control responses now have PC evidence. See PLAYABILITY.md.
- This agent's new work stays PC-only. Owner screenshots and the matching
  e16 PS5 log verify 16:9 gameplay, but further fixes need a new console run.
  The final observed five-second window was 44.60 FPS, not stable 60.
- Presentation validation: final portrait centering, every HUD animation,
  two-player layouts, save-menu header placement and mode-select logo anchoring
  need reviewed images. Native 4K remains deferred by the owner.
- Local unpublished SDK/foundation pins require their source bundles for
  restore; public upstream clones alone do not contain all local integrations.
- No new-code distribution license has been selected. Existing MIT/framework
  notices remain intact. No legal game content may be included in source or
  public build archives. This does not block local owner testing.

The previous working PS5 port is preserved separately and supplies no game
config, hooks or generated game source to this project. The MIT runtime fork
is an explicit credited dependency, not a newly authored replacement engine.

2026-11-10: Launcher self-update backend remains unimplemented (current action is a version check). It needs a compatible public-only update bundle, download integrity checks, staged helper handoff, atomic promotion/recovery and a save-preservation test before enabling automatic installation. The latest public v1.0.0 packages are not compatible PS5-only updates. Explicit game-update cache handling builds, but requires console validation with installed owner data.
