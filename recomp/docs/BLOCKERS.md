# Current failures and external requirements

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
