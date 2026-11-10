# PS5-only installation

Use a **new PS5-only PPSA99145 launcher package** and your own Original Xbox
**The Sims 2 (USA/NTSC)** ISO. The older published Windows-builder package does
not contain this installer. The console's homebrew environment must already be
prepared by its owner.

1. Fully close the title. Transfer the entire launcher folder, keeping
   `sce_sys/`, `sce_module/`, shaders, compiler/native resources and the supplied
   `assets/README.txt` placeholder. Transferring only eboot.bin is insufficient.
2. Put the ISO beside eboot.bin in PPSA99145. Keep enough free storage for the
   ISO, extracted files, generated code and compilation/link caches.
3. Launch the title. The initial check hides sections until installation state
   is known. An existing verified game opens Play directly.
4. Otherwise choose Install and press Cross. The launcher validates the supported
   executable, extracts/patches the owner's private copy, generates native source,
   compiles it and creates the private executable on the console. Progress and
   detailed failure diagnostics are written to klog.
5. Only after successful preparation, choose **Keep ISO** or **Delete ISO**.
   The source image is never deleted automatically.
6. Choose Play. The launcher fades to black before handing off to the game.

L1/R1, D-pad or left stick move sections. Right stick changes previews; they also
advance automatically during installation. Section changes are locked while the
game is being prepared. Touchpad toggles launcher music and saves the choice.

## Updates and saves

Update launcher is accessible even before installing a game. A newer compatible
public release is required for the automatic downloader; older legacy release
assets are not treated as compatible updates. Update game is separate and becomes
available when the current launcher includes changed game preparation data.
Verified cached units can be reused.

Keep `hdd/`, settings, launcher preferences, source ISO and private build/cache
files when replacing public launcher files manually. Never replace a running
title's executable or erase saves to repair a build problem. Older prepared game
files with mode0644 are repaired by the new launcher after SHA verification.

Do not upload your ISO, extracted files, generated game C++, compiled game
objects, personal saves or the private game executable to GitHub.

The PC builder remains in source for legacy releases and maintainer tooling;
it is not required by the new PS5-only player installation path.
