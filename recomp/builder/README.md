# The Sims 2 Builder

Public toolkit: Python GUI/CLI plus prepared public runtime, RADV, SDK headers,
Clang/LLD and XboxXBERecomp tools. The owner selects an original Xbox USA ISO;
the builder generates and compiles their private game functions, links a native
PS5 executable, validates it, and extracts/patches their game data. RADV and the
public runtime are **not rebuilt** on the player's machine.

No Python installation, VM, Docker, WSL, console compiler, or developer tools
will be required in the packaged GUI distribution. The GUI uses Python/Tk;
the same core supports a command-line interface. Host bundles contain native
tools for their specific OS/CPU; a Windows kit cannot run on macOS.

Target platforms: Windows 8.1+ x64 and macOS Catalina 10.15+ Intel. Apple Silicon
requires a separate bundle and macOS 11+. These are compatibility targets,
not claims of tests on hardware we do not have. All bundled native tools must
also pass that OS baseline before it is advertised as supported. Python 3.11
is the packaging baseline. Minimum RAM and disk requirements must be measured;
the initial kit reserves 4 GiB free for build/extraction and limits compilation
to two tasks at once. Python 3.12 also supports Windows 8.1 per its official
platform documentation; the first local GUI package uses Python 3.12.14.

Packaged workflow:

1. Open The Sims 2 Builder.
2. Select your USA Xbox ISO and a **new** output folder.
3. Click Build. The source ISO is read only and is kept.
4. Close the title on PS5, then copy the completed PPSA99145 folder.
5. When updating an existing installation, preserve its `hdd` folder/saves.

The result is the full private game executable, replacing the public instruction
title. This avoids a new runtime module loader. Do not overwrite a running eboot.
The public instruction title does not execute the game by itself.

Source invocation (maintainers/testers):

```text
python -m builder --kit PATH/kit --iso PATH/owner.iso --output PATH/NEW/PPSA99145
python -m builder.gui
```

Logs, generated sources, compiled objects and matching `llvm-pie.elf` remain in
the adjacent `.sims2-build-<id>` folder for diagnosis; they are private owner
artifacts. `OWNER-BUILD.json` in the finished title records hashes, build ID and
build-only status. A successful build does not claim a successful console run.

The kit is checked against a positive file/hash inventory before executing tools.
Only download a kit from a trusted release; this is an integrity check, not a
signature or a sandbox for untrusted manifests. The Windows kit uses the
template/RADV linker flags and input ordering captured by
`tools/builder/export_native_recipe.py`; it does not invent another RADV recipe.

Public kits must exclude the generated guest archive, translated game source,
private eboot, ISO, XBE, assets, saves and owner logs. Include complete
corresponding public source and all third-party licenses. Local generation does
not license the original game's content. The authored builder is GPL-3.0-only;
dependency licenses remain separate.

Single Windows exe embeds its audited kit; no separate kit download is required.
The public packager requires prepare_public_notices.py before bundling.
