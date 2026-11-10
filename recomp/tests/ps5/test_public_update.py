# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the production C++ updater with real checksums and interruption.

Run with --driver <host executable>. No network/console access or owner data.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('bundle', ROOT / 'tools/ps5/package_launcher_update.py')
bundle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bundle)
DRIVER = None


class UpdateTests(unittest.TestCase):
    def setUp(self):
        scratch = ROOT/'out/ps5-installer/update-tests'
        scratch.mkdir(parents=True, exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(prefix='case-', dir=scratch)
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / 'title'
        self.source = Path(self.tmp.name) / 'new'
        self.old = {'eboot.bin': b'old self', 'installer-manifest.json': b'{"version":"1.1.0"}',
                    'sce_sys/param.json': b'{"titleId":"PPSA99148"}',
                    'sce_module/libc.prx': b'stable module', 'public-resources/font': b'old font'}
        self.new = dict(self.old, **{'eboot.bin': b'new self', 'installer-manifest.json': b'{"version":"1.1.1"}',
                                    'public-resources/font': b'new font', 'public-resources/new': b'new image'})
        self.private = {'hdd/E/UDATA/EA/save': b'personal save', 'assets/DATA/index.ind': b'owner data',
                        '.sims2-build/cache.json': b'private receipt', 'game-' + 'a'*64 + '.bin': b'owner code',
                        '.sims2-launcher-prefs': b'player preference', 'settings.toml': b'user config',
                        'my.iso': b'owner iso'}
        self.write(self.root, dict(self.old, **self.private))
        self.write(self.source, self.new)
        self.archive = Path(self.tmp.name) / 'update.s2u'
        self.info = bundle.write_bundle({n: self.source/n for n in self.new}, 'PPSA99148', '1.1.1', self.archive)

    @staticmethod
    def write(root, entries):
        for name, data in entries.items():
            target = root/name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)

    def call(self, operation, *args, code=0):
        result = subprocess.run([str(DRIVER), operation, str(self.root), *map(str, args)], capture_output=True, text=True)
        self.assertEqual(result.returncode, code, result.stdout + result.stderr)
        return result

    def prepare(self, code=0):
        return self.call('prepare', self.archive, bundle.digest(self.archive), '1.1.1', '1.1.0', code=code)

    def verify(self, expected):
        for name, data in dict(expected, **self.private).items():
            self.assertEqual((self.root/name).read_bytes(), data, name)

    def mutate(self, change):
        data = self.archive.read_bytes()
        length = struct.unpack('<I', data[8:12])[0]
        manifest = json.loads(data[12:12+length])
        change(manifest)
        header = json.dumps(manifest).encode()
        self.archive.write_bytes(b'S2UPD001' + struct.pack('<I', len(header)) + header + data[12+length:])

    def test_apply_preserves_all_private_data(self):
        self.prepare()
        self.assertEqual((self.root/'.sims2-update/helper.bin').read_bytes(), self.old['eboot.bin'])
        self.call('apply')
        self.verify(self.new)
        job = next((self.root/'.sims2-update').glob('job-*'))
        self.assertFalse((job/'backup/sce_module/libc.prx').exists(), 'Unchanged mapped module must not be replaced')
        self.call('recover')
        self.assertFalse((self.root/'.sims2-update/pending').exists())

    def test_caught_failures_roll_back_every_position(self):
        self.prepare()
        for count in range(1, len(self.new)+1):
            self.call('apply', 'fail', count, code=1)
            self.verify(self.old)
            self.assertFalse((self.root/'public-resources/new').exists())
        self.call('apply')
        self.verify(self.new)

    def test_process_crashes_recover_every_position(self):
        self.prepare()
        for count in range(1, len(self.new)+1):
            self.call('apply', 'crash', count, code=23)
            self.assertTrue((self.root/'eboot.bin').is_file(), 'Shell entry must never disappear')
            self.call('recover')
            self.verify(self.old)
            self.assertFalse((self.root/'public-resources/new').exists())
        self.call('apply')
        self.verify(self.new)

    def test_corrupt_archive(self):
        self.archive.write_bytes(self.archive.read_bytes() + b'bad')
        self.call('prepare', self.archive, self.info['sha256'], '1.1.1', '1.1.0', code=1)
        self.verify(self.old)

    def test_unsafe_and_duplicate_paths(self):
        original = self.archive.read_bytes()
        for path in ['../hdd/save', 'public-resources/../x', 'public-resources//x',
                     'public-resources/x.', 'public-resources/x ', 'public-resources\\x',
                     'hdd/new', 'assets/new', 'settings.toml', '/eboot.bin', 'EBOOT.BIN']:
            self.archive.write_bytes(original)
            self.mutate(lambda m: m['files'][0].update(name=path))
            self.prepare(code=1)
            self.verify(self.old)
        self.archive.write_bytes(original)
        self.mutate(lambda m: m['files'][0].update(name=m['files'][1]['name']))
        self.prepare(code=1)

    def test_title_abi_version_and_bounds(self):
        original = self.archive.read_bytes()
        for field, value in [('title_id', 'PPSA99999'), ('launcher_abi', 2), ('version', '9.0.0'), ('schema', 2)]:
            self.archive.write_bytes(original)
            self.mutate(lambda m: m.update({field: value}))
            self.prepare(code=1)
        self.archive.write_bytes(original)
        self.mutate(lambda m: m['files'][0].update(offset=1))
        self.prepare(code=1)

    def test_changed_runtime_module_rejected(self):
        (self.root/'sce_module/libc.prx').write_bytes(b'other ABI')
        self.prepare(code=1)
        self.assertEqual((self.root/'eboot.bin').read_bytes(), self.old['eboot.bin'])

    def test_changed_stage_rejected_before_mutation(self):
        self.prepare()
        job = next((self.root/'.sims2-update').glob('job-*'))
        (job/'stage/eboot.bin').write_bytes(b'corrupt')
        self.call('apply', code=1)
        self.verify(self.old)

    def test_recovery_refuses_unknown_external_change(self):
        self.prepare()
        self.call('apply', 'crash', len(self.new), code=23)
        (self.root/'eboot.bin').write_bytes(b'unrelated file')
        self.call('recover', code=1)
        self.assertEqual((self.root/'eboot.bin').read_bytes(), b'unrelated file')

    def test_marker_rejects_unknown_workspace(self):
        (self.root/'.sims2-update').mkdir()
        (self.root/'.sims2-update/owner').write_text('not ours')
        self.prepare(code=1)
        self.verify(self.old)

    def test_staged_identity_and_version_checked(self):
        for name, data in [('sce_sys/param.json', b'{"titleId":"PPSA99145"}'),
                           ('installer-manifest.json', b'{"version":"9.9.9"}')]:
            original = (self.source/name).read_bytes()
            (self.source/name).write_bytes(data)
            bundle.write_bundle({n: self.source/n for n in self.new}, 'PPSA99148', '1.1.1', self.archive)
            self.prepare(code=1)
            self.verify(self.old)
            (self.source/name).write_bytes(original)

    def test_case_alias_collision(self):
        self.mutate(lambda m: next(x for x in m['files'] if x['name']=='public-resources/new').update(name='public-resources/FONT'))
        self.prepare(code=1)

    def test_public_packager_excludes_live_settings_and_placeholder(self):
        (self.source/'eboot.bin').write_bytes(b'\x4f\x15\x3d\x1dpublic tool')
        self.write(self.source, {'settings.toml': b'public defaults',
                                'assets/README.txt': b'SIMS2-INSTALLER-PLACEHOLDER-1\n'})
        inventory = dict(self.new, **{'settings.toml': b'', 'assets/README.txt': b''})
        manifest = dict(kind='ps5_owner_iso_installer', status='BUILD_PASS_CONSOLE_NOT_TESTED',
                        files={n: dict(sha256=bundle.digest(self.source/n)) for n in inventory})
        path = Path(self.tmp.name)/'build-manifest.json'
        path.write_text(json.dumps(manifest))
        bundle.package(self.source, path, self.archive)
        data = self.archive.read_bytes()
        meta = json.loads(data[12:12+struct.unpack('<I', data[8:12])[0]])
        self.assertFalse({'settings.toml', 'assets/README.txt'} & {x['name'] for x in meta['files']})
        (self.source/'assets/README.txt').write_bytes(b'unrelated content')
        manifest['files']['assets/README.txt']['sha256'] = bundle.digest(self.source/'assets/README.txt')
        path.write_text(json.dumps(manifest))
        with self.assertRaises(ValueError):
            bundle.package(self.source, path, self.archive)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--driver', type=Path, required=True)
    args, rest = parser.parse_known_args()
    DRIVER = args.driver.resolve()
    unittest.main(argv=[__file__, *rest])
