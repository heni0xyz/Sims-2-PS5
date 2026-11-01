import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/ps5'))
from patch_prompts import apply, digest, PatchError, read_index


def fixture():
    data = bytearray(struct.pack('<IIQ', 0, 0x10000, 0))
    entries = []
    payload_header = bytearray(32)
    struct.pack_into('<HH', payload_header, 16, 32, 32)
    payload_header[22] = 3
    mips = bytes(range(256)) * 5 + bytes(range(64))
    for name in ('button_xbx_a', 'unrelated'):
        start = len(data)
        name_bytes = name.encode() + b'\0'
        payload = payload_header + mips
        data += struct.pack('<4I', 9, 0x5458464C, 0xFFFFFFFF, len(name_bytes))
        data += name_bytes + struct.pack('<I', len(payload)) + payload
        entries.append((name, start, len(data) - start))
    index = len(data)
    data += struct.pack('<I', len(entries))
    for name, start, size in entries:
        data += struct.pack('<III', 123, start, size) + name.encode() + b'\0' + bytes(8)
    struct.pack_into('<I', data, 0, index)
    replacement = bytes([0xA5]) * len(mips)
    mip_offset = entries[0][1] + 20 + len(entries[0][0]) + 1 + 32
    patched = bytearray(data)
    patched[mip_offset:mip_offset + len(mips)] = replacement
    sha = lambda b: hashlib.sha256(b).hexdigest()
    manifest = {'schema': 1, 'patch': 'fixture', 'source_archive_sha256': sha(data),
                'source_entry_count': 2, 'patched_archive_sha256': sha(patched),
                'replacements': [{'resource': 'button_xbx_a', 'mips_file': 'a.bc2',
                                  'header_sha256': sha(payload_header),
                                  'original_mips_sha256': sha(mips),
                                  'mips_sha256': sha(replacement)}]}
    return bytes(data), bytes(patched), replacement, manifest


class PromptPatchTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / 'original.arc'
        self.output = self.root / 'patched.arc'
        original, self.expected, replacement, self.manifest = fixture()
        self.original = original
        self.source.write_bytes(original)
        (self.root / 'a.bc2').write_bytes(replacement)
        self.save_manifest()

    def save_manifest(self):
        (self.root / 'manifest.json').write_text(json.dumps(self.manifest))

    def test_only_requested_mips_change_and_source_is_preserved(self):
        result = apply(self.source, self.output, self.root)
        self.assertEqual(self.output.read_bytes(), self.expected)
        self.assertEqual(self.source.read_bytes(), self.original)
        self.assertEqual(result['output_sha256'], digest(self.output))
        self.assertFalse(result['game_runtime_verified'])

    def test_existing_output_and_original_cannot_be_overwritten(self):
        self.output.write_bytes(b'owner changes')
        for path in (self.output, self.source):
            with self.assertRaises(PatchError):
                apply(self.source, path, self.root)
        self.assertEqual(self.output.read_bytes(), b'owner changes')

    def test_wrong_version_and_already_modified_source_are_rejected(self):
        self.source.write_bytes(self.expected)
        with self.assertRaisesRegex(PatchError, 'SHA-256'):
            apply(self.source, self.output, self.root)
        self.assertFalse(self.output.exists())

    def test_tampered_replacement_is_rejected(self):
        (self.root / 'a.bc2').write_bytes(bytes(1344))
        with self.assertRaisesRegex(PatchError, 'signature'):
            apply(self.source, self.output, self.root)
        self.assertFalse(self.output.exists())

    def test_wrong_final_hash_is_rejected_before_publish_and_temp_removed(self):
        self.manifest['patched_archive_sha256'] = '0' * 64
        self.save_manifest()
        with self.assertRaisesRegex(PatchError, 'Final archive'):
            apply(self.source, self.output, self.root)
        self.assertFalse(self.output.exists())
        self.assertEqual(list(self.root.glob('.sims2-prompts-*')), [])

    def test_path_traversal_rejected(self):
        self.manifest['replacements'][0]['mips_file'] = '../a.bc2'
        self.save_manifest()
        with self.assertRaisesRegex(PatchError, 'basename'):
            apply(self.source, self.output, self.root)

    def test_record_and_index_names_must_agree(self):
        changed = bytearray(self.original)
        changed[16 + 16] = ord('c')
        self.source.write_bytes(changed)
        self.manifest['source_archive_sha256'] = digest(self.source)
        self.save_manifest()
        with self.assertRaisesRegex(PatchError, 'name mismatch'):
            apply(self.source, self.output, self.root)

    def test_truncated_bad_bounds_and_overlapping_index_rejected(self):
        index = struct.unpack_from('<I', self.original)[0]
        cases = [self.original[:10], self.original[:-1]]
        for offset in (0, 0xFFFFFFFF):
            data = bytearray(self.original)
            struct.pack_into('<I', data, 0, offset)
            cases.append(bytes(data))
        data = bytearray(self.original)
        struct.pack_into('<I', data, index + 8, 0)
        cases.append(bytes(data))
        data = bytearray(self.original)
        second = index + 4 + 12 + len('button_xbx_a') + 1 + 8
        struct.pack_into('<I', data, second + 4, 16)
        cases.append(bytes(data))
        for case in cases:
            with self.subTest(size=len(case)):
                with self.assertRaises(PatchError):
                    read_index(io.BytesIO(case))


if __name__ == '__main__':
    unittest.main()
