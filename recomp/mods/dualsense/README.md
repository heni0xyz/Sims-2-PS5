# DualSense prompts — Kenney CC0

Filled-white Default sources: kenney/PNG and kenney/SVG.
Original upstream license: kenney/License.txt. Source: https://kenney.nl/assets/input-prompts
Patch implementation: OpenAI Codex, for szampan. Full authorship: ../../CREDITS.txt.

Run tools/ps5/patch_prompts.py from the repository or the standalone patch ZIP.
Instructions and coverage limits: ../../docs/UI_PATCHES.md.
The .bc2 files contain only replacement mip data; no original game bytes.
SVG text is stored with LF line endings, enforced by .gitattributes; selector
SHA-256 values refer to those exact bytes on both Windows and Linux.
Keep the raw owner archive unchanged. Use the independent staging tool with
--dualsense to create a separate private title; never mix executables/packages.
