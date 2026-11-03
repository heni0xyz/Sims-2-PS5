<img width="96" height="96" alt="sims2" src="https://github.com/user-attachments/assets/5cfc80ca-34a7-480a-8b94-15688251a262" />



# Sims 2 PS5
 A recompilation project of The Sims 2 (Xbox) for the PS5

This project is a fan-made creation. It is not affiliated with EA, Maxis, or any of their affiliates or subsidiaries. This project does not provide game data; you need a legal retail copy of The Sims 2 (USA/NTSC) for the Original Xbox.

Discord server: https://discord.gg/gcATz3kvzt

Native homebrew port of the **USA Original Xbox** release, using
[XboxXBERecomp](https://github.com/TGP482/XboxXBERecomp).
Not affiliated with EA or Maxis. **Early alpha: full-game compatibility,
long-term stability and stable 60 FPS are not guaranteed.**

## Prepare your game

1. Download the **Windows Portable ZIP** from [Releases](https://github.com/heni0xyz/Sims-2-PS5/releases) and extract the whole folder.
2. Run the executable, and select your own USA Xbox ISO.
3. Choose a **new** output folder and click **Build**.
4. Close the title on PS5, then copy the complete generated **PPSA99145** folder.

The optional **The-Sims-2-PS5-Setup-WindowsLink.zip** contains a guide on your PS5.
Supported XBE SHA-1:
`16f9e41ce26c315a758f25d43ba1fa9d3809d1ad`.

**Updates:** preserve your existing `hdd/` folder and saves. Never replace a
running eboot. See [INSTALL.md](INSTALL.md) and [KNOWN_ISSUES.md](KNOWN_ISSUES.md).

## Features and status

- 16:9 world projection, corrected UI proportions and anchored HUD.
- Native 4K rendering policy, with a 60 FPS cap; sustained 4K60 is not claimed.
- White filled DualSense prompts (Kenney CC0), Open Sans preparation screen.
- Owner-copy data extraction with PlayStation/storage-device text patches.
- Ordered GPU callbacks, cached handles and selective synchronization wakeups.

Windows builder and native title packaging have host-side checks. The public
preparation screen was confirmed working on PS5 after including its required
`assets/README.txt` directory marker. The Windows-linked full game still needs
PS5 execution testing. Earlier owner tests verify basic gameplay through the previous route.
Continue may occasionally show a thin background; performance can vary.

Target: Windows 8.1+ x64. Tests so far use the owner's Windows PC; Windows 8.1
has not been tested. macOS Catalina+ Intel / macOS11+ Apple Silicon bundles are
planned, **not included or verified in this Windows release**.

## Source

All authored source is in this one repository: `recomp/` contains the game
configuration/runtime/builder and vendored Xbox SDK integrations;
`PS5_Sims2Independent/` the game-title foundation;
`PS5_Sims2Bootstrap/` the public instruction title. The exact locally modified
native SDK source is in `vendor/PS5_PayloadSDK/`. Other public dependency pins,
source locations and build instructions are in [SOURCES.md](SOURCES.md) and
[BUILD.md](BUILD.md). No unpublished Xbox SDK submodule commit needs fetching.

Authored code: **GPL-3.0-only**. Artwork by **szampan**: CC0. Open Sans: OFL-1.1.
Upstream code retains its notices; see [CREDITS.txt](CREDITS.txt),
[LICENSING.md](LICENSING.md) and the included dependency licenses.
The project license grants no rights to the original game content.
