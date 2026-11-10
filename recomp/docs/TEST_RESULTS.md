# Runtime bring-up and preparation tests

## 2026-11-10: 8db5af5a PPSA99145 repair

Native build/import/SELF/package and all 3,258 ready hashes PASS. Host UI/startup/
navigation/carousel/audio/extraction, 13 update transaction tests, 10 mocked HTTP
cases and complete 513,919,916-byte updater bundle staging/apply/private-fixture
preservation PASS. Real console cached-game handoff/guest memory/Vulkan rendering/
60-second normal shutdown PASS; existing game bytes unchanged, execute mode
repaired 0644 to 0777. Subsequent real launcher screenshot PASS, repair not needed.
Home presentation file hashes and title registration PASS; Home visual/audio
owner acceptance pending. Console GitHub checker reached Current. This is not
gameplay/FPS acceptance or full update-helper hardware proof.

Evidence and reproducible commands: LAUNCHER_PLAY_FIX_2026-11-10.md;
out/ps5-launcher-99145/console-8db5/ and
out/ps5-installer/host-validation/20261110T184806Z/.

## 2026-11-10: local navigation, minimal Play and updater candidate

Latest local native installer **7e81417b20d88218**, PPSA99148: build, captured RADV link, import/SELF validation and final package/source hashes PASS. No console deployment, launch, GitHub push or PR occurred. The console remains on 1bd9b9f22e9e99bb.

Three supplied gameplay captures, left-stick/D-pad navigation, moving selection/content fade, matching disabled-choice recoil/vibration and the thin glowing bottom rim are implemented. Play now shows only a large centered X Play, retaining the common header/navigation. Activation fades the whole screen over 180 ms, then submits an unconditional black frame before native handoff. Host renders verify the fully black terminal frame; real PS5 timing/haptics/handoff remain NOT TESTED.

Thirteen production C++ transaction tests and ten mocked HTTP-contract cases PASS. The complete 513,587,921-byte public updater bundle (3,254 files) was staged/applied/hash-verified on the host; synthetic saves, settings, owner data, ISO, private code and cache were preserved. Unchanged modules/resources are not rewritten. Actual PS5 HTTPS, helper LoadExec, filesystem durability and Home metadata refresh remain release gates.

Fresh owner ISO extraction/patching, legacy CRLF placeholder, save/cache fixtures and generation of all 71 files/65 units matching the reviewed oracle PASS. A placeholder line-ending bug and Keep ISO closing the launcher were corrected. This is stage evidence, not a full fresh ISO-to-Play console pass. Temporary extracted test copies were removed after verification to recover disk space; the original owner ISO and canonical working assets remain separate. Packaging was retried successfully after temporary outputs filled the disk.

Evidence: LAUNCHER_NAVIGATION_RESULT_2026-11-10.json and LAUNCHER_NAVIGATION_UPDATE_2026-11-10.md. Local PR text: PR_DRAFT_LAUNCHER.md. Artifact/debug paths: out/ps5-installer/package-result.json. The installer still uses the f419-derived public runtime kit; latest performance kit integration and stable 60 FPS remain separate work.

## 2026-11-10: background, persistent music mute and separate update actions

Candidate 1bd9b9f22e9e99bb built/linked/import-checked as a native SELF and deployed to PPSA99148. Twenty changed files read back with matching SHA-256. Previous eboot/metadata backed up privately; settings and the separate working game title were preserved. User closed the launcher; idle state verified before/after. No application was launched by the agent.

Home background now has real 3840x2160 BC7_UNORM DX10 pic0.dds/pic1.dds, generated from the original CC0 source; mean channel error 0.0588. Eleven Home presentation files verified, title re-registered through PS5 Upload. Read-only registry check confirms pic0Info points at the new DDS. No direct database writes.

Launcher gain is 20% of original PCM. Touchpad toggles music with a short gain ramp; .sims2-launcher-prefs atomically retains the last choice outside public resources/saves. Host tests PASS for gain bounds, mute ramp, muted startup, full-track decode, atomic preference replacement, malformed input and failed-save cleanup. Ten actual 4K UI scenarios/navigation checks PASS. New Touchpad/D-pad prompts reuse Kenney CC0. Console audio/preference/haptics acceptance remains the owner's test.

