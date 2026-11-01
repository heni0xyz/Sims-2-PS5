# Font reconstruction and Continue readiness candidate

Work remains PC-only. Existing PS5 deliveries, owner files and saves are
preserved. New code is authored for szampan under GPL-3.0-only; runtime upstream
MIT notices remain. No generated guest source is edited.

## Font-only 4x reconstruction

`src/game/font_upscale.cpp` recognizes three exact USA English font atlases by
dimensions plus complete first-level data and palette hashes. Public code
contains fingerprints, not glyph pixels. It decodes the actual bound owner
P8 texture using the existing SDK decoder and reconstructs it once with
separable Catmull-Rom interpolation, limited between neighboring coverage
samples to prevent ringing into empty atlas padding.

Frankfurter24 and OneStroke24 become 2048x2048 from 512x512; System12 becomes
1024x512 from 256x128. Original UVs, glyph positions, spacing and palette
colour/alpha are preserved. No AI service, new font license or game texture
download is required. This is smoother bitmap reconstruction, **not recovery
of original vector outlines**. Native 4K quality requires visual review.

The optional palette-aware host replacement seam is shared with the CC0 HD
prompts. It never changes guest bytes or archive sizes, so the font path also
works with direct ISO streaming. Nonmatching textures pass through. Each
matching atlas has one immutable host level to avoid mip bleeding; the GPU
uses the existing sampler. Maximum retained CPU pixels for all three atlases
are 35,651,584 bytes (34 MiB), plus their GPU copies and a temporary resampling
buffer during first use. No per-frame resampling occurs.

Native setting: `ui_font_upscale=true`. Set false for the original atlas path.
PC runner: `--font-upscale`; live diagnostic `headless_command.py
--font-upscale on|off` requires `--ui-layout-trace`.

Asset-free regression PASS: constant colour/coverage, 2x/4x extents, smooth
synthetic stroke edges, empty padding without halos, premultiplied synthetic
colours and invalid-input rejection. Windows Vulkan compilation PASS. Run
20261031T221339Z-3c19d5 bound Frankfurter24 at 65.624 seconds, generated
16,777,216 bytes and ran to its 900-second watchdog without fatal records.
Its readback was still 640x480 backing with a 3840x2160 final blit: that is
**not** font-quality acceptance at native 4K.

Run 20261101T103306Z-3c9725 again bound the atlas, reached the mode menu and
enabled actual 3840x2880 backing / 3840x2688 picture at 448 seconds. It then
failed the unchanged five-second Vulkan fence wait (result 2) at 494.949
seconds under SwiftShader while other local build work was active. Pending
resources were retained and the failure propagated. No timeout was relaxed
or mistaken for success.

Quiet repeat **20261101T105629Z-7e0740** completed its900-second bound without
fatal records. It produces three verified native4K readback pairs, frames1186,
1220 and1306: backing3840x2880, picture3840x2688, final3840x2160. The first
image has font4x enabled; calibration at482.861 disables it before frame1220;
calibration at609.617 enables it again. Frame capture also verifies the actual
512x512 guest font binding at82126380 in the Continue Game carousel draws.
The private comparison crop is `out/evidence/font4x/comparison.png` (original
above,4x below), lossless/unscaled crops from actual final frames. Visual gain
is **subtle smoothing**, with unchanged text placement/size; this is not a
native-outline font remaster. Remaining atlas/source-detail limits are visible.
This run did not reach the save-list dialog or gameplay at4K, and ended by
watchdog, not clean shutdown. PS5 font quality/stability remain NOT TESTED.

## Thin Continue background: concrete evidence and correction

Owner fragment `out/console/thin-menu-20261031T214847Z/klog.raw.log`, SHA-256
`b197f9520931519d196aec6fa4aa5bb4933fb4a94294de6389807f0ceb8e9892`,
identifies build 7bd91d7c and one run. Correct earlier background reads are
353/89. Faulty entries read **32/32 before** the original controller computes
321/57; no corrected provider reread appears afterward. The screenshot's
narrow background is consistent with stale initializer geometry. PC timing
usually rereads the correct values, so a correct slow-PC image did not
disprove the owner's intermittent console failure.

