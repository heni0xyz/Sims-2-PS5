# Native 4K corner-logo correction

Build a80d231d1fa2e8cc35a1523c, PPSA99145. Authored by OpenAI Codex for
szampan, with the updated packaged CREDITS.txt. Source checkpoint
4f7fe88783eb2023cfffc706e7665d1ce264cabe; runtime SDK
85bea0eb9666a9dbfe4c3ea54377509038d29e1b; tools SDK unchanged at
faeb0eaca270e01a9e83bb2bc587c824c458ac32. Four verified source bundles
are retained under out/source-bundles/4f7fe88783eb (no owner data).

The mode-menu logo had retained the centered UI gutter in 16:9. The new
policy identifies the exact original logo texture and requests the evaluated
guest bounds for that draw. Only its small upper-left use is left anchored;
the centered mode selector retains its mapping. The same logo's large startup
use is excluded. No artwork, generated source or original executable changes.
See MENU_LOGO.md for fingerprint provenance, guard conditions and diagnostics.

## Evidence and limits

| Category | Result | Evidence/limit |
| --- | --- | --- |
| A. Native build/package | PASS | O2, 110 audited commands; established template/RADV link and SELF inspection; local package/ELF validation recorded alongside artifacts |
| B. New PS5 runtime | NOT TESTED | no console contacted or deployment performed |
| C. Logo/menu | PASS on PC; NOT TESTED on PS5 | actual final 1280x720 Vulkan readbacks show left logo and centered selector with PS confirm prompt |
| D. 4K/FPS | configured; new PS5 performance NOT TESTED | previous uniform native-detail policy retained; output 3840x2160, backing expected 3840x2880; 60 FPS cap is not a measured rate |

Corrected PC run 20261031T153852Z-ac1922, EXE SHA-256
ff1256314391f81f979fd4873cb78192b49d6ae1d5e95cde2b2bb873fde26fb4.
At 94.228 seconds the renderer selected the actual logo bounds
32.031,38.431..135.431,141.831 with x_scale=0.803571463 and x_offset=0.
Readbacks 2023/2057 were reviewed. Final PNG is a lossless format conversion,
not resized/repainted. The center gutter removed is 125.714 pixels at 1280
or 377.143 at 3840 output width; width, height and Y placement are preserved.
The observation ended by an explicit stop at 284.906 seconds, no fatal
records; clean_process_exit=false. No 4K gameplay or clean-exit claim.
Asset-free UI/portrait/matte/render-density tests plus new logo selection,
startup exclusion and margin-mapping regressions PASS.

## Artifacts and reproducibility

Paths are relative to the shared workspace above work/sims2-independent:

- Native-only title, zip, ELF, manifests and logs:
  outputs/PS5-Independent/Native4K-Logo-a80d231d.
- Complete private owner title:
  outputs/PS5-Independent/FullTest-Native4K-Logo-a80d231d/PPSA99145.
- Actual corrected menu preview: Native4K-Logo-a80d231d/PC-logo-left.png.
- UI evidence: out/evidence/menu-logo-anchor.json and MENU_LOGO.md.

eboot SHA-256:
b568b7a81ff2634c6b89ad5b2907c1178ec98ff9819248ee0ee3abb684b46ea5.
Debug ELF SHA-256:
23f5bb0d77eb24867797571c8ebfdb19fdbb2c5e1538b542a26e9d59a359a8bc.
Zip digest and all 36 native file digests are recorded by local-validation.json.
Local validation PASS: zip SHA-256
28af876f967e3d70e834097c12464ad3bce8c037d08cae974ae91924b34e1045.
The complete owner-copy staging also PASS: 36 native + 55 verified owner
files, 1,986,340,175 owner bytes, no HDD or saves. The debug ELF retains the
new lazy probe, native-resolution initializer and original portrait hook;
symbol evidence is in out/evidence/logo-native-symbols.txt. Packaged credits
match the source file. The local VM was shut down cleanly; serial output
confirms filesystem synchronization and power down.

Executed: documented `tools/ps5/build_objects.ps1 -Mode Profile -DeepProfile
-ObjectWaitChannels -Jobs 1` with the paths in ENVIRONMENT.md (127.59 s);
`tools/ps5/export_link_inputs.py out/build/ps5-game-profile
private/logo-a80d-link-inputs.tar.gz` (49 hashed inputs, 25,904,120 bytes).
The local Debian VM ran `tools/ps5/link_and_package.py` with that archive,
the existing foundation/Vulkan/SDK paths and `--resolution 4k` (100.58 s).
The established RADV link recipe, cached driver, SDK and foundation revisions
are unchanged. Validate locally with `tools/ps5/validate_delivery.py --extract`;
stage the owner copy with `tools/ps5/stage_private_title.py --dualsense
--playstation-text`, supplying the native/assets/output paths.

Settings remain 3840x2160, render_percent=100, native_render_resolution=true,
16:9, UI/edge/portrait switches enabled, 60 FPS cap, async callbacks disabled.
Game assets are identical to the prior 4K owner copy: no data patch is needed
for logo positioning. When upgrading that exact 4K copy, replacing eboot.bin
is sufficient for behaviour; updated CREDITS.txt accompanies the new package.
Keep existing HDD/saves. Earlier e9dd/d8e6 artifacts remain intact. No push,
publication, console security/firmware change or remote compilation occurred.
