# Current public release direction

One repository; owner-copy PC/Mac builder.
Read ../../README.md and BUILDER_RELEASE.md. Earlier ISO-only directions below are historical.

# Independent runtime: current resumable state

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

1 November: current source candidate adds optional font-only 4x reconstruction,
Continue background readiness refresh, 17 English storage-device notices and
deterministic source-release inventory verification. Runtime SDK pin is
e91175246fe9ded3ab8c820e1d3f2b31f6aea5b7. Native object build **7b73fdc02feb0835d8a55a7d**
has passed (235.58 seconds, 115 audited compile commands); established
RADV link/package also PASS (123.13 seconds). New 45-member native delivery,
matching debug ELF and complete private owner folder are validated under
outputs/PS5-Independent/*Font-Continue-7b73fdc0. See DELIVERY_FONT_2026-11-01.md and
FONT_CONTINUE_2026-11-01.md for evidence, the preserved software-Vulkan timeout,
and remaining acceptance gates. Source-only release preparation includes the
local SDK trees, GPL/CC0 notices and no owner/generated data. Nothing published.

Quiet PC run20261101T105629Z-7e0740 completed900 seconds without fatal records:
three real native4K backing/final readback pairs and original/upscaled font
comparison reviewed. Gain is subtle smoothing, not restored font outlines.
The run reaches the Continue Game carousel, not the save-list/gameplay at4K;
watchdog termination is not clean shutdown. Font quality/Continue regression
on new7b73 PS5 build remain NOT TESTED. Source restore passes19 Python tests,
rebuilds public prompts byte-identically and builds the host tools; fresh
translation reproduces all71 generated files exactly. See the delivery note.

New owner 7bd log fragments verify execution and the reported 4K target sizes;
they do not establish clean shutdown or stable60. Gameplay windows23..29
average44.83FPS, last39.23FPS with about6.98ms measured GPU work. The thin-menu
fragment shows stale32/32 background reads before the original321/57 geometry;
the candidate refreshes the existing initializer when those values are ready.
No console has been contacted for this PC-only work.

31 October evening: a80 owner klogs now verify PS5 startup/gameplay and
4K target dimensions. Last full gameplay window averages 45.24 FPS; stable60
is not accepted. New work adds GPLv3/CC0 declarations, title artwork/name,
256px host-only Kenney prompts, right HUD child compensation, strict root-ISO
boot and bounded numeric Continue diagnostics. New native build 7bd91d7c2cd2127150a991e1 is compiled, linked, packaged
and locally validated (45 native/public members, matching symbols). A complete
private owner title is ready. Matching PS5 runtime/performance remains NOT
TESTED. See DELIVERY_HD_ISO_2026-10-31.md and DISTRIBUTION.md. No console
was contacted. Earlier delivery paragraphs below are historical.

Additional correction, 31 October: small mode-menu `s2c_logo` now anchors
left instead of receiving the centered safe-area gutter. PC run
20261031T153852Z-ac1922 shows the corrected logo, centered mode selector
and PlayStation prompt in actual final 1280x720 Vulkan readbacks. Source
checkpoint 4f7fe88783eb2023cfffc706e7665d1ce264cabe, runtime SDK
85bea0eb9666a9dbfe4c3ea54377509038d29e1b. New native build ID
a80d231d1fa2e8cc35a1523c; objects compiled in 127.59 seconds, 110 commands,
O2 with the existing 4K/profile settings. See MENU_LOGO.md. New PS5 runtime
and FPS remain NOT TESTED; prior 4K and baseline deliveries stay preserved.
Native link/package and local 36-member/ELF validation now PASS. The complete
private title (55 verified owner files) is under
outputs/PS5-Independent/FullTest-Native4K-Logo-a80d231d/PPSA99145 in the shared
workspace. Native artifacts and the actual corrected PC preview are under
Native4K-Logo-a80d231d. See DELIVERY_LOGO_2026-10-31.md. The local build VM
was shut down cleanly after copying and validating the artifacts.

31 October, additional native 4K candidate: **e9dd23d580f6407a3f3bc9b2**.
Code checkpoint cece5e9c8b240720468f7146842d1fd166017c50, runtime SDK
743c86dfaf2fc25dfa9cd8863573f368d85697ce. Native objects compiled O2;
the established RADV/title recipe linked and packaged successfully in
99.82 seconds. PC software Vulkan produced actual 3840x2880 scene targets,
3840x2688 cropped pictures and 3840x2160 final images. Three paired readbacks
and no-upscale logs pass verification. The 1,000-second bounded observation
reached the rendered island/mode menu, not a 4K gameplay lot. See RENDER_4K.md.
PS5 runtime and gameplay performance of the 4K binary remain NOT TESTED.
The d8e6 baseline below remains intact; it has not been replaced.

Earlier baseline delivery: d8e6e08de24284b0a69153a5 is linked, packaged and locally
validated. The complete private test title exists under
outputs/PS5-Independent/FullTest-UIFix-d8e6e08d/PPSA99145 in the shared
workspace. Its matching debug ELF is preserved. See DELIVERY_2026-10-31.md
for hashes, executed configuration, evidence and the next hardware test.

Current work, 31 October 2026: native e16fc615a5546cb6d2323393 is linked,
packaged and locally validated, with a private complete owner test folder.
The owner's PS5 klog and screenshots verify real gameplay of that candidate.
Its final observed five-second window averages 44.60 FPS; stable 60 is NOT
accepted. Display is 1080p, presentation 16:9, cap 60. Internal 4K is deferred.
Earlier packages remain intact. No console was contacted by this agent.

The named HUD roots now anchor at the edges. A real image rejected the
XML-only portrait fix and a cross-thread draw-tag experiment. The identified
0MOD_head camera rectangle is now corrected through original setter 0x1CBC20
at the exact head-renderer call site. Actual bounds and final 1280x720 gameplay
readbacks verify the corrected head/backdrop alignment on PC. See
HUD_PORTRAIT.md. Ineffective draw tags were removed.
New cinematic-matte policy and 22 English PlayStation notice replacements
are also being checked. See OPEN_UI_ISSUES.md for current failures/evidence.

PASS: 9 prompt regressions; 10 archive-patch tests; numeric UI/layout/matte
checks; actual generated aspect and fast-constant ABI probes. The fast-
constant wrapper already calls a bound HLE tail: leaving its entry unbound
preserves its guest scratch mirror. No speculative new binding was applied.

PC run 20261030T223028Z-53f2cc completed 1,000 seconds / 4,380 observed
Vulkan frames without a fatal record, using the new runtime and previous
assets/cache. Its actual 1280x720 save panel is correct. Three other runs
timed out at the GPU fence, including an unchanged previously successful
EXE with a fresh cache/new asset view. Thus the asynchronous-callback
prototype is not established as the cause. It remains OFF by default.
Fresh-cache behavior and the intermittent save panel are still open. The new
asset view then completed 1,200 seconds with a warm cache in
20261031T100900Z-add6f6. Run 20261031T102903Z-9b8edd completed 780 seconds,
and its actual 1280x720 Story-arrival frame verifies full-width cinematic
bands (out/evidence/letterbox-result.json PASS). Neither is clean title exit.

Native candidate d8e6e08de24284b0a69153a5 compiled (109 audited commands,
238.36 seconds) and passed the established native link/SELF/package checks, with O2,
handle caching/object wait channels and deep frame/GPU telemetry enabled.
Sampled per-guest-call profiling is OFF to remove that diagnostic overhead;
the earlier e16 profile remains available for comparison. Async callbacks
remain OFF. No new performance result follows from changing these flags.

Baseline execution: original guest boot, island main
menu, new-game warning, player count, generated Sim selection and the actual
Create-a-Sim editor. CPU reference images confirm the editor; real Vulkan
readbacks confirm the menu and generated Sim with corrected colour/alpha.
Actual lots now load on CPU reference and real Vulkan, including tutorial
dialogs and HUD. A generated Sim save is written and restored after restart in
the isolated PC test HDD. Classic camera/target navigation and movement from
the call box into the house after switching control modes are observed.
Object interactions/goal completion remain open. Audio output,
stability and performance gates are not passed yet.
No independent PS5 run has been collected or instrumented by this agent.

- Separate project and previous working port remain preserved.
- Fresh owner-disc extraction: 55 files, 1,986,340,175 bytes. All 55 files used
  by VM tests match the new ISO inventory by size and SHA-256.
- PASS: 27 native host compatibility checks; 7 asset preflight regressions.
- Fixed: raw GPU reset path by independently verified Reset binding;
  omitted owner DSP data; missing GPU completion callback at 0x275750.
- Local SDK pins: recompiler/harness faeb0eaca270e01a9e83bb2bc587c824c458ac32;
  runtime 0ad9d7ee179d5151da1c81c0b0dfbe5c76895015. Recompiler tool code is unchanged.
- PASS: first native SELF/title build 43b06fd7761a70083cf682c9, with retained
  debug ELF. This predates the completion callback fix and is not playable proof.
- Previous native candidate f81c23654aab302f6c41a417: native link and SELF/title
  validation PASS after all 106 audited compile commands. PPSA99145;
  eboot SHA-256 5e91b3d6efa5fcff245111b1d235e69f43a9d310143645ef5d0c88ccf69f0593.
  Matching debug ELF SHA-256 75b2140dadb78dc36304ead2dd12d193460c57e5f3f8c34a079bbf44c15f2e63.
  Local delivery: outputs/PS5-Independent/Bringup-f81c2365 in the parent workspace.
  Runtime on PS5 remains NOT TESTED. Native-only package contains no owner assets.
- PASS: Windows native null-backend host/game compilation, link and bounded
  execution. The 30 FPS harness cap controls test cadence; its frame count is
  not a rendered/displayed gameplay FPS result.
- Fixed gray P8 art/alpha: the scanner missed SetPalette at 0x272020, verified
  independently from this XBE. Real Vulkan after images show correct colours
  and transparency. The CPU reference also needed palette lookup/cache support;
  16 asset-free regression checks pass.
- Full HLSL/SPIR-V Windows Vulkan target builds and runs with the existing
  per-process SwiftShader ICD. Fixed the windowless initialization mutex.
  Real GPU completion callbacks advance; software ICD counters are not PS5 FPS.
- Fixed unchecked capture-path canonicalization after a native probe reproduced
  Access denied in this host sandbox. Earlier run summaries remain INCOMPLETE;
  their manually inspected real BMPs are retained. The repeated 1,200-second
  collector test passes with its real lot/tutorial readbacks.
- Current native candidate: c71c8a89b47567af4b35247c, including SetPalette,
  windowless correction and capture handling. BUILD PASS: 106 audited commands,
  593-second object build, 199-second established RADV/title link and package.
  SELF integrity and all 35 packaged file hashes verified, matching debug ELF
  retained. Local delivery: outputs/PS5-Independent/Palette-c71c8a89 in the
  parent workspace. eboot SHA-256:
  9702e482863c548838ad5b9fe5ef7173d9d2987dfc418dcf33f1b2909cd953b1.
  Debug ELF SHA-256:
  b76cf82681b437ce38de395b85a34d72e5dfb3bce1f210778ff6ac00b6524a65.
  No PS5 runtime result is inferred from this build.
- Complete private test title: outputs/PS5-Independent/FullTest-c71c8a89/
  PPSA99145 in the parent workspace. All 35 native members and 55 owner files
  (1,986,340,175 bytes) verified after copying. No saves included. Keep this
  owner-data folder private; the separate native-only zip remains unchanged.
- Current tests/config baseline: original 4:3, 30 FPS cap. Earlier 16:9,
  controller icon, 60 FPS and 4K changes are not yet included in this candidate.

Private evidence: out/evidence/long-probe (EA frame, log, run summary and full
asset verification); out/evidence/intro-stacks.gdb.log; VM run IDs
20261029T212800Z-1d9918 and 20261029T213510Z-a88864. Full logs/symbols stay private.

Windows evidence: out/runs/20261029T221703Z-9a5fbc (menu/save slots);
20261030T150452Z-3ff0f4 (CPU Create-a-Sim editor, 15,000 frames/600 seconds);
20261030T151836Z-91447a (corrected real Vulkan menu/generated Sim images).
20261030T153021Z-205d26 completes a 1,200-second actual Vulkan test with
HOST_VULKAN_FRAMES_CAPTURED, 5,700 offscreen frames and no fatal record. Real
readbacks include 8 Rockpile Rd and its tutorial dialog. Watchdog termination
is not a clean exit, gameplay acceptance or a measured PS5 frame rate.
The separate CPU test 20261030T153503Z-8ebbd1 reaches a lot with the newly
created female Sim and writes Config/SaveGame in synthetic E/UDATA.
It completed 1,200 seconds with 27,180 logical frames and no fatal record;
images include the control-mode tutorial and the player Wants panel. The
restart test 20261030T155613Z-949b01 clones that completed synthetic HDD,
recognizes the saved game, confirms disk load and restores the created Sim's
lot/tutorial after selecting One Player. PC save/restart/reload is observed;
The restart probe completed 600 seconds with 15,840 logical frames and no
fatal record. Its recorded BACK/LSD inputs and final image show the Sim inside
the house after classic camera/target movement near the call box. This is a
basic PC gameplay observation; object interaction/goal acceptance, clean title
exit and console persistence remain unverified. observations.json retains
the reviewed image digest and source/reloaded SaveGame hashes.
Personal saves remain untouched.

Offline source restore: out/source-bundles/283066c46d0d contains verified Git
bundles for this project, tools SDK, runtime SDK and foundation, with SHA-256
manifest. Later source changes need a fresh bundle snapshot as documented.
out/source-bundles/866175253e83 adds the delivery validator, complete private
owner-copy staging tool and recorded native/lot evidence. All four bundles
verify; no owner content, generated game code, binaries or saves is included.
Later tests remain separately recorded under out/runs/.

Resume by reading BUILD.md, TEST_RESULTS.md and PLAYABILITY.md. Continue normal
controller/object interactions and bounded goal-completion scenarios,
in-lot saving/clean exit and bounded gameplay. Validate the new PPSA99145 title
on authorized hardware when PC-only scope is lifted. Do not regenerate from the old game project
or manually edit generated code. Keep all changes local under the user's
no-push/no-PR instruction. Console tests remain deferred under PC-only scope.

## Historical initial bootstrap, before runtime imports

29 October 2026. This is the separate alternative requested by szampan; the
existing working PS5 port is retained independently. No existing game-project
config, hooks, runtime adapter or generated source is an input here.

## Results

- PASS: 46-step Windows host build of the SDK recompiler, signature scanner and
  a new ISO extraction wrapper using the SDK's read-only Disc parser.
- PASS: default.xbe extracted from the owner's 7,825,162,240-byte ISO;
  3,399,680 bytes, SHA-1 `16f9e41ce26c315a758f25d43ba1fa9d3809d1ad`.
- PASS: fresh automatic signature analysis and C++ emission (71 files,
  65 C++ translation units). Config SHA-256
  `3cf2d1860a583d67e4f25049507c11b786629ea7789bd29f089eac7cce93b2a9`.
- PASS: all 65 generated translation units pass the SDK freestanding header
  syntax checks with the Windows MSVC target ABI.
- NOT IMPLEMENTED: new PS5 runtime adapter, PS5 link/package and deployment.
- NOT TESTED: execution of this new recompilation, graphics or gameplay.

These passes establish a repeatable initial pipeline, not a playable port.
The scanner resolves only 131/243 function signatures and 11/12 data signatures.
The initial analysis has 3,823 overlapping decodes and reports unsupported x86
instructions; data/code classification and missing HLE require investigation.
The original generated code is never edited directly.

## Commands actually run

With the installed Clang/CMake/Ninja on PATH, from this project directory:

```powershell
cmake -S . -B out/host -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build out/host --parallel 2
python tools/analyze.py --iso "$env:USERPROFILE/Desktop/Sims 2, The (USA).iso"
python tools/check_generated.py --clang (Get-Command clang++).Source --jobs 2
```

The local run supplied absolute installed compiler/build-tool paths to CMake;
the commands above express the same build with those tools on PATH. Tested
versions: Windows x86-64, LLVM-MinGW Clang 23.1.4 (revision
7a941dfb2877a0b8cf30323fefa85ad019e0b0f5), CMake 4.4.4, Ninja
1.13.2.git.kitware.jobserver-pipe-1. SDK revision is in repositories.lock.json.

The first syntax attempt used the compiler's GNU Windows default ABI and failed
the SDK's MSVC weak-symbol pragmas. Correcting the target to
`x86_64-pc-windows-msvc` resolved all 65 units. The failed and passing diagnostics
are retained privately under out/syntax-gnu-first-failure and out/syntax.

Private artifacts: out/analysis/{summary.json,game.auto.toml,symbols.log,
recompile.log}, out/syntax/summary.json, generated/independent, assets/default.xbe.
All are excluded from version control. No console network configuration exists
in this workspace. No PS5 was contacted for these checks.

## Historical bootstrap next step (superseded above)

Audit the automatic analysis against the verified XBE: resolve executable/data
ranges, overlapping decodes, callbacks and missing HLE signatures. Produce a
bounded host runtime baseline before importing a PS5 adapter. Any future reuse
of existing port code needs a file-by-file provenance entry. This effort does
not assert strict clean-room independence after prior source exposure.

The independent project now contains its own authored UI integration. The
earlier port remains separate and preserved.
