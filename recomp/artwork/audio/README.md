Launcher music supplied by szampan on 2026-11-09.

`LTqj8VkTC8z-tTY7vztlF.mp3` is the unmodified original, including its ID3
AI-generated/Boppy provenance tags. `source-metadata.json` records its SHA-256
and embedded tags. This audio is separate from the CC0 title artwork and the
GPLv3 application source; no original Sims game audio is used here.

Boppy's published song-use terms allow personal, creative and commercial use:
https://boppy.me/terms (section 8), https://boppy.me/ai-model
Checked 2026-11-09. The service does not promise exclusivity or copyright protection.
The supplied original and its provenance remain in the source distribution.

`snd0.at9` is a 48 kHz stereo, 120 kb/s ATRAC9 Home selection track: the complete
120-second song with an RIFF `smpl` loop. This bitrate keeps the full track
below the PS5 2 MiB limit. The earlier six-second non-looping draft was
superseded at szampan's request; the original MP3 remains unchanged.
Rebuild with `tools/audio/prepare_music.py --cxx <host-clang++>`.
The output and original hashes are in `home-sound.json`; the listening preview
is written to the ignored `out/audio/home-preview.wav`.

Encoder: PSGFX / PS5 AT9 Converter by BlackBearReloaded, GPL-3.0-or-later;
LibAtrac9 tables by Alex Barney, MIT. Pinned sources and their full notices
are under `tools/audio/third_party/psgfx_at9`.

Host container and fade checks do not establish PS5 Home playback or the
shell's handling of a looping selection track. That requires a console
listening test after title metadata is refreshed.
