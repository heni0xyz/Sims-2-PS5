# 0.1.0-alpha.1 — validation status

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
No private klogs, game screenshots or save files are included in public source.