Update section is accessible without a game. Up/Down chooses Update launcher or Update game. Game update requires a verified existing private executable plus changed generator/compiler/native/data identities in the current launcher, and reuses verified cached units. Startup now reports an available game update instead of automatically rebuilding. Completed receipts associate the game with launcher_version. Live settings.toml is not treated as a fixed compiler resource. Owner data migration still requires a verified migration, never silent replacement.

Limitation: Update launcher currently checks GitHub versions; automatic download, transaction/helper handoff and rollback are NOT implemented. Public release v1.0.0 still contains the older PC-builder package and instruction title, not this PS5-only installer. Do not call this a working automatic launcher updater or a full ISO-to-Play acceptance PASS. Game update operation also needs an installed-game console test. Private evidence under out/audio/deploy-1bd9, home-refresh-1bd9 and out/ps5-installer/preview-preferences.


## 2026-11-10: launcher sections and Home presentation

Build 78ebceeb197ec7e2: native build and 18-file console deployment/hash readback PASS. Play/Install/Update UI, locked-section nudge/rumble, requested headings, actual owner CC0 shell icon and full 120-second loop under 2 MiB. Nine 4K host view scenarios and navigation PASS; console visual/audio/haptics AWAITING_OWNER_TEST. Update tab is a real HTTPS release checker with manual fallback, not an automatic installer. Home registration evidence under out/audio/home-refresh-78eb. User requested manual runtime testing; application was not launched.


## 9 November 2026: audio host checks

Full 120-second MP3 decode, callback partition equivalence, loop wrap, missing-input silence and gain bound PASS. Home RIFF/ATRAC9 profile, six-second sample count, no-loop flag, 2 MiB size bound and processed fade-to-zero PASS. Console listening NOT TESTED.


## 9 November 2026: real-console performance capture

0aaecaff startup/identity and CPU/GPU timer support PASS. Owner-reported close/far
gameplay captured: selected averages 59.68/58.91 FPS; max intervals 49.64/63.90 ms.
Stable 60 FAIL in this short scenario. No screenshot/autonomous visual acceptance.
No captured crash; process exited at 300.8 s, own-shutdown marker not captured.
Original signed executable restored/hash verified; separate original startup and
own-shutdown test PASS. See PERFORMANCE_CONSOLE_2026-11-09.md and JSON receipt.

## 9 November 2026: FPS instrumentation build

0aaecaff5112dfb3dddcca1f: optimized PS5 object build and native ELF/import/SELF
checks PASS; deep and sampled guest profiling enabled. Six performance-analyzer
regressions PASS. Asset-free host telemetry math/lifecycle/reporting test PASS;
no game or GPU performance measurement implied. PS5 execution/timer support and
sustained 60 FPS NOT TESTED. See PERFORMANCE_DIAGNOSTIC_2026-11-09.md.

## 9 November 2026: CAS widescreen

Host numeric/policy suites PASS. Real game Vulkan A/B shows symmetric 126-pixel
edge correction at 1280x720; main CAS, Body, Fashion and Head Accessories reviewed.
Modal-guard run 20261109T162527Z-21a70b ended by requested test stop at 853 s,
without fatal records; not a clean-exit or performance test. Final atomic flag
change rebuilt on host and PS5; policy suites rerun PASS. PS5 native runtime
f419954392867d74f193d70e: link/import/ELF/SELF checks PASS. Console rendering and
complete installer update path NOT TESTED. Full evidence: CAS_WIDESCREEN.md.

## 9 November 2026: larger text and section vibration

Build c9fc2599be2d33a5: PS5 objects compile; native RADV link, native import/export
checks and SELF integrity PASS. Larger-font host renders at 3840x2160 PASS for
missing data, compiling, ready/Play, ISO deletion question and error. All five
states were inspected for overlap and clipping. Existing host carousel checks
also PASS. Logs: out/ps5-installer/preview-tv-haptics.log and
out/ps5-installer/package-tv-haptics.log.

