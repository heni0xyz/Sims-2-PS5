# Local delivery — font and Continue candidate

Native build **7b73fdc02feb0835d8a55a7d**, title PPSA99145 / The Sims 2.
Prepared PC-only; no console contact or publication. Earlier title folders
and personal saves are preserved.

## Artifacts in the shared workspace

```text
outputs/PS5-Independent/Native4K-Font-Continue-7b73fdc0/
  PPSA99145/eboot.bin
  llvm-pie.elf
  Sims2Independent-PPSA99145.zip
  build-manifest.json
  local-validation.json
outputs/PS5-Independent/FullTest-Native4K-Font-Continue-7b73fdc0/PPSA99145/
```

The latter is the complete **private owner** test title: 45 verified native/
public files and 55 verified owner files, 1,986,340,175 owner bytes, no saves.
Do not publish it. Use that folder as a coherent test copy; it contains the
new extracted English PlayStation/storage notices as well as DualSense icons.
Keep existing saves separately; this preparation does not deploy/erase them.

- eboot SHA-256: `4d5c1a803e2e2d1d37807bbb4a448185b285147534e1f4c36333107412e83e99`
- debug ELF SHA-256: `fb3ff3f89782890f6749fe27a3fb1cc1fa59278ec0a561f4c050c5f30d8193bc`
- native ZIP SHA-256: `bba75e95acd5da4bff7088f9f109e1cec037eb757b46812b8350a4731dba9ba9`
- debug ELF size: 167,859,296 bytes.

Native object build:235.58s, O2/g1,115 audited compile commands. The established
foundation/RADV linker and native validation/package took123.13s. The copied
45-member package and matching ELF pass local hash/inventory checks. Build
production checkpointfb5a3f01944c; subsequent source-export fixes do not change
compiled code/build identity. Runtime SDK pin is
e91175246fe9ded3ab8c820e1d3f2b31f6aea5b7.

Settings: 3840x2160, native_render_resolution=true, render_percent100,
16:9/edge anchors/native portrait, cap60, HD prompts, font4x and Continue refresh
enabled. Async completion remains **false**. Profiling is enabled. This is a
correctness/diagnostic candidate; performance is not represented as release-ready.

## Acceptance

| Category | Result | Evidence/remaining check |
|---|---|---|
| Native build/package | PASS | ELF/SELF import validation,45-member local delivery inventory |
| New PS5 runtime | NOT TESTED | Owner test deferred; no console contacted |
| Continue refresh | PC PASS / PS5 NOT TESTED | Correct provider rereads on entry/reentry; repeated hardware entries needed |
| Font reconstruction | PC PASS / PS5 NOT TESTED | Three native4K image pairs; subtle smoothing, unchanged text placement |
| Performance | Target FAIL on previous7bd |44.83FPS aggregate slow windows; new candidate not measured |

First hardware review: compare original/upscaled font at4K, reenter both
Story/Freeplay Continue repeatedly, then save/load and play a representative
lot. Capture klog with this build ID. A bounded run/valid package does not
establish a full game playthrough or sustained60.

The source-only package excludes owner/generated/native data. Its restore
coverage and dependency limits are in SOURCE-RELEASE.md and
FONT_CONTINUE_2026-11-01.md. The local Linux build machine was shut down after
artifact retrieval (`reboot: Power down` verified in its serial log).
