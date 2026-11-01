# Owner-copy builder: release record

One repository: `heni0xyz/Sims-2-PS5`. Authored code GPL-3.0-only; user artwork
CC0, Kenney CC0, Open Sans OFL-1.1; retain dependency notices/source directions.
No remote writes or publication by the agent.

The Windows GUI/CLI uses Python/Tk, bundled Clang/LLD and the existing native
packaging implementation. The public kit contains public compiled runtime,
RADV, platform libraries, SDK headers, recompiler tools and public resources.
It excludes generated guest C++, the guest archive, full game eboot, XBE/ISO,
game data and saves. A maintainer-defined symbol audit rejects generated game
definitions before packaging the public runtime inputs.

The owner selects ISO and a NEW output directory. The supported XBE SHA-1 is
16f9e41ce26c315a758f25d43ba1fa9d3809d1ad. The builder checks that hash, regenerates
all 71 reviewed files, checks their hashes, compiles 65 units at the same O2
settings, creates a fresh guest archive, links the native title and checks imports
and ELF/SELF integrity. It then extracts/verifies/patches game data and promotes
the complete directory. The ISO is kept, and existing output/saves are refused.
Failed stages keep logs/objects/debug symbols; explicit retry reuses only
matching ISO/compiler/source fingerprints and checked object hashes.

RADV input ordering and flags come from the established template and RADV
recipe. The SDK's linker wrapper is evaluated too, preserving PIE, 16 KiB pages,
emulated TLS and GNU hash. Included linker scripts and the SDK's empty system
archives are part of its library search path. Do not suppress unresolved symbols
or substitute another hand-written RADV command.

Windows evidence: 65 fresh generated units compiled in 557.84 s with two tasks.
The initial link failed because wrapper/search-path export was incomplete. It
was corrected; a retry reused all 65 checksum-verified units and completed the
native title and private asset install in 52.64 s. Link: 3.734 s, peak working
set 696 MiB. Native conversion: 1.734 s; sign: 1.438 s; install: 37.063 s.
These times exclude a future end-user download and are not a minimum-RAM test.
Fresh compilation also overlapped maintainer tasks, so timings are indicative.

Private game result: eboot SHA-256
e208bdb05eac1c905362f46bf64e301d563d66e85c48eacb8cdf3dc877c4777a.
Matching private debug ELF:
75e0b46aade71ffdda8c7dc1cc8e317cfd0ad5f331c8927e7f19b42d456d2974.
This output is private and must not be put in the public release.

Public preparation title: 29 compiled application objects, native link/convert/
sign/inspect PASS, no translated game definitions. eboot SHA-256:
ed6279e16133bd9949aac013d7caa79988aea8b1939ba6f22457965ab6351afa.
Native Open Sans instruction screen at 3840x2160. PS5 execution of this new
instruction title and Windows-built game remains NOT TESTED. Earlier owner
gameplay reports apply to the previous build route; sustained 60 FPS remains open.

Compatibility targets: Windows8.1+ x64 and macOS Catalina10.15+ Intel; Apple
Silicon requires macOS11+ and its own bundle. Native tests here are on the owner's
Windows PC. There is no Mac available to compile/test/sign an actual `.app`.
Do not label source scripts or a renamed executable as a tested Mac release.

Maintainer commands and entry points:

```text
python -m unittest discover -s tests -p test_builder.py
python tools/builder/build_native_tool.py --help
python tools/builder/export_native_recipe.py --help
python tools/builder/assemble_windows_kit.py --help
python tools/builder/package_gui.py --help
python tools/builder/build_bootstrap_title.py --help
python -m builder --kit KIT --iso OWNER.iso --output NEW/PPSA99145
```

Source-only configuration and build instructions are separate from the end-user
builder. The PS5 instruction title contains no game code and cannot play by
itself. Copy the private complete title while closed and preserve existing hdd
saves. Local generation limits what is distributed; it is not a legal guarantee.
