# Console performance capture — 9 November 2026

Runtime `0aaecaff5112dfb3dddcca1f`, run `fps-0aaecaff-20261109-01` executed on the
authorized console. Startup build identity and raw deployed SELF digest matched.
Owner XBE SHA-1 matched the supported USA image. Ordered WRITE GPU completion,
fast handles and selective object wakes were observed. OS thread CPU timers and
64-bit GPU timestamps (10 ns period) are supported on this console.

The owner navigated into gameplay and reported close/far camera transitions.
These are approximate host-receipt markers, not synchronized in-game events.
Analysis excludes two seconds after each marker and one second before the next
boundary, and retains only complete detailed frame windows within each phase.
Menu/intro data is excluded from the following comparison.

| Measurement | Close camera | Far camera |
| --- | ---: | ---: |
| Complete detailed windows / frame intervals | 5 / 1507 | 4 / 1188 |
| Frame-weighted FPS | 59.68 | 58.91 |
| Mean interval (ms) | 16.757 | 16.974 |
| One-second FPS report range | 57.7–60.1 | 55.9–60.4 |
| Window p99 range (ms) | 17.45–18.30 | 17.74–21.54 |
| Longest captured interval (ms) | 49.64 | 63.90 |
| Mean CPU renderer draw wall time (ms/frame) | 6.63 | 8.66 |
| Approximate GPU segment time per frame (ms) | 4.34–6.68 | 6.38–6.84 |
| Guest thread 0x24 OS CPU utilization | 75.5% | 98.7% |

Output is 3840x2160; logged internal backing target is 3840x2880, with the
3840x2688 source-picture region. These distinct dimensions remain in the raw log.
No claim that all original assets acquire new detail is implied.

The short measured scene is close to 60 FPS on average, but it **fails stable
60** acceptance because of repeated slower windows and occasional long frames.
The window-local 1% lows must not be presented as a global whole-run statistic.
The profiler changes runtime cost; this is not a production-performance verdict.
No console screenshot was captured; gameplay/camera phase acceptance is based
on the owner's report, not autonomous visual verification.

## Interpretation and next experiment

One guest thread saturates as camera distance increases; native renderer CPU
draw cost rises about 2 ms/frame. GPU segment cost stays below the 16.67 ms frame
budget in these windows. This makes CPU submission/game work a concrete next
investigation, without proving that the GPU can never limit another scene.
Inclusive CPU scopes and approximate GPU/frame windows cannot be added into a
single critical-path total. Saturated CPU can include polling, not only useful
simulation work.

Late reports show about 1683 draws, 111k vertices and 11.7 MB captured upload per
frame. XDK draw time is about 9 ms, including about 2.95 ms send-state and 4.87 ms
recording; these overlap. Sampled wall hotspots include the known wait wrapper
0x1F9370 and kernel ordinal 234 (NtWaitForSingleObjectEx). Their blocked time is
not evidence of an expensive pure CPU function. No wait/fence or initialization
was removed to raise a counter.

An important baseline difference is now recorded: `VkPerfClock` uses the fast
TSC-backed deep clock only when deep profiling is enabled, while the ordinary
profile uses `steady_clock`. The current console measured deep wall reads at
11 ns and thread CPU queries at 865 ns. The ordinary steady-clock read cost was
not measured. Before choosing a clock/capture optimization, measure that cost
and compare the same scene using consistent profiling clocks. Do not attribute
this test's improvement over older reports solely to game optimizations.

## Termination and restoration

The test requested a 300-second in-title limit. The process disappeared at
300.8 seconds, with no captured fatal signal, GPU fault or runtime CRASH record.
The shutdown milestone was not captured in this diagnostic run; report this as
observed termination, not a verified clean-shutdown marker. The host runner did
not issue an external kill.

The raw signed original eboot was backed up with FTP SELF decoding disabled,
inspected (valid integrity), restored and verified by readback digest. It is
runtime `f48a3b24684ecd4f2479a7c0`. A separate 30-second startup check observed
its guest entry and queued frame, its own SHUTDOWN milestone and exit to an idle
console. Assets, settings and hdd were not replaced or removed by deployment.
The game may perform its ordinary save/cache writes while running.

Private evidence: `out/ps5-debug/fps-0aaecaff-20261109-01/` contains raw/app logs,
phase markers, phase/performance summaries, original raw eboot, settings snapshot,
backup integrity report and restore-run evidence. Small public-safe numeric
receipt: PERFORMANCE_CONSOLE_RESULT_2026-11-09.json. No code was published.

`tools/ps5/run_game_perf_console.py` reuses the existing console runner. It checks
idle state and source/deployment identity, validates the owner XBE, preserves the
raw eboot, stages a consumed one-shot duration file, captures before launch and
restores the previous executable only while the console is idle. Gameplay
navigation remains manual. A matching build plus process exit is not a gameplay
or stable-FPS PASS.

## Follow-up optimizations

Three independently built and console-tested variants retain the same O2/deep/guest profiling configuration. New observation clock benchmark measured 994.713 ns for steady_clock versus 14.911 ns for the calibrated TSC helper; guest timing/pacing were not changed. Variant 2ab40efe averaged 59.73 FPS far-camera, reducing guest thread CPU to about 56.7%, but still showed an 87.69 ms worst interval.

275cc359 compared complete pipeline-cache snapshots before persistence. It exposed synchronous cache writes around 34 ms. Variant 4801f87a moved checked atomic persistence to one bounded worker, coalescing pending snapshots. Console log showed successful writes, zero persistence failures, background I/O 36–41 ms, main-thread query around 0.25 ms. Far-camera phase: 10520 measured intervals, weighted 59.8387 FPS, guest thread CPU about 51.95%; worst interval still 48.660 ms. Some later five-second windows stayed near 59.94 FPS with maxima around 17 ms. This is a substantial CPU improvement; stable 60 for the whole scenario remains unproven. No fatal application/GPU event captured. Own shutdown marker missing; original signed eboot restored/hash verified after each test. No full installer-to-Play acceptance yet.

Exact private logs/phase summaries: out/ps5-debug/fps-fastclock-2ab-20261109-01, fps-cache-275-20261109-01, fps-async-4801-20261109-01. SDK commits: a18ec60 (clock), 02dd146 (snapshot), b7d6b48 (atomic background persistence). Host clock/cache/concurrency tests and full host game link passed. Next compare a lighter performance-logging build and identify remaining long intervals before claiming stable 60. Public installer kit has not yet been refreshed to this accepted performance candidate.
