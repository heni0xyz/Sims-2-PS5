#!/usr/bin/env bash
# Link a verified Windows/Linux PS5 object bundle using the proven title recipe.
set -euo pipefail
[[ $# == 3 ]] || { echo 'usage: link_exported.sh INPUT_DIR FOUNDATION_DIR VULKAN_DIR' >&2; exit 2; }
inputs=$(cd -- "$1" && pwd)
foundation=$(cd -- "$2" && pwd)
vulkan=$(cd -- "$3" && pwd)
python3 - "$inputs" <<'PY'
import hashlib,json,sys
from pathlib import Path
root=Path(sys.argv[1]);manifest=json.loads((root/'link-input-manifest.json').read_text())
if 'CREDITS.txt' not in manifest['files']: raise SystemExit('Missing hashed patch credits; re-export the input bundle')
for name,record in manifest['files'].items():
    path=(root/name).resolve()
    if not path.is_relative_to(root) or not path.is_file(): raise SystemExit('Invalid link input path')
    with path.open('rb') as stream: digest=hashlib.file_digest(stream,'sha256').hexdigest()
    if path.stat().st_size!=record['bytes'] or digest!=record['sha256']: raise SystemExit('Link input digest mismatch: '+name)
print('LINK_INPUTS_VERIFIED build='+manifest['build_id'])
PY
sdk=$(bash "$foundation/ps5/tools/setup-sdk.sh")
[[ $(cat "$sdk/.ps5-sdk-revision") == 60ea4c66bf0944e85ea1d3eb078ec45079e7616b ]]
cd -- "$inputs"
bash "$foundation/ps5/tools/link-title.sh" "$inputs" "$vulkan" "$sdk" "@$inputs/inputs.rsp"
title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$foundation/ps5/sce_sys/param.json")
[[ $title_id =~ ^PPSA[0-9]{5}$ ]] || { echo 'Invalid title identity for credits' >&2; exit 2; }
cp -- "$inputs/CREDITS.txt" "$foundation/dist/$title_id/CREDITS.txt"
