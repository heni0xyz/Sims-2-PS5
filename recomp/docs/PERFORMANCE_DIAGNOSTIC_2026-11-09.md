# FPS diagnostic build — 9 November 2026

The CAS f419 build emitted basic renderer FPS/work/wait averages. Detailed frame
intervals, per-thread CPU snapshots, scope timing and GPU timestamp queries were
compiled out. New runtime **0aaecaff5112dfb3dddcca1f** enables the existing deep
and sampled-guest profilers, retaining O2, NDEBUG, g1, fast handles, selective
object wakes and all current CAS/UI/graphics fixes. This is an instrumentation
build, not a claimed new speedup or verified stable 60 FPS build.

## Owner test

Replace only the private game's `PPSA99145/eboot.bin` with the file in
`outputs/PS5-Ready/FPS-Diagnostic-0aaecaff` at the workspace root. Preserve the
existing assets, hdd, saves and settings. This is the game executable, not the
PPSA99148 installer. No ISO extraction or recompilation is needed for this test.
The regular installer kit remains untouched.

Capture klogsrv from before launch through the end of gameplay. Use the same
lot/save, normal simulation speed and current graphics settings. Allow initial
loading/shader warm-up, then observe about one minute with the camera close and
one minute at maximum distance. Note the transition time if possible. Save the
complete log, including startup (profiling configuration and GPU support).
Keep the normal f419 eboot as rollback/reference. Compare the same scene with
that low-overhead build before accepting any performance gain.

## Measurements and limits

- Every roughly one second: FPS/mean frame time, renderer draw/clear/copy/present
  wall costs, title recording/wait breakdown, XDK/send-state cost, draw/vertex/
  upload counts, shader/PSO creation and framebuffer reuse.
- Every roughly five seconds: frame-interval mean, p50/p95/p99/max and window
  1% low; per-thread OS CPU time/support/error and snapshot age; inclusive
  kernel/locking/wait/relock/file/shader/pipeline/acquire/submit/present/fence/
  audio/upload/input scopes; memory peak where the OS supports it.
- GPU timestamps are queried only after the existing submission fence has
  completed, with availability and counter-width checks. No extra blocking
  query wait is added. These measure submitted segments, not GPU utilization,
  scanout latency or precisely correlated full-frame GPU time. Unsupported or
  failed queries remain explicitly unavailable.
- Sampled indirect guest/kernel call targets and wall time identify candidates.
  They include blocking/child work and are not per-function CPU attribution.
  Direct guest calls outside those seams are not a complete call profile.
- The startup `perf: build-config` marker records deep/sample flags and current
  FPS cap/vsync. Existing presentation records identify actual internal targets
  separately from output dimensions. Build/run IDs prevent mixing sessions.
- Scopes overlap and must not be added into total CPU utilization. Thread
  snapshots can be stale. Logging, CPU queries and instrumentation add overhead;
  these results cannot independently certify final release performance.

`tools/analyze_console_perf.py <klog> --output <summary.json>` retains the
configuration, scope, thread, renderer and resolution evidence. It warns about
unsupported CPU readings and missing GPU measurements. Zero GPU segments mean
unknown GPU cost, not zero milliseconds. Window-local 1% lows are not a global
whole-run 1% low. Menus, loading and gameplay must be assessed separately.

## Built and checked

- PS5 native object build: PASS, 122 audited commands, O2/g1, deep ON, guest
  sampling ON, fast handles and object wait channels ON; startup probe OFF.
  Actual command is the CAS command in CAS_WIDESCREEN.md plus
  `-DeepProfile -GuestProfile`. Build log: `out/fps-diagnostic/build-objects.log`.
- Evaluated RADV recipe retained. Native ELF layout/import checks, conversion,
  signing and SELF integrity: PASS. Debug symbols preserved at
  `out/fps-diagnostic/native-0aaecaff/llvm-pie.elf` (and eboot.elf).
- Eboot SHA-256:
  `9b62d1933fc183c0362fb81351b9cff65f41db61b7412c78574e2e63b1987a71`.
- Six analyzer regressions PASS, including mixed sessions, frame weighting,
  wrong-window/error GPU rejection, missing GPU queries and attribution fields.
- Asset-free host telemetry test PASS: timestamp wrap/unsupported inputs,
  frame percentile/slow-frame retention, drain-once behavior, active scopes,
  sampled-call aggregation and periodic reporting. This is a synthetic test,
  not a game/GPU/console measurement. Logs: `out/fps-diagnostic/telemetry-*.log`.
- SDK source unchanged. The normal public installer kit still contains f419;
  the diagnostic kit is isolated in `out/builder-dev/kit-fps-0aaecaff`.

At packaging time console execution and live timer support were not tested.
The subsequent console run verified both timers and captured owner-reported
gameplay, averaging 59.68 FPS close and 58.91 FPS far in the selected windows;
stable 60 FPS still fails. See PERFORMANCE_CONSOLE_2026-11-09.md for measured
CPU/GPU costs, phase limits, termination and restoration evidence.
