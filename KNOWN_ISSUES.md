# Current PS5-only launcher candidate

**8db5af5a, PPSA99145:** native build/package checks PASS; the owner completed
the preceding full extraction/compilation. Real PS5 cached-game handoff, memory
and Vulkan initialization/presentation, 60-second and repeated 30-second normal
shutdowns PASS. Older private game mode0644 is repaired without recompilation.
Initial checking now gates input and selects Play when ready.

Host tests cover 13 update transactions, 10 HTTP cases, complete public-bundle
apply/private-data preservation, UI/startup/navigation/carousel, audio and
extraction contracts. Real PS5 HTTPS metadata checking PASS. Full public update
download/helper/apply/recovery still needs hardware acceptance. Home files and
registration are verified; Home appearance/audio and rumble strength need owner
observation. A complete fresh ISO-to-Play repeat remains separate from cache tests.

The bundled game kit is f419-derived. Latest runtime optimization source is
included, but gameplay performance must be measured with a refreshed kit.
Stable 60 FPS, long-term stability and full story/freeplay coverage are open.
The intermittent thin Continue background remains under investigation.

Owner-supplied gameplay previews are presentation resources. Extracted game
assets, private klogs and personal saves are excluded from source. See
recomp/docs/LAUNCHER_PLAY_FIX_RESULT_2026-11-10.json for machine-readable scope.

## Historical 0.1.0-alpha.1 validation

Candidate: native build `f48a3b24684ecd4f2479a7c0`, title **PPSA99145**, display
name **The Sims 2**. The faster configuration uses ordered WRITE-completion
callbacks (`gpu_callbacks_async=true`); it uses the same binary as the earlier
metadata-fix build.

| Area | Evidence / limitation |
| --- | --- |
| Native build/package | PASS locally: SELF and debug ELF validated, all package file hashes checked. |
| PS5 runtime | Owner reports successful launch, menus, gameplay and improved speed with ordered callbacks. |
| Game bring-up | Limited gameplay works; full story/freeplay coverage is NOT TESTED. |
| Performance | 60 FPS cap; sustained 60 FPS NOT VERIFIED. The preceding synchronous f48 log was about 44 FPS over its final 15 reports. No quantitative follow-up log was supplied for the faster settings. |
| 4K | 3840x2160 output; native raster policy backing 3840x2880, picture region 3840x2688 before presentation. Sustained 4K60 NOT VERIFIED. |
| Installer | Checked extraction/readback and optional source-ISO deletion are implemented and host-tested. Full latest PS5 install/delete acceptance remains unconfirmed. |
| Continue UI | Intermittent thin menu background remains OPEN. |
| Stability | Long-term gameplay, all interactions and clean exit scenarios NOT TESTED comprehensively. |

Host regressions cover guest/platform contracts, UI policy, clipping/anchors,
font filtering, installer safety and completion ordering. Software Vulkan PC
readbacks establish rendering correctness for observed frames, not console FPS.
No private klogs, extracted game data or save files are included in public source.
