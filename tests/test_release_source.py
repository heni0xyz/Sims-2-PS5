# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import importlib.util,unittest
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

if __name__=='__main__':unittest.main()