Section feedback uses the documented template pad_vibrate API and a nonblocking
40 ms deadline. No controller hardware test performed; actual intensity and stop
latency remain NOT TESTED. No new full-installer or gameplay validation claimed.

## 9 November 2026: launcher B / GAMEPLAY

- Build 9ba17d4ff6cdfb53: native PS5 build, RADV link, import/export checks,
  conversion and SELF integrity PASS. Matching ELF preserved locally.
- Host: actual ImGui view rasterized at 3840x2160 for missing data, compiling,
  ready/Play, source-ISO deletion question and error states. All five reviewed.
  Carousel tests cover drift, one tilt, delayed repeat, neutral reset, forward
  wrapping and immediate opposite tilt. PASS; no controller hardware claim.
- Packaging regression: first invocation exceeded Windows CreateProcess's
  length limit. Retained identical ordered inputs in a response file; rebuild
  and complete packaging PASS, full argv retained in link-arguments.json.
- Console UI and complete ISO-to-Play flow: NOT TESTED for this build.
  Prior 3b47 launch was refused; its missing screenshot is not a success.
- The reviewed host image contains a private temporary game capture. The
  public package contains only CC0 placeholders pending the user's three images.

## 8 November 2026: complete real game compilation on PS5 PASS

First unit effac3b70e2d9484 and all-unit 56ffbab2e32b8ea7 hardware tests PASS.
All 65 private ELF objects were fetched and independently checked; normal exits
and idle state confirmed. Canonical numbers and evidence locations:
CONSOLE_COMPILATION_RESULT_2026-11-08.json. The native-title linking/conversion
adapter builds but has not yet run on PS5. No new game execution or performance
claim. Current parser suite: 42 tests PASS, including incomplete full-unit and
native-title captures that must not be accepted as success.

## 8 November 2026: console generation PASS; first game unit awaiting hardware

See CONSOLE_GENERATION_RESULT_2026-11-08.json for the independently checked
71-file console generation result, exact timing, run identity and private
evidence. Output overwrite refusal, prior synthetic execution checks and normal
exit PASS. SDK control-file adapter replaces unsupported filesystem imports;
the generator's instruction analysis/code emission is unchanged.

effac3b70e2d9484: first real game-unit diagnostic BUILD PASS through the unchanged
RADV title recipe and native import/SELF checks. Generated game code is absent
from the title; only one private unit will be compiled on PS5. This candidate's
runtime is NOT TESTED because services were unavailable and console testing was
deferred. No new gameplay, FPS or installer-completion claim. 32 parser tests
PASS, including refusal to count generation alone as game-unit compilation.
See compiler/CONSOLE_GENERATION_TEST.md for reproduction.

## 8 November 2026: PS5 compile/link/load/execute PASS

f00dde58ac5ec7c4 / 1794165660-2725256835: all compiler/I/O/LLD checks PASS,
invalid object rejected as a module, 16 oracle calls in each of two loaded
modules PASS, both unload and executable allocation counts restore to 0/0.
Worker: 3.699577 s. Normal title exit and final idle confirmed. Returned identical
50808-byte FreeBSD ELF modules independently inspected. No actual game code was
included or executed. 22 parser regressions PASS. See compiler/CONSOLE_EXECUTION_TEST.md
and out/ps5-debug/console-f00-20261108-01 for evidence and exact limits.

## 8 November 2026: automated native C++ compiler on PS5 PASS

Build b04be1035b09b705, run 1794164828-1892274508. Exact remote build/resource
hashes verified; I/O PASS; compiler results 0/0/1/0/0; linker results 0/1/0;
repeated object and ELF identities PASS; worker 3.568512 s. The expected errors
recover normally. Title ends status 0 and exits on its own; kernel records and
final idle query confirm termination. Retrieved ELF64 x86-64 ET_DYN binaries
are independently checked and identical (2200 bytes). Generated code execution
is still NOT TESTED; this is not full-game bring-up. Evidence directory:
out/ps5-debug/console-b04-20261108-02. Previous failures follow historically.

## 8 November 2026: first compiler hardware failure and correction

