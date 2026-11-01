# Runtime bring-up, updated 31 October 2026

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
