# Verified local environment, updated 8 November 2026

Current native compiler environment: LLVM 22.1.8, PS5_LLVM
be5b58f51c93c67947b7e18cd45c2f9913f2c311; Windows cross-compiler Clang 23.1.4.
The on-console compiler uses its own LLVM 22 builtin headers. SDK tool pin is
f57d28fe65117a20612abc72f2f20d1cd3c8c8e4 (PS5 stat/mkdir control-file adapter;
host path unchanged). The pinned platform/renderer title recipe below remains
in use. No VM or server compilation is needed for these diagnostic increments.

Direct authorized console access and the public control payload were verified
for earlier compiler/load/execute/generation runs. Generation evidence is in
CONSOLE_GENERATION_RESULT_2026-11-08.json. Latest first-unit candidate is BUILD
PASS, CONSOLE NOT TESTED: services were unreachable, and console startup was
deferred. Private endpoint/settings remain outside source control. Older
environment and game-rendering entries below describe their recorded dates.

31 October evening update: the newer owner a80d231d logs now establish
PS5 startup, real gameplay and runtime-reported 3840x2880 targets (scale 6,
3840x2688 picture, 3840x2160 output). The final full gameplay window is
22.103136 ms / 45.24 FPS; measured GPU work is about 6.174 ms/frame.
This fails a stable-60 target. The earlier NOT TESTED statements below refer
to the state before these owner logs arrived. No console was contacted.
See HD_ISO_2026-10-31.md for current changes, evidence and outstanding work.


The host is Windows x86-64. Linux builds run in the already configured local
Debian QEMU machine over loopback SSH; hardware virtualization is unavailable.
LLVM 19 preprocessing/linking runs there, while the local compiler relay runs
LLVM code generation on Windows. No remote compilation server or PS5 execution
is involved in these tests.

| Component | Verified revision/version |
| --- | --- |
| Recompiler SDK public base | TGP482/XboxXBERecomp 91146ff6e2a218b19c1109dc665d06020472ab57 |
| Local tools/harness pin | faeb0eaca270e01a9e83bb2bc587c824c458ac32; harness-only changes |
| MIT Linux/Vulkan runtime public base | abilliontokens/XboxXBERecomp adc40ab688be14da932514bd9f6f59a29dab3c65 |
| Previous local runtime pin | 8f5b2796ce7577d5264ed60505af3bac8f506551; new GPU callback change tracked separately |
| Current local runtime pin | 163c80f2a9a2ac5b35b71eeebacae5526988b3de; native startup presentation without guest crop/scale; earlier CPU-front/palette/UI policies preserved |
| PS5 Vulkan driver checkout | f410e67a0241b3e8edcdbcd2a9b285dfc4a412ed |
| PS5_PayloadSDK source checkout | 3d466ee984c2bcf40358deb26d870a7de73b140c |
| Native SDK cache selected by title recipe | 60ea4c66bf0944e85ea1d3eb078ec45079e7616b |
| Template | 832e783f0230326b36266e27f1482beb5d3b2565 |
| Separate generated foundation | 1d9a274d0615d8995fae9c0138421fe34e76b2db; PPSA99145 |
| klogsrv source checkout | c1b64bd3d3b069a09b1a173a82858cba97a5656f; not started for this work |
| glslang checkout | 1062752a891c95b2bfeed9e356562d88f9df84ac |
| Windows tool compiler | LLVM-MinGW Clang 23.1.4, revision 7a941dfb2877a0b8cf30323fefa85ad019e0b0f5 |
| CMake / Ninja | 4.4.4 / 1.13.2.git.kitware.jobserver-pipe-1 |
| VM debugger | GDB 16.3 |

The native build reuses the completed RADV archives and unchanged established
template linker. The installed Windows SDK command wrappers needed a quoted
argument repair for workspace paths containing spaces and parentheses. The
project retains the repair script, backups, and before/after hashes privately
under out/host-driver-fix; its regression creates a real ELF64 object with
emulated TLS. This changes host command parsing, not PS5 SDK libraries or ABI.

Owner data: Desktop USA ISO, 7,825,162,240 bytes; extracted default.xbe is
3,399,680 bytes, SHA-1 16f9e41ce26c315a758f25d43ba1fa9d3809d1ad.
The 2,960-byte sims2.bin SHA-256 is
5d8512e350d3de422cc0e86c6879f995da6d59c2153b3b4e94e360ab54ee6284.
No ISO/XBE, extracted assets, generated game C++, private save or linked game
artifact belongs in source control. Test runs create separate HDD directories.

Capabilities verified: fresh tools/generation, Linux null-backend linkage,
kernel boot and movie asset loading, and native PS5 SELF/title packaging.
Owner e16 gameplay log/screenshots now verify limited PS5 execution. Full-game,
stability and stable-60 acceptance remain open; new fixes are PC-verified only.

Native Windows null-backend compilation and bounded menu/Create-a-Sim tests
also work. The default Windows hardware Vulkan instance fails with result -9;
the already installed Edge SwiftShader CPU ICD creates a Vulkan 1.3 device.
An isolated per-process ICD file selects that DLL without changing browser,
registry or system drivers. Actual Vulkan frame readbacks confirm corrected
P8 colours/transparency after the independently decoded SetPalette binding.
Software rendering supplies correctness evidence, not representative game FPS.

Artwork build dependencies: Pillow 12.3.0, resvg_py 0.5.0. resvg is installed
only under workspace out/python-deps; no system Python modification.
Native 7bd build reuses existing generated guest objects/RADV archives.

1 November installer resources: supplied Open Sans Regular 3.003 (OFL-1.1),
130832 bytes, SHA-256 33e93bec67d91c396876db50694213802a39e43a911bb5c22322f0dbdf4d5e43.
License reference pin is in repositories.lock.json; Pillow 12.3.0 compiles the
public font resource, with no FreeType library required by the PS5 executable.