PPSA99147 `14fb8665bc404469`: console **FAIL**, SIGSEGV during LLVM's physical-VFS
constructor. Five critical resources verified; no compilation/I/O PASS claimed.
Matched-ELF stack and indirect call identify libc getcwd. The SDK's documented
platform replacement was present but unbound. Corrected `a72d982ec4cb8260` binds
getcwd/realpath and logs VFS setup. Native rebuild PASS 92.619 s; final fresh pass
PASS 77.263 s; 16 log-parser regressions PASS. Console retest remains required.
Corrected package-attempt-0003 native import/ELF/SELF checks PASS; local aliases
match ps5_getcwd/ps5_realpath and neither remains imported. Delivery ZIP CRC and
3076 per-file hashes PASS, 60,861,248 bytes, SHA-256
b46ec8dea8c0a1a6524964eac19eb59a1e0f0b183b620a3350e40af9fa65f77d.
See CONSOLE_COMPILER_CRASH_2026-11-08.md. Earlier delivery NOT TESTED entries below
describe the evidence available before this capture arrived.

## 8 November 2026: native compiler diagnostic

| Check | Result | Evidence |
| --- | --- | --- |
| Complete native Clang/LLD libraries | PASS | build-ps5.json, 1491.439 s after resume |
| Final fresh dependency build | PASS | build-ps5-final.json, 71.227 s |
| PPSA99147 14fb8665bc404469 compilation/link/import checks | PASS | console-compiler-probe package-attempt-0002 receipts |
| Native conversion/signing/SELF integrity | PASS | inspect.log: signed plaintext, 12 segments, integrity valid |
| Tester ZIP inventory, CRC and SHA-256 per member | PASS | 3076 title files, delivery-manifest.json |
| Compiler/linker klog parser regressions | PASS | 16 tests, 0.010 s |
| Previous tester linker capture, updated parser | PASS | preserved capture-summary-current.json |
| C++ compilation and I/O on PS5 | NOT TESTED | PPSA99147 tester required |
| Generated output execution, ISO-only installation | NOT TESTED | not performed by this diagnostic |

Delivery ZIP: 60,861,111 bytes, SHA-256
491b70d5a6d2031bcb122ea852b4991acb439278ab1875ffb70475d4793e4e52.
Eboot SHA-256: 019ab77207fab54433adfbe23a0320908aa79b85b7eab6e7c563085fb29a18e6.
Full reproduction and limitations: compiler/CONSOLE_COMPILER_TEST.md.
This work changes no game behavior and supplies no new FPS evidence.

## Previous runtime results (historical)

Final local alpha: standalone exe full owner-ISO pipeline PASS, exit0,
811.629 seconds wall. Final notice-only repack verification PASS:1009 outer
entries, same PYZ/native bootloader and all compile/link/runtime resources;
only notices differ. Final exe self-test PASS3299 files/Tk. Consolidated source
inventory PASS3868 files; nine safety/native-layout tests PASS from that snapshot.
Public setup ZIP CRC/hash/exclusion checks PASS. Standard bootstrap font/license
staging PASS with unchanged eboot/font hashes. New PS5 execution, gameplay and
performance remain NOT TESTED. No Mac app is claimed. Final files/report in
workspace outputs/Release-Candidate/v0.1.0-alpha.1. No remote publication.

Owner-copy builder: 9 safety/native-layout regressions PASS. Fresh reviewed
generation and all65 O2 guest compilation units PASS (557.84s, two workers).
Initial wrapper/search-path export failure corrected; explicit retry of65
verified cached objects PASS through linking/import validation/native conversion/
SELF sign+inspect and checked owner-data installation (52.64s). Public bootstrap
29-object native title PASS, no generated game definitions. New PS5 runtime and
Mac/Catalina/Windows8.1 tests NOT TESTED. See docs/BUILDER_RELEASE.md.

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

