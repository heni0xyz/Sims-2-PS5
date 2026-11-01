# Playability acceptance

A successful native link is build evidence. It does not certify a playable
game. The owner has verified basic PS5 gameplay; full-game/stability acceptance
remains incomplete. New HD/ISO changes currently have PC evidence only.
No finite test suite proves that every possible save and play session works.

## What is compared

- Input identity: exact USA default.xbe SHA-1 and owner-disc DSP data hash.
- Translation: fresh static x86-to-C++ generation from that executable; the
  native ELF is not expected to match the Xbox executable byte for byte.
- ABI: guest 32-bit addresses, stack cleanup, callbacks and HLE results.
- Behaviour: bounded scenarios with recorded inputs, build identity, logs,
  screenshots and persistent-state checks. Deterministic buffers can be
  compared exactly; whole-game memory differs because of timing and addresses.
- Rendering: actual frame images, including correct targets, HUD and menus.
  A null renderer frame counter is only CPU/HLE progress evidence.

## Required scenarios

| Gate | Acceptance evidence | Current status |
| --- | --- | --- |
| Data | required files, USA XBE/DSP hashes, fresh isolated test HDD | PASS on PC after correcting missing root files |
| Boot | no guest faults, working device initialization | PASS on Windows null backend; actual Vulkan validation in progress |
| Movies | EA, Maxis and intro progress with picture and audio | partial: EA frame captured and GPU callback fixed; full sequence not passed |
| Menus | mode selection, navigation, back and confirm | observed: island carousel, save slots/back, new-game confirmation and player count; actual Vulkan colours/alpha corrected |
| Create a Sim | create/modify and accept a Sim | generated female/male, editor navigation, Done and save-slot selection observed; created female loads into a lot |
| Story gameplay | load lot, walk, interact, advance simulation, meet a goal | basic PC observation: created Sim restored, simulation advances, classic camera/target responds and Sim moves into house after direct-mode input; object/goal gate pending |
| Freeplay | load/create family and representative lots | partial: actual Vulkan loads default Freebers at 8 Rockpile Rd and tutorial dialogs; full gameplay gate pending |
| Build/buy | place, rotate, delete, buy and sell objects | NOT TESTED |
| Audio/input | stereo playback, prompts, controls and reconnection | NOT TESTED on independent title hardware |
| Persistence | save, clean shutdown, restart and reload same state | PC save/restart/reload observed: created Sim and same lot/tutorial restored; clean shutdown and PS5 save validation pending |
| Stability | bounded extended gameplay; no fatal faults or growing allocations | NOT TESTED |
| PS5 | verified matching native artifact, real frame and working controls | Owner verified basic gameplay; a80 klog identity matches. New candidate requires hardware test |
| Performance | representative gameplay frame times and 1% lows | FAIL stable60: a80 final window 45.24 FPS; more representative scenarios needed |
| 4K | measured internal detail covering 3840x2160 | PC readbacks and a80 PS5 extent logs verify 3840x2880 backing / 3840x2688 picture; stable 4K60 fails current test |

Every failure remains attached to its run. Timeout, loss of logs, a static
error screen, and a clean process exit before the expected milestone cannot
pass a gameplay gate. Initial 4:3/30 FPS settings establish correctness before
porting the earlier independent UI and performance changes.
