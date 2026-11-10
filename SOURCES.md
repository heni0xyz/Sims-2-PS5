# Corresponding source and dependency directions

This repository contains the authored title, builder, native adapters, reviewed
analysis configuration and complete local Xbox SDK integrations. It also
contains the exact native SDK additions at `vendor/PS5_PayloadSDK`:
60ea4c66bf0944e85ea1d3eb078ec45079e7616b (a local revision, not an upstream URL).
Its original public base is identified by its source notices/build scripts.

The new PS5-only installer foundation is included as `PS5_Sims2Installer/`.
Generator/runtime SDKs are complete source directories under recomp, including
local PS5 adapters. Their local provenance SHAs need not be fetched upstream.
`source-component-manifest.json` records exported source hashes. Native SDK
header symlinks are flattened only to contents within the pinned public source
archive, including FreeBSD implementation headers.

Other unmodified public components can be obtained at these exact revisions.
Keep these source directions beside the binary downloads; do not substitute
newer revisions when reproducing this release:

| Component | Source |
|---|---|
| PS5-hosted Clang/LLVM/LLD | https://github.com/mihawk-99/PS5_LLVM/tree/be5b58f51c93c67947b7e18cd45c2f9913f2c311 |
| PS5_Vulkan/native packaging/CRT/link scripts | https://github.com/mihawk-99/PS5_Vulkan/tree/f410e67a0241b3e8edcdbcd2a9b285dfc4a412ed |
| Mesa/RADV | https://github.com/mihawk-99/PS5_Mesa/tree/1ba9874f478657ccf6da72ce55f0260164cc0bd0 |
| glslang | https://github.com/KhronosGroup/glslang/tree/1062752a891c95b2bfeed9e356562d88f9df84ac |
| SPIR-V dependencies | glslang's pinned known_good.json and update_glslang_sources.py at that revision |
| Zydis | https://github.com/zyantific/zydis/tree/a2278f1d254e492f6a6b39f6cb5d1f5d515659dc |
| toml++ | https://github.com/marzer/tomlplusplus/tree/30172438cee64926dc41fdd9c11fb3ba5b2ba9de |
| zlib1.3.2 | https://zlib.net/zlib-1.3.2.tar.gz |
| Windows LLVM/MinGW tool distribution | https://github.com/mstorsjo/llvm-mingw/releases/tag/20261020 |
| Python3.12.14 | https://www.python.org/downloads/release/python-31214/ |
| PyInstaller6.16.0 | https://github.com/pyinstaller/pyinstaller/tree/v6.16.0 |
| Open Sans | supplied unmodified3.003 font under OFL, included in recomp/third_party/opensans |

zlib source SHA256: bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16.
PS5 foundation sources retain original MIT notices; new title/builder code is
GPL-3.0-only. Native-tool/platform sources have GPL-3.0-or-later notices. LLVM
has Apache2 with LLVM exception; other dependency licenses remain their own.

Compiled generic libraries are included in the executable's audited public kit,
with notices. The exact evaluated link recipe and hashes are in that kit. No
translated game code is supplied in this repository or public setup title.
The player's generated functions/assets and complete game eboot remain private.
