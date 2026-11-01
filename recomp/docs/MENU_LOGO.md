# Mode-menu logo left margin — 31 October 2026

The owner identified an excessive horizontal margin in the 4K preview.
The general UI aspect policy compresses around the center, adding a gutter
to artwork authored at the left edge. The logo needs a left anchor while
the mode selector remains centered. Authored correction: OpenAI Codex for
szampan. No game executable, generated code or owner artwork is modified.

## Identification and correction

PC-only, bounded texture/palette observations identified the exact USA
`s2c_logo`: P8, 512x512, seven mips. The first-mip hash with the pinned SDK's
Hash function is 01e493ab7db0d458, independently matched to the original ARC
record and actual bound GPU texture. The palette can change; it is not used
to identify the artwork. Only irreversible metadata is stored in source.
Original pixels/observations remain ignored under the private run directory.

The same texture is also shown large and centered during startup. Therefore
the correction additionally requires the known direct UI shader and evaluated
guest draw bounds entirely in the upper-left quadrant. Large/centered logo
draws, unknown textures, other shader paths, 4:3 and disabled edge anchoring
retain their previous mapping. The selected logo keeps its width/height and
Y position, but uses x_offset=0 with x_scale=45/56 rather than adding the
center gutter. This removes 125.714 pixels at a 1280-wide output or 377.143
pixels at 3840. No special case depends on the selected render resolution.

The SDK adds a mutually exclusive optional extended draw callback. It offers
a read-only lazy query of evaluated guest geometry, valid only during that
callback. Shader evaluation occurs only when the title asks for the bounds
of this candidate texture; world and ordinary UI draws take the old path.
Default/legacy callback users are unchanged. Guest memory, vertices, shader
programs, depth state and the runtime's 4K render-scale policy remain intact.

## Actual evidence

Baseline mode-menu frame: PC run 20261031T152131Z-98284a, present 2045.
Texture research run: 20261031T152953Z-b4947f, 480.25-second bound, no fatal
records. Actual texture and palette images/metadata: ui-textures/ in that run.
Corrected run: **20261031T153852Z-ac1922**, EXE SHA-256
ff1256314391f81f979fd4873cb78192b49d6ae1d5e95cde2b2bb873fde26fb4.

At 94.228 seconds the renderer selected the actual logo bounds
32.031,38.431..135.431,141.831, scale 0.803571463 and left anchor.
Actual 1280x720 readbacks at presents 2023 and 2057 show the logo at the left
and the mode selector still centered, with the PS confirm prompt. Reviewed
PNG: out/screens/20261031T153852Z-ac1922/presented_02057.png, converted
losslessly without cropping/resizing. The run ended by an explicit diagnostic
stop at 284.906 seconds, with continuing frames and no fatal records;
clean_process_exit=false. No console was contacted.

Asset-free regression tests cover exact texture/shader selection, corner
bounds, rejection of the centered startup logo/other same-size textures and
the left-margin transform, alongside existing UI/HUD/matte/portrait checks.
PC menu verification PASS. New PS5 runtime/performance remains NOT TESTED.
The prior 4K buffer evidence belongs to the e9dd candidate; this patch leaves
that policy unchanged but is not a new 4K gameplay/FPS acceptance test.

## Diagnostic tools

`tools/run_headless.py --ui-texture-trace` opts into bounded owner-art
observations on PC; ordinary UI tracing does not enable this heavier probe.
`tools/headless_command.py --capture-draws` requests the existing one-shot
GPU capture and requires `--ui-layout-trace` on the runner. Both write only
under the active private test run. Never publish the extracted artwork.
