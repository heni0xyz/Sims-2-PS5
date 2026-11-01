"""Apply public replacement mip data to a copy of the verified USA texture ARC.

The source archive is never opened for writing. No game bytes are in this tool
or its manifest. Python's standard library is sufficient to apply the patch.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import tempfile


class PatchError(ValueError):
    pass


def digest(path: Path) -> str:
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def read_index(stream) -> dict[str, tuple[int, int]]:
    """USA ARC: 16-byte header, records, then count/hash/offset/size/name/time index."""
    stream.seek(0, os.SEEK_END)
    size = stream.tell()
    stream.seek(0)
    header = stream.read(16)
    if len(header) != 16:
        raise PatchError('Truncated ARC header')
    offset, version = struct.unpack_from('<II', header)
    if version != 0x10000 or not 16 <= offset <= size - 4 or size - offset > 4 * 1024 * 1024:
        raise PatchError('Unsupported or invalid ARC header/index')
    stream.seek(offset)
    index = stream.read()
    count = struct.unpack_from('<I', index)[0]
    if not 0 < count <= 100000:
        raise PatchError('Invalid ARC entry count')
    pos = 4
    entries = {}
    for _ in range(count):
        if pos + 12 > len(index):
            raise PatchError('Truncated ARC entry')
        _, start, length = struct.unpack_from('<III', index, pos)
        pos += 12
        end = index.find(b'\0', pos, min(pos + 257, len(index)))
        if end < 0 or end + 9 > len(index):
            raise PatchError('Invalid ARC name/timestamp')
        try:
            name = index[pos:end].decode('ascii')
        except UnicodeDecodeError as error:
            raise PatchError('Non-ASCII ARC name') from error
        pos = end + 9
        if not name or name in entries or start < 16 or length < 20 or start + length > offset:
            raise PatchError('Duplicate or out-of-bounds ARC entry')
        entries[name] = start, length
    if pos != len(index):
        raise PatchError('Unexpected trailing ARC index bytes')
    regions = sorted(entries.values())
    for (start, length), (next_start, _) in zip(regions, regions[1:]):
        if start + length > next_start:
            raise PatchError('Overlapping ARC entries')
    return entries


def texture_mips(stream, entry, name):
    offset, size = entry
    stream.seek(offset)
    record = stream.read(size)
    if len(record) != size or size < 20:
        raise PatchError('Truncated texture record: ' + name)
    kind, tag, unknown, name_size = struct.unpack_from('<4I', record)
    encoded = name.encode('ascii') + b'\0'
    if (kind, tag, unknown, name_size) != (9, 0x5458464C, 0xFFFFFFFF, len(encoded)):
        raise PatchError('Unexpected texture record header: ' + name)
    if record[16:16 + name_size] != encoded or 20 + name_size > size:
        raise PatchError('Texture record/index name mismatch: ' + name)
    payload_size = struct.unpack_from('<I', record, 16 + name_size)[0]
    payload = record[20 + name_size:]
    if payload_size != len(payload) or len(payload) != 1376:
        raise PatchError('Expected 32x32/16x16/8x8 BC2 texture: ' + name)
    if struct.unpack_from('<HH', payload, 16) != (32, 32) or payload[22] != 3:
        raise PatchError('Unexpected texture size/mip count: ' + name)
    return offset + 20 + name_size + 32, payload[:32], payload[32:]


def apply(source: Path, output: Path, patch_dir: Path) -> dict:
    source, output, patch_dir = source.resolve(), output.resolve(), patch_dir.resolve()
    if source == output or output.exists():
        raise PatchError('Output must be a new file, different from the original archive')
    manifest = json.loads((patch_dir / 'manifest.json').read_text(encoding='utf-8'))
    if manifest.get('schema') != 1:
        raise PatchError('Unsupported patch manifest schema')
    if digest(source) != manifest['source_archive_sha256']:
        raise PatchError('Archive SHA-256 mismatch; use the unmodified supported USA textures.arc')
    replacements = []
    with source.open('rb') as stream:
        entries = read_index(stream)
        if len(entries) != manifest['source_entry_count']:
            raise PatchError('Unexpected ARC resource count')
        seen = set()
        for item in manifest['replacements']:
            name = item['resource']
            if name in seen or name not in entries:
                raise PatchError('Missing or duplicate patch resource: ' + name)
            seen.add(name)
            offset, header, original = texture_mips(stream, entries[name], name)
            if hashlib.sha256(header).hexdigest() != item['header_sha256'] or hashlib.sha256(original).hexdigest() != item['original_mips_sha256']:
                raise PatchError('Original texture signature mismatch: ' + name)
            filename = item['mips_file']
            if Path(filename).name != filename or '/' in filename or '\\' in filename:
                raise PatchError('Patch resource filename must be a basename')
            data = (patch_dir / filename).read_bytes()
            if len(data) != len(original) or hashlib.sha256(data).hexdigest() != item['mips_sha256']:
                raise PatchError('Replacement mip signature/size mismatch: ' + name)
            replacements.append((offset, data, name))
    if not replacements:
        raise PatchError('Empty replacement patch')
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix='.sims2-prompts-', delete=False) as dest:
            temporary = Path(dest.name)
            with source.open('rb') as original:
                shutil.copyfileobj(original, dest, 1024 * 1024)
            for offset, data, _ in replacements:
                dest.seek(offset)
                dest.write(data)
            dest.flush()
            os.fsync(dest.fileno())
        result_hash = digest(temporary)
        if result_hash != manifest['patched_archive_sha256']:
            raise PatchError('Final archive hash mismatch; output has not been published')
        # Atomic publication without clobbering an existing file, including races.
        # Apply on a local filesystem with hard-link support, then copy to the PS5.
        if os.name == 'nt':
            # Windows rename is atomic and refuses an existing destination.
            # Unlike hard links it also works in the host's restricted token.
            os.rename(temporary, output)
        else:
            os.link(temporary, output)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return {'schema': 1, 'patch': manifest['patch'], 'source_sha256': manifest['source_archive_sha256'],
            'output_sha256': result_hash, 'size': output.stat().st_size,
            'replaced_resources': [name for _, _, name in replacements],
            'original_preserved': True, 'game_runtime_verified': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True, help='Original USA DATA/textures.arc')
    parser.add_argument('--output', type=Path, required=True, help='New local textures.arc (original is kept)')
    parser.add_argument('--patch-dir', type=Path, default=Path(__file__).resolve().parents[2] / 'mods/dualsense')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        report = apply(args.source, args.output, args.patch_dir)
    except (PatchError, OSError, KeyError, json.JSONDecodeError) as error:
        parser.exit(1, f'Prompt patch failed: {error}\n')
    text = json.dumps(report, indent=2) + '\n'
    if args.report:
        args.report.write_text(text, encoding='utf-8')
    print(text, end='')


if __name__ == '__main__':
    main()
