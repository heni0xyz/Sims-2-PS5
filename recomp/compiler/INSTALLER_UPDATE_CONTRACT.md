# ISO-only installer and updates

The shell entry remains the shared launcher for both unprepared and prepared
installations. Once preparation is verified, its Play tab becomes active and is
selected; starting a prepared title must still show this same launcher, not skip
straight to a different game-only menu. The public launcher eboot is never
overwritten by the private game. The private executable is handed off only when
Play is activated. This remains part of the pending full installer-to-Play
console acceptance test; standalone private performance builds do not prove it.

Requested outcome: a public title containing tools, runtime, current patches,
Open Sans and the existing filled-white Kenney DualSense artwork. The player
supplies the supported Xbox USA ISO and performs preparation on PS5. A private
game executable is generated beside the public installer, never over its running
eboot. The shell's documented LoadExec handoff replaces the process after the
installer has drained Vulkan. Subsequent launches validate and reuse the result.

Current proof: complete console code generation and compilation, native linking,
conversion and FSELF creation pass. Canonical evidence is in the three console
result JSON documents. The extraction/compilation/launch UI is now integrated
and builds as PPSA99148 with the selected layout B / GAMEPLAY. Its five screen
states have host layout coverage. The complete ISO-to-Play workflow and new
launcher still require console validation; a diagnostic PASS alone is not a
complete installer.

## Version boundaries

The public manifest must separately identify:

- source XBE SHA-1 and extraction/data-patch version;
- generator, analysis config and XDK descriptions;
- native compiler revision, target flags and CPU runtime headers;
- each generated source and the shared generated header;
- each compiled object's digest and the inputs that produced it;
- public runtime/HLE/hook objects, native libraries and captured link recipe;
- converter version and the final private ELF/SELF digests.

The installer checks these identities on the console. It must not trust file
presence or a stale "installed" marker. An interrupted build retains completed,
verified units and diagnoses the failing step. Temporary output never becomes
the current game until complete validation and promotion.

## Expected update behavior

External textures/fonts/artwork can update without regenerating guest code.
Changes to compiled rendering, HLE or authored hooks require relinking with
the new public objects; the unchanged guest objects are reused. Generator or
analysis changes regenerate source, then only source units whose inputs changed
are recompiled. A shared header, CPU header, compiler or ABI/flag change invalidates
all affected units. Conservative invalidation is preferable to running a stale
binary with a new ABI.

Every fresh installation uses the current public manifest and patch resources,
so a new player receives the included graphics, 16:9 and controller changes.
It is not necessary to distribute translated game code to achieve that. Changes
to patched owner data need a retained original input or a verified source ISO;
an update must not silently overwrite an unknown user modification. Saves/HDD
are separate and are never replaced by preparation or updates.

The source ISO may be deleted only after the prepared data and native executable
are verified and the player explicitly chooses Delete. Keep is always available.
Merely downloading a newer release must not delete ISO, saves or private cache.
The launcher updater now accepts only the fixed public `Sims2-PS5-Launcher.s2u`
asset from a newer stable release in the configured owner repository. GitHub's
SHA-256 asset digest, archive inventory, title identity and version must match.
Older PC-builder ZIP releases remain manual-only. The public-file transaction
is applied by a copy of the current launcher after clean Vulkan shutdown; the
shell eboot is atomically published last. Startup recovery runs before loading
UI resources. Saves, settings, ISO, extracted assets and private build cache are
outside the positive update allowlist. SDK/module changes require a manual full
package because the helper must keep the currently loaded libc ABI stable.

Host transaction/HTTP-contract tests pass. Actual PS5 HTTPS, helper arguments,
LoadExec, write permissions, interruption recovery and Home metadata refresh
still require console acceptance. Host tests do not establish power-loss safety.

## Current working locations

PS5_Sims2Installer was created using the official template factory, PPSA99148,
name The Sims 2, 60 Hz. It is a separate foundation for integration; the established
PPSA99145 game and PPSA99147 diagnostic remain untouched. The final UI must not
present Compiler Diagnostics as the normal player flow. Gameplay startup and
controller/graphics behavior must be verified with the console-created executable
before a public release is claimed.
