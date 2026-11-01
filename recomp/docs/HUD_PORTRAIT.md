# Native portrait camera correction

Authored by OpenAI Codex for szampan, 31 October 2026. This is presentation
glue derived from the owner's supported USA executable, not imported game
hooks or edited generated output.

The 0MOD root offset correctly moves the blue 2D backdrop. Its CB child does
not consume that root transform for the head's separate camera. Changing the
CB's XML x/width did not fix the real image and was rejected.

The fresh pointer table at 0x339124 names 0MOD_head. Registration at 0x147380
installs callback 0x147D60. A verified read-only PC inspection identified its
actor->5EC renderer, vtable 0x2F7CB0 and method +8 at 0x67D60. That method
calls the camera-rectangle setter 0x1CBC20 at 0x67E67. The setter copies four
normalized bounds and computes native camera/pixel state. The original
function executes; no initialization or drawing is suppressed.

A first thread-local draw-tag experiment failed: the callback runs on the
game/UI thread, while the game queues rendering to a different thread. An
actual one-frame capture contains 978 full-screen viewports and no tagged
draws, despite callback-entry logs. PE inspection independently verified that
the dispatch table resolves to the strong authored hook, rejecting a weak-
symbol/linker hypothesis. The tag API/state and that hook were removed.

The current hook selects only the setter's return address 0x67E6C, 16:9,
enabled correction and an observed full-height single-player HUD. It maps
normalized left/right bounds by 45/56 and adds the centered-safe-area offset
only when edge anchoring is disabled. Vertical bounds stay unchanged. The
input rectangle is restored after the original setter (also on exceptions);
the original setter's computed camera state retains the correction for the
game's own queued renderer. Unknown/world/split-screen paths pass through.

PC run 20261031T104328Z-0a30de logs the actual camera transition:
x 0.18906300..0.27356300 -> 0.15192562..0.21982740,
y 0.78862703..0.91262704 unchanged. Actual 1280x720 final readbacks
presented_03391.bmp and presented_03497.bmp verify the head centered on its
own blue backdrop, separate from the complete left-anchored circle/needs HUD.
This is not a claim of PS5 validation or a 128x128 head-texture replacement.

The earlier captured 128x128 cached target is a full-body preview on magenta.
Keep it, the 64 original texture records and nine Flash records in the private
research archive for later 4K work. Raw images/owner records never enter Git.