1 November update: native **7b73fdc02feb0835d8a55a7d** object compilation PASS,
235.58 seconds, 115 audited O2/g1 compile commands, guest/deep/performance/wait
profiling enabled. Established RADV link/package PASS in123.13 seconds;
45-member native delivery and matching ELF validated locally. Font/UI policy C++ regressions
PASS; 17 existing Python PS5-tool tests plus two new source-inventory tests PASS.
The latter reject changed/missing/extra owner members against a complete
manifest. Continue PC first-entry/reentry geometry-provider sequence PASS;
console acceptance remains open. Frankfurter4x runtime binding PASS; quiet
900-second run7e0740 has three verified native4K image pairs and reviewed font
A/B captures without fatal records (watchdog end). Smoothing is subtle and
does not restore outlines. Earlier SwiftShader timeout is preserved. Exact
run IDs/hashes and measured 7bd FPS are in FONT_CONTINUE_2026-11-01.md.

31 October evening update: the newer owner a80d231d logs now establish
PS5 startup, real gameplay and runtime-reported 3840x2880 targets (scale 6,
3840x2688 picture, 3840x2160 output). The final full gameplay window is
22.103136 ms / 45.24 FPS; measured GPU work is about 6.174 ms/frame.
This fails a stable-60 target. The earlier NOT TESTED statements below refer
to the state before these owner logs arrived. No console was contacted.
See HD_ISO_2026-10-31.md for current changes, evidence and outstanding work.


Mode-logo correction: asset-free logo identity, shader/corner bounds,
centered-startup exclusion, unrelated-texture rejection, 720p margin mapping
and existing UI/HUD/matte/portrait regressions PASS. Actual PC mode menu
in 20261031T153852Z-ac1922 confirms left anchoring and unchanged centered
selector. Observed original bounds: 32.031,38.431..135.431,141.831; x_offset=0.
Readbacks 2023/2057 were reviewed; evidence is out/evidence/menu-logo-anchor.json.
The bounded PC observation was stopped at 284.906 seconds, no fatal records,
clean_process_exit=false. This verifies layout at 1280x720, not new PS5
gameplay or FPS. The 4K render policy is unchanged. See MENU_LOGO.md.

Additional 4K candidate e9dd23d580f6407a3f3bc9b2: native compile/link/package
PASS; runtime on PS5 NOT TESTED. Asset-free render-density, invalid input,
post/depth scale, UI, matte and portrait regressions PASS. Existing 13 Python
PS5-tool/prompt/text/performance regressions PASS.
PC software Vulkan run 20261031T113329Z-ad15eb completed 1,000.235 seconds
at its watchdog bound: no fatal records, seven paired capture sets, three
at native 4K (2251, 2272, 2299). The real 3D menu's backing is 3840x2880,
cropped picture 3840x2688 and final image 3840x2160. Extent log + actual BMP
headers + SHA-256 verification PASS in out/evidence/4k-raster-result.json.
Real PNGs are under out/screens/20261031T113329Z-ad15eb. The process was
terminated by the bounded runner (clean_process_exit=false). A 4K lot/HUD,
full 4K startup, real controller/audio, clean exit, stable FPS and PS5 memory
use are NOT TESTED. Render detail was enabled dynamically after low-resolution
startup; PC software frame rate is not representative of console performance.

Latest local build delivery is d8e6e08de24284b0a69153a5: genuine SELF,
36 verified package members, matching symbols, private complete owner copy.
New PS5 runtime/performance validation remains NOT TESTED.
Previous owner-verified delivery is e16fc615a5546cb6d2323393, genuine SELF,
matching debug ELF and 36 verified public package members. The owner supplied
matching PS5 gameplay screenshots and klog. This establishes limited real
hardware execution, not complete playability/stability. The final observed
five-second window averages 44.60 FPS; stable 60 fails this observation.
The new portrait-camera and matte fixes have actual PC image evidence;
the notice archive passes structural/content checks and real game loading.
See OPEN_UI_ISSUES.md for failed and passing A/B runs, including timeouts.

