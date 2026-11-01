# The Sims 2: HD/ISO diagnostic candidate

Build **7bd91d7c2cd2127150a991e1**, title **PPSA99145**. This new artifact has
not been executed on PS5. No console was contacted in this work.

## Results

| Category | Result | Evidence / limit |
| --- | --- | --- |
| A: native build/package | PASS | ELF64 x86-64, inspected SELF, 45 hashed title members, matching debug ELF |
| B: new PS5 runtime | NOT TESTED | Owner test deferred; earlier a80 hardware logs are not acceptance of this binary |
| C: new game bring-up | PASS limited PC observation; PS5 NOT TESTED | Original ISO, fresh cache, restored synthetic save, rendered lot/HUD/tutorial and input; no full-game/stability claim |
| D: new performance | NOT TESTED on PS5 | Previous a80 final window 45.24 FPS fails stable60; software PC rendering is not a console FPS estimate |

Compiled on Windows in 271.20 seconds, audited 113 native/guest compile
commands: O2, g1, fast_handles ON, object_wait_channels ON, deep_profile ON,
sampled guest_profile ON, startup probe OFF. No generated guest edits.
Native link/package used the unchanged pinned foundation/RADV recipe in the
local Debian VM; 102.83 seconds. The VM was shut down after artifact retrieval.

## Artifacts

Paths relative to the shared workspace, outside this source checkout:

- outputs/PS5-Independent/Native4K-HD-ISO-7bd91d7c: native-only zip,
  extracted title, manifests, SELF inspection, link log, matching debug ELF.
- outputs/PS5-Independent/FullTest-Native4K-HD-ISO-7bd91d7c/PPSA99145:
  complete private owner test copy, 45 native/public files and 55 verified
  owner files (1,986,340,175 bytes of game data). No personal saves/HDD copied.
  Earlier owner test folders remain untouched.

| Artifact | SHA-256 |
| --- | --- |
| eboot.bin | acc23a6d77e9ef684418e0f2a973c0bcfdb3786143df971f3454f5928a5b1ea9 |
| llvm-pie.elf | 9a3a6e97994294571f5b232cf9956038936abbad64ce2aca08ca77d6fbb2204c |
| native-only zip | 547ab84c9371ea61026da8c045a839ab97d45884dc18ce17ace96ac8a61b7792 |

Local validation PASS: zip inventory exactly matches build-manifest.json;
all files and debug ELF hashes match; no ISO, XBE, extracted game data or
saves in the native-only zip. The eboot still contains translated game code;
this is not a public binary-distribution clearance. See DISTRIBUTION.md.

## Included behavior

Display name The Sims 2, szampan icon/background, source-only PXZ preserved,
GPLv3 project code and CC0 title art. Supplied PNG/PXZ bytes match provenance.
icon0.png=512x512 RGB, pic0/pic1.dds=1920x1080 RGBA.

Settings: 3840x2160 output, native_render_resolution=true (uniform 6x target
policy), render_percent=100, 16:9 correction, edge/native portrait policies,
fps_cap=60, HD prompts=true, ui_menu_diagnostics=true, async callbacks=false.
The 17-texture public HD pack hash matches its compiler manifest. It supports
both original Xbox and earlier Kenney-patched USA archive fingerprints.

The full private folder also retains the previous authored English
Xbox-console-to-PlayStation notice patch. ISO streaming uses the untouched
disc and currently retains original notice text, as documented.

## Owner test procedure

Transfer the complete new PPSA99145 folder, including public-resources/ and
sce_sys/. Preserve your existing hdd/ saves separately; the supplied folder
has no hdd/ and does not instruct deleting saves. It already contains the
complete private assets/ copy and needs no ISO for that test.

For a separate ISO-path test use the extracted native-only title and exactly
one owner USA ISO beside eboot.bin. Keep that trial separate from the known
complete extracted copy so the mounted source is unambiguous. This workflow
has PC evidence; PS5 large-file/mount testing is still pending.

Capture klog from launch, enter Continue, back out and enter again in the
same app. If the panel collapses, retain several seconds so numeric geometry/
provider records can be compared. Then collect about one minute of gameplay
with a farther camera. Test pause/fast-forward placement on the right edge.
The new bounded menu and sampled guest-call reports are intended to diagnose
remaining failures; neither stable60 nor the thin-panel repair is claimed.

## Reproduction

Use the exact dependency/foundation/glslang paths already recorded privately
in out/build/ps5-game-profile/configure.log and compile_commands.json:

```text
tools/ps5/build_objects.ps1 -Mode Profile -GuestProfile -DeepProfile -ObjectWaitChannels -Jobs 1 <recorded dependency path arguments>
python tools/ps5/export_link_inputs.py out/build/ps5-game-profile private/hd-iso-7bd91d7c-link-inputs.tar.gz
```

In the configured Linux VM, the executed command was:

```sh
python3 /path/to/build-host/sims2-work/sims2-independent-hd-iso/tools/ps5/link_and_package.py --inputs /path/to/build-host/hd-iso-7bd91d7c-link-inputs.tar.gz --foundation /path/to/build-host/sims2-work/PS5_Sims2Independent --vulkan /path/to/build-host/sims2-work/PS5_Vulkan --payload-sdk-fork /path/to/build-host/sims2-work/PS5_PayloadSDK-Sims2 --resolution 4k
```

Back on Windows, validate_delivery.py <native-directory> --extract and
stage_private_title.py --native <native-directory> --assets private/owner-assets
--output <fresh-owner-directory> --dualsense --playstation-text both passed.
The private link-input bundle contains 64 verified input/resource files.

Next required evidence: new matching PS5 klog/screens for menu geometry,
right HUD indicators and measured gameplay frame timing. Family-photo/detail
work and disabled-gray-icon alpha remain open. No new optimization speedup
is asserted by this diagnostic delivery.
