#!/bin/bash
# Use the pinned SDK's own build recipe. Generated game code remains private.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
exec bash "$root/sdk/harness/build.sh" -r "$root" -g "$root/generated/independent" \
    -o "$root/out/headless" -n Sims2Independent -x '/gpu/texturedumper.cpp'
# texturedumper belongs to the Windows D3D12 texture-pack path (WIC/COM).
# The SDK null backend does not call it. Keep the upstream source unchanged.
