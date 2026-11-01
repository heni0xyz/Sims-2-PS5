# Native4K installer and descriptor metadata correction

1 November correction f48a3b24: owner9bb installer capture fails before copying
at the no-follow metadata query (lstat/posix_stat EPERM). Source/staging checks
now use real O_NOFOLLOW descriptors plus fstat in the PS5 adapter, preserving
link refusal/errors. Inspected retry cleanup removes only the recorded files
and directories without libc++ remove_all/lstat. New INSTALL_SOURCE_FAILED
logs cover failures before worker start. PS5 retest remains required.

Installer canvas/font are now native3840x2160 (56/96/144px Open Sans), no
640x480 enlargement or game448-line crop. Full-resolution Kenney256px icons
are sampled for the startup prompts. Public font pack SHA256:
aca1b3f83c2fa14facae060810d3ff4f4477a87cd91fad05ee49af21aba5b624.
PC Vulkan five-screen and backend-recreation test PASS, exact3840x2160 frames
and scale1.0, private evidence out/installer-native4k-f48. Host synthetic
transaction tests PASS. Latest PS5 runtime and extraction NOT TESTED.

Native compile PASS207.29s,121 audited O2 commands; pinned RADV link/package
PASS106.44s; local50-file validation PASS. Buildf48a3b24684ecd4f2479a7c0.
EBoot SHA25692a47c8c21296fd153dcaebde5834b5366c4e6930348b232f0767276a24525bd.
ELF SHA25609c4095eefd1d1eddaf3c801437a773d8e89f875b31b01d4a19fd14037e72deb.
ZIP SHA2565e2fe6ddb18d3ed9a3f9e8915b986c38c1b966063f40107ade7c0f5e60b67d51.
Artifacts: outputs/PS5-Independent/Installer-Native4K-MetadataFix-f48a3b24.
Update ALL native/public files including the new font pack. Preserve existing
ISO/assets/hdd. No owner data is included in this delivery. New PS5 runtime
NOT TESTED; old9bb metadata failure remains retained evidence. No console
contact or publication. Linux descriptor test PASS (regular/dir/missing,
live/dangling link refusal). Full synthetic POSIX transaction test PASS on the local Linux VM with
SIMS2_INSTALL_POSIX_METADATA_TEST: copy/readback, cancel/retry, promotion,
unknown-file preservation, path/duplicate rejection and patch failure.
The NULL host build source lists exclude the unused Vulkan display unit.
