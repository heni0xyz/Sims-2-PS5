# CPU reference image and native Vulkan comparison

30 October 2026. Both the initial CPU reference and actual Vulkan readbacks
showed gray P8 UI/logo art and black sprite rectangles. The fresh scanner had
missed D3DDevice_SetPalette. Inspection of the owner's XBE established the
0x272020 stdcall binding and device palette offset 0xF98. Enabling the existing
HLE fixes both colour and transparency in the actual Vulkan output, including
the island carousel and generated Sim. Owner textures were not changed.

The CPU reference additionally ignored all palettes while decoding. It now
supplies stage colours/alpha and keys its diagnostic cache by palette contents.
The asset-free reference regression passes 16 checks, including palette changes
in one frame and all four palette sizes. See tools/test_reference_palette.py.

Native PC probing found no default hardware Vulkan ICD. The already installed
Edge SwiftShader library can provide an isolated software Vulkan device:
instance creation succeeds, physical device "SwiftShader Device (Subzero)",
API 1.3.0. Its shipped relative DLL path failed in the system loader; a private
out/swiftshader-icd.json with the absolute existing library path fixes loading.
VK_DRIVER_FILES selects it only for the test process. No system/browser driver
file or registry setting was changed. Capability logs remain under out/.

The additional Windows offscreen target enables the same SDK Vulkan backend
and shader emitter used on PS5, with the template's public volk loader and
pinned glslang. Full SPIR-V Tools legalization/validation is required for HLSL;
the initial compiler configuration without it was rejected for visual testing.
The pinned validator needs <iterator> explicitly with libc++ 23. A per-source
forced include supplies it without changing the shared glslang checkout.

The full Vulkan title builds and executes. Its first windowless run exposed a
nested acquisition of the nonrecursive handover mutex. Calling VulkanInstance
outside Init's lock fixes startup. Later runs execute real submissions and
guest completion callbacks, and capture actual game images. This remains PC
software Vulkan evidence. Native PS5 rendering has not been tested here.

Private before/after evidence: out/runs/20261030T150904Z-8b1262 shows the gray
title; out/runs/20261030T151836Z-91447a has corrected island/menu images at
front_01427_host.bmp and front_02043_host.bmp, and generated Sim at
front_03885_host.bmp. Those original runs report INCOMPLETE because their
collector expected cap/vulkan while the runtime wrote into the run root.
Preserve those summaries; do not relabel them as fully accepted runs.

A standalone native probe reproduced weakly_canonical returning Access denied
for the configured path in this host sandbox. The unchecked error produced an
empty path. Periodic capture now uses the configured absolute lexical path,
checks directory/file writes and acknowledges a request only after both real
readbacks are saved. The repeat test uses cap/vulkan and remains separately
recorded. No permission/driver setting was changed to work around this.

Reference snapshots can take several seconds per frame. A pending snap request
must finish before another is issued; use image modification times and the
request timestamp to avoid mistaking a previous image for the new game state.
