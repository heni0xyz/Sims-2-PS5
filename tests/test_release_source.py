# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import hashlib,importlib.util,json,unittest
spec=importlib.util.spec_from_file_location('package',Path(__file__).resolve().parents[1]/'tools/package_source.py')
package=importlib.util.module_from_spec(spec);spec.loader.exec_module(package)

class ReleaseSafety(unittest.TestCase):
    def test_source_output_nesting_refused(self):
        with self.assertRaisesRegex(ValueError,'outside'):
            package.export(package.ROOT,package.ROOT/'tests/../dist')

    def test_game_private_and_credentials_refused(self):
        for name,data in [('recomp/assets/default.xbe',b'XBEH'),('recomp/generated/guest.cpp',b'code'),
                          ('eboot.bin',b'not shipped'),('readme.txt',b'MZbinary'),
                          ('src/token.txt',b'ghp_'+b'A'*36),('src/key.txt',b'-----BEGIN '+b'PRIVATE KEY-----'),
                          ('docs/screen.png',b'\x89PNG'),('../escape.cpp',b'code')]:
            with self.subTest(name=name),self.assertRaises(ValueError):package.inspect(name,data)

    def test_authored_art_and_notices_accepted(self):
        for name,data in [('recomp/artwork/source/sims2.pxz',b'PKeditable'),
                          ('recomp/mods/dualsense/kenney/PNG/button.png',b'\x89PNG'),
                          ('recomp/sdk/LICENSE',b'MIT'),('src/main.cpp',b'// GPL-3.0-only')]:
            package.inspect(name,data)

    def test_native_sdk_internal_headers_are_source_not_owner_data(self):
        package.inspect('vendor/PS5_PayloadSDK/include/freebsd/private/bsdstat/bsdstat.h',b'/* public FreeBSD header */')
        for name in ('vendor/PS5_PayloadSDK/private/save.dat','recomp/private/save.dat',
                     'vendor/PS5_PayloadSDK/include/freebsd/private/secret.key'):
            with self.subTest(name=name),self.assertRaises(ValueError):package.inspect(name,b'private data')

    def test_exported_component_inventory_matches_every_source_byte(self):
        manifest=json.loads((package.ROOT/'source-component-manifest.json').read_text())
        self.assertEqual(set(manifest['components']),{'project','sdk','runtime-sdk','ps5-installer','ps5-foundation','native-sdk'})
        for component in manifest['components'].values():
            for name,record in component['files'].items():
                data=(package.ROOT/component['archive_prefix']/name).read_bytes()
                self.assertEqual(len(data),record['bytes'],name)
                self.assertEqual(hashlib.sha256(data).hexdigest(),record['sha256'],name)
        self.assertTrue((package.ROOT/'vendor/PS5_PayloadSDK/include/freebsd/private/bsdstat/bsdstat.h').is_file())

if __name__=='__main__':unittest.main()
