# Draft: Integrate the PS5 ISO launcher and recoverable public updates

Latest hardware evidence (8db5af5a, PPSA99145): verified-cache permission repair,
Play handoff, game boot/Vulkan presentation and bounded normal shutdown PASS.
Initial inspection now gates UI/input and selects Play directly. New Home-only
artwork/global icon, installation carousel/section lock, quieter music and subtle
selected-label pulse are included. See LAUNCHER_PLAY_FIX_2026-11-10.md. The owner
completed extraction/compilation with the preceding launcher; a complete fresh
repeat and the actual public update-helper transaction are still release gates.

The shared launcher now accepts the player's supported USA Xbox ISO, prepares a
private game executable on PS5, and keeps Play, Install and Update in one UI.
Users can navigate with shoulder buttons, D-pad or left stick; the right stick
browses the three supplied gameplay previews. Selection/fade/rejection animation
and a rounded bottom-panel rim provide consistent controller feedback.

Public launcher updates download only a verified, compatible owner-repository
release bundle. A separate copy of the trusted launcher applies the journaled
transaction after renderer shutdown, preserving saves, owner data and settings.
Native game updates remain separate and reuse verified generated/object caches.

Validation: production C++ host transaction and interruption tests, mocked PS5
HTTP metadata contract, 4K ImGui layout/navigation frames, synthetic extraction
tests and fresh owner ISO extraction/generator parity. Native ELF/import/SELF
validation is provided by the existing packager. Details and exact commands:
LAUNCHER_NAVIGATION_UPDATE_2026-11-10.md.

Do not submit this draft as a release-ready claim. A complete fresh ISO-to-Play
repeat, public update helper LoadExec/recovery and haptic acceptance remain open.
HTTPS metadata checking and Home file/registration refresh now PASS on PS5;
Home visual/audio acceptance remains with the owner. No push, GitHub PR or
release upload is authorized by this preparation step.

Before creating the source PR, include the installer/compiler/native writer and
their pinned notices as one dependency-complete change. Export authored source
and explicit release artwork only; exclude out/, private/, owner ISO/XBE,
generated guest C++, guest objects, private game ELF/SELF, logs, settings and saves.
The local runtime-sdk performance commits also need a reproducible published
dependency disposition rather than an unfetchable submodule SHA.

Upstream disposition: the title UI, install protocol and controlled updater
hooks are local integration work. No upstream submission is performed during
this explicitly local-only step. Any general template change must be reviewed
separately with that project's contribution policy and required console proof.