The new scoped hook runs original geometry function 0x140D00 unchanged. Its
original Reflow passes through the existing bridge. Only when the real item
count and both dimensions are ready, the target `0M2M_swf_container` Reflow
also restarts the dialog's existing `0M2M_swf` **a0 initializer**. That frame
and `swfAnim` interface were verified in owner data/original bridge use. No
fixed panel height, fake save entries, skipped initialization or generated
code edits are used. Other widgets/operations pass through; an already
initialized command is idempotent. Setting `ui_continue_refresh=false`
retains the old path for a controlled comparison.

PC run 20261031T215903Z-8ddb7d completed 540 seconds without fatal records:
first entry reads32 -> refresh -> original321/57 -> reads353/89; reentry
again reads32 -> refresh -> original321/57 -> reads353/89. Its save-panel
readback has a full-height background. This supports the correction but does
not prove the intermittent PS5 defect closed. Repeated Story/Freeplay Continue
entry on the matching new native build remains the acceptance test.

## Visible English storage wording

The private extracted-data patcher still replaces 22 console notices and now
replaces 17 storage references in three whole visible phrases. Shortened
wording preserves original UTF-16 lengths/padding, offsets and terminators:

- `Checking storage device. Don't turn off your PlayStation.` (10)
- `No save found on storage device.` (4)
- `Load save from the storage device?` (3)

Tests verify paths/identifiers and unrelated data remain unchanged. The owner
original is preserved. New private QuickData SHA-256:
`191803e45f121aac999dd1a627348807165d35488da24325be95f768fd28a582`.
Direct ISO streaming still retains original notice text; a runtime text
overlay has not been implemented.

## Performance stays open

Owner FPS fragment `out/console/fps-drops-20261031T214151Z/klog.raw.log`,
SHA-256 `c52c1d4a1d86dccf5430de6ab95cb28bbdcc32a474b2c29a0927cbc9d6390c24`,
is build7bd, one run and 47.59 seconds, not a complete boot/exit capture.
Its windows23..29 cover1587 frames with weighted mean22.304882ms =44.8332FPS.
Last window29 is25.488314ms =39.23FPS; measured GPU work is approximately
6.976ms/frame. These overlap and cannot simply be subtracted into exact CPU
cost. CPU recording/wait/draw overhead needs a controlled native comparison.

The parser now correlates only identical window IDs, keeps runs/builds apart,
and exposes missing/error GPU data as unknown. Sampled guest call-wall time
includes waits and is not statistical CPU attribution.

An opt-in WRITE callback completion experiment rendered the Story lot's
cinematic fly-through in 480-second run20261031T214738Z-0a2882 with the actual
recording backend. The reviewed image is not interactive gameplay proof. This
does not establish PS5 speed or mixed READ/WRITE callback ordering. The title
continues to package **gpu_callbacks_async=false**. No sustained60 claim.

## Source-release restore checks

The first strict exporter check caught the foundation's actual license filename
(`LICENSE.md`), which is now verified correctly. A subsequent restore caught
Git archive line-ending conversion of SVGs: `.gitattributes` now enforces LF
for those hash-identified public assets. No PNG, mip pixels or owner selector
identities changed.

Source checkpoint7d3e70c exports1,755 files. Its restored tree passes all19
Python PS5-tool tests and rebuilds the public HD pack with the identical SHA-256
`eee5edbed7960a310474f3a3222a70eef51bedbdb3b782ade0ccecba719a335a`.
The preceding3247513 export, with identical C++ code, builds host `xbe_recomp`
and `iso_verify` through46 Ninja steps using the public cached Zydis/toml++
sources. The new verifier passes owner ISO preflight. The newly built
recompiler regenerates all71 output files (65 C++), **all byte-identical**
to the current reviewed generated baseline. This proves source/generation
restoration, not complete game correctness. No game content is added to the
archive; the private restore fixture is outside Git/release content.
