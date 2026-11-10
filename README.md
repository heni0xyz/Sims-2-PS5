<img width="96" height="96" alt="The Sims 2" src="recomp/artwork/source/20261110/sims2.png" />

# The Sims 2 PS5

A native homebrew port of **The Sims 2 (USA, Original Xbox)** using
[XboxXBERecomp](https://github.com/TGP482/XboxXBERecomp).

This is a fan-made project, unaffiliated with EA or Maxis. You supply your own
supported retail ISO; original game data and translated game code are not
included in this repository or the public launcher.

Discord: https://discord.gg/gcATz3kvzt

## PS5-only installation

The new **PPSA99145** launcher prepares your game on the console:

1. Copy the complete PS5-only launcher folder to your prepared homebrew setup.
2. Put your USA Xbox ISO in the same folder as `eboot.bin`.
3. Open **The Sims 2**, select **Install** and press Cross. Extraction,
   generation, compilation and native linking happen on PS5.
4. After successful preparation, choose whether to keep the source ISO.
5. Select **Play**. Later launches reuse your verified private game.

The existing published v1.0.0 download is the **legacy Windows-builder release**.
These instructions require a matching new PS5-only launcher package; updating
source alone does not change an older release download. See [INSTALL.md](INSTALL.md).

Supported `default.xbe` SHA-1: `16f9e41ce26c315a758f25d43ba1fa9d3809d1ad`.
Other game revisions are refused.

## Launcher and game features

- Shared Play, Install and Update sections, with DualSense prompts and feedback.
- L1/R1, D-pad or left stick navigation; right stick browses gameplay previews.
- Installation slideshow, gated startup check and a smooth fade into the game.
- Quieter looping music; Touchpad mute is remembered across launches.
- Separate public launcher and private game updates, preserving saves/settings.
- Corrected 16:9 projection/UI, anchored HUD, native 4K raster policy and 60 FPS cap.
- Kenney filled-white DualSense prompts, runtime font filtering and owner-copy
  PlayStation/storage-device text patches.

Candidate **8db5af5a** passed native build/package checks and real PS5 cached-game
boot, Vulkan presentation and bounded normal shutdown. It also repairs the
executable permissions of older prepared games without recompiling them.
The preceding full installation/compilation was completed by the owner.
Stable gameplay 60 FPS, full-game coverage and the complete automatic update
helper/recovery sequence are not yet verified. See [KNOWN_ISSUES.md](KNOWN_ISSUES.md)
and the [test evidence](recomp/docs/LAUNCHER_PLAY_FIX_2026-11-10.md).

## Source

Authored code is GPL-3.0-only; dependency notices remain applicable. Local SDK
integrations are included as full source, with no unpublished submodule fetch
required. [BUILD.md](BUILD.md) describes maintainer builds; [SOURCES.md](SOURCES.md)
records dependencies. Reproduce the source ZIP with:

```text
python tools/package_source.py --output ../source-package --version 1.1.0
```

The archive contains source and the supplied presentation resources, excluding
ISO/XBE, extracted game assets, generated guest code, compiled titles, private
logs and saves.