New authored UI candidate: 9 prompt tests PASS (all 17 independently decoded
mip chains match the CC0 PNGs), numeric viewport/dimmer policy PASS, logical
anchor/reflow/preserved-CB-fields and pre-clip scissor regressions PASS. Fresh generated
aspect function probe PASS: normal 1.333333373, wide 1.777777791, stack/saved
registers intact. Source observations are private, not bundled into tests.
The actual Vulkan lot run 20261030T165232Z-f25a01 completed 700 seconds,
3,240 offscreen frames and zero fatal records. The full circle is visible
after the first clipping iteration was rejected. Final portrait correction
and real 1280x720 presentation readback validation are in progress. These
software-renderer frame counts are not PS5 FPS measurements.

| Check | Result | Limitation |
| --- | --- | --- |
| Host recompiler and scanner | PASS | build tools only |
| Fresh static generation | PASS, 65 C++ units | code-discovery warnings remain under review |
| Initial MSVC syntax checks | PASS, 65/65 | not a runtime test |
| Raw upstream Linux harness build | PASS after two platform fixes | WIC utility excluded; missing error-message declarations supplied |
| Raw upstream CPU/HLE run | FAIL, SIGSEGV after 1.84 seconds | kernel imports resolved; zero presented frames |
| GDB reproduction | FAIL at __imp__sub_0027AA61 | original GPU hardware path under reset 0x271570 |
| Reset ABI probe | PASS | one 640x480 presentation pointer, format 0x1e, ret 4 |
| Windows SDK quoted-path fixture | PASS | real ELF64 x86-64 and emulated-TLS symbol from a path with spaces/parentheses |
| Target-runtime headless build | PASS | separate cache using runtime-sdk; 443.75-second callback rebuild |
| Missing owner DSP/root-file diagnosis | FIXED | deliberate disc-error loop reproduced; complete extraction now checked before launch |
| Full owner-disc integrity | PASS, all 55 files | 1,986,340,175 bytes match fresh ISO extraction SHA-256 inventory |
| GPU completion callback | observed on null and real Vulkan backends | real submissions, callbacks and images on software Vulkan; PS5 pending |
| Bounded longer CPU/HLE probe | 3,600 frames, no reported fatal | 180-second limit; not clean exit or gameplay acceptance |
| Reference image | PASS, decoded EA logo visible | movie frame only; menus/lot not verified |
| Asset preflight regressions | PASS, 7/7 | synthetic fixtures, no game content |
| Native host compatibility | PASS, 27 checks | pointers, RAM aliases, paths, pad mapping, CPU context/stack cleanup; hardware/allocation tests separate |
| First native SELF/title | PASS, build 43b06fd7761a70083cf682c9 | not console tested; predates callback fix |
| PS5 native UI candidate | BUILD PASS, e16fc615a5546cb6d2323393 | 36 package members and matching ELF hashes validated; owner PS5 execution verified by matching log |
| Native Windows null-backend executable | PASS, host + 65 fresh guest units | real Windows services; no GPU/audio output |
| Native Windows bounded menu run | 600 seconds, 13,200 logical frames, no fatal record | paced 30; forced end at watchdog, not clean shutdown or game FPS |
| Island menu and save selection | visually observed, confirm/back work | left stick selects modes; gameplay pending |
| P8 palette binding | FIXED, 0x272020 independently decoded | actual Vulkan logo/UI/Sim now coloured with correct alpha; no texture replacement |
| Reference palette regression | PASS, 16 checks | synthetic colours/alpha, same-frame updates, alternate palettes and all size codes; no game assets |
| Create-a-Sim | generated female/male and editor reached | editor completion/save/gameplay not yet passed |
| Full Windows HLSL/SPIR-V compiler | BUILD PASS | legalization enabled; initial build without it rejected for testing |
| Vulkan ICD probe | PASS, SwiftShader CPU device, API 1.3 | isolated process setting, existing public DLL; no hardware GPU or PS5 claim |
| First Vulkan title test | STALLED; FIXED on repeat | nested first-use mutex corrected; later real Vulkan runs reach menu/generated Sim |
| Capture collection | earlier runs INCOMPLETE, real BMPs manually reviewed; repeat passed | configured capture path and acknowledgement verified in complete 1,200-second run |
| Complete actual Vulkan collector run | HOST_VULKAN_FRAMES_CAPTURED, 1,200 seconds, 5,700 offscreen frames, zero fatal records | menu, lot selection, 8 Rockpile Rd/tutorial images verified; watchdog end, not clean exit/gameplay FPS |
| Created Sim lot/HUD and controls | observed on CPU reference | classic camera/target responds; Sim moves from call box into house after mode/input sequence; object/goal completion pending |
| Reload probe | 600 seconds, 15,840 logical frames, no fatal record | save/Sim/lot restored; screenshots/input events/hash observations retained; watchdog end is not clean exit |
| Save creation/reload | Config + 1,204,248-byte SaveGame; restart restored created Sim's lot/tutorial | isolated synthetic E/UDATA, no personal saves; clean exit/PS5 persistence pending |
| Native delivery | PASS, all 35 files and matching ELF hashes | outputs/PS5-Independent/Palette-c71c8a89; no owner data included |
| Private owner test folder | PASS, 35 native + all 55 owner files verified after copy | outputs/PS5-Independent/FullTest-c71c8a89/PPSA99145; no saves, keep private |
| PS5 runtime/gameplay | owner-reported gameplay plus matching e16 log/screenshots | limited scenario; remaining HUD/menu bugs; no extended/full-game acceptance |
| PS5 stable 60 FPS | FAIL for the provided e16 observation | final five-second window 44.60 FPS; prior wait/cache optimizations remain enabled |
| Native portrait camera | actual PC bounds and final 1280x720 HUD images PASS | exact original setter call; original CB fields retained; new PS5 check pending |
| Cinematic mattes | synthetic policy and real Story-arrival image PASS | full-width black upper/lower bands verified on PC; new PS5 check pending |
| English PlayStation notices | 22 equal-length UTF-16 substitutions; archive regression PASS | visual verification pending; other Xbox references/languages excluded |
| Ordered async GPU completion | recorder/GPU-gate/FIFO/exactly-once/default-failure tests PASS | full software-Vulkan timeout; OFF by default, no PS5 performance acceptance |
| Actual fast-constant wrapper ABI | PASS, 16 generated-code cases | existing bound tail verified; no speculative new HLE entry binding |
| Private UI preservation | 64 texture and nine Flash records / 11,443-entry texture index plus GPU images | 128x128 capture is a full-body preview, not an identified HUD head |

