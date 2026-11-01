# Independent PS5 presentation patches

31 October: mode-select logo anchoring now has actual PC verification.
The identified small upper-left logo uses a left anchor, while the same
artwork's large startup draws and centered selector retain their mapping.
See MENU_LOGO.md for identification, bounds, real readbacks and limitations.

Authored by OpenAI Codex for szampan. Kenney Input Prompts 1.5A artwork is
CC0-1.0, with its original notice in mods/dualsense/kenney/License.txt and in
the native title. Import provenance and original hashes: PATCH_IMPORTS.json.
No inherited game configuration, hooks.cpp, generated code or owner art is
included in this patch source.

## Presentation and controller prompts

The new candidate selects 1920x1080 display output, 16:9 presentation and a
60 FPS cap. Internal guest render resolution is still separate; neither
native 1080p/4K rendering nor sustained 60 FPS is established by those settings.
The actual fresh sub_001C32C0 aspect probe returns 1.777777791 in wide mode,
without an extra camera FOV multiplier. The original guest timer remains intact.

The UI callback selects the known USA direct/local/orthographic shader paths
and screen-space draws. It corrects the authored 640x448 picture by 45/56
horizontally. Movies and full-canvas fades/dimmers retain their own extents.
Unknown world shaders pass through. The original 640x480 front buffer is
presented using its visible y=16..464 region.

The fresh widget bridge at 0x1E080 applies edge offsets to the observed
single-player left groups 0MOD/0MOT/0SKL/0WAF and right groups 0ACT/0CXM.
The original native CB fields are preserved. XML-only portrait corrections
were rejected by actual images. A thread-local drawing tag was also rejected:
the game queues rendering on another thread. The exact native head-camera
rectangle setter call is now corrected instead (HUD_PORTRAIT.md). Centered
dialogs/build/buy contents and unknown screens remain unchanged. The original
translated bridge executes with a temporary XML argument; the owner's files
are never changed. No captured game XML is stored in source control.

A real Vulkan image rejected the first root-offset implementation: moving
geometry beyond the old clip volume cropped the left circle. The runtime now
offers an optional affine transform before clip rejection. Full-width scissors
stay full-width; smaller widget masks follow the transform. This is opt-in and
identity by default. D3D12 constants remain zero/default. The second actual
Vulkan run shows the complete circle, but the portrait still needed the new
native callback work. Two-player layouts,
every animated HUD state and mode-select logo anchoring remain unverified.

Seventeen named Xbox prompt textures receive filled-white PlayStation symbols,
including face buttons, shoulders/triggers, Options/touchpad, sticks and D-pad.
Only the existing BC2 mip payloads (32/16/8) change: 22,848 bytes total. This is
not a replacement of every textual reference to Xbox controls or every glyph.

## Reproduce checks and prepare an owner copy

```powershell
python -m unittest discover -s tests/ps5 -p 'test_prompt*.py' -v
python tools/test_ui_aspect.py --clang <installed-clang++>
python tools/ps5/verify_guest_aspect.py --help
python tools/ps5/stage_private_title.py --native <validated-native-directory> --assets private/owner-assets --output <new-private-directory> --dualsense
```

The staging tool verifies all 55 supplied owner files and the native manifest,
patches a new textures.arc and preserves the raw source. Raw textures SHA-256:
c4b8efc39156b9091ed7050810cedc4bb5c2080775bb208e8ab77447a802304e.
Patched SHA-256:
f85fd71249aca0750c104fec286f4b2adfd67b56317cd99904f19017d1005bf4.
The complete staged folder contains copyrighted owner data and stays private.

Use a fresh, separate title folder for this candidate. The game caches ARCs in
its emulated Z drive: a prior textures.arc cache can hide the new prompts.
PC --hdd-from tests support --fresh-cache, which copies the synthetic E save
while omitting Z from the new copy; no previous HDD or personal save is deleted.
The staged PS5 title contains no HDD/saves/cache at all.

## Evidence boundaries

Numeric policy/layout tests and 9 prompt checks pass. The fresh generated aspect
function passes its ABI/register/stack probe. PC Vulkan run
20261030T165232Z-f25a01 completed 700 seconds, 3,240 reported offscreen frames,
with zero fatal records and a real lot image showing the corrected circle,
and PS symbols. Its raw 640x480 images are not display screenshots or
PS5 FPS measurements. The newer PC capture path reads the final 1280x720 Vulkan
presentation image, after the real crop/aspect blit, without image resizing.
The owner's matching e16fc615 PS5 log and screenshots establish real 16:9
gameplay and remaining UI defects. The final observed five-second frame window
averages 44.60 FPS. New uncommitted UI fixes are not console validated.

All contributions remain local under the owner's no-push/no-PR instruction.
