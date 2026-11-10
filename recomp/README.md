# The Sims 2 independent recompilation

The current development workflow prepares the owner's game **on PS5**: the
PPSA99145 launcher extracts the supported USA Xbox ISO, generates and compiles
its private game code, links a native title and enables Play. A PC compiler is
not required by this new player workflow. The older published v1.0.0 Windows
builder remains a legacy release until a matching PS5-only package is published.

The official source repository is https://github.com/heni0xyz/Sims-2-PS5.
Its `recomp/` directory contains this project; `sdk/` and `runtime-sdk/` are
complete vendored source trees, including local changes. See the repository's
BUILD.md and source-origin.json for layout and dependency revisions.

Display title: **The Sims 2**. Authored code is GPL-3.0-only; dependency licenses
remain separate. szampan's title artwork and Kenney prompts are CC0-1.0. Open
Sans is supplied with its OFL notice, and the original music MP3 retains metadata.

Supported default.xbe SHA-1: `16f9e41ce26c315a758f25d43ba1fa9d3809d1ad`.
The owner supplies the ISO. Original game files, generated game C++, guest
objects, private native executables, saves and logs are excluded from source.
The review configuration and authored hooks preserve the private generation
workflow; generated guest source must never be edited directly.

The launcher shares Play, Install and Update, supports DualSense navigation,
retains a music preference and uses separate public-launcher/private-game update
paths. Saves and settings are preserved by the updater. See
[the verified Play fix](docs/LAUNCHER_PLAY_FIX_2026-11-10.md) and
[current progress](docs/PROGRESS.md).

Candidate `8db5af5a5dd85551` passed native build checks and real PS5 cached-game
boot/rendering/bounded shutdown tests. The owner completed the preceding full
ISO installation and compilation. Fresh full-install repetition, complete
update-helper recovery, full-game coverage and stable gameplay 60 FPS remain
separate acceptance items. A 60 FPS cap or 4K display output alone proves neither.

The game-specific configuration, hooks and translated C++ from
abilliontokens/sims2-recomp are not inputs to this independent pipeline.
TGP482's XboxXBERecomp toolchain and the credited MIT runtime fork remain explicit
dependencies; prior platform imports are listed in docs/PLATFORM_IMPORTS.json.
This is not a strict clean-room claim.

Maintainer instructions: [docs/BUILD.md](docs/BUILD.md),
[SOURCE-RELEASE.md](SOURCE-RELEASE.md), [builder/README.md](builder/README.md)
(legacy PC builder), and the console compiler/installer scripts under tools/ps5.
Source publication does not include the player's game or its generated code.