Private complete diagnostics remain under out/ and local test artifacts. Test
summaries distinguish bounded observation from clean exit and visual proof.
The null renderer cannot verify Vulkan, input/audio on PS5 or playable gameplay.
Its optional CPU reference renderer can provide diagnostic images only.
No console was contacted for these checks.

Failures corrected so far: Windows CRLF in transferred shell scripts; an unused
Windows-only WIC module selected by the Linux harness; missing headless error
dialog names; quoted argument inspection in the installed SDK Windows driver;
and quoting the forced Vulkan include path in CMake. Original diagnostics and
before/after host-driver hashes are retained. The native link recipe is unchanged.

GPU failure propagation was hardened in the Vulkan SDK: failed submits/waits
throw before reaching a guest completion callback. A failed fence remains
pending and its resources are not recycled; fence waits are bounded at five
seconds. Native compilation and successful Vulkan execution are verified;
failure injection is NOT TESTED. No clean gameplay result is claimed from this change.

Console deployment c85392b72f89130e to PPSA99148 PASS: 75 files read back and SHA-256 verified, previous raw signed eboot/manifest/param/settings backed up under ignored out/audio/deploy-c853/backup. Existing settings preserved; PPSA99145 untouched. Console idle before/after. Application deliberately not launched, per owner request to test manually. Home/launcher audio remains AWAITING_OWNER_TEST.

Home refresh 2026-11-10 PASS: seven presentation files backed up/uploaded/readback verified, PS5 Upload APP_REGISTER succeeded for PPSA99148. Read-only verification confirms the title registry now references the correct snd0.at9 and icon0.png with new cache timestamps. No direct database writes, uninstalls, launches or reboots were performed. Visual/audio/haptic acceptance remains the owner's test. Receipt: LAUNCHER_RESULT_2026-11-10.json.
