# Independent game pipeline

1 November first-run installer: a title-owned checked extraction transaction
runs before kernel/game initialization when complete extracted assets are
missing. A temporary existing Vulkan display draws Open Sans/Kenney public
resources and is destroyed before the game backend. Disc/crypto services are
shared; source ISO stays read-only until a separate explicit post-install
choice. Receipt/notice validation and versioned Z-cache backup protect updates.
See INSTALLER_WORK_2026-11-01.md. Host-only streamed ISO diagnostics remain.


1 November: the same optional host-texture seam now receives the resolved
palette. The title can identify/reconstruct exact owner font atlases at 4x;
glyph metrics/UVs and guest bytes stay unchanged. Continue refresh is scoped
to real computed geometry and replays the existing dialog initializer through
the original bridge. No fixed-height or generated-code patch is used. See
FONT_CONTINUE_2026-11-01.md for algorithm, memory and acceptance boundaries.

New optional public-resource path: selected 32x32 BC2 prompts are identified
by exact original/patched mip fingerprints and replaced only on the host GPU
with immutable 256x256 BGRA mip chains. Guest descriptors, pointers, UVs and
readback sizes remain original. The default callback is null for other titles.
New title-owned ISO bootstrap reuses SDK Disc streaming and strict USA hash;
root ISO input is explicit and rejects ambiguity. Continue diagnostics wrap
the original geometry/provider functions without changing their results.
See HD_ISO_2026-10-31.md and DISTRIBUTION.md.

The additional 4K mode registers an optional host render-scale policy in the
Vulkan backend. It raises the **entire** colour/depth/post chain uniformly,
retaining guest surfaces, address calculations, readback byte counts and
shader semantics. At 3840x2160/100% it uses scale 6: backing 3840x2880,
visible picture 3840x2688. Presentation downsamples vertically. The previous
height-only default remains selectable. Actual dimensions are logged once
per size change and paired with real backing/final GPU readbacks for PC
verification. See RENDER_4K.md for the crop rationale and exact limitations.

The title configuration and generated C++ start from the verified USA XBE.
No config, hooks or generated game source from the existing Sims 2 game project
is used. These stages have separate responsibilities:

| Stage | Implementation | Evidence |
| --- | --- | --- |
| Read ISO / verify XBE | public SDK Disc parser, iso_xbe wrapper | exact USA SHA-1 |
| Resolve XDK bindings | public xbe_symbols and licensed SDK signature metadata | independent scanner logs and config/game.toml |
| Static translation | TGP482 tools under sdk/tools | private generated/independent, generation manifest |
| Xbox CPU and HLE | MIT runtime-sdk dependency | pinned separately; authors credited |
| Native platform | prior Codex PS5 adapters, individually attributed | PLATFORM_IMPORTS.json |
| Graphics / title | runtime Vulkan backend, generated PS5 foundation, completed RADV | established template link recipe |

The upstream SDK remains the source for the recompiler. The Linux/Vulkan SDK
fork is an explicit third-party runtime dependency, including its public HLE
fixes. Its code is not represented as newly authored game-engine code. The two
headless builds use separate caches so their inline CPU helper versions cannot
be mixed inadvertently. The original working port is retained separately.

The first raw upstream baseline built, resolved kernel imports and loaded game
data, then faulted in original Xbox graphics hardware code. GDB identified the
caller at 0x271570. Its one argument is a presentation-parameter pointer with
640x480 and format 0x1e; decoded guest code returns with ret 4. The independent
config now binds it to the runtime's D3DDevice_Reset HLE implementation. This
performs software presentation reset; it does not skip initialization.

All generated output is regenerated through tools/recompile.py. Fixes belong
in config, the SDK or authored hooks, never directly in generated C++.

The PS5 candidate has a separate title identity, PPSA99145. The first baseline
used 4:3/30. The new UI candidate selects 16:9/60 with authored prompt/layout
patches; see UI_PATCHES.md for the clip-space seam and evidence boundaries.
The original guest clock remains intact. Native 4K and performance need
independent validation.
No new console or gameplay result follows from successful linking.

## Completion callbacks

The independent decoded body at 0x275750 takes three stack arguments and
returns with ret 0x0C. It writes the callback and context at NV2A methods
0x1D8C/0x1D90 and requests software interrupt 6 or 7. The game's command 3
at 0x1FC530 uses it to call 0x1FA8C0, which releases the frame semaphore.
Dropping those hardware methods leaves the main thread waiting at 0x1C2520
after the first two primed frames. The portable SDK now has an optional
D3DDevice_InsertCallback binding: drain preceding commands, finish submitted
GPU work, and invoke the guest callback outside the GPU lock with its context.
This conservative synchronous implementation still needs measured runtime
validation before asynchronous optimization. No generated C++ is patched.

## Owner data

The required root files include default.NGH and the 2,960-byte sims2.bin DSP
image, as well as default.xbe, DATA and movies. The initial test directory
omitted the first two. The resulting deliberate disc-error loop at 0x1F7EE0
was a data-preparation failure, not a missing game-code patch. The new
iso_assets tool extracts directly through the SDK Disc parser into a new
directory; check_assets.py refuses a launch with missing required members or
wrong XBE/DSP hashes. Full content and generated output stay outside Git.

## Palettized images

Both the CPU reference and real Vulkan initially rendered gray P8 logo/UI art
with opaque black rectangles. The fresh scanner left D3DDevice_SetPalette at
zero. Independent inspection of the owner XBE identified 0x272020: two stack
arguments (stage, palette), device palette array at +0xF98, NV2A method
0x1B20+stage*0x40 and ret 8. This matches the scanned CDevice_Palettes layout.
Binding that function to the existing SDK HLE supplies the palette to Vulkan;
the corrected real readback shows the coloured logo, UI and transparency.

The CPU reference also unconditionally passed a null palette to DecodeLevel.
It now supplies the stage palette and includes palette contents in the image
cache key, preserving same-frame palette changes. The asset-free regression
tests actual colour/alpha decoding and cache reuse/invalidation for all four
palette size codes. No owner texture or generated C++ is modified.
