# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Package a verified public installer title for the in-launcher updater.

Never point this at an owner-prepared title. The native package manifest is the
allowlist: extra saves, ISO images, extracted assets and private game binaries
are not collected. No network upload is performed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

ASSET_NAME = 'Sims2-PS5-Launcher.s2u'
LIMIT = 1024 * 1024 * 1024


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as source:
        for block in iter(lambda: source.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def managed(name):
    if not name or len(name) > 512 or any(ord(c) < 32 or ord(c) > 126 for c in name):
        return False
    if '\\' in name or ':' in name or any(p in ('', '.', '..') or p[-1:] in (' ', '.') for p in name.split('/')):
        return False
    return name in ('eboot.bin', 'installer-manifest.json', 'CREDITS.txt', 'LICENSE', 'LEGAL.txt') or name.startswith(
        ('sce_sys/', 'sce_module/', 'public-resources/', 'compiler-resources/', 'native-title-kit/', 'shaders/', 'licenses/'))


def write_bundle(files, title_id, version, destination):
    """Deterministic writer shared with adversarial transaction test fixtures."""
    records, offset, seen = [], 0, set()
    for name, path in sorted(files.items()):
        if not managed(name) or name.lower() in seen:
            raise ValueError('Unsafe or duplicate public member: ' + name)
        seen.add(name.lower())
        size = path.stat().st_size
        if size > 512 * 1024 * 1024 or offset + size > LIMIT:
            raise ValueError('Public update exceeds protocol limits')
        records.append(dict(name=name, size=size, offset=offset, sha256=digest(path)))
        offset += size
    if not {'eboot.bin', 'installer-manifest.json', 'sce_sys/param.json'} <= set(files):
        raise ValueError('Incomplete public title')
    if len(records) > 10000:
        raise ValueError('Too many public files')
    header = json.dumps(dict(schema=1, launcher_abi=1, title_id=title_id, version=version, files=records),
                        sort_keys=True, separators=(',', ':')).encode('utf-8')
    if len(header) > 4 * 1024 * 1024:
        raise ValueError('Manifest exceeds protocol limit')
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix('.s2u.tmp')
    with temporary.open('wb') as out:
        out.write(b'S2UPD001' + struct.pack('<I', len(header)) + header)
        for record in records:
            with files[record['name']].open('rb') as source:
                for block in iter(lambda: source.read(1024 * 1024), b''):
                    out.write(block)
    if temporary.stat().st_size > LIMIT:
        raise ValueError('Complete update archive exceeds the download limit')
    temporary.replace(destination)
    result = dict(schema=1, asset=destination.name, version=version, title_id=title_id,
                  sha256=digest(destination), bytes=destination.stat().st_size, files=len(records))
    destination.with_suffix('.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


def package(title, manifest_path, destination):
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    if manifest.get('kind') != 'ps5_owner_iso_installer' or manifest.get('status') != 'BUILD_PASS_CONSOLE_NOT_TESTED':
        raise ValueError('Requires a verified public installer native package manifest')
    parameter = json.loads((title / 'sce_sys/param.json').read_text(encoding='utf-8'))
    installer = json.loads((title / 'installer-manifest.json').read_text(encoding='utf-8'))
    if parameter.get('titleId') not in ('PPSA99145','PPSA99148') or not re.fullmatch(r'\d+\.\d+\.\d+', installer['version']):
        raise ValueError('Unexpected installer title/version')
    if installer.get('title_id') and installer['title_id']!=parameter['titleId']:
        raise ValueError('Installer manifest and title identity differ')
    files = {}
    for name, record in manifest['files'].items():
        if not managed(name) and name not in ('assets/README.txt', 'settings.toml'):
            raise ValueError('Unmanaged file in public manifest: ' + name)
        path = title / name
        if any(parent.is_symlink() for parent in [title, *list(path.parents)[:len(Path(name).parts)-1]]):
            raise ValueError('Symlink in public package path: ' + name)
        if path.is_symlink() or not path.is_file() or digest(path) != record['sha256']:
            raise ValueError('Public package checksum/type mismatch: ' + name)
        if name == 'assets/README.txt':
            if (title / name).read_bytes() != b'SIMS2-INSTALLER-PLACEHOLDER-1\n':
                raise ValueError('Owner content found in the public placeholder')
            continue
        if name == 'settings.toml':
            continue  # A player's live preferences are never update payload.
        if not managed(name):
            raise ValueError('Unmanaged file in public manifest: ' + name)
        files[name] = path
    if (title / 'eboot.bin').read_bytes()[:4] != b'\x4f\x15\x3d\x1d':
        raise ValueError('Public launcher is not an SCE SELF')
    return write_bundle(files, parameter['titleId'], installer['version'], destination)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--title', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(package(args.title, args.manifest, args.output)))
