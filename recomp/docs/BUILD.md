# Reproduce the independent build

Run from the project root. Versions/source pins are in ENVIRONMENT.md and
repositories.lock.json. Keep the previous working port and personal saves
separate. Generated game artifacts and owner data below are private.

## Host tools and data

With recorded Clang/CMake/Ninja on PATH:

```powershell
cmake -S . -B out/host -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build out/host --parallel 2
python tools/analyze.py --iso "$env:USERPROFILE/Desktop/Sims 2, The (USA).iso"
python tools/recompile.py
python tools/extract_assets.py --iso "$env:USERPROFILE/Desktop/Sims 2, The (USA).iso"
python tools/check_assets.py private/owner-assets --manifest private/owner-assets/owner-asset-manifest.json
python tools/test_assets.py
python tools/test_platform.py --clang <installed-clang++>
python tools/test_reference_palette.py --clang <installed-clang++>
```

Local runs supplied absolute installed compiler/build-tool paths, retained by
CMakeCache.txt. Do not substitute a new automatic scan for the reviewed config.
Recompile preserves timestamps only for byte-identical fresh generator output;
generated source is never manually edited.

## CPU/HLE tests

The Linux build uses the SDK's actual harness script:

```sh
CXX=clang++ JOBS=2 bash tools/build_runtime_headless.sh
python3 tools/run_headless.py --exe out/headless-runtime/Sims2Independent --assets private/owner-assets --seconds 30
python3 tools/run_headless.py --exe out/headless-runtime/Sims2Independent --assets private/owner-assets --seconds 180 --script tests/intro_to_menu.pad --snapshot-after 110
```

The configured local VM compiler relay supplies CXX, keeping preprocessing and
Linux linkage there while doing code generation on Windows. Separate object
caches prevent mixing runtime variants. Each run gets isolated logs, input,
asset check, summary and HDD. A timeout is a bounded observation, not a clean
shutdown. CPU reference rendering is slow under QEMU software emulation.

The Windows diagnostic target runs directly on the host:

```powershell
cmake -S host/windows-headless -B out/windows-headless -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE= -DSIMS2_WINDOWS_BACKEND=NULL
cmake --build out/windows-headless --target independent-host --parallel 2
cmake --build out/windows-headless --parallel 2
python tools/run_headless.py --exe out/windows-headless/Sims2Independent.exe --assets private/owner-assets --fps 30 --seconds 600 --press-at 40:A:8 --press-at 50:START:8 --press-at 60:A:8 --press-at 90:LSR:8 --snapshot-after 100
```

It uses the same SDK/HLE and harness with real Windows OS services. Only the
harness environment variables and error-message routing are adapted. Linux,
Windows, Vulkan and PS5 validation are recorded separately.

