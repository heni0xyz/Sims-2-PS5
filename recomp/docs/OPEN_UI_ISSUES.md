# Owner test feedback and active investigations — 30 October 2026

The linked e16fc615a5546cb6d2323393 candidate is a TEST build. It contains
16:9/60-cap settings, CC0 PS prompts and partial UI correction, not complete
UI acceptance or stable-60 proof. Previous packages and raw owner data remain
intact. No console was contacted by this agent during this work.

## Confirmed observations

- Owner PS5 screenshots and klog establish real execution/gameplay of e16.
  The default 4:3 baseline and the new 16:9 candidate must stay distinguishable.
- Circle/HUD edge anchoring works, but the native head remains right of its
  backdrop. Synthetic XML correspondence tests are not evidence that the
  native callback consumes those fields. Both attempted CB adjustments are
  rejected as a runtime fix. Native callback 0x147D60 has now been independently
  identified. Its thread-local tag did not survive the game's own render queue,
  so that approach and API were removed. The exact head-camera setter call is
  now transformed instead; actual PC gameplay images verify corrected alignment.
  See HUD_PORTRAIT.md. New PS5 image verification remains pending.
- The cached 128x128 target was initially misidentified as the HUD head.
  Its actual readback shows a full-body Sim preview on magenta. Preserve it
  for 4K research, but do not label it as a captured HUD head texture.
- Save selection 0M2M sometimes has a collapsed top panel, with text extending
  below it. Owner reproduces through Freeplay and Story -> Continue. A correct
  PC image also exists, so a single passing screenshot does not close this bug.
  The owner did not wait 5..10 seconds and is unsure whether it self-expands.
  Do not assume it is a harmless transition. Re-entry sometimes changes it.
  The original Flash panel obtains _boxHeight/_boxHeight2 via getVariable;
  PC-only bridge/state traces are being collected before any layout workaround.
- Story arrival cinematic mattes: corrected on PC. The real final 1280x720
  image in 20261031T102903Z-9b8edd/presented_03270.bmp has full-width black
  upper/lower bands with the actual scene between. Pixel evidence passes in
  out/evidence/letterbox-result.json. New PS5 validation remains pending.
- The disabled gray HUD icon has a black rectangular background; investigate
  its source alpha, combiner and blend state rather than blanket alpha changes.
- Owner requests UI text Xbox -> PlayStation. This should apply only to visible
  localization text, preserving save paths, asset identifiers and executable data.

## Performance evidence and concrete hypothesis

Private preserved log: out/console/e16fc615-20261030-215907. Raw SHA-256
78805edc3931ff9e23ce6d2a6e746b4666119a5de14c96234a329e66ef18a428.
It contains two different run IDs; tools/analyze_console_perf.py keeps them
separate. The new run covers about 50.9 seconds: reported 1-second windows
26.6..60.4 FPS, median 58.75; 2,682 observed frame intervals have weighted mean
18.85823 ms (53.03 FPS). Startup/loading and steady play are not separated by
those aggregate figures. The final five-second window averages 22.42372 ms
(44.60 FPS). There is no accepted stable 60 FPS result.

Verified SDK ancestry and actual console counters confirm that handle caching,
object-specific wakeups and file-case alias caching are inherited from 8f5b279.
Both old/new native profiles use O2, fast_handles=ON, object_wait_channels=ON,
guest/deep profiling=ON. Those optimizations have not been lost.

A concrete difference: the old project supplies an immediate completion
callback in its game glue; this independent runtime conservatively flushes
preceding GPU work before CallGuest. The log shows 5..8 ms of flush waiting
per frame at the farther camera. A possible optimization is ordered asynchronous
completion delivery after a real successful GPU fence. Do not revert to an
immediate callback, suppress GPU errors, or claim a speedup before measurement.

An opt-in FIFO completion prototype has an actual recording-backend unit test
(GPU gate, producer nonblocking, FIFO, exactly once and default failure
propagation). It remains OFF in both title and PC defaults.
The full software-Vulkan run 20261030T213500Z-5938ff failed with VK_TIMEOUT
after 703.84 seconds. Conservative runs 20261030T221730Z-fde6c3 and
20261030T222338Z-88ae2f failed at 224.30 / 131.88 seconds. Crucially the
previously successful unchanged EXE also timed out at 137.07 seconds in
20261030T222653Z-9566c1, with the new asset view and a new Z cache. The new
runtime with old assets/warm cache then ran 1,000 seconds in
20261030T223028Z-53f2cc without fatal records. This does not isolate which
asset/cache/host condition causes the timeouts. Keep evidence and continue
single-variable comparisons. The five-second fence failure remains enabled;
resources are not recycled and guest completion is never fabricated.

The sole nonzero SDK-symbol difference relative to the old configuration is
SetVertexShaderConstant1Fast at 0x272FD0. The actual generated wrapper passes
ECX/EDX/stack dword count to the already-bound 0x272EF0, after its conditional
guest scratch copy. Sixteen ABI cases pass. Binding it directly is not an
established rendering fix and would omit that scratch-mirror behavior.

tools/ps5/patch_ui_text.py passes equal-length UTF-16/offset/canary checks and
patches 22 English checking/loading/saving notices in a NEW private quickdat.arc.
Other Xbox references/languages are not claimed changed. Patched source SHA-256
33e1699b4c1ad5ebd5ce82b197f2ee68d3c3d3803395bbbcd3cff13d519b83fa.
The raw owner archive is unchanged. Actual notice-image verification is pending.

## Retained research data

tools/preserve_ui_resources.py stores complete original records under ignored
private/ui-research, retaining source hashes and native headers. There are 64
selected UI records and a full index of 11,443 texture entries. One-shot Vulkan
captures retain cached small targets, exact BGRA bytes, lossless RGBA PNGs and
frame traces. The current archive with seven targets is
private/ui-research/20261030T211526.496073Z. The newer archive
private/ui-research/20261031T101628.506810Z also preserves nine complete
original Flash records. All such owner content stays private.

All source work remains local. Known issues above remain open until tested.
