#!/bin/bash
# Validate the actual PS5 runtime dependency with independently generated code.
# A separate object cache prevents mixing the two SDKs' inline CPU helpers.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
export XBE_RUNTIME_ROOT="$root/runtime-sdk/runtime"
export XBE_HARNESS_ROOT="$root/runtime-sdk/harness"
exec bash "$root/sdk/harness/build.sh" -r "$root" -g "$root/generated/independent" \
    -o "$root/out/headless-runtime" -n Sims2Independent \
    -x '/gpu/(vulkan|spirv).cpp|/audio/sdl_audio.cpp|/game/install_display.cpp'
