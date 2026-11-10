# Setup Fix1 — missing template resource directory

Owner klog identifies `sample thesims2` and `exit(-1)` before Vulkan creation.
The return PC0x49a158 minus the native image base0x400000 resolves to0x9a158:
VulkanExampleBase constructor, base/vulkanexamplebase.cpp:1046. This is the
failure branch of `stat(getAssetPath())`, compiled as `/app0/assets/`.

The public package omitted that directory because it contains no game data.
The constructor still requires it before reaching the authored instruction UI.
SIGSYS is observed during the unsupported exit path; the meaningful first
failure is the missing directory. No shader/game/ISO compilation is involved.

Fix: package `assets/README.txt` so extraction creates the directory. The
instruction eboot remains byte-identical:
ed6279e16133bd9949aac013d7caa79988aea8b1939ba6f22457965ab6351afa.
Only public documentation is allowed there in the setup archive; no game files.
The owner confirmed that adding the README displays the instruction screen.

Both bootstrap staging paths now create the directory. An asset-free staging/
ZIP regression verifies it survives packaging, the Open Sans font is staged,
and no ISO/XBE/ARC/XMV is included. This validates the public instruction title;
the Windows-built full game and gameplay/performance remain separate tests.

Windows UX: prefer the portable folder ZIP over the previous one-file exe.
It keeps Python/Tk, native tools and the kit beside the small executable and
avoids extracting roughly half a gigabyte before each GUI launch. macOS native
tools/app remain unavailable and are not advertised as an included download.
