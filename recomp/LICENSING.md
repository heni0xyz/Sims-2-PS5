# License scope

Selected explicitly by szampan on 31 October 2026:

- Project-authored glue, hooks, tools and tests: **GPL-3.0-only**, LICENSE.
  Retain source provenance and CREDITS.txt. Codex-authored implementations
  were prepared at szampan's direction.
- szampan's four title-art source files under artwork/source/ and generated
  icon/backgrounds: **CC0-1.0**, artwork/CC0-1.0.txt.
- Kenney Input Prompts: **CC0-1.0**, original notice under mods/dualsense/kenney.
- Installer Open Sans Regular3.003 and its glyph resource: **OFL-1.1**,
  third_party/opensans/OFL.txt. The TTF remains unmodified; font copyright and
  license ship separately. OFL permits bundling with software, without
  relicensing the font or changing the project's GPL-3.0-only code license.
- sdk/, runtime-sdk/, the PS5 foundation, driver and toolchain dependencies:
  retain their own original licenses and notices. MIT-licensed upstream
  components and the earlier MIT runtime integration are not represented as
  original project code; see PROVENANCE.md and the import inventories.
  New GPL-covered SDK extensions are identified separately in
  runtime-sdk/LOCAL_CHANGES_LICENSE.md; original MIT grants/notices remain.

The original game executable, translated/generated game code, game assets,
disc images and personal saves are **not** covered by this project's GPL or
artwork dedication. They stay outside the source repository. A native game
eboot contains translated game code even when the release has no textures or
ISO. No public binary-distribution permission is inferred from an SDK's MIT
license, the user's own dump, a clean Git status or a source-only audit.

Publishing a combined binary and its required corresponding source needs a
separate distribution review, including original game rights and all linked
dependencies. The source/build tooling can be prepared locally while that
review is pending. No push, upload or release publication is authorized here.
