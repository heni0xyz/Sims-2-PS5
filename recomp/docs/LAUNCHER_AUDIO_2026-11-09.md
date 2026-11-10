
## 2026-11-10: background, persistent music mute and separate update actions

Candidate 1bd9b9f22e9e99bb built/linked/import-checked as a native SELF and deployed to PPSA99148. Twenty changed files read back with matching SHA-256. Previous eboot/metadata backed up privately; settings and the separate working game title were preserved. User closed the launcher; idle state verified before/after. No application was launched by the agent.

Home background now has real 3840x2160 BC7_UNORM DX10 pic0.dds/pic1.dds, generated from the original CC0 source; mean channel error 0.0588. Eleven Home presentation files verified, title re-registered through PS5 Upload. Read-only registry check confirms pic0Info points at the new DDS. No direct database writes.

Launcher gain is 20% of original PCM. Touchpad toggles music with a short gain ramp; .sims2-launcher-prefs atomically retains the last choice outside public resources/saves. Host tests PASS for gain bounds, mute ramp, muted startup, full-track decode, atomic preference replacement, malformed input and failed-save cleanup. Ten actual 4K UI scenarios/navigation checks PASS. New Touchpad/D-pad prompts reuse Kenney CC0. Console audio/preference/haptics acceptance remains the owner's test.

Update section is accessible without a game. Up/Down chooses Update launcher or Update game. Game update requires a verified existing private executable plus changed generator/compiler/native/data identities in the current launcher, and reuses verified cached units. Startup now reports an available game update instead of automatically rebuilding. Completed receipts associate the game with launcher_version. Live settings.toml is not treated as a fixed compiler resource. Owner data migration still requires a verified migration, never silent replacement.

Limitation: Update launcher currently checks GitHub versions; automatic download, transaction/helper handoff and rollback are NOT implemented. Public release v1.0.0 still contains the older PC-builder package and instruction title, not this PS5-only installer. Do not call this a working automatic launcher updater or a full ISO-to-Play acceptance PASS. Game update operation also needs an installed-game console test. Private evidence under out/audio/deploy-1bd9, home-refresh-1bd9 and out/ps5-installer/preview-preferences.

---
# Current audio and launcher state — 2026-11-10

Build `78ebceeb197ec7e2` adds visible Play / Install / Update sections,
post-install locks, direction-aware rejection animation and short strong
rumble, requested Enjoy the game! / The Sims 2 PS5 headings. Update currently
checks release metadata and offers manual update guidance; automatic payload
installation is not implemented and must not be claimed. Nine 4K host layout
scenarios and navigation/rejection timing checks PASS. Native PS5 link/import/SELF
checks PASS. Deployment: 18 changed files read back and SHA-256 verified;
settings/saves and the separate game title PPSA99145 preserved.

Home icon now derives from szampan's supplied CC0 plumbob, not the template TS
placeholder. Home sound is the full 120-second original song, re-encoded to
48 kHz stereo / 120 kb/s ATRAC9 with a whole-track loop: 1800488 bytes, under
2 MiB. The original MP3 and tags stay unchanged. Host PCM/RIFF/loop bounds PASS.
Console Home visual/audio acceptance and actual rumble still require owner
testing. Re-registration and presentation-copy results: out/audio/home-refresh-78eb.

The records below describe earlier drafts, including the superseded six-second
Home sound. This source README correction will enter the next regenerated
installer resource set; the deployed draft has correct audio/metadata hashes.

---

# Launcher and Home selection audio

Original supplied by szampan: `artwork/audio/LTqj8VkTC8z-tTY7vztlF.mp3`.
120 seconds, stereo 48 kHz; original bytes and ID3 provenance retained.
SHA-256: `5091f1d51b379088e4f4d159323b0cb9e63650b061d627f3927e82617c072a70`.

Home track: `artwork/audio/snd0.at9`, 144484 bytes, 192 kb/s ATRAC9.
Five seconds of music followed by one-second fade, six seconds total.
No RIFF `smpl` loop. Host validation checks RIFF bounds, channel/rate profile,
288000 sample count, file size and fade reaching digital silence in processed
PCM. Original metadata stays in the MP3 in source, and the same original MP3
is copied unchanged to `public-resources/launcher-music.mp3`.

Launcher decodes its full MP3 once, then feeds the foundation's existing 48 kHz
AudioOut callback. No disk I/O, allocations or decoding occurs in that callback.
Playback has a conservative background volume and short end fades at loop wrap.
Audio stops/joins before handing off to the private game executable, and during
launcher teardown. Music failure is logged and does not stop the installer.

Host test PASS: missing input silence, full 120-second decode, identical output
for different callback partitions, bounded volume, source-end wrap and failed
reload clearing old audio. PS5 source compilation PASS. First link found the
template's namespaced sample wrapper had locally redeclared the music class;
the common prelude now includes its header globally, as for the other launcher
interfaces. Rebuild/relink evidence is under `out/audio/`.

Encoder sources are pinned to PSGFX
`5ee7aa9e7d91c9f518322847d13a6bda5ba97837` with original licences/notices.
This is the public GPLv3 encoder, not a proprietary SDK converter.

Console Home listening, no-repeat behaviour, launcher AudioOut and Play handoff
with music are NOT TESTED. No PS5 deployment or gameplay/performance PASS follows
from these host/container checks. The Home shell may need title metadata refresh;
do not modify firmware or its databases to force a result.

Reproduce with the existing host clang++:

```
python tools/audio/prepare_music.py --cxx <host-clang++>
<host-clang++> -std=c++20 -O2 -static -pthread -Icompiler tests/ps5/installer_music.cpp compiler/installer_music.cpp -o out/audio/test_installer_music.exe
out/audio/test_installer_music.exe artwork/audio/LTqj8VkTC8z-tTY7vztlF.mp3
```

Then use the existing `tools/ps5/build_installer.py` prepare/package phases with
the configured settings and public platform headers. Resource staging verifies
both audio hashes and includes the original provenance and minimp3 notice.

Separate pending work: Update tab/transactional public updater is not implemented
yet; current release metadata only provides a version check. Latest accepted
performance experiment is private runtime `4801f87a`, not the older game runtime
inside the launcher's current public kit. Full installer-to-Play validation is
still pending. Do not describe this audio build as the finished release.

Audio launcher package c85392b72f89130e: native link/import/SELF and packaged audio readback hashes PASS. Location: out/ps5-installer/resources-8403ec2bd477785a/package-attempt-0002/PPSA99148. Console audio NOT TESTED.

Console deployment c85392b72f89130e to PPSA99148 PASS: 75 files read back and SHA-256 verified, previous raw signed eboot/manifest/param/settings backed up under ignored out/audio/deploy-c853/backup. Existing settings preserved; PPSA99145 untouched. Console idle before/after. Application deliberately not launched, per owner request to test manually. Home/launcher audio remains AWAITING_OWNER_TEST.

Home refresh 2026-11-10 PASS: seven presentation files backed up/uploaded/readback verified, PS5 Upload APP_REGISTER succeeded for PPSA99148. Read-only verification confirms the title registry now references the correct snd0.at9 and icon0.png with new cache timestamps. No direct database writes, uninstalls, launches or reboots were performed. Visual/audio/haptic acceptance remains the owner's test. Receipt: LAUNCHER_RESULT_2026-11-10.json.