The timed presses are a recorded menu probe, not a guaranteed script for every
host speed. Inspect captures before proceeding. The title uses the left stick
for the main mode carousel (LSR/LSL); a digital RIGHT press alone did not move
that menu. `headless_command.py --buttons A --frames 8` sends ordinary input to
the current bounded run; `--run <id>` chooses a particular active local run.
`--snapshot` requests a reference image (null) or GPU readback (Vulkan). Wait
until the run's snap file disappears before reading the new image. The null
backend writes snap_1.tga/snap_2.tga; Vulkan writes cap/vulkan/*.bmp.

For the actual offscreen Vulkan comparison on Windows, configure with
`-DSIMS2_WINDOWS_BACKEND=VULKAN`, `-DCMAKE_C_COMPILER=clang`,
`-DGLSLANG_SOURCE=<pinned-glslang-checkout>` and, if needed,
`-DPython3_EXECUTABLE=<python>`. Use the recorded foundation's public Vulkan
headers and volk. Full glslang HLSL legalization is enabled; do not disable it.
The resulting executable is Sims2IndependentVulkan.exe. Run it with
`--backend vulkan --snapshot-after 20` in addition to the common test options.
On this PC, VK_DRIVER_FILES points to a private absolute-path ICD JSON for the
already installed Edge SwiftShader DLL. See VISUAL_DIAGNOSTICS.md. No Vulkan
driver or browser setting is installed or modified. This is a CPU renderer
correctness test and cannot supply representative PS5 performance measurements.

The runner stages and hashes a private executable copy per run. This preserves
the tested build and permits rebuilding the source target while its old copy
is still running. Earlier direct runs lock the original Windows executable;
finish those before relinking. PC tests accept a watchdog limit of 1..1800
seconds; a timeout is not a clean title shutdown. `--hdd-from <completed local
run ID>` copies synthetic caches into the next isolated run, preserving the
source evidence and excluding personal saves.

## PS5 native objects and title

The local object build uses the existing completed dependency cache:

```powershell
$deps = 'C:/path/to/existing/public-toolchain-cache/deps'
$oldWork = Split-Path -Parent $deps
./tools/ps5/build_objects.ps1 -Mode Profile -GuestProfile -DeepProfile -ObjectWaitChannels -Jobs 2 -Dependencies $deps -FoundationDirectory ../PS5_Sims2Independent -VulkanDirectory "$oldWork/PS5_Vulkan" -GlslangSource "$oldWork/glslang" -GlslangBuild "$oldWork/sims2-recomp/out/build/ps5-glslang"
python tools/ps5/export_link_inputs.py out/build/ps5-game-profile out/private-link-inputs.tar.gz
```

Transfer the private bundle to the configured local Linux machine and use
the project wrapper with foundation/driver/SDK paths. Actual local command:

```sh
python3 tools/ps5/link_and_package.py --inputs /home/debian/independent-f81-link-inputs.tar.gz --foundation /home/debian/sims2-work/PS5_Sims2Independent --vulkan /home/debian/sims2-work/PS5_Vulkan --payload-sdk-fork /home/debian/sims2-work/PS5_PayloadSDK-Sims2
```

The wrapper checks every object/archive hash, invokes the unchanged foundation
link-title.sh/RADV recipe, validates the real SELF/ELF, and retains symbols and
native-only zip under out/native/<build-id>. It includes credits and explicit
test-candidate status. The retained c71 baseline is 4:3/30; the new UI candidate
selects 16:9/60 and 1080p display. This does not verify native 4K or performance.

--retry resumes a recorded failed link with identical hashed inputs;
--validate-only packages an already linked matching ELF. Neither option deploys
or runs a title. Use the retained matching debug ELF for symbolization.

After downloading the native zip, debug ELF, build-manifest.json, status.json
and artifact-inspect.log into a new output directory, verify every packaged
member and the matching ELF before extraction:

```powershell
python tools/ps5/validate_delivery.py <download-directory> --extract
```

The validator refuses mismatched hashes/inventories, traversal, duplicate
members, owner-data paths and an existing extraction directory. It writes
local-validation.json. This validates delivery, not PS5 runtime execution.

To create a separate private owner test copy after validation/extraction:

```powershell
python tools/ps5/stage_private_title.py --native <download-directory> --assets private/owner-assets --output <new-private-output-directory> --dualsense
```

This copies the native manifest members and all inventoried owner-disc files
into PPSA99145/assets, then verifies both copied inventories. It refuses an
existing destination and includes no saves. The resulting folder contains
copyrighted owner game content: keep it outside Git and never publish it.
No deployment or console execution occurs.

Add --playstation-text to patch the 22 English checking/loading/saving console
notices and 17 English storage-device notices in the new private QuickData copy.
Source files remain untouched. This step currently applies to extracted assets;
direct ISO streaming retains original notice text.
Other Xbox references/languages are not claimed replaced. The current camera/
matte candidate uses -Mode Profile -DeepProfile -ObjectWaitChannels (guest-call
sampling OFF). E16 used -GuestProfile as well; both retain frame/GPU telemetry.
This removes sampling overhead without changing guest code/timers. Any PS5
speedup needs its own matching run; async completion remains OFF.

The --dualsense option patches only the new private texture archive, preserves
the raw source and records its separate hash. UI tests and Z-cache precautions
are documented in UI_PATCHES.md. For PC saved-game probes use --fresh-cache
alongside --hdd-from when changing owner textures. Never mix the old native
executable/settings with the new private title copy.

Upstream disposition: all changes stay local under the user's explicit request
not to push, open external pull requests, or publish releases. Dependency
licenses and credits remain applicable.
