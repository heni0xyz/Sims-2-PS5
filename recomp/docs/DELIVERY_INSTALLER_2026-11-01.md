# Private installer / low-overhead 4K candidate — 1 November 2026

Build ID **9bb7544bbe7b0d09d2f17db7**, PPSA99145, The Sims 2.
Production source checkpoint839e423cb3cd (later documentation/tests do not
change native inputs), runtime SDK c4fbcb6ca991730971651c636789c01df9837b45.
No console contacted or public release made. Previous deliveries preserved.

## Evidence and exact artifacts

Native object build PASS in193.99 seconds:121 audited O2/g1 compile commands.
Established PS5 template/RADV link/package PASS in106.44 seconds. Windows
Vulkan game/probe links PASS. Asset-free five-screen Open Sans/Vulkan probe
and backend recreation PASS; actual private owner ISO extraction/readbacks,
notice patch/cache preservation and synthetic source-deletion guards PASS.
All19 Python tool tests and six C++ UI/render/texture/font/menu-policy suites
PASS. The menu diagnostic ABI test preserves original context/state/exception
results; it is not proof that thin Continue is fixed.

New native artifacts are under:
`outputs/PS5-Independent/Installer-OpenSans-LowOverhead4K-9bb7544b/`.
The delivery validator verified50 native/public files and the matching ELF:

- eboot SHA-256:53cd66a53312d0e554c0b04fe777a896ae5edddbc06f84dd1175ce4721bc1ba9
- ELF SHA-256:efea25efa863d3229044cc99115f96f62da224ffe2722ec000b4fb1eeaec78f4
- ZIP SHA-256:abb2d8496b820f96f95e06f2122debf9079e78208c6388a26aa962883be59b2f

The bare PPSA99145 folder includes installer/font/public notices, with no owner
data. Put the owner ISO beside eboot.bin to exercise extraction. A separate
complete private owner folder is staged under
`outputs/PS5-Independent/FullTest-Installer-OpenSans-LowOverhead4K-9bb7544b/PPSA99145`.
It contains extracted data and bypasses first-run extraction. No saves/ISO are
included. Preserve the console's existing hdd when updating. Never publish the
private owner folder or generated game code.

## Configuration and limits

Output3840x2160, native detail enabled (expected backing3840x2880/picture
3840x2688), aspect16:9, cap60, HDprompts/font4x/anchors/portrait enabled.
Handle caching and selective object wakeups remain ON. Performance reports
remain ON; expensive guest-call sampling and deep per-kernel/lock timing are
OFF. This tests profiler overhead; no speedup is claimed before native data.
GPU WRITE completion experiment stays OFF. The game still preserves real GPU
completion/failure behavior. The installer is new, uses Open Sans/OFL and
applies the existing exact notice patch to an extracted private copy.

Thin Continue is still OPEN after the owner's failed7b73 test. New bounded
state-2/controller/variable diagnostics add evidence without changing the
original controller/provider bodies. No new visual fix is claimed.

## Commands executed

```powershell
./tools/ps5/build_objects.ps1 -Mode Profile -ObjectWaitChannels -Jobs 1 -Dependencies <recorded-deps> -FoundationDirectory ../PS5_Sims2Independent -VulkanDirectory <pinned-PS5_Vulkan> -GlslangSource <pinned-glslang> -GlslangBuild <existing-ps5-glslang>
python tools/ps5/export_link_inputs.py out/build/ps5-game-profile private/installer-opensans-9bb7544b-link-inputs.tar.gz
```

On the existing local Linux VM, from the new committed source checkout:

```text
python3 tools/ps5/link_and_package.py --inputs /path/to/build-host/installer-opensans-9bb7544b-link-inputs.tar.gz --foundation /path/to/build-host/sims2-work/PS5_Sims2Independent --vulkan /path/to/build-host/sims2-work/PS5_Vulkan --payload-sdk-fork /path/to/build-host/sims2-work/PS5_PayloadSDK-Sims2 --resolution 4k
```

After copying results locally:

```text
python tools/ps5/validate_delivery.py <new-native-directory> --extract
python tools/ps5/stage_private_title.py --native <new-native-directory> --assets private/owner-assets --output <fresh-private-directory> --dualsense --playstation-text
python -m unittest discover -s tests/ps5
python tools/test_ui_aspect.py --clang <recorded-clang++>
```

## Owner acceptance still required

BUILD:PASS. NEW PS5 RUNTIME:NOT TESTED. NEW GAME BRING-UP:NOT TESTED.
PERFORMANCE:NOT TESTED; previous7b73 heavy mean42.69FPS fails sustained60.
No host image or successful eboot link establishes any of those runtime gates.

Use the same lot/save and normal speed for one minute far camera, then one
minute close camera, retaining klog from launch. Separate thin-Continue
reproduction: enter/back/reenter Story and Freeplay Continue and retain the
log when the panel collapses. INSTALL_WAIT/BEGIN/COMPLETE records apply only
to the new installation path, not game-playability milestones.
