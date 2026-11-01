# Build from source (maintainers)

Players use the packaged builder described in INSTALL.md. These instructions
are for rebuilding public tooling and runtime dependencies.

1. Read `recomp/docs/BUILD.md`, `recomp/builder/README.md` and
   `recomp/docs/BUILDER_RELEASE.md`. Install public Clang/LLVM, CMake, Ninja,
   Python and Git. Generate reviewed guest sources from your own USA ISO.
2. Build host tools with `recomp/tools/host/CMakeLists.txt` and PS5 objects with
   `recomp/tools/ps5/build_objects.ps1` (its `-Dependencies` option selects the
   documented toolchain layout). Preserve the source/config hashes and O2 flags.
3. Prepare PS5_Vulkan/RADV at the exact source pins in SOURCES.md and use the
   included native SDK fork. Follow the foundation's actual setup/build scripts.
   A Linux development environment is currently needed to build RADV itself.
4. Build the original native packaging implementation with
   `recomp/tools/builder/build_native_tool.py`. It uses the original four-source
   recipe and checked zlib1.3.2 source archive.
5. Export an already working template/RADV native recipe with
   `export_native_recipe.py`; this evaluates the SDK wrapper too and retains
   PIE, 16 KiB pages, TLS, library search files and exact input ordering.
6. Assemble the public Windows kit with `assemble_windows_kit.py`. Run
   `prepare_public_notices.py --kit KIT` before packaging; the packager checks
   the authored GPLv3 scope and dependency notices. The kit assembler audits
   defined symbols and omits the private guest archive. Package the one-file
   Python/Tk GUI using `package_gui.py --kit KIT --output OUTPUT` on Windows.
7. The public instruction title is the CMake `samples` target of
   `PS5_Sims2Bootstrap/ps5`. For a split Windows SDK, pass the pinned SDK fork's
   `platform/include` through `PS5_PLATFORM_INCLUDE_DIR`. Preserve quoted paths.
   `build_bootstrap_title.py` links its CMake object inventory through the same
   evaluated native recipe and checks it contains no generated game functions.

All builder scripts have `--help`. From `recomp/`:

```text
python -m unittest discover -s tests -p test_builder.py
python -m builder --kit PATH/kit --iso PATH/owner.iso --output PATH/NEW/PPSA99145
```

SDK header symlinks were flattened into regular source files for portable ZIP
extraction; their content resolves within the exact pinned archive. Compiled
public kits/toolchains are release artifacts, not Git source. Generated guest
sources and binaries remain private. macOS native host tools/app still need a
real Catalina-compatible build host and validation; common Python source alone
does not make the Windows kit usable on a Mac.
