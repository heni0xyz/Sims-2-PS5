# Native-detail 4K candidate — 31 October 2026

This is an additional mode for the independent recompilation. It preserves
the existing guest code, virtual surfaces, texture formats, 16:9 projection,
UI anchoring, portrait camera and controller prompt patches. No generated
source or original executable is edited. Authored by OpenAI Codex for szampan.

## Actual dimensions

The retail frame is 640x480; the visible picture occupies y=16..464,
640x448. Existing height-based scaling at a 2160-high output selects 4.5,
yielding a 2880x2160 backing and only 2880x2016 visible pixels. That is
upscaled to 3840x2160 and is not native 4K detail.

The optional `native_render_resolution=true` policy selects the larger of
output-width/guest-width and output-height/picture-height (at render_percent
100), while retaining any higher baseline scale. For 4K it selects **6**:

| Stage | Dimensions |
| --- | --- |
| Guest frame and guest pointers | 640x480, unchanged |
| Host colour/depth frame | 3840x2880 |
| Host visible picture | 3840x2688 |
| Display/presented image | 3840x2160 |

The extra vertical detail is downsampled. Neither picture axis is upscaled.
This is deliberately uniform scaling throughout colour, depth and post
targets: existing copy rectangles and shader texel-step code assume a
single scale. Changing only the frame to an anisotropic size would break
those assumptions. Normal textures and movies retain original source detail.
Guest readbacks retain original dimensions and byte counts. Device image and
framebuffer limits are checked; target allocation failures are fatal with
dimensions in the log, rather than skipped draws.

Default settings leave the policy disabled, retaining existing desktop/Linux
behaviour. The new 4K package selects 3840x2160, policy enabled, render_percent
100, 16:9, 60 FPS cap, and the previous UI/portrait switches. Async GPU
callbacks remain disabled. A cap is not a measured frame rate. 4K increases
GPU work and memory; the unchanged earlier d8e6 delivery remains available.

## Reproduce

Use the recorded `tools/ps5/build_objects.ps1 -Mode Profile -DeepProfile
-ObjectWaitChannels -Jobs 1` with the paths in ENVIRONMENT.md. Export with
`tools/ps5/export_link_inputs.py`, then in the configured Debian VM run
`tools/ps5/link_and_package.py` with its documented inputs/foundation/Vulkan/
SDK arguments **plus `--resolution 4k`**. The established template title
linker and RADV archives are reused unchanged. Validate with
`tools/ps5/validate_delivery.py --extract`, then create the private owner
copy with `tools/ps5/stage_private_title.py --dualsense --playstation-text`.
No owner assets are included in source bundles or the native-only zip.

The PC runner has `--output 3840x2160 --native-resolution` for a full 4K
offscreen run. To avoid software-rasterized 4K movies during inspection,
start without `--native-resolution`, with `--ui-layout-trace`; once a lot
is loaded, `tools/headless_command.py --native-resolution on` asks the
normal renderer settings refresh to rebuild targets at scale 6. Capture via
`--snapshot`. This diagnostic is PC-only and never skips guest startup.
Every run remains bounded; the existing five-second GPU fence bound remains.

PC run `20261031T113329Z-ad15eb` completed its 1,000-second bound with no
fatal records. At 772.523 seconds the diagnostic enabled native detail;
at 776.199 seconds the real renderer reported scale 6 and no upscaling.
Paired host/final readbacks at frames 2251, 2272 and 2299 confirm the above
dimensions. `tools/verify_4k_capture.py RUN_ID` checks the pairs, extent log,
nonempty image and hashes. The reviewed image is the actual 3D island/mode
menu, with the PlayStation confirm prompt. PNG conversion is lossless;
`out/screens/20261031T113329Z-ad15eb/presented_02251.png` is 3840x2160.
The lower-resolution controls preceding the switch were diagnostic startup,
not a full 4K-from-launch test. A lot was not reached in this 4K observation;
Story gameplay/HUD, clean exit and PS5 performance are not accepted by it.
The runner ended the process at its bound (`clean_process_exit=false`).

## Evidence requirements

The renderer logs actual `backing`, cropped `picture`, `output`, `viewport`,
`scale` and `no_upscale` only when their dimensions change. Offscreen readback
files preserve both the actual host target and final image. A 3840x2160 PNG
alone is insufficient: correlate its frame and log with a 3840x2880 host
readback. PS5 output, frame pacing, visual correctness and stability must
still be verified on the new binary; PC software Vulkan is not PS5 evidence.
