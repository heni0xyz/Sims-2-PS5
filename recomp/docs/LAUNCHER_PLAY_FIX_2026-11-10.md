# PPSA99145: executable permissions, startup gate and presentation

Candidate **8db5af5a5dd85551**, version 1.1.0, official title PPSA99145.
The existing content/concept identity is preserved. No GitHub push, PR or
release upload was performed.

## Observed failure and correction

The owner's previous launcher completed ISO extraction, generation, compilation
and private packaging. Play submitted the black frame and handed off, but the
system rejected the private game before its entry point: errno 13/EACCES. FTP
showed the existing private game as 0644, while the root launcher was executable.

`compiler/executable_file.h` now verifies a non-symlink regular file through an
open descriptor and sets/verifies mode 0777 on POSIX/PS5. Windows checks regular
files only; its host tests do not prove POSIX execute permissions. Packaging,
verified-cache inspection and Play preflight call this helper. The updater also
marks its trusted helper, executable backups and staged replacement eboot as
executable **before** promotion. Permission failure is reported, never hidden.

Cache inspection repairs the already verified private game without recompiling
its generated units or changing its bytes. The first console run logged
`mode_before=0644 mode_after=0777 repaired=1`; subsequent launches logged 0777
with no repair. The game SHA-256 remained
`48d341ca2e3ce764e106cfee05e7232a274aa43449d9646b7671a589f9c23172`.

The initial installation check starts alongside platform/renderer preparation.
Until its result is known, a centered Checking your game view hides sections
and ignores input. A verified installed game selects Play directly. The real
console logged `initial_check_complete phase=8 section=0`.

## Requested presentation changes

- New szampan CC0 icon is global: launcher atlas and Home icon. Original PNGs
  and provenance remain under `artwork/source/20261110/`.
- New 3840x2160 Home-only background is converted to native BC7/DX10 DDS.
  The old in-launcher gradient is retained, with identical resource SHA-256.
- Launcher music gain is 10% of original PCM, half its former 20%. Home music
  is unchanged. Touchpad preference and the short click-preventing ramp remain.
- During installation, screenshots advance every seven seconds. Manual right
  stick input delays the next automatic advance for ten seconds.
- Installation locks section navigation with the existing rejection feedback.
  Update becomes available again after installation stops/completes.
- The selected bottom label subtly pulses between near-white values 239..249.

## Evidence and scope

Native build/link/import/SELF validation PASS. All 3,258 ready-file SHA-256 checks
PASS; the public package contains no owner ISO/XBE/private generated game.

Host validation `out/ps5-installer/host-validation/20261110T184806Z/` PASS:
13 production transaction tests, ten mocked HTTP cases, actual UI draw-list
renders/navigation/startup/locked-section/carousel checks, full-track audio
decode/loop/gain tests and extraction contracts. The full 513,919,916-byte public
update bundle (3,256 managed files) also passes staging/apply/hash checks and
preserves synthetic saves/settings/ISO/private code/cache. The host fixture was
corrected to use the candidate's title identity instead of fixed PPSA99148.

On the owner's explicitly authorized, idle PS5:

1. Raw signed old eboot, metadata and cache receipt were backed up privately.
2. Twenty-one changed public files were uploaded/read back with matching hashes;
   owner settings and existing game content were preserved.
3. Klog capture preceded launch. Play repaired 0644, reused the verified cache,
   faded to black and entered the independently generated game runtime.
   Guest memory and Vulkan initialized; game rendering/presentation continued.
4. The 60-second game hook requested normal shutdown. No fatal signal or
   executable-access denial occurred. The title exited without watchdog kill.
5. A second bounded launcher run captured the real Play screen; mode 0777
   persisted. The screenshot is 1920x1080; launcher diagnostics report its
   configured 3840x2160 dimensions. Do not infer gameplay resolution/FPS from it.
6. Eleven Home presentation files were uploaded/hash-verified and the title was
   re-registered through PS5 Upload. Home visual/audio acceptance remains the
   owner's observation, distinct from verified file/registration results.
7. The real console HTTPS checker reached enum Current (state 1), verifying the
   GitHub metadata path. No newer compatible public release was applied.

The template's normal `LoadExec("exit")` produces a system spawn diagnostic with
errno 22/EINVAL after the application's shutdown marker. It is preserved in the
raw log and distinguished from the original errno 13 game-spawn failure.

Private evidence: `out/ps5-launcher-99145/console-8db5/`; reproducible bounded
handoff tool: `tools/ps5/test_launcher_handoff_console.py`. One initial tool-only
attempt failed before mutation because the console FTP server rejects NLST;
the tool now uses the established LIST command. Its subsequent 30-second
repeated handoff/boot/shutdown run PASS with no access denial, fatal signal or
private game byte changes, including after Home re-registration.

This verifies cached ISO-prepared game boot, presentation and bounded shutdown.
It does not verify fresh extraction/compilation again, playable input, stable
60 FPS, haptic strength or the complete download/helper/update/recovery path on
hardware. The public runtime kit is still f419-derived; newer performance work
must be integrated and measured separately.

## Reproduce

Use the recorded local settings and pinned platform headers:

```powershell
python tools/ps5/build_installer.py --settings out/console-compiler/build-settings.json --platform-headers <SDK-platform-include> --title-id PPSA99145 --output out/ps5-launcher-99145/qol-fix --phase prepare
python tools/ps5/build_installer.py --settings out/console-compiler/build-settings.json --platform-headers <SDK-platform-include> --title-id PPSA99145 --output out/ps5-launcher-99145/qol-fix --phase package
python tools/ps5/test_launcher_host.py --settings out/console-compiler/build-settings.json --resource-state out/ps5-launcher-99145/qol-fix/resource-state.json
python tools/ps5/test_launcher_bundle.py --package-result out/ps5-launcher-99145/qol-fix/package-result.json --driver <host-update-test.exe>
python tools/ps5/test_launcher_handoff_console.py --console-tools <PS5_Vulkan-tools> --package-result out/ps5-launcher-99145/qol-fix/package-result.json --output out/ps5-debug/<new-run-id> --seconds 60
```

The last command requires the matching launcher already deployed and an
authorized idle target configured by the existing console tools. It refuses
other running applications, mismatched eboots/receipts and pre-existing hooks.
It never uploads game code or changes the user's source ISO.

Ready pointer: `out/ps5-launcher-99145/qol-ready-result.json`.
Debug ELF and immutable build manifest remain beside the native package.
